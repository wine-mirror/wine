#if 0
#pragma makedep unix
#endif

#if defined(__riscv) && (__riscv_xlen == 64)

#include "config.h"

#include <assert.h>
#include <stddef.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_SYSCALL_H
# include <syscall.h>
#else
# ifdef HAVE_SYS_SYSCALL_H
#  include <sys/syscall.h>
# endif
#endif
#ifdef HAVE_SYS_SIGNAL_H
# include <sys/signal.h>
#endif
#ifdef HAVE_SYS_UCONTEXT_H
# include <sys/ucontext.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winnt.h"
#include "winternl.h"
#include "wine/asm.h"
#include "unix_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);

#define REGn_sig(reg, context) ((context)->uc_mcontext.__gregs[(reg)])
#define PC_sig(context) REGn_sig( REG_PC, context )
#define RA_sig(context) REGn_sig( REG_RA, context )
#define SP_sig(context) REGn_sig( REG_SP, context )

struct syscall_frame
{
    ULONG64 x[32];
    ULONG64 pc;
    ULONG64 sp;
    ULONG64 ra;

    ULONG64 f[32];
    ULONG64 fcsr;

    ULONG restore_flags;

    struct syscall_frame *prev_frame;
    void *syscall_cfa;
    ULONG syscall_id;
};

C_ASSERT( sizeof( struct syscall_frame ) % 16 == 0 );
C_ASSERT( offsetof( struct syscall_frame, x ) == 0 );
C_ASSERT( offsetof( struct syscall_frame, pc ) == 256 );
C_ASSERT( offsetof( struct syscall_frame, sp ) == 264 );
C_ASSERT( offsetof( struct syscall_frame, ra ) == 272 );
C_ASSERT( offsetof( struct syscall_frame, f ) == 280 );
C_ASSERT( offsetof( struct syscall_frame, fcsr ) == 536 );
C_ASSERT( offsetof( struct syscall_frame, restore_flags ) == 544 );
C_ASSERT( offsetof( struct syscall_frame, prev_frame ) == 552 );
C_ASSERT( offsetof( struct syscall_frame, syscall_cfa ) == 560 );
C_ASSERT( offsetof( struct syscall_frame, syscall_id ) == 568 );

static struct syscall_frame *init_syscall_frame( PRTL_THREAD_START_ROUTINE entry,
                                                 void *arg, TEB *teb )
{
    struct thread_data *data = get_thread_data();
    struct syscall_frame *frame = get_syscall_frame( data );
    CONTEXT context = { CONTEXT_ALL };
    CONTEXT *ctx;
    ULONG_PTR stack_top;

    /* The initial thread gets its frame in signal_init_process(), but
     * subsequent threads need their own frame on their kernel stack. */
    if (!frame)
    {
        alloc_syscall_frame( sizeof(*frame) );
        frame = get_syscall_frame( data );
    }

    memset( frame, 0, sizeof(*frame) );

    stack_top = (ULONG_PTR)teb->Tib.StackBase & ~(ULONG_PTR)15;
    WARN("RISCV64 initial stack base=%p limit=%p top=%p frame=%p\n",
         teb->Tib.StackBase, teb->Tib.StackLimit, (void *)stack_top, frame);

    context.Gpr.X[10] = (ULONG64)entry;
    context.Gpr.X[11] = (ULONG64)arg;
    /* Keep x4/tp reserved for native ELF thread-local storage. */
    context.Gpr.X[4]  = (ULONG64)__builtin_thread_pointer();
    context.Gpr.X[2]  = stack_top;
    context.Pc        = (ULONG64)pRtlUserThreadStart;

    /* The RISC-V LP64 ABI requires a 16-byte aligned stack pointer.
     * sizeof(CONTEXT) need not itself be a multiple of 16. */
    ctx = (CONTEXT *)((stack_top - sizeof(*ctx)) & ~(ULONG_PTR)15);
    *ctx = context;
    ctx->ContextFlags = CONTEXT_FULL;

    /*
     * First enter LdrInitializeThunk with the initial CONTEXT.
     * It will initialize the loader and continue at RtlUserThreadStart.
     */
    frame->pc = (ULONG64)pLdrInitializeThunk;
    frame->sp = (ULONG64)ctx;
    frame->x[10] = (ULONG64)ctx;
    frame->x[4] = (ULONG64)__builtin_thread_pointer();
    frame->x[2] = frame->sp;
    frame->x[1] = frame->ra;

    frame->restore_flags = CONTEXT_CONTROL | CONTEXT_INTEGER;

    return frame;
}


void set_process_instrumentation_callback( void *callback )
{
}


NTSTATUS signal_set_full_context( CONTEXT *context )
{
    struct thread_data *data = get_thread_data();
    struct syscall_frame *frame = get_syscall_frame( data );
    NTSTATUS status;

    WARN("RISCV64 requested context pc=%016llx sp=%016llx ra=%016llx flags=%08x\n",
         (unsigned long long)context->Pc, (unsigned long long)context->Gpr.X[2],
         (unsigned long long)context->Gpr.X[1], context->ContextFlags);
    status = NtSetContextThread( GetCurrentThread(), context );

    if (!status && frame && (context->ContextFlags & CONTEXT_INTEGER))
        frame->restore_flags |= CONTEXT_INTEGER;

    return status;
}


NTSTATUS WINAPI NtGetContextThread( HANDLE handle, CONTEXT *context )
{
    struct thread_data *data = get_thread_data();
    struct syscall_frame *frame = get_syscall_frame( data );
    DWORD flags = context->ContextFlags & ~CONTEXT_RISCV64;

    if ((context->ContextFlags & CONTEXT_RISCV64) != CONTEXT_RISCV64)
        return STATUS_INVALID_PARAMETER;
    if (flags & ~(CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_FLOATING_POINT))
        return STATUS_NOT_SUPPORTED;

    if (handle != GetCurrentThread())
    {
        BOOL self = FALSE;
        NTSTATUS ret = get_thread_context( handle, context, &self, IMAGE_FILE_MACHINE_RISCV64 );
        if (ret || !self) return ret;
    }

    if (!frame)
        return STATUS_ACCESS_DENIED;

    if (flags & CONTEXT_INTEGER)
        memcpy( context->Gpr.X, frame->x, sizeof(frame->x) );

    /* x0 is architecturally hardwired to zero. */
    if (flags & CONTEXT_INTEGER)
        context->Gpr.X[0] = 0;

    if (flags & CONTEXT_CONTROL)
    {
        context->Pc = frame->pc;
        context->Gpr.X[2] = frame->sp;
        context->Gpr.X[1] = frame->ra;
    }

    if (flags & CONTEXT_FLOATING_POINT)
    {
        unsigned int i;
        for (i = 0; i < 32; i++) context->F[i].Q = frame->f[i];
        context->Fcsr = frame->fcsr;
    }

    return STATUS_SUCCESS;
}


NTSTATUS WINAPI NtSetContextThread( HANDLE handle, const CONTEXT *context )
{
    struct thread_data *data = get_thread_data();
    struct syscall_frame *frame = get_syscall_frame( data );
    DWORD flags = context->ContextFlags & ~CONTEXT_RISCV64;

    if ((context->ContextFlags & CONTEXT_RISCV64) != CONTEXT_RISCV64)
        return STATUS_INVALID_PARAMETER;
    if (flags & ~(CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_FLOATING_POINT))
        return STATUS_NOT_SUPPORTED;

    if (handle != GetCurrentThread())
    {
        BOOL self = FALSE;
        NTSTATUS ret = set_thread_context( handle, context, &self, IMAGE_FILE_MACHINE_RISCV64 );
        if (ret || !self) return ret;
    }

    if (!frame)
        return STATUS_ACCESS_DENIED;

    if (flags & CONTEXT_INTEGER)
        memcpy( frame->x, context->Gpr.X, sizeof(frame->x) );

    if (flags & CONTEXT_INTEGER)
        frame->x[0] = 0;

    if (flags & CONTEXT_CONTROL)
    {
        frame->pc = context->Pc;
        frame->sp = context->Gpr.X[2];
        frame->ra = context->Gpr.X[1];
        frame->x[1] = frame->ra;
        frame->x[2] = frame->sp;
    }

    if (flags & CONTEXT_FLOATING_POINT)
    {
        unsigned int i;
        for (i = 0; i < 32; i++) frame->f[i] = context->F[i].Q;
        frame->fcsr = context->Fcsr;
    }

    frame->restore_flags |= flags & (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_FLOATING_POINT);
    WARN("RISCV64 saved frame pc=%016llx sp=%016llx ra=%016llx flags=%08x\n",
         (unsigned long long)frame->pc, (unsigned long long)frame->sp,
         (unsigned long long)frame->ra, frame->restore_flags);
 
    return STATUS_SUCCESS;
}


NTSTATUS set_thread_wow64_context( HANDLE handle, const void *ctx, ULONG size )
{
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS get_thread_wow64_context( HANDLE handle, void *ctx, ULONG size )
{
    return STATUS_NOT_IMPLEMENTED;
}


/* Keep this layout synchronized with the PE-side APC entry point. */
struct riscv64_apc_stack_layout
{
    void (*func)(ULONG_PTR, ULONG_PTR, ULONG_PTR);
    ULONG_PTR args[3];
    ULONG_PTR alertable;
    ULONG_PTR reserved;
    CONTEXT context;
};
C_ASSERT( offsetof( struct riscv64_apc_stack_layout, context ) == 0x30 );

NTSTATUS call_user_apc_dispatcher( CONTEXT *context_ptr, unsigned int flags,
                                   ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3,
                                   void (*func)(ULONG_PTR, ULONG_PTR, ULONG_PTR),
                                   NTSTATUS status )
{
    struct thread_data *data = get_thread_data();
    struct syscall_frame *frame = get_syscall_frame( data );
    struct riscv64_apc_stack_layout *stack;
    ULONG_PTR sp;
    NTSTATUS ret;

    if (!frame) return STATUS_ACCESS_DENIED;
    if (flags) FIXME( "unsupported APC flags %#x\\n", flags );
    sp = context_ptr ? context_ptr->Gpr.X[2] : frame->sp;
    stack = (void *)((sp - sizeof(*stack)) & ~(ULONG_PTR)15);
    if (context_ptr)
    {
        stack->context = *context_ptr;
        ret = NtSetContextThread( GetCurrentThread(), &stack->context );
    }
    else
    {
        stack->context.ContextFlags = CONTEXT_FULL;
        ret = NtGetContextThread( GetCurrentThread(), &stack->context );
        stack->context.Gpr.X[10] = status;
    }
    if (ret) return ret;
    stack->func = func;
    stack->args[0] = arg1;
    stack->args[1] = arg2;
    stack->args[2] = arg3;
    stack->alertable = TRUE;
    stack->reserved = 0;
    frame->sp = (ULONG_PTR)stack;
    frame->x[2] = frame->sp;
    frame->pc = (ULONG_PTR)pKiUserApcDispatcher;
    frame->restore_flags |= CONTEXT_CONTROL;
    return status;
}

void call_raise_user_exception_dispatcher( struct thread_data *data )
{
    struct syscall_frame *frame = get_syscall_frame( data );

    frame->pc = (ULONG64)pKiRaiseUserExceptionDispatcher;
    frame->restore_flags |= CONTEXT_CONTROL;
}


struct riscv64_exception_stack_layout
{
    CONTEXT context;
    EXCEPTION_RECORD record;
};

NTSTATUS call_user_exception_dispatcher( struct thread_data *data,
                                         EXCEPTION_RECORD *rec,
                                         CONTEXT *context )
{
    struct syscall_frame *frame = get_syscall_frame( data );
    struct riscv64_exception_stack_layout *stack;
    NTSTATUS status;

    if (!frame) return STATUS_ACCESS_DENIED;
    status = NtSetContextThread( GetCurrentThread(), context );
    if (status) return status;

    stack = (void *)(((ULONG_PTR)context->Gpr.X[2] - sizeof(*stack)) & ~(ULONG_PTR)15);
    stack->context = *context;
    stack->record = *rec;
    frame->sp = (ULONG_PTR)stack;
    frame->pc = (ULONG_PTR)pKiUserExceptionDispatcher;
    frame->x[2] = frame->sp;
    frame->restore_flags |= CONTEXT_CONTROL;
    return STATUS_SUCCESS;
}


NTSTATUS get_thread_ldt_entry( HANDLE handle,
                               THREAD_DESCRIPTOR_INFORMATION *info,
                               ULONG len )
{
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS signal_alloc_thread( TEB *teb )
{
    return STATUS_SUCCESS;
}


void signal_free_thread( TEB *teb )
{
}


/* Translate synchronous Linux traps into NT exceptions.  Keep the signal
 * handler itself free of loader and wineserver calls: those run after the
 * kernel restores the interrupted thread context. */
struct riscv64_signal_exception
{
    EXCEPTION_RECORD record;
    CONTEXT context;
};

static void DECLSPEC_NORETURN riscv64_raise_signal_exception( struct riscv64_signal_exception *exc )
{
    NTSTATUS status = NtRaiseException( &exc->record, &exc->context, TRUE );

    /* A handled first-chance exception may return after updating CONTEXT.
     * Continue with the interrupted register image rather than aborting. */
    if (!status) status = NtContinue( &exc->context, FALSE );
    ERR( "RISC-V64 exception continuation failed: %#lx\\n", (unsigned long)status );
    abort();
}

static void riscv64_fault_handler( int sig, siginfo_t *info, void *opaque )
{
    ucontext_t *uc = opaque;
    struct riscv64_signal_exception *exc;
    ULONG_PTR sp = SP_sig( uc );
    unsigned int i;

    /* Resolve guard-page and stack-growth faults before writing to the
     * interrupted stack.  That stack may itself be the faulting mapping. */
    if (sig == SIGSEGV || sig == SIGBUS)
    {
        EXCEPTION_RECORD fault = {0};
        fault.ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
        fault.ExceptionAddress = (void *)(ULONG_PTR)PC_sig( uc );
        fault.NumberParameters = 2;
        fault.ExceptionInformation[0] = EXCEPTION_READ_FAULT;
        fault.ExceptionInformation[1] = (ULONG_PTR)info->si_addr;
        if (!virtual_handle_fault( get_thread_data(), &fault, (void *)sp )) return;
    }

    /* Allocate the exception payload on the interrupted stack, preserving
     * the LP64D 16-byte stack alignment.  SA_ONSTACK protects delivery. */
    exc = (void *)((sp - sizeof(*exc)) & ~(ULONG_PTR)15);
    memset( exc, 0, sizeof(*exc) );
    exc->context.ContextFlags = CONTEXT_FULL;
    exc->context.Pc = PC_sig( uc );
    for (i = 1; i < 32; ++i) exc->context.Gpr.X[i] = REGn_sig( i, uc );
    exc->context.Gpr.X[2] = sp;
#if defined(__linux__) && defined(__riscv_flen) && (__riscv_flen >= 64)
    /* Linux RISC-V signal ABI stores the D-extension register image
     * directly in mcontext, not behind an fpregs pointer. */
    for (i = 0; i < 32; ++i)
        exc->context.F[i].Q = uc->uc_mcontext.__fpregs.__d.__f[i];
    exc->context.Fcsr = uc->uc_mcontext.__fpregs.__d.__fcsr;
    exc->context.ContextFlags |= CONTEXT_FLOATING_POINT;
#endif
    exc->record.ExceptionAddress = (void *)(ULONG_PTR)exc->context.Pc;

    switch (sig)
    {
    case SIGSEGV:
    case SIGBUS:
        exc->record.ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
        exc->record.NumberParameters = 2;
        exc->record.ExceptionInformation[0] = EXCEPTION_READ_FAULT;
        exc->record.ExceptionInformation[1] = (ULONG_PTR)info->si_addr;
        break;
    case SIGILL:
        exc->record.ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
        break;
    case SIGTRAP:
        exc->record.ExceptionCode = EXCEPTION_BREAKPOINT;
        break;
    case SIGFPE:
        exc->record.ExceptionCode = EXCEPTION_FLT_INVALID_OPERATION;
#ifdef FPE_INTDIV
        if (info->si_code == FPE_INTDIV) exc->record.ExceptionCode = EXCEPTION_INT_DIVIDE_BY_ZERO;
#endif
#ifdef FPE_INTOVF
        if (info->si_code == FPE_INTOVF) exc->record.ExceptionCode = EXCEPTION_INT_OVERFLOW;
#endif
#ifdef FPE_FLTDIV
        if (info->si_code == FPE_FLTDIV) exc->record.ExceptionCode = EXCEPTION_FLT_DIVIDE_BY_ZERO;
#endif
        break;
    default:
        exc->record.ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
    }

    /* Faults on Wine's Unix syscall stack cannot be dispatched using
     * the interrupted kernel-stack pointer.  Return the exception status
     * through the saved Windows syscall frame instead. */
    if (is_inside_syscall( get_thread_data(), sp ))
    {
        struct syscall_frame *frame = get_syscall_frame( get_thread_data() );
        if (frame)
        {
            frame->x[10] = exc->record.ExceptionCode;
            REGn_sig( REG_A0, uc ) = (ULONG_PTR)frame;
            PC_sig( uc ) = (ULONG_PTR)__wine_syscall_dispatcher_return;
            return;
        }
    }

    SP_sig( uc ) = (ULONG_PTR)exc;
    REGn_sig( REG_A0, uc ) = (ULONG_PTR)exc;
    PC_sig( uc ) = (ULONG_PTR)riscv64_raise_signal_exception;
}

void signal_init_process( TEB *teb )
{
    struct sigaction action;

    alloc_syscall_frame( sizeof(struct syscall_frame) );
    memset( &action, 0, sizeof(action) );
    action.sa_mask = server_block_set;
    action.sa_flags = SA_SIGINFO | SA_RESTART | SA_ONSTACK;
    action.sa_sigaction = riscv64_fault_handler;
    if (sigaction( SIGSEGV, &action, NULL ) == -1 ||
        sigaction( SIGBUS, &action, NULL ) == -1 ||
        sigaction( SIGILL, &action, NULL ) == -1 ||
        sigaction( SIGTRAP, &action, NULL ) == -1 ||
        sigaction( SIGFPE, &action, NULL ) == -1)
    {
        perror( "sigaction" );
        exit( 1 );
    }
}


void DECLSPEC_NORETURN signal_start_thread( PRTL_THREAD_START_ROUTINE entry,
                                            void *arg, TEB *teb )
{
    struct syscall_frame *frame;

    frame = init_syscall_frame( entry, arg, teb );

    /*
     * Enter the RV64 dispatcher-return path with the syscall
     * frame pointer in a0.
     *
     * The indirect register form avoids an R_RISCV_JAL relocation
     * against an externally-bound symbol when building ntdll.so.
     */
    __asm__ volatile (
        "mv a0, %0\n"
        "jr %[dispatcher]\n"
        :
        : "r"(frame),
          [dispatcher] "r"(__wine_syscall_dispatcher_return)
        : "a0", "memory"
    );

    __builtin_unreachable();
}

/* A normal C function may introduce a prologue before the register restore.
 * GCC's naked attribute prevents it; the body contains basic asm only.
 * This does not yet solve complete t6 restoration or nested dispatch. */
void __attribute__((naked)) __wine_syscall_dispatcher_return( void )
{
    /*
     * a0 = struct syscall_frame *
     *
     * Use t1 (x6) for the frame pointer and t0 (x5) for the
     * destination PC. This permits restoration of t6 (x31).
     * A direct jalr requires one scratch GPR for the branch target,
     * so t0 cannot simultaneously be restored from an arbitrary CONTEXT.
     * Exact all-register continuation requires a sigreturn-based path.
     */
    __asm__ volatile (
        "mv t1, a0\n"
        "ld t0, 256(t1)\n"

        /* Restore floating-point state only for an explicit context update. */
        "lw t2, 544(t1)\n"
        "andi t2, t2, 4\n"
        "beqz t2, 1f\n"
        "fld f0, 280(t1)\n"
        "fld f1, 288(t1)\n"
        "fld f2, 296(t1)\n"
        "fld f3, 304(t1)\n"
        "fld f4, 312(t1)\n"
        "fld f5, 320(t1)\n"
        "fld f6, 328(t1)\n"
        "fld f7, 336(t1)\n"
        "fld f8, 344(t1)\n"
        "fld f9, 352(t1)\n"
        "fld f10, 360(t1)\n"
        "fld f11, 368(t1)\n"
        "fld f12, 376(t1)\n"
        "fld f13, 384(t1)\n"
        "fld f14, 392(t1)\n"
        "fld f15, 400(t1)\n"
        "fld f16, 408(t1)\n"
        "fld f17, 416(t1)\n"
        "fld f18, 424(t1)\n"
        "fld f19, 432(t1)\n"
        "fld f20, 440(t1)\n"
        "fld f21, 448(t1)\n"
        "fld f22, 456(t1)\n"
        "fld f23, 464(t1)\n"
        "fld f24, 472(t1)\n"
        "fld f25, 480(t1)\n"
        "fld f26, 488(t1)\n"
        "fld f27, 496(t1)\n"
        "fld f28, 504(t1)\n"
        "fld f29, 512(t1)\n"
        "fld f30, 520(t1)\n"
        "fld f31, 528(t1)\n"
        "ld t2, 536(t1)\n"
        "fscsr t2\n"
        "1:\n"

        "ld x1,  272(t1)\n"
        "ld x2,   16(t1)\n"
        /* Preserve native ELF gp, as well as tp. */
        /* x4/tp is native ELF TLS and must not be restored from CONTEXT. */
        
        "ld x7,   56(t1)\n"
        "ld x8,   64(t1)\n"
        "ld x9,   72(t1)\n"
        "ld x10,  80(t1)\n"
        "ld x11,  88(t1)\n"
        "ld x12,  96(t1)\n"
        "ld x13, 104(t1)\n"
        "ld x14, 112(t1)\n"
        "ld x15, 120(t1)\n"
        "ld x16, 128(t1)\n"
        "ld x17, 136(t1)\n"
        "ld x18, 144(t1)\n"
        "ld x19, 152(t1)\n"
        "ld x20, 160(t1)\n"
        "ld x21, 168(t1)\n"
        "ld x22, 176(t1)\n"
        "ld x23, 184(t1)\n"
        "ld x24, 192(t1)\n"
        "ld x25, 200(t1)\n"
        "ld x26, 208(t1)\n"
        "ld x27, 216(t1)\n"
        "ld x28, 224(t1)\n"
        "ld x29, 232(t1)\n"
        "ld x30, 240(t1)\n"

        "ld sp, 264(t1)\n"
        "ld x31, 248(t1)\n"\n        "ld x6, 48(t1)\n"

        "jr t0\n"
    );

    __builtin_unreachable();
}

/* RV64 NT-service bridge. The first eight arguments arrive in a0-a7;
 * additional arguments are read from the original caller stack before
 * the service is invoked on the Unix stack. */
static NTSTATUS __attribute__((used)) riscv64_dispatch_syscall( ULONG id, const ULONG64 *args, const ULONG64 *stack_args )
{
    struct thread_data *data = get_thread_data();
    SYSTEM_SERVICE_TABLE *tables;
    SYSTEM_SERVICE_TABLE *table;
    ULONG index = id & 0xfff;
    ULONG table_id = (id >> 12) & 3;
    ULONG arg_size;
    /* RV64 uses a0-a7 followed by eight-byte stack slots.  Some
     * win32u services have 17 arguments; allow up to 20 in total. */
    typedef NTSTATUS (*service_t)(ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR,
                                  ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR,
                                  ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR,
                                  ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR,
                                  ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR);
    service_t service;

    if (!data || !data->teb) return STATUS_INVALID_SYSTEM_SERVICE;
    tables = get_teb_data(data)->syscall_table;
    if (!tables) return STATUS_INVALID_SYSTEM_SERVICE;
    table = &tables[table_id];
    if (!table->ServiceTable || !table->ArgumentTable || index >= table->ServiceLimit)
        return STATUS_INVALID_SYSTEM_SERVICE;
    arg_size = table->ArgumentTable[index];
    /* The service argument table records a byte count, while the RV64 ABI
     * passes arguments in eight-byte slots.  Reject malformed entries
     * before reading any of the original caller's stack arguments. */
    if (arg_size % sizeof(ULONG_PTR))
    {
        ERR( "RISCV64 syscall %04x has misaligned argument size %u\n", id, arg_size );
        return STATUS_INVALID_SYSTEM_SERVICE;
    }
    if (arg_size > 20 * sizeof(ULONG_PTR))
    {
        ERR( "RISCV64 syscall %04x requires %u bytes of arguments (maximum %u)\n",
             id, arg_size, (unsigned int)(20 * sizeof(ULONG_PTR)) );
        return STATUS_NOT_IMPLEMENTED;
    }
    service = (service_t)table->ServiceTable[index];
    if (!service) return STATUS_INVALID_SYSTEM_SERVICE;
    return service( args[0], args[1], args[2], args[3],
                    args[4], args[5], args[6], args[7],
                    arg_size > 8 * sizeof(ULONG_PTR) ? stack_args[0] : 0,
                    arg_size > 9 * sizeof(ULONG_PTR) ? stack_args[1] : 0,
                    arg_size > 10 * sizeof(ULONG_PTR) ? stack_args[2] : 0,
                    arg_size > 11 * sizeof(ULONG_PTR) ? stack_args[3] : 0,
                    arg_size > 12 * sizeof(ULONG_PTR) ? stack_args[4] : 0,
                    arg_size > 13 * sizeof(ULONG_PTR) ? stack_args[5] : 0,
                    arg_size > 14 * sizeof(ULONG_PTR) ? stack_args[6] : 0,
                    arg_size > 15 * sizeof(ULONG_PTR) ? stack_args[7] : 0,
                    arg_size > 16 * sizeof(ULONG_PTR) ? stack_args[8] : 0,
                    arg_size > 17 * sizeof(ULONG_PTR) ? stack_args[9] : 0,
                    arg_size > 18 * sizeof(ULONG_PTR) ? stack_args[10] : 0,
                    arg_size > 19 * sizeof(ULONG_PTR) ? stack_args[11] : 0 );
}

/* The Windows stub tail-jumps here with t2 = NT service id, ra = caller.
 * Save arguments on the original stack before using the C ABI to locate
 * the per-thread syscall frame.  The syscall frame is at the high end of
 * Wine's Unix kernel stack; C calls grow downwards from it. */
__ASM_GLOBAL_FUNC( __wine_syscall_dispatcher,
                   "addi sp, sp, -96\n\t"
                   "sd ra, 0(sp)\n\t"
                   "sd s0, 8(sp)\n\t"
                   "sd s1, 16(sp)\n\t"
                   "sd a0, 24(sp)\n\t"
                   "sd a1, 32(sp)\n\t"
                   "sd a2, 40(sp)\n\t"
                   "sd a3, 48(sp)\n\t"
                   "sd a4, 56(sp)\n\t"
                   "sd a5, 64(sp)\n\t"
                   "sd a6, 72(sp)\n\t"
                   "sd t2, 80(sp)\n\t" /* syscall id */
                   "sd a7, 88(sp)\n\t" /* eighth argument */
                   "mv a0, sp\n\t"
                   "call " __ASM_NAME("riscv64_prepare_syscall") "\n\t"
                   "beqz a0, 2f\n\t"
                   "mv t0, a0\n\t"        /* frame */
                   "ld t1, 80(sp)\n\t"    /* service id */
                   "sw t1, 568(t0)\n\t"
                   "sw zero, 544(t0)\n\t" /* clear stale restore flags */
                   "ld t1, 0(sp)\n\t"     /* return address */
                   "sd t1, 272(t0)\n\t"
                   "sd t1, 256(t0)\n\t"
                   "addi t1, sp, 96\n\t"
                   "sd t1, 264(t0)\n\t"
                   "ld t1, 8(sp)\n\t"
                   "sd t1, 64(t0)\n\t"
                   "ld t1, 16(sp)\n\t"
                   "sd t1, 72(t0)\n\t"
                   "sd s2, 144(t0)\n\t"
                   "sd s3, 152(t0)\n\t"
                   "sd s4, 160(t0)\n\t"
                   "sd s5, 168(t0)\n\t"
                   "sd s6, 176(t0)\n\t"
                   "sd s7, 184(t0)\n\t"
                   "sd s8, 192(t0)\n\t"
                   "sd s9, 200(t0)\n\t"
                   "sd s10, 208(t0)\n\t"
                   "sd s11, 216(t0)\n\t"
                   "ld t1, 24(sp)\n\t"
                   "sd t1, 80(t0)\n\t"
                   "ld t1, 32(sp)\n\t"
                   "sd t1, 88(t0)\n\t"
                   "ld t1, 40(sp)\n\t"
                   "sd t1, 96(t0)\n\t"
                   "ld t1, 48(sp)\n\t"
                   "sd t1, 104(t0)\n\t"
                   "ld t1, 56(sp)\n\t"
                   "sd t1, 112(t0)\n\t"
                   "ld t1, 64(sp)\n\t"
                   "sd t1, 120(t0)\n\t"
                   "ld t1, 72(sp)\n\t"
                   "sd t1, 128(t0)\n\t"
                   "ld t1, 88(sp)\n\t"
                   "sd t1, 136(t0)\n\t"
                   "mv sp, t0\n\t"
                   "lw a0, 568(sp)\n\t"
                   "addi a1, sp, 80\n\t"
                   "ld a2, 264(sp)\n\t" /* original Windows stack args */
                   "call " __ASM_NAME("riscv64_dispatch_syscall") "\n\t"
                   /* A successful context-setting syscall must resume at the
                    * requested PC instead of returning to the syscall stub. */
                   "bnez a0, 1f\n\t"
                   "lw t1, 544(sp)\n\t"
                   "andi t1, t1, 7\n\t"
                   "beqz t1, 1f\n\t"
                   "mv a0, sp\n\t"
                   "la t1, " __ASM_NAME("__wine_syscall_dispatcher_return") "\n\t"
                   "jr t1\n\t"
                   "1:\t mv t0, sp\n\t"
                   "ld s0, 64(t0)\n\t"
                   "ld s1, 72(t0)\n\t"
                   "ld s2, 144(t0)\n\t"
                   "ld s3, 152(t0)\n\t"
                   "ld s4, 160(t0)\n\t"
                   "ld s5, 168(t0)\n\t"
                   "ld s6, 176(t0)\n\t"
                   "ld s7, 184(t0)\n\t"
                   "ld s8, 192(t0)\n\t"
                   "ld s9, 200(t0)\n\t"
                   "ld s10, 208(t0)\n\t"
                   "ld s11, 216(t0)\n\t"
                   "ld ra, 272(t0)\n\t"
                   "ld sp, 264(t0)\n\t"
                   "ret\n"
                   "2:\tld ra, 0(sp)\n\t"
                   "ld s0, 8(sp)\n\t"
                   "ld s1, 16(sp)\n\t"
                   "addi sp, sp, 96\n\t"
                   "li a0, 0xc000001c\n\t" /* STATUS_INVALID_SYSTEM_SERVICE */
                   "ret" )

/* Called before switching stacks.  Its return value is an existing
 * per-thread frame, or NULL if thread initialization is incomplete. */
static struct syscall_frame *__attribute__((used)) riscv64_prepare_syscall( void *unused )
{
    struct thread_data *data = get_thread_data();
    return data && data->teb ? get_syscall_frame(data) : NULL;
}
 

/* Unix-call entry: a0 = function table, a1 = function index,
 * a2 = argument block.  The entry runs on the Windows stack and the
 * selected Unix function must execute on Wine's kernel stack.
 *
 * This first implementation supports the normal (non-nested) call path.
 * Context-changing callbacks and signal unwinding still need integration.
 */
static ULONG_PTR __attribute__((used)) riscv64_dispatch_unix_call( const ULONG_PTR *table,
                                                                    ULONG index, void *args )
 {
    typedef ULONG_PTR (*unix_call_t)(void *);
    unix_call_t func;

    if (!table) return STATUS_INVALID_PARAMETER;
    func = (unix_call_t)table[index];
    if (!func) return STATUS_INVALID_PARAMETER;
    return func( args );
}

__ASM_GLOBAL_FUNC( __wine_unix_call_dispatcher,
                   "addi sp, sp, -128\n\t"
                   "sd ra, 0(sp)\n\t"
                   "sd s0, 8(sp)\n\t"
                   "sd s1, 16(sp)\n\t"
                   "sd a0, 24(sp)\n\t"
                   "sd a1, 32(sp)\n\t"
                   "sd a2, 40(sp)\n\t"
                   "sd s2, 48(sp)\n\t"
                   "sd s3, 56(sp)\n\t"
                   "sd s4, 64(sp)\n\t"
                   "sd s5, 72(sp)\n\t"
                   "sd s6, 80(sp)\n\t"
                   "sd s7, 88(sp)\n\t"
                   "sd s8, 96(sp)\n\t"
                   "sd s9, 104(sp)\n\t"
                   "sd s10, 112(sp)\n\t"
                   "sd s11, 120(sp)\n\t"
                   "mv a0, sp\n\t"
                   "call " __ASM_NAME("riscv64_prepare_syscall") "\n\t"
                   "beqz a0, 1f\n\t"
                   "mv t0, a0\n\t"
                   "ld t1, 0(sp)\n\t"
                   "sd t1, 272(t0)\n\t"
                   "addi t1, sp, 128\n\t"
                   "sd t1, 264(t0)\n\t"
                   "ld t1, 8(sp)\n\t"
                   "sd t1, 64(t0)\n\t"
                   "ld t1, 16(sp)\n\t"
                   "sd t1, 72(t0)\n\t"
                   "ld t1, 48(sp)\n\t"
                   "sd t1, 144(t0)\n\t"
                   "ld t1, 56(sp)\n\t"
                   "sd t1, 152(t0)\n\t"
                   "ld t1, 64(sp)\n\t"
                   "sd t1, 160(t0)\n\t"
                   "ld t1, 72(sp)\n\t"
                   "sd t1, 168(t0)\n\t"
                   "ld t1, 80(sp)\n\t"
                   "sd t1, 176(t0)\n\t"
                   "ld t1, 88(sp)\n\t"
                   "sd t1, 184(t0)\n\t"
                   "ld t1, 96(sp)\n\t"
                   "sd t1, 192(t0)\n\t"
                   "ld t1, 104(sp)\n\t"
                   "sd t1, 200(t0)\n\t"
                   "ld t1, 112(sp)\n\t"
                   "sd t1, 208(t0)\n\t"
                   "ld t1, 120(sp)\n\t"
                   "sd t1, 216(t0)\n\t"
                   "ld a0, 24(sp)\n\t"
                   "ld a1, 32(sp)\n\t"
                   "ld a2, 40(sp)\n\t"
                   "mv sp, t0\n\t"
                   "call " __ASM_NAME("riscv64_dispatch_unix_call") "\n\t"
                   "mv t0, sp\n\t"
                   "ld s0, 64(t0)\n\t"
                   "ld s1, 72(t0)\n\t"
                   "ld s2, 144(t0)\n\t"
                   "ld s3, 152(t0)\n\t"
                   "ld s4, 160(t0)\n\t"
                   "ld s5, 168(t0)\n\t"
                   "ld s6, 176(t0)\n\t"
                   "ld s7, 184(t0)\n\t"
                   "ld s8, 192(t0)\n\t"
                   "ld s9, 200(t0)\n\t"
                   "ld s10, 208(t0)\n\t"
                   "ld s11, 216(t0)\n\t"
                   "ld ra, 272(t0)\n\t"
                   "ld sp, 264(t0)\n\t"
                   "ret\n"
                   "1:\tld ra, 0(sp)\n\t"
                   "ld s0, 8(sp)\n\t"
                   "ld s1, 16(sp)\n\t"
                   "ld s2, 48(sp)\n\t"
                   "ld s3, 56(sp)\n\t"
                   "ld s4, 64(sp)\n\t"
                   "ld s5, 72(sp)\n\t"
                   "ld s6, 80(sp)\n\t"
                   "ld s7, 88(sp)\n\t"
                   "ld s8, 96(sp)\n\t"
                   "ld s9, 104(sp)\n\t"
                   "ld s10, 112(sp)\n\t"
                   "ld s11, 120(sp)\n\t"
                   "addi sp, sp, 128\n\t"
                   "li a0, 0xc000000d\n\t" /* STATUS_INVALID_PARAMETER */
                   "ret" )

/***********************************************************************
 *           unwind_builtin_dll
 *
 * RISC-V ELF unwinding is not implemented yet.
 */
NTSTATUS unwind_builtin_dll( void *args )
{
    return STATUS_UNSUCCESSFUL;
}

void *get_native_context( CONTEXT *context )
{
    return context;
}


void *get_wow_context( CONTEXT *context )
{
    return NULL;
}

NTSTATUS DECLSPEC_EXPORT KeUserModeCallback( ULONG id, const void *args, ULONG len,
                             void **ret_ptr, ULONG *ret_len )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI NtCallbackReturn( void *ret_ptr, ULONG ret_len, NTSTATUS status )
{
    return STATUS_NOT_IMPLEMENTED;
}

#endif /* defined(__riscv) && (__riscv_xlen == 64) */

