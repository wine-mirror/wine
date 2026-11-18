/*
 * RISC-V64 native signal handling and syscall entry points.
 *
 * This file provides build-enabling stubs for the native RISC-V64 port.
 * Context handling, exception dispatch, unwinding, and thread startup
 * require proper implementations.
 */

#if defined(__riscv) && (__riscv_xlen == 64)

#include <stdarg.h>
#include <stddef.h>
#include "ntstatus.h"
#include "windef.h"
#include "winternl.h"
#include "ddk/wdm.h"
#include "wine/asm.h"
#include "ntdll_misc.h"
#include "ntsyscalls.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(riscv64);

#define SYSCALL_ENTRY(id,name,args) __ASM_SYSCALL_FUNC( id, name )
ALL_SYSCALLS
#undef SYSCALL_ENTRY

/* Registered TEB handlers are used by Wine's builtin exception machinery.
 * PE language handlers still require RV64 unwind metadata decoding. */
static BOOL riscv64_valid_seh_frame( const EXCEPTION_REGISTRATION_RECORD *frame )
{
    ULONG_PTR addr = (ULONG_PTR)frame;
    NT_TIB *tib = &NtCurrentTeb()->Tib;

    return !(addr & (sizeof(void *) - 1)) &&
           addr >= (ULONG_PTR)tib->StackLimit &&
           addr < (ULONG_PTR)tib->StackBase &&
           (ULONG_PTR)tib->StackBase - addr >= sizeof(*frame);
}

/* Native Winelib modules use ELF CFI, including modules without a PE
 * runtime function table. Follow the x86-64 builtin unwind bridge. */
static NTSTATUS virtual_unwind( ULONG type, DISPATCHER_CONTEXT *dispatch, CONTEXT *context )
{
    LDR_DATA_TABLE_ENTRY *module;
    ULONG_PTR pc = context->Pc;
    NTSTATUS status;

    ULONG_PTR old_sp = context->Gpr.X[2], old_ra = context->Gpr.X[1];

    dispatch->ControlPc = pc;
    dispatch->ControlPcIsUnwound = !!(context->ContextFlags & CONTEXT_UNWOUND_TO_CALL);
    /* RISC-V instructions may be compressed; one byte selects the call site. */
    if (pc && dispatch->ControlPcIsUnwound) pc--;
    dispatch->ImageBase = 0;
    dispatch->ScopeIndex = 0;
    dispatch->HandlerData = NULL;
    dispatch->LanguageHandler = NULL;
    dispatch->FunctionEntry = RtlLookupFunctionEntry( pc, &dispatch->ImageBase,
                                                     dispatch->HistoryTable );
    if (!dispatch->FunctionEntry &&
        (LdrFindEntryForAddress( (void *)pc, &module ) || (module->Flags & LDR_WINE_INTERNAL)))
    {
        struct unwind_builtin_dll_params params =
            { type, (struct _DISPATCHER_CONTEXT *)dispatch, context };

        status = WINE_UNIX_CALL( unix_unwind_builtin_dll, &params );
        if (status != STATUS_UNSUCCESSFUL)
        {
            TRACE( "RV64 native unwind status=%#lx pc=%p sp=%p ra=%p -> pc=%p sp=%p ra=%p establisher=%p handler=%p\n",
                   (unsigned long)status, (void *)dispatch->ControlPc, (void *)old_sp,
                   (void *)old_ra, (void *)(ULONG_PTR)context->Pc,
                   (void *)(ULONG_PTR)context->Gpr.X[2],
                   (void *)(ULONG_PTR)context->Gpr.X[1],
                   (void *)dispatch->EstablisherFrame, dispatch->LanguageHandler );
            return status;
        }
    }
    return RtlVirtualUnwind2( type, dispatch->ImageBase, pc, dispatch->FunctionEntry,
                             context, NULL, &dispatch->HandlerData, &dispatch->EstablisherFrame,
                             NULL, NULL, NULL, &dispatch->LanguageHandler, 0 );
}

NTSTATUS call_seh_handlers( EXCEPTION_RECORD *rec, CONTEXT *original )
{
    EXCEPTION_REGISTRATION_RECORD *frame = NtCurrentTeb()->Tib.ExceptionList;
    EXCEPTION_REGISTRATION_RECORD *next;
    CONTEXT context = *original;
    DISPATCHER_CONTEXT dispatch = {0};
    ULONG_PTR establisher, sp, prev_sp = 0, prev_pc = 0;
    PEXCEPTION_ROUTINE handler;
    unsigned int depth = 0;
    LONG status;
    EXCEPTION_DISPOSITION disposition;

    dispatch.ContextRecord = &context;
    WARN( "RV64 dispatch ENTER exception=%p code=%#lx flags=%#lx address=%p original_pc=%p sp=%p ra=%p parent=%p\n",
          rec, (unsigned long)rec->ExceptionCode, (unsigned long)rec->ExceptionFlags,
          rec->ExceptionAddress, (void *)(ULONG_PTR)original->Pc,
          (void *)(ULONG_PTR)original->Gpr.X[2],
          (void *)(ULONG_PTR)original->Gpr.X[1], rec->ExceptionRecord );
    while (depth++ < 1024)
    {
        sp = context.Gpr.X[2];
        if (!sp || sp >= (ULONG_PTR)NtCurrentTeb()->Tib.StackBase ||
            sp < (ULONG_PTR)NtCurrentTeb()->Tib.StackLimit)
        {
            WARN( "RV64 dispatch stopped on invalid stack depth=%u pc=%p sp=%p\n",
                  depth, (void *)(ULONG_PTR)context.Pc, (void *)sp );
            return STATUS_BAD_STACK;
        }
        if (prev_sp && (sp < prev_sp || (sp == prev_sp && context.Pc == prev_pc)))
        {
            WARN( "RV64 dispatch stopped on repeated/nonprogressing frame depth=%u pc=%p sp=%p previous_pc=%p previous_sp=%p\n",
                  depth, (void *)(ULONG_PTR)context.Pc, (void *)sp,
                  (void *)prev_pc, (void *)prev_sp );
            return STATUS_BAD_STACK;
        }
        prev_sp = sp;
        prev_pc = context.Pc;

        TRACE( "RV64 dispatch UNWIND_BEGIN depth=%u exception=%p pc=%p sp=%p ra=%p\n",
               depth, rec, (void *)(ULONG_PTR)context.Pc, (void *)(ULONG_PTR)sp,
               (void *)(ULONG_PTR)context.Gpr.X[1] );
        status = virtual_unwind( UNW_FLAG_EHANDLER, &dispatch, &context );
        if (status)
        {
            WARN( "RV64 dispatch UNWIND_FAILED exception=%p status=%#lx depth=%u original_pc=%p current_pc=%p current_sp=%p\n",
                  rec, (unsigned long)status, depth,
                  (void *)(ULONG_PTR)original->Pc, (void *)(ULONG_PTR)context.Pc,
                  (void *)(ULONG_PTR)context.Gpr.X[2] );
            WARN( "RV64 dispatch unwind failed status=%#lx pc=%p sp=%p ra=%p depth=%u\n",
                  (unsigned long)status, (void *)prev_pc, (void *)prev_sp,
                  (void *)(ULONG_PTR)context.Gpr.X[1], depth );
            return status;
        }
        TRACE( "RV64 dispatch frame depth=%u pc=%p sp=%p ra=%p establisher=%p handler=%p\n",
               depth, (void *)(ULONG_PTR)context.Pc, (void *)(ULONG_PTR)context.Gpr.X[2],
               (void *)(ULONG_PTR)context.Gpr.X[1],
               (void *)dispatch.EstablisherFrame, dispatch.LanguageHandler );
        if (context.Gpr.X[2] < sp ||
            (context.Gpr.X[2] == sp && context.Pc == prev_pc))
        {
            WARN( "RV64 dispatch unwind made no progress depth=%u pc=%p sp=%p next_pc=%p next_sp=%p\n",
                  depth, (void *)prev_pc, (void *)sp,
                  (void *)(ULONG_PTR)context.Pc, (void *)(ULONG_PTR)context.Gpr.X[2] );
            return STATUS_BAD_STACK;
        }
        establisher = dispatch.EstablisherFrame;
        handler = dispatch.LanguageHandler;
        if (handler)
        {
            disposition = handler( rec, (void *)establisher, original, &dispatch );
            if (disposition == ExceptionContinueExecution)
                return (rec->ExceptionFlags & EXCEPTION_NONCONTINUABLE) ?
                       STATUS_NONCONTINUABLE_EXCEPTION : STATUS_SUCCESS;
            if (disposition != ExceptionContinueSearch)
            {
                ERR( "RV64 language handler=%p returned invalid disposition=%d frame=%p pc=%p sp=%p\n",
                     handler, disposition, (void *)establisher,
                     (void *)(ULONG_PTR)context.Pc, (void *)(ULONG_PTR)context.Gpr.X[2] );
                return STATUS_INVALID_DISPOSITION;
            }
        }
        else while (frame && riscv64_valid_seh_frame( frame ) &&
                    (ULONG_PTR)frame < context.Gpr.X[2])
        {
            next = frame->Prev;
            disposition = ((PEXCEPTION_ROUTINE)frame->Handler)( rec, frame, original, &dispatch );
            if (disposition == ExceptionContinueExecution)
                return (rec->ExceptionFlags & EXCEPTION_NONCONTINUABLE) ?
                       STATUS_NONCONTINUABLE_EXCEPTION : STATUS_SUCCESS;
            if (disposition == ExceptionNestedException) rec->ExceptionFlags |= EXCEPTION_NESTED_CALL;
            else if (disposition != ExceptionContinueSearch)
            {
                ERR( "RV64 SEH handler=%p returned invalid disposition=%d frame=%p pc=%p sp=%p\n",
                     frame->Handler, disposition, frame,
                     (void *)(ULONG_PTR)context.Pc, (void *)(ULONG_PTR)context.Gpr.X[2] );
                return STATUS_INVALID_DISPOSITION;
            }
            if (next && next != (void *)(ULONG_PTR)-1 && (ULONG_PTR)next <= (ULONG_PTR)frame)
            {
                rec->ExceptionFlags |= EXCEPTION_STACK_INVALID;
                return STATUS_BAD_STACK;
            }
            frame = next;
        }
        if (frame && frame != (void *)(ULONG_PTR)-1 && !riscv64_valid_seh_frame( frame ))
        {
            rec->ExceptionFlags |= EXCEPTION_STACK_INVALID;
            return STATUS_BAD_STACK;
        }
    }
    if (depth > 1024)
    {
        WARN( "RV64 dispatch unwind depth limit exceeded pc=%p sp=%p\n",
              (void *)(ULONG_PTR)context.Pc, (void *)(ULONG_PTR)context.Gpr.X[2] );
        return STATUS_BAD_STACK;
    }
    return STATUS_UNHANDLED_EXCEPTION;
}

void WINAPI process_breakpoint(void)
{
    __asm__ volatile("ebreak");
}


/* Stack probing. */
__ASM_GLOBAL_FUNC( __chkstk, "ret" )

/* Capture integer, control, and floating-point state at the caller boundary. */
__ASM_GLOBAL_FUNC( RtlCaptureContext,
                   "sd t0, 48(a0)\n\t"
                   "li t0, 0x800007\n\t"
                   "sw t0, 0(a0)\n\t"
                   "sd zero, 8(a0)\n\t"
                   "sd x1, 16(a0)\n\t"
                   "sd x2, 24(a0)\n\t"
                   "sd x3, 32(a0)\n\t"
                   "sd x4, 40(a0)\n\t"
                   "sd x6, 56(a0)\n\t"
                   "sd x7, 64(a0)\n\t"
                   "sd x8, 72(a0)\n\t"
                   "sd x9, 80(a0)\n\t"
                   "sd x11, 96(a0)\n\t"
                   "sd x12, 104(a0)\n\t"
                   "sd x13, 112(a0)\n\t"
                   "sd x14, 120(a0)\n\t"
                   "sd x15, 128(a0)\n\t"
                   "sd x16, 136(a0)\n\t"
                   "sd x17, 144(a0)\n\t"
                   "sd x18, 152(a0)\n\t"
                   "sd x19, 160(a0)\n\t"
                   "sd x20, 168(a0)\n\t"
                   "sd x21, 176(a0)\n\t"
                   "sd x22, 184(a0)\n\t"
                   "sd x23, 192(a0)\n\t"
                   "sd x24, 200(a0)\n\t"
                   "sd x25, 208(a0)\n\t"
                   "sd x26, 216(a0)\n\t"
                   "sd x27, 224(a0)\n\t"
                   "sd x28, 232(a0)\n\t"
                   "sd x29, 240(a0)\n\t"
                   "sd x30, 248(a0)\n\t"
                   "sd x31, 256(a0)\n\t"
                   "sd a0, 88(a0)\n\t" /* capture the context pointer passed in a0 */
                   "sd ra, 264(a0)\n\t"
                   "frcsr t0\n\t"
                   "sd t0, 272(a0)\n\t"
                   "fsd f0, 280(a0)\n\t"
                   "fsd f1, 288(a0)\n\t"
                   "fsd f2, 296(a0)\n\t"
                   "fsd f3, 304(a0)\n\t"
                   "fsd f4, 312(a0)\n\t"
                   "fsd f5, 320(a0)\n\t"
                   "fsd f6, 328(a0)\n\t"
                   "fsd f7, 336(a0)\n\t"
                   "fsd f8, 344(a0)\n\t"
                   "fsd f9, 352(a0)\n\t"
                   "fsd f10, 360(a0)\n\t"
                   "fsd f11, 368(a0)\n\t"
                   "fsd f12, 376(a0)\n\t"
                   "fsd f13, 384(a0)\n\t"
                   "fsd f14, 392(a0)\n\t"
                   "fsd f15, 400(a0)\n\t"
                   "fsd f16, 408(a0)\n\t"
                   "fsd f17, 416(a0)\n\t"
                   "fsd f18, 424(a0)\n\t"
                   "fsd f19, 432(a0)\n\t"
                   "fsd f20, 440(a0)\n\t"
                   "fsd f21, 448(a0)\n\t"
                   "fsd f22, 456(a0)\n\t"
                   "fsd f23, 464(a0)\n\t"
                   "fsd f24, 472(a0)\n\t"
                   "fsd f25, 480(a0)\n\t"
                   "fsd f26, 488(a0)\n\t"
                   "fsd f27, 496(a0)\n\t"
                   "fsd f28, 504(a0)\n\t"
                   "fsd f29, 512(a0)\n\t"
                   "fsd f30, 520(a0)\n\t"
                   "fsd f31, 528(a0)\n\t"
                   "ret" )

struct riscv64_jump_buffer
{
    ULONG64 frame, reserved;
    ULONG64 s[12];
    ULONG64 ra, sp, fcsr, reserved2;
    ULONG64 fs[12];
};
/* NtContinue performs the non-returning context transfer. */
void WINAPI RtlRestoreContext( CONTEXT *context, EXCEPTION_RECORD *record )
{
    NTSTATUS status;
    unsigned int i;

    if (record && record->ExceptionCode == STATUS_LONGJUMP && record->NumberParameters >= 1)
    {
        const struct riscv64_jump_buffer *jmp = (void *)(ULONG_PTR)record->ExceptionInformation[0];
        if (!jmp) RtlRaiseStatus( STATUS_INVALID_PARAMETER );
        for (i = 0; i < 12; ++i)
        {
            unsigned int reg = i < 2 ? i + 8 : i + 16;
            context->Gpr.X[reg] = jmp->s[i];
            context->F[reg].Q = jmp->fs[i];
        }
        context->Gpr.X[1] = jmp->ra;
        context->Gpr.X[2] = jmp->sp;
        context->Pc = jmp->ra;
        context->Fcsr = jmp->fcsr;
        context->ContextFlags |= CONTEXT_FULL;
    }
    status = NtContinue( context, FALSE );
    ERR( "RISCV64 RtlRestoreContext: NtContinue returned %#lx\\n", (unsigned long)status );
    __builtin_trap();
}

/* Exception dispatch. */
/* Raise a Windows exception through the NT exception path instead of silently
 * returning to the caller.  Full SEH dispatch still requires the RV64
 * exception dispatcher and signal translation machinery. */
void WINAPI RtlRaiseException( EXCEPTION_RECORD *record )
{
    CONTEXT context;

    RtlCaptureContext( &context );
    record->ExceptionAddress = (void *)(ULONG_PTR)context.Pc;
    NtRaiseException( record, &context, TRUE );
}
/* Unix signal delivery and the syscall return path arrange a CONTEXT followed
 * by an EXCEPTION_RECORD at the current stack pointer.  The helper is
 * deliberately noreturn: dispatch_exception either continues via NtContinue
 * or raises the resulting failure status. */
struct riscv64_exception_stack
{
    CONTEXT context;
    EXCEPTION_RECORD record;
};

void WINAPI DECLSPEC_NORETURN riscv64_dispatch_exception( struct riscv64_exception_stack *stack )
{
    NTSTATUS status;

    WARN( "RV64 KiUserExceptionDispatcher enter stack=%p context=%p record=%p code=%08x flags=%08x pc=%p sp=%p ra=%p\n",
          stack, &stack->context, &stack->record, (unsigned int)stack->record.ExceptionCode,
          stack->context.ContextFlags, (void *)(ULONG_PTR)stack->context.Pc,
          (void *)(ULONG_PTR)stack->context.Gpr.X[2],
          (void *)(ULONG_PTR)stack->context.Gpr.X[1] );
    if ((stack->context.ContextFlags & CONTEXT_CONTROL) != CONTEXT_CONTROL ||
        !stack->context.Pc || !stack->context.Gpr.X[2] ||
        (stack->context.Gpr.X[2] & 15))
    {
        ERR( "RV64 exception dispatcher received invalid context; refusing dispatch\n" );
        RtlRaiseStatus( STATUS_INVALID_PARAMETER );
    }
    status = dispatch_exception( &stack->record, &stack->context );
    ERR( "RV64 exception dispatch returned status=%#lx pc=%p sp=%p\n",
         (unsigned long)status, (void *)(ULONG_PTR)stack->context.Pc,
         (void *)(ULONG_PTR)stack->context.Gpr.X[2] );
    RtlRaiseStatus( status );
    __builtin_unreachable();
}

__ASM_GLOBAL_FUNC( KiUserExceptionDispatcher,
                   "mv a0, sp\n\t"
                   "call riscv64_dispatch_exception\n\t"
                   "ebreak" )
/* Mirrors the Unix-side riscv64_apc_stack_layout.  The dispatcher
 * executes on the Windows stack and resumes the saved context afterward. */
struct riscv64_apc_stack
{
    void (*func)(ULONG_PTR, ULONG_PTR, ULONG_PTR);
    ULONG_PTR args[3];
    ULONG_PTR alertable;
    ULONG_PTR reserved;
    CONTEXT context;
};
C_ASSERT( offsetof( struct riscv64_apc_stack, context ) == 0x30 );

void WINAPI DECLSPEC_NORETURN riscv64_dispatch_apc( struct riscv64_apc_stack *stack )
{
    NTSTATUS status;
    stack->func( stack->args[0], stack->args[1], stack->args[2] );
    status = NtContinue( &stack->context, stack->alertable );
    RtlRaiseStatus( status );
    __builtin_unreachable();
}

__ASM_GLOBAL_FUNC( KiUserApcDispatcher,
                   "mv a0, sp\n\t"
                   "call riscv64_dispatch_apc\n\t"
                   "ebreak" )
/* Windows-side half of KeUserModeCallback.  The Unix half builds the same
 * header as ARM64: arguments pointer, byte count, callback index, then
 * a machine-frame record.  Keep the callback on the Windows stack so it
 * can issue nested NT syscalls. */
struct riscv64_callback_stack
{
    void *args;
    ULONG len;
    ULONG id;
    ULONG64 ra;
    ULONG64 sp;
    ULONG64 pc;
    BYTE args_data[0];
};

static void __attribute__((used,noreturn)) riscv64_dispatch_callback( struct riscv64_callback_stack *stack )
{
    NTSTATUS (WINAPI *callback)( void *, ULONG );
    NTSTATUS status;
    void **table = NtCurrentTeb()->Peb->KernelCallbackTable;

    if (!table) RtlRaiseStatus( STATUS_INVALID_PARAMETER );
    callback = table[stack->id];
    if (!callback) RtlRaiseStatus( STATUS_INVALID_PARAMETER );
    status = callback( stack->args, stack->len );
    status = NtCallbackReturn( NULL, 0, status );
    RtlRaiseStatus( status );
    __builtin_unreachable();
}

__ASM_GLOBAL_FUNC( KiUserCallbackDispatcher,
                   "mv a0, sp\n\t"
                   "call " __ASM_NAME("riscv64_dispatch_callback") "\n\t"
                   "ebreak\n\t" )

/* Unwinding. */
/* Walk native ELF and PE frames, invoking termination handlers before
 * restoring the requested continuation.  This follows the arm64/x86-64
 * unwind loop, using the RISC-V64 context and the ELF unwind bridge. */
void WINAPI RtlUnwindEx( PVOID end_frame, PVOID target_ip, EXCEPTION_RECORD *rec,
                         PVOID retval, CONTEXT *context, UNWIND_HISTORY_TABLE *table )
{
    EXCEPTION_REGISTRATION_RECORD *teb_frame = NtCurrentTeb()->Tib.ExceptionList;
    EXCEPTION_RECORD record;
    DISPATCHER_CONTEXT dispatch = {0};
    CONTEXT next;
    ULONG_PTR sp, old_sp, old_pc, limit;
    EXCEPTION_DISPOSITION disposition = ExceptionContinueSearch;
    NTSTATUS status;
    unsigned int depth = 0;

    RtlCaptureContext( context );
    next = *context;
    TRACE( "RV64 unwind begin end_frame=%p target=%p pc=%p sp=%p ra=%p fp=%p\n",
           end_frame, target_ip, (void *)(ULONG_PTR)next.Pc,
           (void *)(ULONG_PTR)next.Gpr.X[2], (void *)(ULONG_PTR)next.Gpr.X[1],
           (void *)(ULONG_PTR)next.Gpr.X[8] );

    if (!rec)
    {
        record.ExceptionCode = STATUS_UNWIND;
        record.ExceptionFlags = 0;
        record.ExceptionRecord = NULL;
        record.ExceptionAddress = (void *)(ULONG_PTR)context->Pc;
        record.NumberParameters = 0;
        rec = &record;
    }
    rec->ExceptionFlags |= EXCEPTION_UNWINDING | (end_frame ? 0 : EXCEPTION_EXIT_UNWIND);

    dispatch.TargetPc = (ULONG_PTR)target_ip;
    dispatch.ContextRecord = context;
    dispatch.HistoryTable = table;

    for (;;)
    {
        old_sp = next.Gpr.X[2];
        old_pc = next.Pc;
        if (++depth > 1024) RtlRaiseStatus( STATUS_BAD_STACK );
        if (!old_sp || (old_sp & 15) ||
            old_sp < (ULONG_PTR)NtCurrentTeb()->Tib.StackLimit ||
            old_sp >= (ULONG_PTR)NtCurrentTeb()->Tib.StackBase)
            RtlRaiseStatus( STATUS_BAD_STACK );

        status = virtual_unwind( UNW_FLAG_UHANDLER, &dispatch, &next );
        if (status)
        {
            ERR( "RV64 unwind failure depth=%u status=%#lx pc=%p sp=%p target=%p\n",
                 depth, (unsigned long)status, (void *)old_pc, (void *)old_sp, target_ip );
            RtlRaiseStatus( status );
        }
        TRACE( "RV64 unwind frame depth=%u old_pc=%p old_sp=%p next_pc=%p next_sp=%p establisher=%p handler=%p target=%p\n",
               depth, (void *)old_pc, (void *)old_sp,
               (void *)(ULONG_PTR)next.Pc, (void *)(ULONG_PTR)next.Gpr.X[2],
               (void *)dispatch.EstablisherFrame, dispatch.LanguageHandler, target_ip );
        sp = dispatch.EstablisherFrame;
        if (!sp) break;
        if (!riscv64_valid_seh_frame( (void *)sp ))
            RtlRaiseStatus( STATUS_BAD_STACK );
        if (end_frame && sp > (ULONG_PTR)end_frame)
            RtlRaiseStatus( STATUS_INVALID_UNWIND_TARGET );

        if (sp == (ULONG_PTR)end_frame) rec->ExceptionFlags |= EXCEPTION_TARGET_UNWIND;

        if (dispatch.LanguageHandler)
        {
            disposition = dispatch.LanguageHandler( rec, (void *)sp, context, &dispatch );
            if (disposition == ExceptionCollidedUnwind)
            {
                /* The handler has supplied a replacement dispatcher context. */
                next = *dispatch.ContextRecord;
                rec->ExceptionFlags |= EXCEPTION_COLLIDED_UNWIND;
                continue;
            }
            if (disposition != ExceptionContinueSearch)
                RtlRaiseStatus( STATUS_INVALID_DISPOSITION );
            rec->ExceptionFlags &= ~EXCEPTION_COLLIDED_UNWIND;
        }
        else
        {
            disposition = ExceptionContinueSearch;
            limit = next.Gpr.X[2];
            if (end_frame && limit > (ULONG_PTR)end_frame) limit = (ULONG_PTR)end_frame;
            while (teb_frame && teb_frame != (void *)(ULONG_PTR)-1 &&
                   (ULONG_PTR)teb_frame < limit)
            {
                EXCEPTION_REGISTRATION_RECORD *frame = teb_frame;
                if (!riscv64_valid_seh_frame( frame ))
                    RtlRaiseStatus( STATUS_BAD_STACK );
                teb_frame = frame->Prev;
                disposition = ((PEXCEPTION_ROUTINE)frame->Handler)( rec, frame, context, &dispatch );
                if (disposition == ExceptionCollidedUnwind)
                {
                    next = *dispatch.ContextRecord;
                    rec->ExceptionFlags |= EXCEPTION_COLLIDED_UNWIND;
                    break;
                }
                if (disposition != ExceptionContinueSearch)
                    RtlRaiseStatus( STATUS_INVALID_DISPOSITION );
                rec->ExceptionFlags &= ~EXCEPTION_COLLIDED_UNWIND;
                if (teb_frame && teb_frame != (void *)(ULONG_PTR)-1 &&
                    (ULONG_PTR)teb_frame <= (ULONG_PTR)frame)
                    RtlRaiseStatus( STATUS_BAD_STACK );
            }
            if (disposition == ExceptionCollidedUnwind) continue;
        }
        if (sp == (ULONG_PTR)end_frame) break;
        if (next.Gpr.X[2] < old_sp ||
            (next.Gpr.X[2] == old_sp && next.Pc == old_pc))
            RtlRaiseStatus( STATUS_BAD_STACK );
        *context = next;
    }

    TRACE( "RV64 unwind complete target=%p pc=%p sp=%p ra=%p end_frame=%p depth=%u\n",
           target_ip, (void *)(ULONG_PTR)context->Pc, (void *)(ULONG_PTR)context->Gpr.X[2],
           (void *)(ULONG_PTR)context->Gpr.X[1], end_frame, depth );
    context->Gpr.X[10] = (ULONG_PTR)retval; /* a0: return value */
    if (rec->ExceptionCode != STATUS_UNWIND_CONSOLIDATE)
        context->Pc = (ULONG_PTR)target_ip;
    RtlRestoreContext( context, rec );
}

/* Only leaf functions can be unwound without architecture-specific PE
 * unwind opcodes. Never guess a frame for a non-leaf function. */
LONG WINAPI RtlVirtualUnwind2( ULONG type, ULONG_PTR image_base, ULONG_PTR pc,
                               RUNTIME_FUNCTION *function, CONTEXT *context,
                               BOOLEAN *machine_frame, void **handler_data,
                               ULONG_PTR *establisher, KNONVOLATILE_CONTEXT_POINTERS *nonvol,
                               ULONG_PTR *low_limit, ULONG_PTR *high_limit,
                               PEXCEPTION_ROUTINE *handler, ULONG flags )
{
    ULONG_PTR sp, ra;

    if (!context || !establisher) return STATUS_INVALID_PARAMETER;
    if (handler) *handler = NULL;
    if (handler_data) *handler_data = NULL;
    if (machine_frame) *machine_frame = FALSE;
    sp = context->Gpr.X[2];
    *establisher = sp;
    if (function)
    {
        /* RISC-V64 PE .xdata uses 2-byte instruction units and a 4-byte
         * header.  Decode the common allocation, saved-ra and frame-pointer
         * operations; do not guess at unsupported unwind sequences. */
        const BYTE *codes, *end;
        const DWORD *header_ptr;
        DWORD header, extra, scopes, words, length, offset, i;
        ULONG_PTR new_sp = sp, new_ra = context->Gpr.X[1];
        ULONG_PTR stack_low = (ULONG_PTR)NtCurrentTeb()->Tib.StackLimit;
        ULONG_PTR stack_high = (ULONG_PTR)NtCurrentTeb()->Tib.StackBase;
        CONTEXT next = *context;
        BOOL ended = FALSE;

        if (!image_base || (function->UnwindData & 3) ||
            pc < image_base + function->BeginAddress)
            return STATUS_INVALID_DISPOSITION;
        header_ptr = (const DWORD *)(image_base + function->UnwindData);
        header = *header_ptr++;
        if ((header >> 18) & 3) return STATUS_INVALID_DISPOSITION;
        length = header & 0x3ffff;
        offset = (pc - image_base - function->BeginAddress) / 2;
        if (offset >= length) return STATUS_INVALID_DISPOSITION;
        scopes = (header >> 22) & 31;
        words = (header >> 27) & 31;
        if (!scopes && !words)
        {
            extra = *header_ptr++;
            scopes = extra & 0xffff;
            words = (extra >> 16) & 0xff;
        }
        if (words > 255 || scopes > 65535) return STATUS_INVALID_DISPOSITION;
        /* Prologue/epilogue-aware interpretation is required before restoring
         * any registers there.  Reject these locations rather than inventing
         * an unwind that can corrupt the caller. */
        /* The code array is variable-length.  Its byte count is not a
         * prologue instruction count.  Decode each opcode before deciding
         * which operations have executed at the current control PC. */
        if (!words) return STATUS_INVALID_DISPOSITION;
        codes = (const BYTE *)(header_ptr + ((header & (1 << 21)) ? 0 : scopes));
        end = codes + 4 * words;
        if ((header & (1 << 20)) && handler && (type & (UNW_FLAG_EHANDLER | UNW_FLAG_UHANDLER)))
        {
            *handler = (PEXCEPTION_ROUTINE)(image_base + *(const DWORD *)end);
            if (handler_data) *handler_data = (void *)(end + sizeof(DWORD));
        }
        for (i = 0; codes < end && i < 1024; ++i)
        {
            BYTE op = *codes++;
            ULONG_PTR address;
            unsigned int reg;
            if (op <= 0x1f) new_sp += 16 * op;
            else if ((op & 0xf8) == 0xc0)
            {
                if (codes >= end) return STATUS_INVALID_DISPOSITION;
                new_sp += 16 * (((op & 7) << 8) | *codes++);
            }
            else if (op == 0xd0 || (op & 0xfe) == 0xdc)
            {
                unsigned int first, count = 1;
                ULONG_PTR slot;
                if (end - codes < 2) return STATUS_INVALID_DISPOSITION;
                reg = *codes++;
                slot = *codes++;
                if (op == 0xd0)
                {
                    first = 1 + reg;
                    address = new_sp + 8 * slot;
                    if (first >= 32 || address < stack_low || address > stack_high - 8)
                        return STATUS_BAD_STACK;
                    next.Gpr.X[first] = *(const ULONG64 *)address;
                    if (first == 1) new_ra = next.Gpr.X[1];
                }
                else
                {
                    first = 8 + (reg >> 4) + ((op & 1) << 4);
                    address = new_sp + 8 * (((reg & 15) << 8) | slot);
                    if (first >= 32 || address < stack_low || address > stack_high - 8)
                        return STATUS_BAD_STACK;
                    next.F[first].Q = *(const ULONG64 *)address;
                }
                (void)count;
            }
            else if (op == 0xe0)
            {
                if (end - codes < 3) return STATUS_INVALID_DISPOSITION;
                new_sp += 16 * ((codes[0] << 16) | (codes[1] << 8) | codes[2]);
                codes += 3;
            }
            else if (op == 0xe1) new_sp = next.Gpr.X[8];
            else if (op == 0xe2)
            {
                if (end - codes < 2) return STATUS_INVALID_DISPOSITION;
                new_sp = next.Gpr.X[8] - 8 * ((codes[0] << 8) | codes[1]);
                codes += 2;
            }
            else if (op == 0xe3) continue;
            else if (op == 0xe4 || op == 0xe5) { ended = TRUE; break; }
            else
            {
                WARN( "RV64 unsupported PE unwind opcode %#x pc=%p\n", op, (void *)pc );
                return STATUS_UNSUCCESSFUL;
            }
            if (new_sp < stack_low || new_sp > stack_high || (new_sp & 7))
                return STATUS_BAD_STACK;
        }
        if (!ended || !new_ra || (new_sp & 15) || new_sp < sp ||
            new_sp >= stack_high || (new_sp == sp && new_ra == context->Pc))
            return STATUS_BAD_STACK;
        next.Gpr.X[2] = new_sp;
        next.Gpr.X[1] = new_ra;
        next.Pc = new_ra;
        next.ContextFlags |= CONTEXT_UNWOUND_TO_CALL;
        *establisher = new_sp;
        *context = next;
        TRACE( "RV64 PE unwind pc=%p sp=%p -> pc=%p sp=%p fp=%p\n",
               (void *)pc, (void *)sp, (void *)next.Pc, (void *)new_sp,
               (void *)(ULONG_PTR)next.Gpr.X[8] );
        return STATUS_SUCCESS;
    }
    if (!sp || (sp & 15)) return STATUS_BAD_STACK;
    if (low_limit && sp < *low_limit) return STATUS_BAD_STACK;
    if (high_limit && sp >= *high_limit) return STATUS_BAD_STACK;
    ra = context->Gpr.X[1];
    if (!ra || ra == context->Pc) return STATUS_INVALID_DISPOSITION;
    context->Pc = ra;
    context->ContextFlags |= CONTEXT_UNWOUND_TO_CALL;
    return STATUS_SUCCESS;
}

PEXCEPTION_ROUTINE WINAPI RtlVirtualUnwind( ULONG type, ULONG_PTR image_base,
                                            ULONG_PTR pc, RUNTIME_FUNCTION *function,
                                            CONTEXT *context, void **handler_data,
                                            ULONG_PTR *establisher,
                                            KNONVOLATILE_CONTEXT_POINTERS *nonvol )
{
    PEXCEPTION_ROUTINE handler = NULL;
    LONG status = RtlVirtualUnwind2( type, image_base, pc, function, context, NULL,
                                     handler_data, establisher, nonvol,
                                     NULL, NULL, &handler, 0 );
    if (status) WARN( "RISC-V64 virtual unwind failed at %p: %#lx\n",
                      (void *)pc, (unsigned long)status );
    return handler;
}
ULONG WINAPI RtlWalkFrameChain( void **buffer, ULONG count, ULONG flags )
{
    CONTEXT context;
    DISPATCHER_CONTEXT dispatch = {0};
    UNWIND_HISTORY_TABLE table = {0};
    ULONG i, skip = flags >> 8, num_entries = 0;
    ULONG_PTR sp, pc;

    RtlCaptureContext( &context );
    dispatch.ContextRecord = &context;
    dispatch.HistoryTable = &table;
    for (i = 0; i < count; ++i)
    {
        sp = context.Gpr.X[2];
        pc = context.Pc;
        if (!sp || (sp & 15) || sp < (ULONG_PTR)NtCurrentTeb()->Tib.StackLimit ||
            sp >= (ULONG_PTR)NtCurrentTeb()->Tib.StackBase) break;
        if (virtual_unwind( UNW_FLAG_NHANDLER, &dispatch, &context )) break;
        if (!context.Pc || context.Gpr.X[2] < sp ||
            (context.Gpr.X[2] == sp && context.Pc == pc)) break;
        if (context.Gpr.X[2] >= (ULONG_PTR)NtCurrentTeb()->Tib.StackBase) break;
        if (i >= skip) buffer[num_entries++] = (void *)context.Pc;
    }
    return num_entries;
}
__ASM_GLOBAL_FUNC( __C_specific_handler, "li a0, 1\n\tret" )

/* Thread startup. */
void WINAPI LdrInitializeThunk( CONTEXT *context, ULONG_PTR unk2,
                                ULONG_PTR unk3, ULONG_PTR unk4 )
{
    NTSTATUS status;

    WARN( "RISCV64 LdrInitializeThunk: context=%p pc=%p sp=%p ra=%p\n",
          context,
          (void *)(ULONG_PTR)context->Pc,
          (void *)(ULONG_PTR)context->Gpr.X[2],
          (void *)(ULONG_PTR)context->Gpr.X[1] );

    WARN( "RV64 loader input a0=%p a1=%p a2=%p a3=%p tp=%p gp=%p flags=%#lx\n",
          (void *)(ULONG_PTR)context->Gpr.X[10],
          (void *)(ULONG_PTR)context->Gpr.X[11],
          (void *)(ULONG_PTR)context->Gpr.X[12],
          (void *)(ULONG_PTR)context->Gpr.X[13],
          (void *)(ULONG_PTR)context->Gpr.X[4],
          (void *)(ULONG_PTR)context->Gpr.X[3],
          (unsigned long)context->ContextFlags );
    loader_init( context, (void **)&context->Gpr.X[10] );

    WARN( "RISCV64 LdrInitializeThunk: after loader_init pc=%p sp=%p ra=%p\n",
          (void *)(ULONG_PTR)context->Pc,
          (void *)(ULONG_PTR)context->Gpr.X[2],
          (void *)(ULONG_PTR)context->Gpr.X[1] );

    WARN( "RV64 loader continue target=%p sp=%p ra=%p entry(a0)=%p arg(a1)=%p tp=%p thunk=%p\n",
          (void *)(ULONG_PTR)context->Pc,
          (void *)(ULONG_PTR)context->Gpr.X[2],
          (void *)(ULONG_PTR)context->Gpr.X[1],
          (void *)(ULONG_PTR)context->Gpr.X[10],
          (void *)(ULONG_PTR)context->Gpr.X[11],
          (void *)(ULONG_PTR)context->Gpr.X[4], pBaseThreadInitThunk );
    status = NtContinue( context, TRUE );

    ERR( "RISCV64 NtContinue unexpectedly returned status=%#lx\n",
         (unsigned long)status );

    __builtin_trap();
}

void WINAPI RtlUserThreadStart( PRTL_THREAD_START_ROUTINE entry, void *arg )
{
    /* The loader initially installs a fallback thunk, then replaces it
     * with kernel32's export.  Report the actual transition target before
     * entering the application; a missing export must not become a jump
     * through address zero. */
    void (FASTCALL *thunk)(DWORD,LPTHREAD_START_ROUTINE,void *) = pBaseThreadInitThunk;

    TEB *teb = NtCurrentTeb();

    WARN( "RV64 thread start entry=%p arg=%p BaseThreadInitThunk=%p teb=%p\n",
          entry, arg, thunk, teb );
    if (teb)
        WARN( "RV64 thread TEB self=%p stackbase=%p stacklimit=%p peb=%p\n",
              teb->Tib.Self, teb->Tib.StackBase, teb->Tib.StackLimit, teb->Peb );
    else
        ERR( "RV64 thread start has NULL TEB; cannot safely access thread environment\n" );
    if (!thunk || !entry)
    {
        ERR( "RV64 invalid thread startup: entry=%p thunk=%p\n", entry, thunk );
        RtlExitUserThread( STATUS_INVALID_PARAMETER );
    }

    if (teb)
    {
        ULONG_PTR sp;
        __asm__ volatile ( "mv %0, sp" : "=r"(sp) );
        WARN( "RV64-HEAP thread ENTER sp=%p aligned=%u entry=%p arg=%p thunk=%p teb=%p stack=[%p,%p)\n",
              (void *)sp, !(sp & 15), entry, arg, thunk, teb,
              teb->Tib.StackLimit, teb->Tib.StackBase );
        if (sp & 15) ERR( "RV64-HEAP misaligned thread-entry stack sp=%p\n", (void *)sp );
    }
    thunk( 0, (LPTHREAD_START_ROUTINE)entry, arg );
    ERR( "RV64-HEAP BaseThreadInitThunk unexpectedly returned entry=%p arg=%p\n", entry, arg );

    /* BaseThreadInitThunk should terminate the thread, not return. */
    RtlExitUserThread( STATUS_UNSUCCESSFUL );
    __builtin_unreachable();
}

/* Debugger entry points. */
__ASM_GLOBAL_FUNC( DbgBreakPoint, "ebreak\n\tret" )
__ASM_GLOBAL_FUNC( DbgUserBreakPoint, "ebreak\n\tret" )
__ASM_GLOBAL_FUNC( DbgUiRemoteBreakin, "ret" )

/* RISC-V64 has no Windows x86 XSAVE extended-feature state.  As on
 * ARM64, report no enabled extended context features. */
ULONG64 WINAPI RtlGetEnabledExtendedFeatures( ULONG64 feature_mask )
{
    return 0;
}

/* Processor information. */
/* Do not report every processor feature as unavailable.  The shared user
 * data contains the features exposed to Windows processes by Wine.  RISC-V
 * has no x86/ARM-specific extended feature namespace here. */
BOOLEAN WINAPI RtlIsProcessorFeaturePresent( UINT feature )
{
    if (feature >= PROCESSOR_FEATURE_MAX || !user_shared_data) return FALSE;
    return !!user_shared_data->ProcessorFeatures[feature];
}
/* Mirror the native ARM64 implementation: this API is not architecture-
 * specific and should forward the query rather than report a stub error. */
NTSTATUS WINAPI RtlGetNativeSystemInformation( SYSTEM_INFORMATION_CLASS class,
                                               void *info, ULONG size, ULONG *ret_size )
{
    return NtQuerySystemInformation( class, info, size, ret_size );
}

/* Wine RV64 LP64D jump buffer, shared with include/msvcrt/setjmp.h. */
C_ASSERT( sizeof(struct riscv64_jump_buffer) == 240 );
C_ASSERT( offsetof(struct riscv64_jump_buffer, s) == 0x10 );
C_ASSERT( offsetof(struct riscv64_jump_buffer, ra) == 0x70 );
C_ASSERT( offsetof(struct riscv64_jump_buffer, sp) == 0x78 );
C_ASSERT( offsetof(struct riscv64_jump_buffer, fcsr) == 0x80 );
C_ASSERT( offsetof(struct riscv64_jump_buffer, fs) == 0x90 );

/* a0 = jump buffer, a1 = optional SEH frame; returns zero initially. */
__ASM_GLOBAL_FUNC( NTDLL__setjmpex,
                   "sd a1, 0(a0)\n\t"
                   "sd s0, 16(a0)\n\t"
                   "sd s1, 24(a0)\n\t"
                   "sd s2, 32(a0)\n\t"
                   "sd s3, 40(a0)\n\t"
                   "sd s4, 48(a0)\n\t"
                   "sd s5, 56(a0)\n\t"
                   "sd s6, 64(a0)\n\t"
                   "sd s7, 72(a0)\n\t"
                   "sd s8, 80(a0)\n\t"
                   "sd s9, 88(a0)\n\t"
                   "sd s10, 96(a0)\n\t"
                   "sd s11, 104(a0)\n\t"
                   "sd ra, 112(a0)\n\t"
                   "sd sp, 120(a0)\n\t"
                   "frcsr t0\n\t"
                   "sd t0, 128(a0)\n\t"
                   "fsd fs0, 144(a0)\n\t"
                   "fsd fs1, 152(a0)\n\t"
                   "fsd fs2, 160(a0)\n\t"
                   "fsd fs3, 168(a0)\n\t"
                   "fsd fs4, 176(a0)\n\t"
                   "fsd fs5, 184(a0)\n\t"
                   "fsd fs6, 192(a0)\n\t"
                   "fsd fs7, 200(a0)\n\t"
                   "fsd fs8, 208(a0)\n\t"
                   "fsd fs9, 216(a0)\n\t"
                   "fsd fs10, 224(a0)\n\t"
                   "fsd fs11, 232(a0)\n\t"
                   "li a0, 0\n\t"
                   "ret" )

/* a0 = jump buffer, a1 = requested return value.  This performs a
 * direct nonlocal transfer; full SEH unwind requires RtlUnwindEx support. */
__ASM_GLOBAL_FUNC( NTDLL_longjmp,
                   "mv t0, a0\n\t"
                   "mv a0, a1\n\t"
                   "bnez a0, 1f\n\t"
                   "li a0, 1\n\t"
                   "1:\n\t"
                   "ld s0, 16(t0)\n\t"
                   "ld s1, 24(t0)\n\t"
                   "ld s2, 32(t0)\n\t"
                   "ld s3, 40(t0)\n\t"
                   "ld s4, 48(t0)\n\t"
                   "ld s5, 56(t0)\n\t"
                   "ld s6, 64(t0)\n\t"
                   "ld s7, 72(t0)\n\t"
                   "ld s8, 80(t0)\n\t"
                   "ld s9, 88(t0)\n\t"
                   "ld s10, 96(t0)\n\t"
                   "ld s11, 104(t0)\n\t"
                   "ld ra, 112(t0)\n\t"
                   "ld sp, 120(t0)\n\t"
                   "ld t1, 128(t0)\n\t"
                   "fscsr t1\n\t"
                   "fld fs0, 144(t0)\n\t"
                   "fld fs1, 152(t0)\n\t"
                   "fld fs2, 160(t0)\n\t"
                   "fld fs3, 168(t0)\n\t"
                   "fld fs4, 176(t0)\n\t"
                   "fld fs5, 184(t0)\n\t"
                   "fld fs6, 192(t0)\n\t"
                   "fld fs7, 200(t0)\n\t"
                   "fld fs8, 208(t0)\n\t"
                   "fld fs9, 216(t0)\n\t"
                   "fld fs10, 224(t0)\n\t"
                   "fld fs11, 232(t0)\n\t"
                   "ret" )

#endif /* defined(__riscv) && (__riscv_xlen == 64) */

