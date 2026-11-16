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
        if (status != STATUS_UNSUCCESSFUL) return status;
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
    while (depth++ < 1024)
    {
        sp = context.Gpr.X[2];
        if (!sp || sp >= (ULONG_PTR)NtCurrentTeb()->Tib.StackBase ||
            sp < (ULONG_PTR)NtCurrentTeb()->Tib.StackLimit ||
            (prev_sp && (sp < prev_sp || (sp == prev_sp && context.Pc == prev_pc)))) break;
        prev_sp = sp;
        prev_pc = context.Pc;

        status = virtual_unwind( UNW_FLAG_EHANDLER, &dispatch, &context );
        if (status) return status;
        establisher = dispatch.EstablisherFrame;
        handler = dispatch.LanguageHandler;
        if (handler)
        {
            disposition = handler( rec, (void *)establisher, original, &dispatch );
            if (disposition == ExceptionContinueExecution)
                return (rec->ExceptionFlags & EXCEPTION_NONCONTINUABLE) ?
                       STATUS_NONCONTINUABLE_EXCEPTION : STATUS_SUCCESS;
            if (disposition != ExceptionContinueSearch) return STATUS_INVALID_DISPOSITION;
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
            else if (disposition != ExceptionContinueSearch) return STATUS_INVALID_DISPOSITION;
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
    NTSTATUS status = dispatch_exception( &stack->record, &stack->context );
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
__ASM_GLOBAL_FUNC( KiUserCallbackDispatcher, "ret" )

/* Unwinding. */
__ASM_GLOBAL_FUNC( RtlUnwindEx, "ret" )
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
        FIXME( "RISC-V64 PE unwind opcodes not implemented at pc %p\n", (void *)pc );
        return STATUS_INVALID_DISPOSITION;
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

    loader_init( context, (void **)&context->Gpr.X[10] );

    WARN( "RISCV64 LdrInitializeThunk: after loader_init pc=%p sp=%p ra=%p\n",
          (void *)(ULONG_PTR)context->Pc,
          (void *)(ULONG_PTR)context->Gpr.X[2],
          (void *)(ULONG_PTR)context->Gpr.X[1] );

    status = NtContinue( context, TRUE );

    ERR( "RISCV64 NtContinue unexpectedly returned status=%#lx\n",
         (unsigned long)status );

    __builtin_trap();
}

void WINAPI RtlUserThreadStart( PRTL_THREAD_START_ROUTINE entry, void *arg )
{
    pBaseThreadInitThunk( 0, (LPTHREAD_START_ROUTINE)entry, arg );

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

