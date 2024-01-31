/*
 * ARM64 signal handling routines
 *
 * Copyright 2010-2013 André Hentschel
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

#ifdef __aarch64__

#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

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
WINE_DECLARE_DEBUG_CHANNEL(threadname);

typedef struct _SCOPE_TABLE
{
    ULONG Count;
    struct
    {
        ULONG BeginAddress;
        ULONG EndAddress;
        ULONG HandlerAddress;
        ULONG JumpTarget;
    } ScopeRecord[1];
} SCOPE_TABLE, *PSCOPE_TABLE;


/* layering violation: the setjmp buffer is defined in msvcrt, but used by RtlUnwindEx */
struct MSVCRT_JUMP_BUFFER
{
    unsigned __int64 Frame;
    unsigned __int64 Reserved;
    unsigned __int64 X19;
    unsigned __int64 X20;
    unsigned __int64 X21;
    unsigned __int64 X22;
    unsigned __int64 X23;
    unsigned __int64 X24;
    unsigned __int64 X25;
    unsigned __int64 X26;
    unsigned __int64 X27;
    unsigned __int64 X28;
    unsigned __int64 Fp;
    unsigned __int64 Lr;
    unsigned __int64 Sp;
    ULONG Fpcr;
    ULONG Fpsr;
    double D[8];
};


static void dump_scope_table( ULONG64 base, const SCOPE_TABLE *table )
{
    unsigned int i;

    TRACE( "scope table at %p\n", table );
    for (i = 0; i < table->Count; i++)
        TRACE( "  %u: %I64x-%I64x handler %I64x target %I64x\n", i,
               base + table->ScopeRecord[i].BeginAddress,
               base + table->ScopeRecord[i].EndAddress,
               base + table->ScopeRecord[i].HandlerAddress,
               base + table->ScopeRecord[i].JumpTarget );
}

/*******************************************************************
 *         syscalls
 */
#define SYSCALL_ENTRY(id,name,args) __ASM_SYSCALL_FUNC( id, name )
ALL_SYSCALLS64
#undef SYSCALL_ENTRY


/**************************************************************************
 *		__chkstk (NTDLL.@)
 *
 * Supposed to touch all the stack pages, but we shouldn't need that.
 */
__ASM_GLOBAL_FUNC( __chkstk, "ret")


/***********************************************************************
 *		RtlCaptureContext (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( RtlCaptureContext,
                    "str xzr, [x0, #0x8]\n\t"        /* context->X0 */
                    "stp x1, x2, [x0, #0x10]\n\t"    /* context->X1,X2 */
                    "stp x3, x4, [x0, #0x20]\n\t"    /* context->X3,X4 */
                    "stp x5, x6, [x0, #0x30]\n\t"    /* context->X5,X6 */
                    "stp x7, x8, [x0, #0x40]\n\t"    /* context->X7,X8 */
                    "stp x9, x10, [x0, #0x50]\n\t"   /* context->X9,X10 */
                    "stp x11, x12, [x0, #0x60]\n\t"  /* context->X11,X12 */
                    "stp x13, x14, [x0, #0x70]\n\t"  /* context->X13,X14 */
                    "stp x15, x16, [x0, #0x80]\n\t"  /* context->X15,X16 */
                    "stp x17, x18, [x0, #0x90]\n\t"  /* context->X17,X18 */
                    "stp x19, x20, [x0, #0xa0]\n\t"  /* context->X19,X20 */
                    "stp x21, x22, [x0, #0xb0]\n\t"  /* context->X21,X22 */
                    "stp x23, x24, [x0, #0xc0]\n\t"  /* context->X23,X24 */
                    "stp x25, x26, [x0, #0xd0]\n\t"  /* context->X25,X26 */
                    "stp x27, x28, [x0, #0xe0]\n\t"  /* context->X27,X28 */
                    "stp x29, xzr, [x0, #0xf0]\n\t"  /* context->Fp,Lr */
                    "mov x1, sp\n\t"
                    "stp x1, x30, [x0, #0x100]\n\t"  /* context->Sp,Pc */
                    "stp q0,  q1,  [x0, #0x110]\n\t" /* context->V[0-1] */
                    "stp q2,  q3,  [x0, #0x130]\n\t" /* context->V[2-3] */
                    "stp q4,  q5,  [x0, #0x150]\n\t" /* context->V[4-5] */
                    "stp q6,  q7,  [x0, #0x170]\n\t" /* context->V[6-7] */
                    "stp q8,  q9,  [x0, #0x190]\n\t" /* context->V[8-9] */
                    "stp q10, q11, [x0, #0x1b0]\n\t" /* context->V[10-11] */
                    "stp q12, q13, [x0, #0x1d0]\n\t" /* context->V[12-13] */
                    "stp q14, q15, [x0, #0x1f0]\n\t" /* context->V[14-15] */
                    "stp q16, q17, [x0, #0x210]\n\t" /* context->V[16-17] */
                    "stp q18, q19, [x0, #0x230]\n\t" /* context->V[18-19] */
                    "stp q20, q21, [x0, #0x250]\n\t" /* context->V[20-21] */
                    "stp q22, q23, [x0, #0x270]\n\t" /* context->V[22-23] */
                    "stp q24, q25, [x0, #0x290]\n\t" /* context->V[24-25] */
                    "stp q26, q27, [x0, #0x2b0]\n\t" /* context->V[26-27] */
                    "stp q28, q29, [x0, #0x2d0]\n\t" /* context->V[28-29] */
                    "stp q30, q31, [x0, #0x2f0]\n\t" /* context->V[30-31] */
                    "mov w1, #0x400000\n\t"          /* CONTEXT_ARM64 */
                    "movk w1, #0x7\n\t"              /* CONTEXT_FULL */
                    "str w1, [x0]\n\t"               /* context->ContextFlags */
                    "mrs x1, NZCV\n\t"
                    "str w1, [x0, #0x4]\n\t"         /* context->Cpsr */
                    "mrs x1, FPCR\n\t"
                    "str w1, [x0, #0x310]\n\t"       /* context->Fpcr */
                    "mrs x1, FPSR\n\t"
                    "str w1, [x0, #0x314]\n\t"       /* context->Fpsr */
                    "ret" )


/**********************************************************************
 *           virtual_unwind
 */
static NTSTATUS virtual_unwind( ULONG type, DISPATCHER_CONTEXT *dispatch, CONTEXT *context )
{
    LDR_DATA_TABLE_ENTRY *module;
    NTSTATUS status;
    DWORD64 pc;

    dispatch->ImageBase        = 0;
    dispatch->ScopeIndex       = 0;
    dispatch->EstablisherFrame = 0;
    dispatch->ControlPc        = context->Pc;
    dispatch->ControlPcIsUnwound = (context->ContextFlags & CONTEXT_UNWOUND_TO_CALL) != 0;
    pc = context->Pc - (dispatch->ControlPcIsUnwound ? 4 : 0);

    if ((dispatch->FunctionEntry = lookup_function_info( pc, &dispatch->ImageBase, &module )))
    {
        dispatch->LanguageHandler = RtlVirtualUnwind( type, dispatch->ImageBase, pc,
                                                      dispatch->FunctionEntry, context,
                                                      &dispatch->HandlerData, &dispatch->EstablisherFrame,
                                                      NULL );
        return STATUS_SUCCESS;
    }

    status = context->Pc != context->Lr ? STATUS_SUCCESS : STATUS_INVALID_DISPOSITION;
    if (module)
        WARN( "exception data not found in %s for pc %p, lr %p\n",
              debugstr_w(module->BaseDllName.Buffer), (void *)context->Pc, (void *)context->Lr );
    else
        WARN( "no module found for pc %p, lr %p\n",
              (void *)context->Pc, (void *)context->Lr );
    dispatch->EstablisherFrame = context->Sp;
    dispatch->LanguageHandler = NULL;
    context->Pc = context->Lr;
    context->ContextFlags |= CONTEXT_UNWOUND_TO_CALL;
    return status;
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

    /* copy the original dispatcher into the current one, except for the TargetPc */
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
extern DWORD WINAPI unwind_handler_wrapper( EXCEPTION_RECORD *rec, DISPATCHER_CONTEXT *dispatch );
__ASM_GLOBAL_FUNC( unwind_handler_wrapper,
                   "stp x29, x30, [sp, #-32]!\n\t"
                   ".seh_save_fplr_x 32\n\t"
                   "mov x29, sp\n\t"
                   ".seh_set_fp\n\t"
                   ".seh_endprologue\n\t"
                   ".seh_handler " __ASM_NAME("unwind_exception_handler") ", @except, @unwind\n\t"
                   "str x1, [sp, #16]\n\t"    /* frame[-2] = dispatch */
                   "mov x3, x1\n\t"
                   "ldr x1, [x3, #0x18]\n\t"  /* dispatch->EstablisherFrame */
                   "ldr x2, [x3, #0x28]\n\t"  /* dispatch->ContextRecord */
                   "ldr x15, [x3, #0x30]\n\t" /* dispatch->LanguageHandler */
                   "blr x15\n\t"
                   "ldp x29, x30, [sp], #32\n\t"
                   "ret" )

/**********************************************************************
 *           call_unwind_handler
 *
 * Call a single unwind handler.
 */
static DWORD call_unwind_handler( EXCEPTION_RECORD *rec, DISPATCHER_CONTEXT *dispatch )
{
    DWORD res;

    TRACE( "calling handler %p (rec=%p, frame=%I64x context=%p, dispatch=%p)\n",
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
extern DWORD WINAPI call_handler_wrapper( EXCEPTION_RECORD *rec, CONTEXT *context, DISPATCHER_CONTEXT *dispatch );
__ASM_GLOBAL_FUNC( call_handler_wrapper,
                   "stp x29, x30, [sp, #-16]!\n\t"
                   ".seh_save_fplr_x 16\n\t"
                   "mov x29, sp\n\t"
                   ".seh_set_fp\n\t"
                   ".seh_endprologue\n\t"
                   ".seh_handler " __ASM_NAME("nested_exception_handler") ", @except\n\t"
                   "mov x3, x2\n\t"           /* dispatch */
                   "mov x2, x1\n\t"           /* context */
                   "ldr x1, [x3, #0x18]\n\t"  /* dispatch->EstablisherFrame */
                   "ldr x15, [x3, #0x30]\n\t" /* dispatch->LanguageHandler */
                   "blr x15\n\t"
                   "ldp x29, x30, [sp], #16\n\t"
                   "ret" )


/**********************************************************************
 *           call_handler
 *
 * Call a single exception handler.
 */
static DWORD call_handler( EXCEPTION_RECORD *rec, CONTEXT *context, DISPATCHER_CONTEXT *dispatch )
{
    DWORD res;

    TRACE( "calling handler %p (rec=%p, frame=%I64x context=%p, dispatch=%p)\n",
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
 *           call_function_handlers
 *
 * Call the per-function handlers.
 */
static NTSTATUS call_function_handlers( EXCEPTION_RECORD *rec, CONTEXT *orig_context )
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
    dispatch.NonVolatileRegisters = (BYTE *)&prev_context.X19;

    for (;;)
    {
        status = virtual_unwind( UNW_FLAG_EHANDLER, &dispatch, &context );
        if (status != STATUS_SUCCESS) return status;

    unwind_done:
        if (!dispatch.EstablisherFrame) break;

        if (!is_valid_frame( dispatch.EstablisherFrame ))
        {
            ERR( "invalid frame %I64x (%p-%p)\n", dispatch.EstablisherFrame,
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
                TRACE_(seh)( "nested exception\n" );
                break;
            case ExceptionCollidedUnwind: {
                ULONG64 frame;

                context = *dispatch.ContextRecord;
                dispatch.ContextRecord = &context;
                RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                                  dispatch.ControlPc, dispatch.FunctionEntry,
                                  &context, &dispatch.HandlerData, &frame, NULL );
                goto unwind_done;
            }
            default:
                return STATUS_INVALID_DISPOSITION;
            }
        }
        /* hack: call wine handlers registered in the tib list */
        else while (is_valid_frame( (ULONG_PTR)teb_frame ) && (ULONG64)teb_frame < context.Sp)
        {
            TRACE( "found wine frame %p rsp %I64x handler %p\n",
                    teb_frame, context.Sp, teb_frame->Handler );
            dispatch.EstablisherFrame = (ULONG64)teb_frame;
            switch (call_teb_handler( rec, orig_context, &dispatch, teb_frame ))
            {
            case ExceptionContinueExecution:
                if (rec->ExceptionFlags & EH_NONCONTINUABLE) return STATUS_NONCONTINUABLE_EXCEPTION;
                return STATUS_SUCCESS;
            case ExceptionContinueSearch:
                break;
            case ExceptionNestedException:
                rec->ExceptionFlags |= EH_NESTED_CALL;
                TRACE_(seh)( "nested exception\n" );
                break;
            case ExceptionCollidedUnwind: {
                ULONG64 frame;

                context = *dispatch.ContextRecord;
                dispatch.ContextRecord = &context;
                RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                                  dispatch.ControlPc, dispatch.FunctionEntry,
                                  &context, &dispatch.HandlerData, &frame, NULL );
                teb_frame = teb_frame->Prev;
                goto unwind_done;
            }
            default:
                return STATUS_INVALID_DISPOSITION;
            }
            teb_frame = teb_frame->Prev;
        }

        if (context.Sp == (ULONG64)NtCurrentTeb()->Tib.StackBase) break;
        prev_context = context;
    }
    return STATUS_UNHANDLED_EXCEPTION;
}


NTSTATUS WINAPI dispatch_exception( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    NTSTATUS status;
    DWORD c;

    TRACE( "code=%lx flags=%lx addr=%p pc=%016I64x\n",
           rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress, context->Pc );
    for (c = 0; c < rec->NumberParameters; c++)
        TRACE( " info[%ld]=%016I64x\n", c, rec->ExceptionInformation[c] );

    if (rec->ExceptionCode == EXCEPTION_WINE_STUB)
    {
        if (rec->ExceptionInformation[1] >> 16)
            MESSAGE( "wine: Call from %p to unimplemented function %s.%s, aborting\n",
                     rec->ExceptionAddress,
                     (char*)rec->ExceptionInformation[0], (char*)rec->ExceptionInformation[1] );
        else
            MESSAGE( "wine: Call from %p to unimplemented function %s.%Id, aborting\n",
                     rec->ExceptionAddress,
                     (char*)rec->ExceptionInformation[0], rec->ExceptionInformation[1] );
    }
    else if (rec->ExceptionCode == EXCEPTION_WINE_NAME_THREAD && rec->ExceptionInformation[0] == 0x1000)
    {
        if ((DWORD)rec->ExceptionInformation[2] == -1 || (DWORD)rec->ExceptionInformation[2] == GetCurrentThreadId())
            WARN_(threadname)( "Thread renamed to %s\n", debugstr_a((char *)rec->ExceptionInformation[1]) );
        else
            WARN_(threadname)( "Thread ID %04lx renamed to %s\n", (DWORD)rec->ExceptionInformation[2],
                               debugstr_a((char *)rec->ExceptionInformation[1]) );

        set_native_thread_name((DWORD)rec->ExceptionInformation[2], (char *)rec->ExceptionInformation[1]);
    }
    else if (rec->ExceptionCode == DBG_PRINTEXCEPTION_C)
    {
        WARN( "%s\n", debugstr_an((char *)rec->ExceptionInformation[1], rec->ExceptionInformation[0] - 1) );
    }
    else if (rec->ExceptionCode == DBG_PRINTEXCEPTION_WIDE_C)
    {
        WARN( "%s\n", debugstr_wn((WCHAR *)rec->ExceptionInformation[1], rec->ExceptionInformation[0] - 1) );
    }
    else
    {
        if (rec->ExceptionCode == STATUS_ASSERTION_FAILURE)
            ERR( "%s exception (code=%lx) raised\n", debugstr_exception_code(rec->ExceptionCode), rec->ExceptionCode );
        else
            WARN( "%s exception (code=%lx) raised\n", debugstr_exception_code(rec->ExceptionCode), rec->ExceptionCode );

        TRACE("  x0=%016I64x  x1=%016I64x  x2=%016I64x  x3=%016I64x\n",
              context->X0, context->X1, context->X2, context->X3 );
        TRACE("  x4=%016I64x  x5=%016I64x  x6=%016I64x  x7=%016I64x\n",
              context->X4, context->X5, context->X6, context->X7 );
        TRACE("  x8=%016I64x  x9=%016I64x x10=%016I64x x11=%016I64x\n",
              context->X8, context->X9, context->X10, context->X11 );
        TRACE(" x12=%016I64x x13=%016I64x x14=%016I64x x15=%016I64x\n",
              context->X12, context->X13, context->X14, context->X15 );
        TRACE(" x16=%016I64x x17=%016I64x x18=%016I64x x19=%016I64x\n",
              context->X16, context->X17, context->X18, context->X19 );
        TRACE(" x20=%016I64x x21=%016I64x x22=%016I64x x23=%016I64x\n",
              context->X20, context->X21, context->X22, context->X23 );
        TRACE(" x24=%016I64x x25=%016I64x x26=%016I64x x27=%016I64x\n",
              context->X24, context->X25, context->X26, context->X27 );
        TRACE(" x28=%016I64x  fp=%016I64x  lr=%016I64x  sp=%016I64x\n",
              context->X28, context->Fp, context->Lr, context->Sp );
    }

    if (call_vectored_handlers( rec, context ) == EXCEPTION_CONTINUE_EXECUTION)
        NtContinue( context, FALSE );

    if ((status = call_function_handlers( rec, context )) == STATUS_SUCCESS)
        NtContinue( context, FALSE );

    if (status != STATUS_UNHANDLED_EXCEPTION) RtlRaiseStatus( status );
    return NtRaiseException( rec, context, FALSE );
}


/*******************************************************************
 *		KiUserExceptionDispatcher (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( KiUserExceptionDispatcher,
                   __ASM_SEH(".seh_context\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   "adr x16, " __ASM_NAME("pWow64PrepareForException") "\n\t"
                   "ldr x16, [x16]\n\t"
                   "cbz x16, 1f\n\t"
                   "add x0, sp, #0x390\n\t"     /* rec (context + 1) */
                   "mov x1, sp\n\t"             /* context */
                   "blr x16\n"
                   "1:\tadd x0, sp, #0x390\n\t" /* rec (context + 1) */
                   "mov x1, sp\n\t"             /* context */
                   "bl " __ASM_NAME("dispatch_exception") "\n\t"
                   "brk #1" )


/*******************************************************************
 *		KiUserApcDispatcher (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( KiUserApcDispatcher,
                   __ASM_SEH(".seh_context\n\t")
                   "nop\n\t"
                   __ASM_SEH(".seh_stackalloc 0x30\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   "ldp x16, x0, [sp]\n\t"        /* func, arg1 */
                   "ldp x1, x2, [sp, #0x10]\n\t"  /* arg2, arg3 */
                   "add x3, sp, #0x30\n\t"        /* context (FIXME) */
                   "blr x16\n\t"
                   "add x0, sp, #0x30\n\t"        /* context */
                   "ldr w1, [sp, #0x20]\n\t"      /* alertable */
                   "bl " __ASM_NAME("NtContinue") "\n\t"
                   "brk #1" )


/*******************************************************************
 *		KiUserCallbackDispatcher (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( KiUserCallbackDispatcher,
                   ".seh_pushframe\n\t"
                   "nop\n\t"
                   ".seh_stackalloc 0x20\n\t"
                   "nop\n\t"
                   ".seh_save_reg lr, 0x18\n\t"
                   ".seh_endprologue\n\t"
                   ".seh_handler " __ASM_NAME("user_callback_handler") ", @except\n\t"
                   "ldr x0, [sp]\n\t"             /* args */
                   "ldp w1, w2, [sp, #0x08]\n\t"  /* len, id */
                   "ldr x3, [x18, 0x60]\n\t"      /* peb */
                   "ldr x3, [x3, 0x58]\n\t"       /* peb->KernelCallbackTable */
                   "ldr x15, [x3, x2, lsl #3]\n\t"
                   "blr x15\n\t"
                   ".globl " __ASM_NAME("KiUserCallbackDispatcherReturn") "\n"
                   __ASM_NAME("KiUserCallbackDispatcherReturn") ":\n\t"
                   "mov x2, x0\n\t"               /* status */
                   "mov x1, #0\n\t"               /* ret_len */
                   "mov x0, x1\n\t"               /* ret_ptr */
                   "bl " __ASM_NAME("NtCallbackReturn") "\n\t"
                   "bl " __ASM_NAME("RtlRaiseStatus") "\n\t"
                   "brk #1" )


/***********************************************************************
 * Definitions for Win32 unwind tables
 */

struct unwind_info
{
    DWORD function_length : 18;
    DWORD version : 2;
    DWORD x : 1;
    DWORD e : 1;
    DWORD epilog : 5;
    DWORD codes : 5;
};

struct unwind_info_ext
{
    WORD epilog;
    BYTE codes;
    BYTE reserved;
};

struct unwind_info_epilog
{
    DWORD offset : 18;
    DWORD res : 4;
    DWORD index : 10;
};

static const BYTE unwind_code_len[256] =
{
/* 00 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* 20 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* 40 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* 60 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* 80 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* a0 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* c0 */ 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
/* e0 */ 4,1,2,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};

/***********************************************************************
 *           get_sequence_len
 */
static unsigned int get_sequence_len( BYTE *ptr, BYTE *end )
{
    unsigned int ret = 0;

    while (ptr < end)
    {
        if (*ptr == 0xe4 || *ptr == 0xe5) break;
        ptr += unwind_code_len[*ptr];
        ret++;
    }
    return ret;
}


/***********************************************************************
 *           restore_regs
 */
static void restore_regs( int reg, int count, int pos, CONTEXT *context,
                          KNONVOLATILE_CONTEXT_POINTERS *ptrs )
{
    int i, offset = max( 0, pos );
    for (i = 0; i < count; i++)
    {
        if (ptrs && reg + i >= 19) (&ptrs->X19)[reg + i - 19] = (DWORD64 *)context->Sp + i + offset;
        context->X[reg + i] = ((DWORD64 *)context->Sp)[i + offset];
    }
    if (pos < 0) context->Sp += -8 * pos;
}


/***********************************************************************
 *           restore_fpregs
 */
static void restore_fpregs( int reg, int count, int pos, CONTEXT *context,
                            KNONVOLATILE_CONTEXT_POINTERS *ptrs )
{
    int i, offset = max( 0, pos );
    for (i = 0; i < count; i++)
    {
        if (ptrs && reg + i >= 8) (&ptrs->D8)[reg + i - 8] = (DWORD64 *)context->Sp + i + offset;
        context->V[reg + i].D[0] = ((double *)context->Sp)[i + offset];
    }
    if (pos < 0) context->Sp += -8 * pos;
}


static void do_pac_auth( CONTEXT *context )
{
    register DWORD64 x17 __asm__( "x17" ) = context->Lr;
    register DWORD64 x16 __asm__( "x16" ) = context->Sp;

    /* This is the autib1716 instruction. The hint instruction is used here
     * as gcc does not assemble autib1716 for pre armv8.3a targets. For
     * pre-armv8.3a targets, this is just treated as a hint instruction, which
     * is ignored. */
    __asm__( "hint 0xe" : "+r"(x17) : "r"(x16) );

    context->Lr = x17;
}

/***********************************************************************
 *           process_unwind_codes
 */
static void process_unwind_codes( BYTE *ptr, BYTE *end, CONTEXT *context,
                                  KNONVOLATILE_CONTEXT_POINTERS *ptrs, int skip )
{
    unsigned int val, len, save_next = 2;

    /* skip codes */
    while (ptr < end && skip)
    {
        if (*ptr == 0xe4 || *ptr == 0xe5) break;
        ptr += unwind_code_len[*ptr];
        skip--;
    }

    while (ptr < end)
    {
        if ((len = unwind_code_len[*ptr]) > 1)
        {
            if (ptr + len > end) break;
            val = ptr[0] * 0x100 + ptr[1];
        }
        else val = *ptr;

        if (*ptr < 0x20)  /* alloc_s */
            context->Sp += 16 * (val & 0x1f);
        else if (*ptr < 0x40)  /* save_r19r20_x */
            restore_regs( 19, save_next, -(val & 0x1f), context, ptrs );
        else if (*ptr < 0x80) /* save_fplr */
            restore_regs( 29, 2, val & 0x3f, context, ptrs );
        else if (*ptr < 0xc0)  /* save_fplr_x */
            restore_regs( 29, 2, -(val & 0x3f) - 1, context, ptrs );
        else if (*ptr < 0xc8)  /* alloc_m */
            context->Sp += 16 * (val & 0x7ff);
        else if (*ptr < 0xcc)  /* save_regp */
            restore_regs( 19 + ((val >> 6) & 0xf), save_next, val & 0x3f, context, ptrs );
        else if (*ptr < 0xd0)  /* save_regp_x */
            restore_regs( 19 + ((val >> 6) & 0xf), save_next, -(val & 0x3f) - 1, context, ptrs );
        else if (*ptr < 0xd4)  /* save_reg */
            restore_regs( 19 + ((val >> 6) & 0xf), 1, val & 0x3f, context, ptrs );
        else if (*ptr < 0xd6)  /* save_reg_x */
            restore_regs( 19 + ((val >> 5) & 0xf), 1, -(val & 0x1f) - 1, context, ptrs );
        else if (*ptr < 0xd8)  /* save_lrpair */
        {
            restore_regs( 19 + 2 * ((val >> 6) & 0x7), 1, val & 0x3f, context, ptrs );
            restore_regs( 30, 1, (val & 0x3f) + 1, context, ptrs );
        }
        else if (*ptr < 0xda)  /* save_fregp */
            restore_fpregs( 8 + ((val >> 6) & 0x7), save_next, val & 0x3f, context, ptrs );
        else if (*ptr < 0xdc)  /* save_fregp_x */
            restore_fpregs( 8 + ((val >> 6) & 0x7), save_next, -(val & 0x3f) - 1, context, ptrs );
        else if (*ptr < 0xde)  /* save_freg */
            restore_fpregs( 8 + ((val >> 6) & 0x7), 1, val & 0x3f, context, ptrs );
        else if (*ptr == 0xde)  /* save_freg_x */
            restore_fpregs( 8 + ((val >> 5) & 0x7), 1, -(val & 0x3f) - 1, context, ptrs );
        else if (*ptr == 0xe0)  /* alloc_l */
            context->Sp += 16 * ((ptr[1] << 16) + (ptr[2] << 8) + ptr[3]);
        else if (*ptr == 0xe1)  /* set_fp */
            context->Sp = context->Fp;
        else if (*ptr == 0xe2)  /* add_fp */
            context->Sp = context->Fp - 8 * (val & 0xff);
        else if (*ptr == 0xe3)  /* nop */
            /* nop */ ;
        else if (*ptr == 0xe4)  /* end */
            break;
        else if (*ptr == 0xe5)  /* end_c */
            break;
        else if (*ptr == 0xe6)  /* save_next */
        {
            save_next += 2;
            ptr += len;
            continue;
        }
        else if (*ptr == 0xe9)  /* MSFT_OP_MACHINE_FRAME */
        {
            context->Pc = ((DWORD64 *)context->Sp)[1];
            context->Sp = ((DWORD64 *)context->Sp)[0];
            context->ContextFlags &= ~CONTEXT_UNWOUND_TO_CALL;
        }
        else if (*ptr == 0xea)  /* MSFT_OP_CONTEXT */
        {
            memcpy( context, (DWORD64 *)context->Sp, sizeof(CONTEXT) );
        }
        else if (*ptr == 0xfc)  /* pac_sign_lr */
        {
            do_pac_auth( context );
        }
        else
        {
            WARN( "unsupported code %02x\n", *ptr );
            return;
        }
        save_next = 2;
        ptr += len;
    }
}


/***********************************************************************
 *           unwind_packed_data
 */
static void *unwind_packed_data( ULONG_PTR base, ULONG_PTR pc, RUNTIME_FUNCTION *func,
                                 CONTEXT *context, KNONVOLATILE_CONTEXT_POINTERS *ptrs )
{
    int i;
    unsigned int len, offset, skip = 0;
    unsigned int int_size = func->RegI * 8, fp_size = func->RegF * 8, regsave, local_size;
    unsigned int int_regs, fp_regs, saved_regs, local_size_regs;

    TRACE( "function %I64x-%I64x: len=%#x flag=%x regF=%u regI=%u H=%u CR=%u frame=%x\n",
           base + func->BeginAddress, base + func->BeginAddress + func->FunctionLength * 4,
           func->FunctionLength, func->Flag, func->RegF, func->RegI, func->H, func->CR, func->FrameSize );

    if (func->CR == 1) int_size += 8;
    if (func->RegF) fp_size += 8;

    regsave = ((int_size + fp_size + 8 * 8 * func->H) + 0xf) & ~0xf;
    local_size = func->FrameSize * 16 - regsave;

    int_regs = int_size / 8;
    fp_regs = fp_size / 8;
    saved_regs = regsave / 8;
    local_size_regs = local_size / 8;

    /* check for prolog/epilog */
    if (func->Flag == 1)
    {
        offset = ((pc - base) - func->BeginAddress) / 4;
        if (offset < 17 || offset >= func->FunctionLength - 15)
        {
            len = (int_size + 8) / 16 + (fp_size + 8) / 16;
            switch (func->CR)
            {
            case 2:
                len++; /* pacibsp */
                /* fall through */
            case 3:
                len++; /* mov x29,sp */
                len++; /* stp x29,lr,[sp,0] */
                if (local_size <= 512) break;
                /* fall through */
            case 0:
            case 1:
                if (local_size) len++;  /* sub sp,sp,#local_size */
                if (local_size > 4088) len++;  /* sub sp,sp,#4088 */
                break;
            }
            len += 4 * func->H;
            if (offset < len)  /* prolog */
            {
                skip = len - offset;
            }
            else if (offset >= func->FunctionLength - (len + 1))  /* epilog */
            {
                skip = offset - (func->FunctionLength - (len + 1));
            }
        }
    }

    if (!skip)
    {
        if (func->CR == 3 || func->CR == 2)
        {
            DWORD64 *fp = (DWORD64 *) context->Fp; /* X[29] */
            context->Sp = context->Fp;
            context->X[29] = fp[0];
            context->X[30] = fp[1];
        }
        context->Sp += local_size;
        if (fp_size) restore_fpregs( 8, fp_regs, int_regs, context, ptrs );
        if (func->CR == 1) restore_regs( 30, 1, int_regs - 1, context, ptrs );
        restore_regs( 19, func->RegI, -saved_regs, context, ptrs );
    }
    else
    {
        unsigned int pos = 0;

        switch (func->CR)
        {
        case 3:
        case 2:
            /* mov x29,sp */
            if (pos++ >= skip) context->Sp = context->Fp;
            if (local_size <= 512)
            {
                /* stp x29,lr,[sp,-#local_size]! */
                if (pos++ >= skip) restore_regs( 29, 2, -local_size_regs, context, ptrs );
                break;
            }
            /* stp x29,lr,[sp,0] */
            if (pos++ >= skip) restore_regs( 29, 2, 0, context, ptrs );
            /* fall through */
        case 0:
        case 1:
            if (!local_size) break;
            /* sub sp,sp,#local_size */
            if (pos++ >= skip) context->Sp += (local_size - 1) % 4088 + 1;
            if (local_size > 4088 && pos++ >= skip) context->Sp += 4088;
            break;
        }

        if (func->H) pos += 4;

        if (fp_size)
        {
            if (func->RegF % 2 == 0 && pos++ >= skip)
                /* str d%u,[sp,#fp_size] */
                restore_fpregs( 8 + func->RegF, 1, int_regs + fp_regs - 1, context, ptrs );
            for (i = (func->RegF + 1) / 2 - 1; i >= 0; i--)
            {
                if (pos++ < skip) continue;
                if (!i && !int_size)
                     /* stp d8,d9,[sp,-#regsave]! */
                    restore_fpregs( 8, 2, -saved_regs, context, ptrs );
                else
                     /* stp dn,dn+1,[sp,#offset] */
                    restore_fpregs( 8 + 2 * i, 2, int_regs + 2 * i, context, ptrs );
            }
        }

        if (func->RegI % 2)
        {
            if (pos++ >= skip)
            {
                /* stp xn,lr,[sp,#offset] */
                if (func->CR == 1) restore_regs( 30, 1, int_regs - 1, context, ptrs );
                /* str xn,[sp,#offset] */
                restore_regs( 18 + func->RegI, 1,
                              (func->RegI > 1) ? func->RegI - 1 : -saved_regs,
                              context, ptrs );
            }
        }
        else if (func->CR == 1)
        {
            /* str lr,[sp,#offset] */
            if (pos++ >= skip) restore_regs( 30, 1, func->RegI ? int_regs - 1 : -saved_regs, context, ptrs );
        }

        for (i = func->RegI / 2 - 1; i >= 0; i--)
        {
            if (pos++ < skip) continue;
            if (i)
                /* stp xn,xn+1,[sp,#offset] */
                restore_regs( 19 + 2 * i, 2, 2 * i, context, ptrs );
            else
                /* stp x19,x20,[sp,-#regsave]! */
                restore_regs( 19, 2, -saved_regs, context, ptrs );
        }
    }
    if (func->CR == 2) do_pac_auth( context );
    return NULL;
}


/***********************************************************************
 *           unwind_full_data
 */
static void *unwind_full_data( ULONG_PTR base, ULONG_PTR pc, RUNTIME_FUNCTION *func,
                               CONTEXT *context, PVOID *handler_data, KNONVOLATILE_CONTEXT_POINTERS *ptrs )
{
    struct unwind_info *info;
    struct unwind_info_epilog *info_epilog;
    unsigned int i, codes, epilogs, len, offset;
    void *data;
    BYTE *end;

    info = (struct unwind_info *)((char *)base + func->UnwindData);
    data = info + 1;
    epilogs = info->epilog;
    codes = info->codes;
    if (!codes && !epilogs)
    {
        struct unwind_info_ext *infoex = data;
        codes = infoex->codes;
        epilogs = infoex->epilog;
        data = infoex + 1;
    }
    info_epilog = data;
    if (!info->e) data = info_epilog + epilogs;

    offset = ((pc - base) - func->BeginAddress) / 4;
    end = (BYTE *)data + codes * 4;

    TRACE( "function %I64x-%I64x: len=%#x ver=%u X=%u E=%u epilogs=%u codes=%u\n",
           base + func->BeginAddress, base + func->BeginAddress + info->function_length * 4,
           info->function_length, info->version, info->x, info->e, epilogs, codes * 4 );

    /* check for prolog */
    if (offset < codes * 4)
    {
        len = get_sequence_len( data, end );
        if (offset < len)
        {
            process_unwind_codes( data, end, context, ptrs, len - offset );
            return NULL;
        }
    }

    /* check for epilog */
    if (!info->e)
    {
        for (i = 0; i < epilogs; i++)
        {
            if (offset < info_epilog[i].offset) break;
            if (offset - info_epilog[i].offset < codes * 4 - info_epilog[i].index)
            {
                BYTE *ptr = (BYTE *)data + info_epilog[i].index;
                len = get_sequence_len( ptr, end );
                if (offset <= info_epilog[i].offset + len)
                {
                    process_unwind_codes( ptr, end, context, ptrs, offset - info_epilog[i].offset );
                    return NULL;
                }
            }
        }
    }
    else if (info->function_length - offset <= codes * 4 - epilogs)
    {
        BYTE *ptr = (BYTE *)data + epilogs;
        len = get_sequence_len( ptr, end ) + 1;
        if (offset >= info->function_length - len)
        {
            process_unwind_codes( ptr, end, context, ptrs, offset - (info->function_length - len) );
            return NULL;
        }
    }

    process_unwind_codes( data, end, context, ptrs, 0 );

    /* get handler since we are inside the main code */
    if (info->x)
    {
        DWORD *handler_rva = (DWORD *)data + codes;
        *handler_data = handler_rva + 1;
        return (char *)base + *handler_rva;
    }
    return NULL;
}


/**********************************************************************
 *              RtlVirtualUnwind   (NTDLL.@)
 */
PVOID WINAPI RtlVirtualUnwind( ULONG type, ULONG_PTR base, ULONG_PTR pc,
                               RUNTIME_FUNCTION *func, CONTEXT *context,
                               PVOID *handler_data, ULONG_PTR *frame_ret,
                               KNONVOLATILE_CONTEXT_POINTERS *ctx_ptr )
{
    void *handler;

    TRACE( "type %lx pc %I64x sp %I64x func %I64x\n", type, pc, context->Sp, base + func->BeginAddress );

    *handler_data = NULL;

    context->Pc = 0;
    if (func->Flag)
        handler = unwind_packed_data( base, pc, func, context, ctx_ptr );
    else
        handler = unwind_full_data( base, pc, func, context, handler_data, ctx_ptr );

    TRACE( "ret: pc=%I64x lr=%I64x sp=%I64x handler=%p\n", context->Pc, context->Lr, context->Sp, handler );
    if (!context->Pc)
    {
        context->Pc = context->Lr;
        context->ContextFlags |= CONTEXT_UNWOUND_TO_CALL;
    }
    *frame_ret = context->Sp;
    return handler;
}

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
 * DW_OP_breg31; sleb128 <OFFSET>       | Load x31 + struct member offset
 * [DW_OP_deref]                        | Dereference, only for CFA
 */
extern void * WINAPI call_consolidate_callback( CONTEXT *context,
                                                void *(CALLBACK *callback)(EXCEPTION_RECORD *),
                                                EXCEPTION_RECORD *rec,
                                                TEB *teb );
__ASM_GLOBAL_FUNC( call_consolidate_callback,
                   "stp x29, x30, [sp, #-0x30]!\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset 48\n\t")
                   __ASM_CFI(".cfi_offset 29, -48\n\t")
                   __ASM_CFI(".cfi_offset 30, -40\n\t")
                   __ASM_SEH(".seh_nop\n\t")
                   "stp x1,  x2,  [sp, #0x10]\n\t"
                   __ASM_SEH(".seh_nop\n\t")
                   "str x3,       [sp, #0x20]\n\t"
                   __ASM_SEH(".seh_nop\n\t")
                   "mov x29, sp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register 29\n\t")
                   __ASM_SEH(".seh_nop\n\t")
                   __ASM_CFI(".cfi_remember_state\n\t")
                   /* Memcpy the context onto the stack */
                   "sub sp, sp, #0x390\n\t"
                   __ASM_SEH(".seh_nop\n\t")
                   "mov x1,  x0\n\t"
                   __ASM_SEH(".seh_nop\n\t")
                   "mov x0,  sp\n\t"
                   __ASM_SEH(".seh_nop\n\t")
                   "mov x2,  #0x390\n\t"
                   __ASM_SEH(".seh_nop\n\t")
                   "bl " __ASM_NAME("memcpy") "\n\t"
                   __ASM_CFI(".cfi_def_cfa 31, 0\n\t")
                   __ASM_CFI(".cfi_escape 0x0f,0x04,0x8f,0x80,0x02,0x06\n\t") /* CFA, DW_OP_breg31 + 0x100, DW_OP_deref */
                   __ASM_CFI(".cfi_escape 0x10,0x13,0x03,0x8f,0xa0,0x01\n\t") /* x19, DW_OP_breg31 + 0xA0 */
                   __ASM_CFI(".cfi_escape 0x10,0x14,0x03,0x8f,0xa8,0x01\n\t") /* x20 */
                   __ASM_CFI(".cfi_escape 0x10,0x15,0x03,0x8f,0xb0,0x01\n\t") /* x21 */
                   __ASM_CFI(".cfi_escape 0x10,0x16,0x03,0x8f,0xb8,0x01\n\t") /* x22 */
                   __ASM_CFI(".cfi_escape 0x10,0x17,0x03,0x8f,0xc0,0x01\n\t") /* x23 */
                   __ASM_CFI(".cfi_escape 0x10,0x18,0x03,0x8f,0xc8,0x01\n\t") /* x24 */
                   __ASM_CFI(".cfi_escape 0x10,0x19,0x03,0x8f,0xd0,0x01\n\t") /* x25 */
                   __ASM_CFI(".cfi_escape 0x10,0x1a,0x03,0x8f,0xd8,0x01\n\t") /* x26 */
                   __ASM_CFI(".cfi_escape 0x10,0x1b,0x03,0x8f,0xe0,0x01\n\t") /* x27 */
                   __ASM_CFI(".cfi_escape 0x10,0x1c,0x03,0x8f,0xe8,0x01\n\t") /* x28 */
                   __ASM_CFI(".cfi_escape 0x10,0x1d,0x03,0x8f,0xf0,0x01\n\t") /* x29 */
                   __ASM_CFI(".cfi_escape 0x10,0x1e,0x03,0x8f,0xf8,0x01\n\t") /* x30 */
                   __ASM_CFI(".cfi_escape 0x10,0x48,0x03,0x8f,0x90,0x03\n\t") /* d8  */
                   __ASM_CFI(".cfi_escape 0x10,0x49,0x03,0x8f,0x98,0x03\n\t") /* d9  */
                   __ASM_CFI(".cfi_escape 0x10,0x4a,0x03,0x8f,0xa0,0x03\n\t") /* d10 */
                   __ASM_CFI(".cfi_escape 0x10,0x4b,0x03,0x8f,0xa8,0x03\n\t") /* d11 */
                   __ASM_CFI(".cfi_escape 0x10,0x4c,0x03,0x8f,0xb0,0x03\n\t") /* d12 */
                   __ASM_CFI(".cfi_escape 0x10,0x4d,0x03,0x8f,0xb8,0x03\n\t") /* d13 */
                   __ASM_CFI(".cfi_escape 0x10,0x4e,0x03,0x8f,0xc0,0x03\n\t") /* d14 */
                   __ASM_CFI(".cfi_escape 0x10,0x4f,0x03,0x8f,0xc8,0x03\n\t") /* d15 */
                   __ASM_SEH(".seh_context\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   "ldp x1,  x2,  [x29, #0x10]\n\t"
                   "ldr x18,      [x29, #0x20]\n\t"
                   "mov x0,  x2\n\t"
                   "blr x1\n\t"
                   "mov sp,  x29\n\t"
                   __ASM_CFI(".cfi_restore_state\n\t")
                   "ldp x29, x30, [sp], #48\n\t"
                   __ASM_CFI(".cfi_restore 30\n\t")
                   __ASM_CFI(".cfi_restore 29\n\t")
                   __ASM_CFI(".cfi_def_cfa 31, 0\n\t")
                   "ret")

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

        context->X19  = jmp->X19;
        context->X20  = jmp->X20;
        context->X21  = jmp->X21;
        context->X22  = jmp->X22;
        context->X23  = jmp->X23;
        context->X24  = jmp->X24;
        context->X25  = jmp->X25;
        context->X26  = jmp->X26;
        context->X27  = jmp->X27;
        context->X28  = jmp->X28;
        context->Fp   = jmp->Fp;
        context->Lr   = jmp->Lr;
        context->Sp   = jmp->Sp;
        context->Fpcr = jmp->Fpcr;
        context->Fpsr = jmp->Fpsr;

        for (i = 0; i < 8; i++)
            context->V[8+i].D[0] = jmp->D[i];
    }
    else if (rec && rec->ExceptionCode == STATUS_UNWIND_CONSOLIDATE && rec->NumberParameters >= 1)
    {
        PVOID (CALLBACK *consolidate)(EXCEPTION_RECORD *) = (void *)rec->ExceptionInformation[0];
        TRACE( "calling consolidate callback %p (rec=%p)\n", consolidate, rec );
        rec->ExceptionInformation[10] = (ULONG_PTR)&context->X19;

        context->Pc = (ULONG64)call_consolidate_callback( context, consolidate, rec, NtCurrentTeb() );
    }

    /* hack: remove no longer accessible TEB frames */
    while (is_valid_frame( (ULONG_PTR)teb_frame ) && (ULONG64)teb_frame < context->Sp)
    {
        TRACE( "removing TEB frame: %p\n", teb_frame );
        teb_frame = __wine_pop_frame( teb_frame );
    }

    TRACE( "returning to %I64x stack %I64x\n", context->Pc, context->Sp );
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

    TRACE( "code=%lx flags=%lx end_frame=%p target_ip=%p pc=%016I64x\n",
           rec->ExceptionCode, rec->ExceptionFlags, end_frame, target_ip, context->Pc );
    for (i = 0; i < min( EXCEPTION_MAXIMUM_PARAMETERS, rec->NumberParameters ); i++)
        TRACE( " info[%ld]=%016I64x\n", i, rec->ExceptionInformation[i] );
    TRACE("  x0=%016I64x  x1=%016I64x  x2=%016I64x  x3=%016I64x\n",
          context->X0, context->X1, context->X2, context->X3 );
    TRACE("  x4=%016I64x  x5=%016I64x  x6=%016I64x  x7=%016I64x\n",
          context->X4, context->X5, context->X6, context->X7 );
    TRACE("  x8=%016I64x  x9=%016I64x x10=%016I64x x11=%016I64x\n",
          context->X8, context->X9, context->X10, context->X11 );
    TRACE(" x12=%016I64x x13=%016I64x x14=%016I64x x15=%016I64x\n",
          context->X12, context->X13, context->X14, context->X15 );
    TRACE(" x16=%016I64x x17=%016I64x x18=%016I64x x19=%016I64x\n",
          context->X16, context->X17, context->X18, context->X19 );
    TRACE(" x20=%016I64x x21=%016I64x x22=%016I64x x23=%016I64x\n",
          context->X20, context->X21, context->X22, context->X23 );
    TRACE(" x24=%016I64x x25=%016I64x x26=%016I64x x27=%016I64x\n",
          context->X24, context->X25, context->X26, context->X27 );
    TRACE(" x28=%016I64x  fp=%016I64x  lr=%016I64x  sp=%016I64x\n",
          context->X28, context->Fp, context->Lr, context->Sp );

    dispatch.TargetPc         = (ULONG64)target_ip;
    dispatch.ContextRecord    = context;
    dispatch.HistoryTable     = table;
    dispatch.NonVolatileRegisters = (BYTE *)&context->X19;

    for (;;)
    {
        status = virtual_unwind( UNW_FLAG_UHANDLER, &dispatch, &new_context );
        if (status != STATUS_SUCCESS) raise_status( status, rec );

    unwind_done:
        if (!dispatch.EstablisherFrame) break;

        if (!is_valid_frame( dispatch.EstablisherFrame ))
        {
            ERR( "invalid frame %I64x (%p-%p)\n", dispatch.EstablisherFrame,
                 NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
            rec->ExceptionFlags |= EH_STACK_INVALID;
            break;
        }

        if (dispatch.LanguageHandler)
        {
            if (end_frame && (dispatch.EstablisherFrame > (ULONG64)end_frame))
            {
                ERR( "invalid end frame %I64x/%p\n", dispatch.EstablisherFrame, end_frame );
                raise_status( STATUS_INVALID_UNWIND_TARGET, rec );
            }
            if (dispatch.EstablisherFrame == (ULONG64)end_frame) rec->ExceptionFlags |= EH_TARGET_UNWIND;
            if (call_unwind_handler( rec, &dispatch ) == ExceptionCollidedUnwind)
            {
                ULONG64 frame;

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
            DWORD64 backup_frame = dispatch.EstablisherFrame;
            while (is_valid_frame( (ULONG_PTR)teb_frame ) &&
                   (ULONG64)teb_frame < new_context.Sp &&
                   (ULONG64)teb_frame < (ULONG64)end_frame)
            {
                TRACE( "found builtin frame %p handler %p\n", teb_frame, teb_frame->Handler );
                dispatch.EstablisherFrame = (ULONG64)teb_frame;
                if (call_teb_unwind_handler( rec, &dispatch, teb_frame ) == ExceptionCollidedUnwind)
                {
                    ULONG64 frame;

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
            if ((ULONG64)teb_frame == (ULONG64)end_frame && (ULONG64)end_frame < new_context.Sp) break;
            dispatch.EstablisherFrame = backup_frame;
        }

        if (dispatch.EstablisherFrame == (ULONG64)end_frame) break;
        *context = new_context;
    }

    context->X0 = (ULONG64)retval;
    context->Pc = (ULONG64)target_ip;
    RtlRestoreContext(context, rec);
}


/***********************************************************************
 *            RtlUnwind  (NTDLL.@)
 */
void WINAPI RtlUnwind( void *frame, void *target_ip, EXCEPTION_RECORD *rec, void *retval )
{
    CONTEXT context;
    RtlUnwindEx( frame, target_ip, rec, retval, &context, NULL );
}

/*******************************************************************
 *		_local_unwind (NTDLL.@)
 */
void WINAPI _local_unwind( void *frame, void *target_ip )
{
    CONTEXT context;
    RtlUnwindEx( frame, target_ip, NULL, NULL, &context, NULL );
}

extern LONG __C_ExecuteExceptionFilter(PEXCEPTION_POINTERS ptrs, PVOID frame,
                                       PEXCEPTION_FILTER filter,
                                       PUCHAR nonvolatile);
__ASM_GLOBAL_FUNC( __C_ExecuteExceptionFilter,
                   "stp x29, x30, [sp, #-96]!\n\t"
                   __ASM_SEH(".seh_save_fplr_x 96\n\t")
                   "stp x19, x20, [sp, #16]\n\t"
                   __ASM_SEH(".seh_save_regp x19, 16\n\t")
                   "stp x21, x22, [sp, #32]\n\t"
                   __ASM_SEH(".seh_save_regp x21, 32\n\t")
                   "stp x23, x24, [sp, #48]\n\t"
                   __ASM_SEH(".seh_save_regp x23, 48\n\t")
                   "stp x25, x26, [sp, #64]\n\t"
                   __ASM_SEH(".seh_save_regp x25, 64\n\t")
                   "stp x27, x28, [sp, #80]\n\t"
                   __ASM_SEH(".seh_save_regp x27, 80\n\t")
                   "mov x29, sp\n\t"
                   __ASM_SEH(".seh_set_fp\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")

                   __ASM_CFI(".cfi_def_cfa x29, 96\n\t")
                   __ASM_CFI(".cfi_offset x29, -96\n\t")
                   __ASM_CFI(".cfi_offset x30, -88\n\t")
                   __ASM_CFI(".cfi_offset x19, -80\n\t")
                   __ASM_CFI(".cfi_offset x20, -72\n\t")
                   __ASM_CFI(".cfi_offset x21, -64\n\t")
                   __ASM_CFI(".cfi_offset x22, -56\n\t")
                   __ASM_CFI(".cfi_offset x23, -48\n\t")
                   __ASM_CFI(".cfi_offset x24, -40\n\t")
                   __ASM_CFI(".cfi_offset x25, -32\n\t")
                   __ASM_CFI(".cfi_offset x26, -24\n\t")
                   __ASM_CFI(".cfi_offset x27, -16\n\t")
                   __ASM_CFI(".cfi_offset x28, -8\n\t")

                   "ldp x19, x20, [x3, #0]\n\t"
                   "ldp x21, x22, [x3, #16]\n\t"
                   "ldp x23, x24, [x3, #32]\n\t"
                   "ldp x25, x26, [x3, #48]\n\t"
                   "ldp x27, x28, [x3, #64]\n\t"
                   /* Overwrite the frame parameter with Fp from the
                    * nonvolatile regs */
                   "ldr x1, [x3, #80]\n\t"
                   "blr x2\n\t"
                   "ldp x19, x20, [sp, #16]\n\t"
                   "ldp x21, x22, [sp, #32]\n\t"
                   "ldp x23, x24, [sp, #48]\n\t"
                   "ldp x25, x26, [sp, #64]\n\t"
                   "ldp x27, x28, [sp, #80]\n\t"
                   "ldp x29, x30, [sp], #96\n\t"
                   "ret")

extern void __C_ExecuteTerminationHandler(BOOL abnormal, PVOID frame,
                                          PTERMINATION_HANDLER handler,
                                          PUCHAR nonvolatile);
/* This is, implementation wise, identical to __C_ExecuteExceptionFilter. */
__ASM_GLOBAL_FUNC( __C_ExecuteTerminationHandler,
                   "b " __ASM_NAME("__C_ExecuteExceptionFilter") "\n\t");

/*******************************************************************
 *		__C_specific_handler (NTDLL.@)
 */
EXCEPTION_DISPOSITION WINAPI __C_specific_handler( EXCEPTION_RECORD *rec,
                                                   void *frame,
                                                   CONTEXT *context,
                                                   struct _DISPATCHER_CONTEXT *dispatch )
{
    SCOPE_TABLE *table = dispatch->HandlerData;
    ULONG i;
    DWORD64 ControlPc = dispatch->ControlPc;

    TRACE( "%p %p %p %p\n", rec, frame, context, dispatch );
    if (TRACE_ON(seh)) dump_scope_table( dispatch->ImageBase, table );

    if (dispatch->ControlPcIsUnwound)
        ControlPc -= 4;

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
            TRACE( "unwinding to target %I64x\n", dispatch->ImageBase + table->ScopeRecord[i].JumpTarget );
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
                   "sub sp, sp, #0x3b0\n\t" /* 0x390 (context) + 0x20 */
                   "stp x29, x30, [sp]\n\t"
                   __ASM_SEH(".seh_stackalloc 0x3b0\n\t")
                   __ASM_SEH(".seh_save_fplr 0\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   __ASM_CFI(".cfi_def_cfa x29, 944\n\t")
                   __ASM_CFI(".cfi_offset x30, -936\n\t")
                   __ASM_CFI(".cfi_offset x29, -944\n\t")
                   "mov x29, sp\n\t"
                   "str x0,  [sp, #0x10]\n\t"
                   "add x0,  sp, #0x20\n\t"
                   "bl " __ASM_NAME("RtlCaptureContext") "\n\t"
                   "add x1,  sp, #0x20\n\t"      /* context pointer */
                   "add x2,  sp, #0x3b0\n\t"     /* orig stack pointer */
                   "str x2,  [x1, #0x100]\n\t"   /* context->Sp */
                   "ldr x0,  [sp, #0x10]\n\t"    /* original first parameter */
                   "str x0,  [x1, #0x08]\n\t"    /* context->X0 */
                   "ldp x4, x5, [sp]\n\t"        /* frame pointer, return address */
                   "stp x4, x5, [x1, #0xf0]\n\t" /* context->Fp, Lr */
                   "str  x5, [x1, #0x108]\n\t"   /* context->Pc */
                   "str  x5, [x0, #0x10]\n\t"    /* rec->ExceptionAddress */
                   "ldr x3, [x18, #0x60]\n\t"    /* peb */
                   "ldrb w2, [x3, #2]\n\t"       /* peb->BeingDebugged */
                   "cbnz w2, 1f\n\t"
                   "bl " __ASM_NAME("dispatch_exception") "\n"
                   "1:\tmov  x2, #1\n\t"
                   "bl " __ASM_NAME("NtRaiseException") "\n\t"
                   "bl " __ASM_NAME("RtlRaiseStatus") /* does not return */ );

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
                   "stp x29, x30, [sp, #-16]!\n\t"
                   __ASM_SEH(".seh_save_fplr_x 16\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   "adr x8, " __ASM_NAME("pBaseThreadInitThunk") "\n\t"
                   "ldr x8, [x8]\n\t"
                   "mov x2, x1\n\t"
                   "mov x1, x0\n\t"
                   "mov x0, #0\n\t"
                   "blr x8\n\t"
                   __ASM_SEH(".seh_handler " __ASM_NAME("call_unhandled_exception_handler") ", @except") )

/******************************************************************
 *		LdrInitializeThunk (NTDLL.@)
 */
void WINAPI LdrInitializeThunk( CONTEXT *context, ULONG_PTR unk2, ULONG_PTR unk3, ULONG_PTR unk4 )
{
    loader_init( context, (void **)&context->X0 );
    TRACE_(relay)( "\1Starting thread proc %p (arg=%p)\n", (void *)context->X0, (void *)context->X1 );
    NtContinue( context, TRUE );
}


/***********************************************************************
 *           process_breakpoint
 */
__ASM_GLOBAL_FUNC( process_breakpoint,
                   ".seh_endprologue\n\t"
                   ".seh_handler process_breakpoint_handler, @except\n\t"
                   "brk #0xf000\n\t"
                   "ret\n"
                   "process_breakpoint_handler:\n\t"
                   "ldr x4, [x2, #0x108]\n\t" /* context->Pc */
                   "add x4, x4, #4\n\t"
                   "str x4, [x2, #0x108]\n\t"
                   "mov w0, #0\n\t"           /* ExceptionContinueExecution */
                   "ret" )

/***********************************************************************
 *		DbgUiRemoteBreakin   (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( DbgUiRemoteBreakin,
                   "stp x29, x30, [sp, #-16]!\n\t"
                   ".seh_save_fplr_x 16\n\t"
                   ".seh_endprologue\n\t"
                   ".seh_handler DbgUiRemoteBreakin_handler, @except\n\t"
                   "ldr x0, [x18, #0x60]\n\t"       /* NtCurrentTeb()->Peb */
                   "ldrb w0, [x0, 0x02]\n\t"        /* peb->BeingDebugged */
                   "cbz w0, 1f\n\t"
                   "bl " __ASM_NAME("DbgBreakPoint") "\n"
                   "1:\tmov w0, #0\n\t"
                   "bl " __ASM_NAME("RtlExitUserThread") "\n"
                   "DbgUiRemoteBreakin_handler:\n\t"
                   "mov sp, x1\n\t"                 /* frame */
                   "b 1b" )

/**********************************************************************
 *              DbgBreakPoint   (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( DbgBreakPoint, "brk #0xf000; ret"
                    "\n\tnop; nop; nop; nop; nop; nop; nop; nop"
                    "\n\tnop; nop; nop; nop; nop; nop" );

/**********************************************************************
 *              DbgUserBreakPoint   (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( DbgUserBreakPoint, "brk #0xf000; ret"
                    "\n\tnop; nop; nop; nop; nop; nop; nop; nop"
                    "\n\tnop; nop; nop; nop; nop; nop" );

#endif  /* __aarch64__ */
