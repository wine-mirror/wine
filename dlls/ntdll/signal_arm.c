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
#include <setjmp.h>

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


/* undocumented, copied from the corresponding ARM64 structure */
typedef union _DISPATCHER_CONTEXT_NONVOLREG_ARM
{
    BYTE  Buffer[8 * sizeof(DWORD) + 8 * sizeof(double)];
    struct
    {
        DWORD  GpNvRegs[8];
        double FpNvRegs[8];
    } DUMMYSTRUCTNAME;
} DISPATCHER_CONTEXT_NONVOLREG_ARM;


/*******************************************************************
 *         syscalls
 */
#define SYSCALL_ENTRY(id,name,args) __ASM_SYSCALL_FUNC( id, name, args )
ALL_SYSCALLS
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
                    "add r0, #0x44\n\t"        /* 0x50 - 0x0c */
                    "vstm r0, {d0-d15}\n\t"    /* context->D0-D15 */
                    "bx lr" )


/**********************************************************************
 *           virtual_unwind
 */
static NTSTATUS virtual_unwind( ULONG type, DISPATCHER_CONTEXT *dispatch, CONTEXT *context )
{
    DISPATCHER_CONTEXT_NONVOLREG_ARM *nonvol_regs;
    DWORD pc = context->Pc;

    dispatch->ScopeIndex = 0;
    dispatch->ControlPc  = pc;
    dispatch->ControlPcIsUnwound = (context->ContextFlags & CONTEXT_UNWOUND_TO_CALL) != 0;
    if (dispatch->ControlPcIsUnwound) pc -= 2;

    nonvol_regs = (DISPATCHER_CONTEXT_NONVOLREG_ARM *)dispatch->NonVolatileRegisters;
    memcpy( nonvol_regs->GpNvRegs, &context->R4, sizeof(nonvol_regs->GpNvRegs) );
    memcpy( nonvol_regs->FpNvRegs, &context->D[8], sizeof(nonvol_regs->FpNvRegs) );

    dispatch->FunctionEntry = RtlLookupFunctionEntry( pc, (DWORD_PTR *)&dispatch->ImageBase,
                                                      dispatch->HistoryTable );
    if (RtlVirtualUnwind2( type, dispatch->ImageBase, pc, dispatch->FunctionEntry, context,
                           NULL, &dispatch->HandlerData, (ULONG_PTR *)&dispatch->EstablisherFrame,
                           NULL, NULL, NULL, &dispatch->LanguageHandler, 0 ))
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
    dispatch->ControlPc          = orig_dispatch->ControlPc;
    dispatch->ImageBase          = orig_dispatch->ImageBase;
    dispatch->FunctionEntry      = orig_dispatch->FunctionEntry;
    dispatch->EstablisherFrame   = orig_dispatch->EstablisherFrame;
    dispatch->LanguageHandler    = orig_dispatch->LanguageHandler;
    dispatch->HandlerData        = orig_dispatch->HandlerData;
    dispatch->HistoryTable       = orig_dispatch->HistoryTable;
    dispatch->ScopeIndex         = orig_dispatch->ScopeIndex;
    dispatch->ControlPcIsUnwound = orig_dispatch->ControlPcIsUnwound;
    *dispatch->ContextRecord     = *orig_dispatch->ContextRecord;
    memcpy( dispatch->NonVolatileRegisters, orig_dispatch->NonVolatileRegisters,
            sizeof(DISPATCHER_CONTEXT_NONVOLREG_ARM) );
    TRACE( "detected collided unwind\n" );
    return ExceptionCollidedUnwind;
}


/**********************************************************************
 *           call_unwind_handler
 */
DWORD WINAPI call_unwind_handler( EXCEPTION_RECORD *rec, ULONG_PTR frame,
                                  CONTEXT *context, void *dispatch, PEXCEPTION_ROUTINE handler );
__ASM_GLOBAL_FUNC( call_unwind_handler,
                   "push {r3,lr}\n\t"
                   ".seh_save_regs {r3,lr}\n\t"
                   ".seh_endprologue\n\t"
                   ".seh_handler unwind_exception_handler, %except\n\t"
                   "ldr ip, [sp, #8]\n\t"  /* handler */
                   "blx ip\n\t"
                   "pop {r3,pc}\n\t" )


/***********************************************************************
 *		call_seh_handler
 */
DWORD WINAPI call_seh_handler( EXCEPTION_RECORD *rec, ULONG_PTR frame,
                               CONTEXT *context, void *dispatch, PEXCEPTION_ROUTINE handler );
__ASM_GLOBAL_FUNC( call_seh_handler,
                   "push {r4,lr}\n\t"
                   ".seh_save_regs {r4,lr}\n\t"
                   ".seh_endprologue\n\t"
                   ".seh_handler nested_exception_handler, %except\n\t"
                   "ldr ip, [sp, #8]\n\t"  /* handler */
                   "blx ip\n\t"
                   "pop {r4,pc}\n\t" )


/**********************************************************************
 *           call_seh_handlers
 *
 * Call the SEH handlers.
 */
NTSTATUS call_seh_handlers( EXCEPTION_RECORD *rec, CONTEXT *orig_context )
{
    EXCEPTION_REGISTRATION_RECORD *teb_frame = NtCurrentTeb()->Tib.ExceptionList;
    DISPATCHER_CONTEXT_NONVOLREG_ARM nonvol_regs;
    UNWIND_HISTORY_TABLE table;
    DISPATCHER_CONTEXT dispatch;
    CONTEXT context;
    NTSTATUS status;
    ULONG_PTR frame;
    DWORD res;

    context = *orig_context;
    dispatch.TargetPc      = 0;
    dispatch.ContextRecord = &context;
    dispatch.HistoryTable  = &table;
    dispatch.NonVolatileRegisters = nonvol_regs.Buffer;

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
            rec->ExceptionFlags |= EXCEPTION_STACK_INVALID;
            break;
        }

        if (dispatch.LanguageHandler)
        {
            TRACE( "calling handler %p (rec=%p, frame=%lx context=%p, dispatch=%p)\n",
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
                RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                                  dispatch.ControlPc, dispatch.FunctionEntry,
                                  &context, (PVOID *)&dispatch.HandlerData, &frame, NULL );
                goto unwind_done;
            default:
                return STATUS_INVALID_DISPOSITION;
            }
        }
        /* hack: call wine handlers registered in the tib list */
        else while (is_valid_frame( (ULONG_PTR)teb_frame ) && (DWORD)teb_frame < context.Sp)
        {
            TRACE( "calling TEB handler %p (rec=%p frame=%p context=%p dispatch=%p) sp=%lx\n",
                   teb_frame->Handler, rec, teb_frame, orig_context, &dispatch, context.Sp );
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
                RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                                  dispatch.ControlPc, dispatch.FunctionEntry,
                                  &context, (PVOID *)&dispatch.HandlerData, &frame, NULL );
                teb_frame = teb_frame->Prev;
                goto unwind_done;
            default:
                return STATUS_INVALID_DISPOSITION;
            }
            teb_frame = teb_frame->Prev;
        }

        if (context.Sp == (DWORD)NtCurrentTeb()->Tib.StackBase) break;
    }
    return STATUS_UNHANDLED_EXCEPTION;
}


/*******************************************************************
 *		KiUserExceptionDispatcher (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( KiUserExceptionDispatcher,
                   ".seh_custom 0xee,0x02\n\t"  /* MSFT_OP_CONTEXT */
                   ".seh_endprologue\n\t"
                   "add r0, sp, #0x1a0\n\t"     /* rec (context + 1) */
                   "mov r1, sp\n\t"             /* context */
                   "bl dispatch_exception\n\t"
                   "udf #1" )


/*******************************************************************
 *		KiUserApcDispatcher (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( KiUserApcDispatcher,
                   ".seh_custom 0xee,0x02\n\t"  /* MSFT_OP_CONTEXT */
                   "nop\n\t"
                   ".seh_stackalloc 0x18\n\t"
                   ".seh_endprologue\n\t"
                   "ldr r0, [sp, #0x04]\n\t"      /* arg1 */
                   "ldr r1, [sp, #0x08]\n\t"      /* arg2 */
                   "ldr r2, [sp, #0x0c]\n\t"      /* arg3 */
                   "ldr ip, [sp]\n\t"             /* func */
                   "blx ip\n\t"
                   "add r0, sp, #0x18\n\t"        /* context */
                   "ldr r1, [sp, #0x10]\n\t"      /* alertable */
                   "bl NtContinue\n\t"
                   "udf #1" )


/*******************************************************************
 *		KiUserCallbackDispatcher (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( KiUserCallbackDispatcher,
                   ".seh_custom 0xee,0x01\n\t"  /* MSFT_OP_MACHINE_FRAME */
                   "nop\n\t"
                   ".seh_save_regs {lr}\n\t"
                   "nop\n\t"
                   ".seh_stackalloc 0xc\n\t"
                   ".seh_endprologue\n\t"
                   "ldr r0, [sp]\n\t"               /* args */
                   "ldr r1, [sp, #0x04]\n\t"        /* len */
                   "ldr r2, [sp, #0x08]\n\t"        /* id */
                   "mrc p15, 0, r3, c13, c0, 2\n\t" /* NtCurrentTeb() */
                   "ldr r3, [r3, 0x30]\n\t"         /* peb */
                   "ldr r3, [r3, 0x2c]\n\t"         /* peb->KernelCallbackTable */
                   "ldr ip, [r3, r2, lsl #2]\n\t"
                   "blx ip\n\t"
                   ".seh_handler user_callback_handler, %except\n\t"
                   ".globl KiUserCallbackDispatcherReturn\n"
                   "KiUserCallbackDispatcherReturn:\n\t"
                   "mov r2, r0\n\t"  /* status */
                   "mov r1, #0\n\t"  /* ret_len */
                   "mov r0, r1\n\t"  /* ret_ptr */
                   "bl NtCallbackReturn\n\t"
                   "bl RtlRaiseStatus\n\t"
                   "udf #1" )



/**********************************************************************
 *           consolidate_callback
 *
 * Wrapper function to call a consolidate callback from a fake frame.
 * If the callback executes RtlUnwindEx (like for example done in C++ handlers),
 * we have to skip all frames which were already processed. To do that we
 * trick the unwinding functions into thinking the call came from somewhere
 * else.
 */
void WINAPI DECLSPEC_NORETURN consolidate_callback( CONTEXT *context,
                                                    void *(CALLBACK *callback)(EXCEPTION_RECORD *),
                                                    EXCEPTION_RECORD *rec );
__ASM_GLOBAL_FUNC( consolidate_callback,
                   "push {r0-r2,lr}\n\t"
                   ".seh_nop\n\t"
                   "sub sp, sp, #0x1a0\n\t"
                   ".seh_nop\n\t"
                   "mov r1, r0\n\t"
                   ".seh_nop\n\t"
                   "mov r0, sp\n\t"
                   ".seh_nop\n\t"
                   "mov r2, #0x1a0\n\t"
                   ".seh_nop_w\n\t"
                   "bl memcpy\n\t"
                   ".seh_custom 0xee,0x02\n\t"  /* MSFT_OP_CONTEXT */
                   ".seh_endprologue\n\t"
                   "ldrd r1, r2, [sp, #0x1a4]\n\t"
                   "mov r0, r2\n\t"
                   "blx r1\n\t"
                   "str r0, [sp, #0x40]\n\t" /* context->Pc */
                   "mov r0, sp\n\t"
                   "mov r1, #1\n\t"
                   "b NtContinue" )



/*******************************************************************
 *              RtlRestoreContext (NTDLL.@)
 */
void CDECL RtlRestoreContext( CONTEXT *context, EXCEPTION_RECORD *rec )
{
    EXCEPTION_REGISTRATION_RECORD *teb_frame = NtCurrentTeb()->Tib.ExceptionList;

    if (rec && rec->ExceptionCode == STATUS_LONGJUMP && rec->NumberParameters >= 1)
    {
        struct _JUMP_BUFFER *jmp = (struct _JUMP_BUFFER *)rec->ExceptionInformation[0];

        memcpy( &context->R4, &jmp->R4, 8 * sizeof(DWORD) );
        memcpy( &context->D[8], &jmp->D[0], 8 * sizeof(ULONGLONG) );
        context->Pc      = jmp->Pc;
        context->Sp      = jmp->Sp;
        context->Fpscr   = jmp->Fpscr;
    }
    else if (rec && rec->ExceptionCode == STATUS_UNWIND_CONSOLIDATE && rec->NumberParameters >= 1)
    {
        PVOID (CALLBACK *consolidate)(EXCEPTION_RECORD *) = (void *)rec->ExceptionInformation[0];
        TRACE( "calling consolidate callback %p (rec=%p)\n", consolidate, rec );
        consolidate_callback( context, consolidate, rec );
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
    DISPATCHER_CONTEXT_NONVOLREG_ARM nonvol_regs;
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
        record.ExceptionAddress = (void *)context->Pc;
        record.NumberParameters = 0;
        rec = &record;
    }

    rec->ExceptionFlags |= EXCEPTION_UNWINDING | (end_frame ? 0 : EXCEPTION_EXIT_UNWIND);

    TRACE( "code=%lx flags=%lx end_frame=%p target_ip=%p\n",
           rec->ExceptionCode, rec->ExceptionFlags, end_frame, target_ip );
    for (i = 0; i < min( EXCEPTION_MAXIMUM_PARAMETERS, rec->NumberParameters ); i++)
        TRACE( " info[%ld]=%08Ix\n", i, rec->ExceptionInformation[i] );
    TRACE_CONTEXT( context );

    dispatch.TargetPc         = (ULONG_PTR)target_ip;
    dispatch.ContextRecord    = context;
    dispatch.HistoryTable     = table;
    dispatch.NonVolatileRegisters = nonvol_regs.Buffer;

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
            rec->ExceptionFlags |= EXCEPTION_STACK_INVALID;
            break;
        }

        if (dispatch.LanguageHandler)
        {
            if (end_frame && (dispatch.EstablisherFrame > (DWORD)end_frame))
            {
                ERR( "invalid end frame %lx/%p\n", dispatch.EstablisherFrame, end_frame );
                raise_status( STATUS_INVALID_UNWIND_TARGET, rec );
            }
            if (dispatch.EstablisherFrame == (DWORD)end_frame) rec->ExceptionFlags |= EXCEPTION_TARGET_UNWIND;

            TRACE( "calling handler %p (rec=%p, frame=0x%lx context=%p, dispatch=%p)\n",
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
                new_context = *context;
                RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                                  dispatch.ControlPc, dispatch.FunctionEntry,
                                  &new_context, &dispatch.HandlerData, &frame,
                                  NULL );
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
                   (DWORD)teb_frame < new_context.Sp &&
                   (DWORD)teb_frame < (DWORD)end_frame)
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
            if ((DWORD)teb_frame == (DWORD)end_frame && (DWORD)end_frame < new_context.Sp) break;
        }

        if (dispatch.EstablisherFrame == (DWORD)end_frame) break;
        *context = new_context;
    }

    if (rec->ExceptionCode != STATUS_UNWIND_CONSOLIDATE)
        context->Pc = (DWORD)target_ip;
    else if (rec->ExceptionInformation[10] == -1)
        rec->ExceptionInformation[10] = (ULONG_PTR)&nonvol_regs;

    context->R0 = (DWORD)retval;
    RtlRestoreContext(context, rec);
}


/*************************************************************************
 *		RtlGetNativeSystemInformation (NTDLL.@)
 */
NTSTATUS WINAPI RtlGetNativeSystemInformation( SYSTEM_INFORMATION_CLASS class,
                                               void *info, ULONG size, ULONG *ret_size )
{
    return NtWow64GetNativeSystemInformation( class, info, size, ret_size );
}


/***********************************************************************
 *           RtlIsProcessorFeaturePresent [NTDLL.@]
 */
BOOLEAN WINAPI RtlIsProcessorFeaturePresent( UINT feature )
{
    return NtWow64IsProcessorFeaturePresent( feature );
}


/*************************************************************************
 *		RtlWalkFrameChain (NTDLL.@)
 */
ULONG WINAPI RtlWalkFrameChain( void **buffer, ULONG count, ULONG flags )
{
    UNWIND_HISTORY_TABLE table;
    RUNTIME_FUNCTION *func;
    PEXCEPTION_ROUTINE handler;
    ULONG_PTR pc, frame, base;
    CONTEXT context;
    void *data;
    ULONG i, skip = flags >> 8, num_entries = 0;

    RtlCaptureContext( &context );

    for (i = 0; i < count; i++)
    {
        pc = context.Pc;
        if (context.ContextFlags & CONTEXT_UNWOUND_TO_CALL) pc -= 2;
        func = RtlLookupFunctionEntry( pc, &base, &table );
        if (RtlVirtualUnwind2( UNW_FLAG_NHANDLER, base, pc, func, &context, NULL,
                               &data, &frame, NULL, NULL, NULL, &handler, 0 ))
            break;
        if (!context.Pc) break;
        if (!frame || !is_valid_frame( frame )) break;
        if (context.Sp == (ULONG_PTR)NtCurrentTeb()->Tib.StackBase) break;
        if (i >= skip) buffer[num_entries++] = (void *)context.Pc;
    }
    return num_entries;
}


/*******************************************************************
 *              __C_ExecuteExceptionFilter
 */
__ASM_GLOBAL_FUNC( __C_ExecuteExceptionFilter,
                   "push {r3-r11,lr}\n\t"
                   ".seh_save_regs_w {r3-r11,lr}\n\t"
                   ".seh_endprologue\n\t"
                   "ldm r3, {r4-r11}\n\t"
                   "blx r2\n\t"
                   "pop {r3-r11,pc}\n\t" )


/***********************************************************************
 *		RtlRaiseException (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( RtlRaiseException,
                    "push {r0, lr}\n\t"
                    ".seh_save_regs {r0, lr}\n\t"
                    "sub sp, sp, #0x1a0\n\t"  /* sizeof(CONTEXT) */
                    ".seh_stackalloc 0x1a0\n\t"
                    ".seh_endprologue\n\t"
                    "mov r0, sp\n\t"  /* context */
                    "bl RtlCaptureContext\n\t"
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
                    "ldr r1, [sp]\n\t"         /* context->ContextFlags */
                    "orr r1, r1, #0x20000000\n\t" /* CONTEXT_UNWOUND_TO_CALL */
                    "str r1, [sp]\n\t"
                    "mov r1, sp\n\t"
                    "mrc p15, 0, r3, c13, c0, 2\n\t" /* NtCurrentTeb() */
                    "ldr r3, [r3, #0x30]\n\t"  /* peb */
                    "ldrb r2, [r3, #2]\n\t"    /* peb->BeingDebugged */
                    "cbnz r2, 1f\n\t"
                    "bl dispatch_exception\n"
                    "1:\tmov r2, #1\n\t"
                    "bl NtRaiseException\n\t"
                    "bl RtlRaiseStatus" )


/***********************************************************************
 *           _setjmp (NTDLL.@)
 *           _setjmpex (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( NTDLL__setjmpex,
                   ".seh_endprologue\n\t"
                   "stm r0, {r1,r4-r11}\n"         /* jmp_buf->Frame,R4..R11 */
                   "str sp, [r0, #0x24]\n\t"       /* jmp_buf->Sp */
                   "str lr, [r0, #0x28]\n\t"       /* jmp_buf->Pc */
                   "vmrs r2, fpscr\n\t"
                   "str r2, [r0, #0x2c]\n\t"       /* jmp_buf->Fpscr */
                   "add r0, r0, #0x30\n\t"
                   "vstm r0, {d8-d15}\n\t"         /* jmp_buf->D[0..7] */
                   "mov r0, #0\n\t"
                   "bx lr" )


/*******************************************************************
 *		longjmp (NTDLL.@)
 */
void __cdecl NTDLL_longjmp( _JUMP_BUFFER *buf, int retval )
{
    EXCEPTION_RECORD rec;

    if (!retval) retval = 1;

    rec.ExceptionCode = STATUS_LONGJUMP;
    rec.ExceptionFlags = 0;
    rec.ExceptionRecord = NULL;
    rec.ExceptionAddress = NULL;
    rec.NumberParameters = 1;
    rec.ExceptionInformation[0] = (DWORD_PTR)buf;
    RtlUnwind( (void *)buf->Frame, (void *)buf->Pc, &rec, IntToPtr(retval) );
}


/***********************************************************************
 *           RtlUserThreadStart (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( RtlUserThreadStart,
                    "push {r4, lr}\n\t"
                   ".seh_save_regs {r4, lr}\n\t"
                   ".seh_endprologue\n\t"
                   "mov r2, r1\n\t"
                   "mov r1, r0\n\t"
                   "mov r0, #0\n\t"
                   "ldr ip, 1f\n\t"
                   "ldr ip, [ip]\n\t"
                   "blx ip\n\t"
                   "nop\n"
                   "1:\t.long pBaseThreadInitThunk\n\t"
                   ".seh_handler call_unhandled_exception_handler, %except" )

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
                   "bl DbgBreakPoint\n"
                   "1:\tmov r0, #0\n\t"
                   "bl RtlExitUserThread\n"
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
