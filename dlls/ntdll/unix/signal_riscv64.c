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

#include "dwarf.h"

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
    CONTEXT context = { CONTEXT_FULL };
    CONTEXT *ctx;
    ULONG_PTR stack_top;
    unsigned int i;

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

    if (data->suspend)
    {
        context.ContextFlags |= CONTEXT_EXCEPTION_REPORTING | CONTEXT_EXCEPTION_ACTIVE;
        wait_suspend( &context );
    }

    /* Apply any debugger-supplied stack before placing the initial context. */
    ctx = (CONTEXT *)((context.Gpr.X[2] - sizeof(*ctx)) & ~(ULONG_PTR)15);
    *ctx = context;
    ctx->ContextFlags = CONTEXT_FULL;

    /*
     * First enter LdrInitializeThunk with the initial CONTEXT.
     * It will initialize the loader and continue at RtlUserThreadStart.
     */
    memcpy( frame->x, ctx->Gpr.X, sizeof(frame->x) );
    for (i = 0; i < 32; ++i) frame->f[i] = ctx->F[i].Q;
    frame->fcsr = ctx->Fcsr;
    frame->ra = ctx->Gpr.X[1];
    frame->pc = (ULONG64)pLdrInitializeThunk;
    frame->sp = (ULONG64)ctx;
    frame->x[10] = (ULONG64)ctx;
    frame->x[4] = (ULONG64)__builtin_thread_pointer();
    frame->x[2] = frame->sp;
    frame->x[1] = frame->ra;

    frame->restore_flags = CONTEXT_FULL & ~CONTEXT_RISCV64;

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

    if (!status && frame && (context->ContextFlags & (CONTEXT_INTEGER & ~CONTEXT_RISCV64)))
        frame->restore_flags |= CONTEXT_INTEGER & ~CONTEXT_RISCV64;

    return status;
}


NTSTATUS WINAPI NtGetContextThread( HANDLE handle, CONTEXT *context )
{
    struct thread_data *data = get_thread_data();
    struct syscall_frame *frame = get_syscall_frame( data );
    DWORD flags = context->ContextFlags & ~CONTEXT_RISCV64;

    if ((context->ContextFlags & CONTEXT_RISCV64) != CONTEXT_RISCV64)
        return STATUS_INVALID_PARAMETER;
    if (flags & ~(CONTEXT_FULL | CONTEXT_EXCEPTION_REQUEST | CONTEXT_EXCEPTION_REPORTING |
                  CONTEXT_EXCEPTION_ACTIVE | CONTEXT_SERVICE_ACTIVE))
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
    {
        context->Gpr.X[0] = 0;
        context->Gpr.X[1] = frame->ra;
        memcpy( context->Gpr.X + 3, frame->x + 3, 29 * sizeof(frame->x[0]) );
    }

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

    set_context_exception_reporting_flags( &context->ContextFlags, CONTEXT_SERVICE_ACTIVE );
    return STATUS_SUCCESS;
}


NTSTATUS WINAPI NtSetContextThread( HANDLE handle, const CONTEXT *context )
{
    struct thread_data *data = get_thread_data();
    struct syscall_frame *frame = get_syscall_frame( data );
    DWORD flags = context->ContextFlags & ~CONTEXT_RISCV64;

    if ((context->ContextFlags & CONTEXT_RISCV64) != CONTEXT_RISCV64)
        return STATUS_INVALID_PARAMETER;
    if (flags & ~(CONTEXT_FULL | CONTEXT_EXCEPTION_REQUEST | CONTEXT_EXCEPTION_REPORTING |
                  CONTEXT_EXCEPTION_ACTIVE | CONTEXT_SERVICE_ACTIVE))
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
    {
        frame->x[0] = 0;
        frame->ra = frame->x[1] = context->Gpr.X[1];
        memcpy( frame->x + 3, context->Gpr.X + 3, 29 * sizeof(frame->x[0]) );
    }

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
    frame->restore_flags |= CONTEXT_CONTROL & ~CONTEXT_RISCV64;
    return status;
}

void call_raise_user_exception_dispatcher( struct thread_data *data )
{
    struct syscall_frame *frame = get_syscall_frame( data );

    frame->pc = (ULONG64)pKiRaiseUserExceptionDispatcher;
    frame->restore_flags |= CONTEXT_CONTROL & ~CONTEXT_RISCV64;
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
    frame->restore_flags |= CONTEXT_CONTROL & ~CONTEXT_RISCV64;
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


/* Linux reserves greg[0] for PC; x0 must never be copied from it. */
static void save_context( CONTEXT *context, const ucontext_t *uc )
{
    unsigned int i;

    memset( context, 0, sizeof(*context) );
    context->ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
    context->Pc = PC_sig( uc );
    for (i = 1; i < 32; ++i) context->Gpr.X[i] = REGn_sig( i, uc );
#if defined(__linux__) && defined(__riscv_flen) && (__riscv_flen >= 64)
    for (i = 0; i < 32; ++i) context->F[i].Q = uc->uc_mcontext.__fpregs.__d.__f[i];
    context->Fcsr = uc->uc_mcontext.__fpregs.__d.__fcsr;
    context->ContextFlags |= CONTEXT_FLOATING_POINT;
#endif
}

static void restore_context( const CONTEXT *context, ucontext_t *uc )
{
    DWORD flags = context->ContextFlags & ~CONTEXT_RISCV64;
    unsigned int i;

    if (flags & CONTEXT_INTEGER)
        for (i = 1; i < 32; ++i)
            /* gp/tp belong to the native ELF runtime, and SP is control state. */
            if (i != 2 && i != 3 && i != 4) REGn_sig( i, uc ) = context->Gpr.X[i];
    if (flags & CONTEXT_CONTROL)
    {
        PC_sig( uc ) = context->Pc;
        SP_sig( uc ) = context->Gpr.X[2];
        RA_sig( uc ) = context->Gpr.X[1];
    }
#if defined(__linux__) && defined(__riscv_flen) && (__riscv_flen >= 64)
    if (flags & CONTEXT_FLOATING_POINT)
    {
        for (i = 0; i < 32; ++i) uc->uc_mcontext.__fpregs.__d.__f[i] = context->F[i].Q;
        uc->uc_mcontext.__fpregs.__d.__fcsr = context->Fcsr;
    }
#endif
}

/* As on ARM64/x86-64, publish the user context while wineserver suspends
 * the thread, then apply any context updates made before ResumeThread. */
static void usr1_handler( int sig, siginfo_t *info, void *opaque )
{
    ucontext_t *uc = opaque;
    struct thread_data *data = get_thread_data();
    CONTEXT context;

    if (!data->teb)
        server_select( NULL, 0, SELECT_INTERRUPTIBLE, 0, NULL, NULL );
    else if (is_inside_syscall( data, SP_sig( uc ) ))
    {
        context.ContextFlags = CONTEXT_FULL | CONTEXT_EXCEPTION_REQUEST;
        if (NtGetContextThread( GetCurrentThread(), &context )) return;
        wait_suspend( &context );
        NtSetContextThread( GetCurrentThread(), &context );
    }
    else
    {
        save_context( &context, uc );
        context.ContextFlags |= CONTEXT_EXCEPTION_REPORTING;
        wait_suspend( &context );
        restore_context( &context, uc );
    }
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
    save_context( &exc->context, uc );
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

/* A kernel-created signal frame is safer than fabricating an rt_sigframe:
 * its extension records, signal mask and altstack layout match this kernel.
 * The signal restorer invokes rt_sigreturn, restoring all GPRs including
 * t0/t6 and the exact requested PC in one atomic kernel transition.
 * Remote context updates are exchanged with wineserver in usr1_handler.
 * SIGUSR2 is reserved here for our synchronous continuation request. */
static __thread struct syscall_frame *riscv64_sigreturn_frame;
static __thread sigset_t riscv64_sigreturn_mask;

static void riscv64_sigreturn_handler( int sig, siginfo_t *info, void *opaque )
{
    ucontext_t *uc = opaque;
    struct syscall_frame *frame = riscv64_sigreturn_frame;
    unsigned int i;

    if (!frame) return;
    riscv64_sigreturn_frame = NULL;
    uc->uc_sigmask = riscv64_sigreturn_mask;
    PC_sig( uc ) = frame->pc;
    for (i = 1; i < 32; ++i)
        if (i != 3 && i != 4) REGn_sig( i, uc ) = frame->x[i];
    REGn_sig( 1, uc ) = frame->ra;
    REGn_sig( 2, uc ) = frame->sp;
    /* gp and tp belong to the host ELF runtime; retain the signal-time values. */
#if defined(__linux__) && defined(__riscv_flen) && (__riscv_flen >= 64)
    if (frame->restore_flags & (CONTEXT_FLOATING_POINT & ~CONTEXT_RISCV64))
    {
        for (i = 0; i < 32; ++i) uc->uc_mcontext.__fpregs.__d.__f[i] = frame->f[i];
        uc->uc_mcontext.__fpregs.__d.__fcsr = frame->fcsr;
    }
#endif
}

/* Called on the Unix stack with the desired CONTEXT already in frame.
 * raise() delivers SIGUSR2 to this thread; the signal handler edits the
 * authentic kernel ucontext and returning from it executes rt_sigreturn.
 * This intentionally does not use a manually forged signal frame. */
static void __attribute__((noreturn,used)) riscv64_sigreturn_continue( struct syscall_frame *frame )
{
    sigset_t set, oldmask;

    sigemptyset( &set );
    sigaddset( &set, SIGUSR2 );
    if (pthread_sigmask( SIG_UNBLOCK, &set, &oldmask )) abort();
    /* A nonlocal continuation can bypass all ordinary dispatcher epilogues.
     * If the target is outside the Unix kernel stack, discard nested frame
     * links before rt_sigreturn resumes the caller's stack.  Keep the
     * supplied frame itself alive until the SIGUSR2 handler has consumed it. */
    {
        struct thread_data *data = get_thread_data();
        ULONG_PTR target_sp = frame->sp;
        ULONG_PTR base = (ULONG_PTR)get_kernel_stack( data );
        if (data->teb && (target_sp < base || target_sp >= base + kernel_stack_size))
        {
            struct syscall_frame *root = get_syscall_frame( data );
            unsigned int depth = 0;
            while (root && root->prev_frame && depth++ < 64) root = root->prev_frame;
            if (root && !root->prev_frame) get_teb_data( data )->syscall_frame = root;
            else WARN( "invalid RISC-V64 nested syscall-frame chain during continuation\n" );
        }
    }
    riscv64_sigreturn_mask = oldmask;
    riscv64_sigreturn_frame = frame;
    if (raise( SIGUSR2 )) abort();
    /* If the signal was not delivered synchronously, do not run with
     * partially restored registers or return to the wrong stack. */
    abort();
}

void signal_init_process( TEB *teb )
{
    struct sigaction action;

    alloc_syscall_frame( sizeof(struct syscall_frame) );
    memset( &action, 0, sizeof(action) );
    action.sa_mask = server_block_set;
    action.sa_flags = SA_SIGINFO | SA_RESTART | SA_ONSTACK;
    action.sa_sigaction = usr1_handler;
    if (sigaction( SIGUSR1, &action, NULL ) == -1)
    {
        perror( "sigaction(SIGUSR1)" );
        exit( 1 );
    }
    action.sa_sigaction = riscv64_sigreturn_handler;
    if (sigaction( SIGUSR2, &action, NULL ) == -1)
    {
        perror( "sigaction(SIGUSR2)" );
        exit( 1 );
    }
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
    pthread_sigmask( SIG_UNBLOCK, &server_block_set, NULL );

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

/* Keep the frame argument intact while tail-calling the continuation helper. */
/* Entry receives the syscall frame in a0 and never returns.  The
 * synchronous SIGUSR2 handler edits the kernel-created signal frame;
 * rt_sigreturn then restores all GPRs and the target PC. */
void __attribute__((naked)) __wine_syscall_dispatcher_return( void )
{
    __asm__ volatile ( "tail " __ASM_NAME("riscv64_sigreturn_continue") "\n" );
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

/* a0 points to 32 double registers followed by the floating-point CSR. */
__ASM_GLOBAL_FUNC( riscv64_save_fpu,
                   "fsd f0, 0(a0)\n\t"
                   "fsd f1, 8(a0)\n\t"
                   "fsd f2, 16(a0)\n\t"
                   "fsd f3, 24(a0)\n\t"
                   "fsd f4, 32(a0)\n\t"
                   "fsd f5, 40(a0)\n\t"
                   "fsd f6, 48(a0)\n\t"
                   "fsd f7, 56(a0)\n\t"
                   "fsd f8, 64(a0)\n\t"
                   "fsd f9, 72(a0)\n\t"
                   "fsd f10, 80(a0)\n\t"
                   "fsd f11, 88(a0)\n\t"
                   "fsd f12, 96(a0)\n\t"
                   "fsd f13, 104(a0)\n\t"
                   "fsd f14, 112(a0)\n\t"
                   "fsd f15, 120(a0)\n\t"
                   "fsd f16, 128(a0)\n\t"
                   "fsd f17, 136(a0)\n\t"
                   "fsd f18, 144(a0)\n\t"
                   "fsd f19, 152(a0)\n\t"
                   "fsd f20, 160(a0)\n\t"
                   "fsd f21, 168(a0)\n\t"
                   "fsd f22, 176(a0)\n\t"
                   "fsd f23, 184(a0)\n\t"
                   "fsd f24, 192(a0)\n\t"
                   "fsd f25, 200(a0)\n\t"
                   "fsd f26, 208(a0)\n\t"
                   "fsd f27, 216(a0)\n\t"
                   "fsd f28, 224(a0)\n\t"
                   "fsd f29, 232(a0)\n\t"
                   "fsd f30, 240(a0)\n\t"
                   "fsd f31, 248(a0)\n\t"
                   "frcsr t0\n\t"
                   "sd t0, 256(a0)\n\t"
                   "ret" )

/* The Windows stub tail-jumps here with t2 = NT service id, ra = caller.
 * Save arguments on the original stack before using the C ABI to locate
 * the per-thread syscall frame.  The syscall frame is at the high end of
 * Wine's Unix kernel stack; C calls grow downwards from it. */
__ASM_GLOBAL_FUNC( __wine_syscall_dispatcher,
                   "addi sp, sp, -368\n\t"
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
                   "addi a0, sp, 96\n\t"
                   "call " __ASM_NAME("riscv64_save_fpu") "\n\t"
                   "mv a0, sp\n\t"
                   "call " __ASM_NAME("riscv64_prepare_syscall") "\n\t"
                   "beqz a0, 2f\n\t"
                   "mv t0, a0\n\t"        /* frame */
                   "addi t1, sp, 96\n\t"
                   "addi t2, t0, 280\n\t"
                   "li a3, 33\n\t"
                   "3: ld a4, 0(t1)\n\t"
                   "sd a4, 0(t2)\n\t"
                   "addi t1, t1, 8\n\t"
                   "addi t2, t2, 8\n\t"
                   "addi a3, a3, -1\n\t"
                   "bnez a3, 3b\n\t"
                   "sd gp, 24(t0)\n\t"
                   "sd tp, 32(t0)\n\t"
                   "ld t1, 80(sp)\n\t"    /* service id */
                   "sw t1, 568(t0)\n\t"
                   "sw zero, 544(t0)\n\t" /* clear stale restore flags */
                   "ld t1, 0(sp)\n\t"     /* return address */
                   "sd t1, 272(t0)\n\t"
                   "sd t1, 256(t0)\n\t"
                   "addi t1, sp, 368\n\t"
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
                   /* Honor context requests even when the service returns a
                    * nonzero status (for example, an interrupted wait). */
                   "lw t1, 544(sp)\n\t"
                   "andi t1, t1, 7\n\t"
                   "beqz t1, 1f\n\t"
                   "lw t1, 544(sp)\n\t"
                   "andi t1, t1, 2\n\t"
                   "bnez t1, 5f\n\t"
                   "sd a0, 80(sp)\n\t"
                   "5:\n\t"
                   "mv t0, sp\n\t"
                   "addi sp, sp, -16\n\t"
                   "sd t0, 0(sp)\n\t"
                   "mv a0, t0\n\t"
                   "call " __ASM_NAME("riscv64_finish_syscall") "\n\t"
                   "ld a0, 0(sp)\n\t"
                   "la t1, " __ASM_NAME("__wine_syscall_dispatcher_return") "\n\t"
                   "jr t1\n\t"
                   "1:\t mv t0, sp\n\t"
                   "addi sp, sp, -16\n\t"
                   "sd a0, 0(sp)\n\t"
                   "mv a0, t0\n\t"
                   "call " __ASM_NAME("riscv64_finish_syscall") "\n\t"
                   "ld a0, 0(sp)\n\t"
                   "addi sp, sp, 16\n\t"
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
                   "fld f8, 344(t0)\n\t"
                   "fld f9, 352(t0)\n\t"
                   "fld f18, 424(t0)\n\t"
                   "fld f19, 432(t0)\n\t"
                   "fld f20, 440(t0)\n\t"
                   "fld f21, 448(t0)\n\t"
                   "fld f22, 456(t0)\n\t"
                   "fld f23, 464(t0)\n\t"
                   "fld f24, 472(t0)\n\t"
                   "fld f25, 480(t0)\n\t"
                   "fld f26, 488(t0)\n\t"
                   "fld f27, 496(t0)\n\t"
                   "ld t1, 536(t0)\n\t"
                   "fscsr t1\n\t"
                   "ld ra, 272(t0)\n\t"
                   "ld sp, 264(t0)\n\t"
                   "ret\n"
                   "2:\tld ra, 0(sp)\n\t"
                   "ld s0, 8(sp)\n\t"
                   "ld s1, 16(sp)\n\t"
                   "fld f8, 160(sp)\n\t"
                   "fld f9, 168(sp)\n\t"
                   "fld f18, 240(sp)\n\t"
                   "fld f19, 248(sp)\n\t"
                   "fld f20, 256(sp)\n\t"
                   "fld f21, 264(sp)\n\t"
                   "fld f22, 272(sp)\n\t"
                   "fld f23, 280(sp)\n\t"
                   "fld f24, 288(sp)\n\t"
                   "fld f25, 296(sp)\n\t"
                   "fld f26, 304(sp)\n\t"
                   "fld f27, 312(sp)\n\t"
                   "ld t1, 352(sp)\n\t"
                   "fscsr t1\n\t"
                   "addi sp, sp, 368\n\t"
                   "li a0, 0xc000001c\n\t" /* STATUS_INVALID_SYSTEM_SERVICE */
                   "ret" )

/* The top-of-kernel-stack frame belongs to the thread.  Nested entries
 * allocate a temporary frame below the interrupted Unix stack pointer,
 * leaving the outer frame and its live C stack untouched.  The entry
 * assembly saves its arguments before calling this helper. */
static struct syscall_frame *__attribute__((used)) riscv64_prepare_syscall( void *entry_sp )
{
    struct thread_data *data = get_thread_data();
    struct syscall_frame *frame, *nested;
    ULONG_PTR sp = (ULONG_PTR)entry_sp;
    ULONG_PTR stack_base, stack_end, frame_addr;

    if (!data || !data->teb) return NULL;
    frame = get_syscall_frame( data );
    if (!frame) return NULL;

    stack_base = (ULONG_PTR)get_kernel_stack( data );
    stack_end = stack_base + kernel_stack_size;
    if (sp < stack_base || sp >= stack_end)
    {
        /* A callback on a different stack cannot be nested safely without
         * tracking the suspended Unix SP.  In particular, do not reuse the
         * base frame while an outer Unix transition is still active. */
        if (frame->prev_frame)
        {
            WARN( "rejecting cross-stack RISC-V64 callback at %p (active frame %p)\n",
                  entry_sp, frame );
            return NULL;
        }
        frame->prev_frame = NULL;
        return frame;
    }

    /* Keep room for the C helper and a guard against kernel-stack exhaustion.
     * All RISC-V LP64 stack pointers and frame allocations are 16-aligned. */
    /* Reserve a scratch gap for this C helper and its callees; placing the
     * frame immediately below entry_sp would overwrite the helper's own
     * stack while memset() initializes the nested frame. */
    if (sp < stack_base + 8192 + 2048 + sizeof(*nested)) return NULL;
    frame_addr = (sp - 2048 - sizeof(*nested)) & ~(ULONG_PTR)15;
    nested = (struct syscall_frame *)frame_addr;
    memset( nested, 0, sizeof(*nested) );
    nested->prev_frame = frame;
    get_teb_data( data )->syscall_frame = nested;
    return nested;
}

/* Called while sp still points to the completed frame.  The caller must
 * preserve the service return value across this C call. */
static void __attribute__((used)) riscv64_finish_syscall( struct syscall_frame *frame )
{
    struct thread_data *data = get_thread_data();

    if (data && data->teb && frame->prev_frame)
        get_teb_data( data )->syscall_frame = frame->prev_frame;
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
                   "addi sp, sp, -400\n\t"
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
                   "addi a0, sp, 128\n\t"
                   "call " __ASM_NAME("riscv64_save_fpu") "\n\t"
                   "mv a0, sp\n\t"
                   "call " __ASM_NAME("riscv64_prepare_syscall") "\n\t"
                   "beqz a0, 1f\n\t"
                   "mv t0, a0\n\t"
                   "addi t1, sp, 128\n\t"
                   "addi t2, t0, 280\n\t"
                   "li a3, 33\n\t"
                   "3: ld a4, 0(t1)\n\t"
                   "sd a4, 0(t2)\n\t"
                   "addi t1, t1, 8\n\t"
                   "addi t2, t2, 8\n\t"
                   "addi a3, a3, -1\n\t"
                   "bnez a3, 3b\n\t"
                   "sd gp, 24(t0)\n\t"
                   "sd tp, 32(t0)\n\t"
                   /* Keep Unix-call frame bookkeeping consistent with the
                    * NT syscall entry path.  The same frame is reused for
                    * successive non-nested transitions; without clearing
                    * restore_flags a previous NtContinue can leave a stale
                    * context request behind. */
                   "sw zero, 544(t0)\n\t"
                   "ld t1, 0(sp)\n\t"
                   "sd t1, 272(t0)\n\t"
                   "sd t1, 256(t0)\n\t"
                   "addi t1, sp, 400\n\t"
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
                   "lw t1, 544(sp)\n\t"
                   "andi t1, t1, 7\n\t"
                   "beqz t1, 4f\n\t"
                   "lw t1, 544(sp)\n\t"
                   "andi t1, t1, 2\n\t"
                   "bnez t1, 5f\n\t"
                   "sd a0, 80(sp)\n\t"
                   "5:\n\t"
                   "mv t0, sp\n\t"
                   "addi sp, sp, -16\n\t"
                   "sd t0, 0(sp)\n\t"
                   "mv a0, t0\n\t"
                   "call " __ASM_NAME("riscv64_finish_syscall") "\n\t"
                   "ld a0, 0(sp)\n\t"
                   "la t1, " __ASM_NAME("__wine_syscall_dispatcher_return") "\n\t"
                   "jr t1\n\t"
                   "4:\n\t"
                   "mv t0, sp\n\t"
                   "addi sp, sp, -16\n\t"
                   "sd a0, 0(sp)\n\t"
                   "mv a0, t0\n\t"
                   "call " __ASM_NAME("riscv64_finish_syscall") "\n\t"
                   "ld a0, 0(sp)\n\t"
                   "addi sp, sp, 16\n\t"
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
                   "fld f8, 344(t0)\n\t"
                   "fld f9, 352(t0)\n\t"
                   "fld f18, 424(t0)\n\t"
                   "fld f19, 432(t0)\n\t"
                   "fld f20, 440(t0)\n\t"
                   "fld f21, 448(t0)\n\t"
                   "fld f22, 456(t0)\n\t"
                   "fld f23, 464(t0)\n\t"
                   "fld f24, 472(t0)\n\t"
                   "fld f25, 480(t0)\n\t"
                   "fld f26, 488(t0)\n\t"
                   "fld f27, 496(t0)\n\t"
                   "ld t1, 536(t0)\n\t"
                   "fscsr t1\n\t"
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
                   "fld f8, 192(sp)\n\t"
                   "fld f9, 200(sp)\n\t"
                   "fld f18, 272(sp)\n\t"
                   "fld f19, 280(sp)\n\t"
                   "fld f20, 288(sp)\n\t"
                   "fld f21, 296(sp)\n\t"
                   "fld f22, 304(sp)\n\t"
                   "fld f23, 312(sp)\n\t"
                   "fld f24, 320(sp)\n\t"
                   "fld f25, 328(sp)\n\t"
                   "fld f26, 336(sp)\n\t"
                   "fld f27, 344(sp)\n\t"
                   "ld t1, 384(sp)\n\t"
                   "fscsr t1\n\t"
                   "addi sp, sp, 400\n\t"
                   "li a0, 0xc000000d\n\t" /* STATUS_INVALID_PARAMETER */
                   "ret" )

/***********************************************************************
 *           unwind_builtin_dll
 *
 * Locate the native ELF FDE for a Winelib call site.  CFI evaluation
 * and register restoration are not implemented by this preparatory change.
 */
/* Virtual unwind a native RISC-V64 ELF frame using the common DWARF
 * instruction interpreter.  This does not mutate the live syscall stack. */
static NTSTATUS riscv64_dwarf_virtual_unwind( CONTEXT *context, DISPATCHER_CONTEXT *dispatch,
                                               const struct dwarf_fde *fde,
                                               const struct dwarf_eh_bases *bases )
{
    const struct dwarf_cie *cie;
    const unsigned char *ptr, *augmentation, *end;
    struct frame_info info;
    struct frame_state saved_states[MAX_SAVED_STATES];
    CONTEXT next = *context;
    ULONG_PTR length, code_end, old_pc = context->Pc, old_sp = context->Gpr.X[2];
    unsigned char lsda_encoding = DW_EH_PE_omit;
    int has_z = 0;

    memset( &info, 0, sizeof(info) );
    info.state_stack = saved_states;
    info.ip = (ULONG_PTR)bases->func;
    cie = (const struct dwarf_cie *)((const char *)&fde->cie_offset - fde->cie_offset);
    if (cie->version != 1 && cie->version != 3 && cie->version != 4)
        return STATUS_INVALID_DISPOSITION;
    ptr = cie->augmentation + strlen((const char *)cie->augmentation) + 1;
    if (cie->version == 4)
    {
        if (ptr[0] != sizeof(ULONG_PTR) || ptr[1]) return STATUS_INVALID_DISPOSITION;
        ptr += 2;
    }
    info.code_align = dwarf_get_uleb128( &ptr );
    info.data_align = dwarf_get_sleb128( &ptr );
    info.retaddr_reg = cie->version == 1 ? *ptr++ : dwarf_get_uleb128( &ptr );
    if (info.retaddr_reg != 1) return STATUS_INVALID_DISPOSITION;
    info.state.cfa_rule = RULE_CFA_OFFSET;

    end = NULL;
    for (augmentation = cie->augmentation; *augmentation; ++augmentation)
    {
        switch (*augmentation)
        {
        case 'z':
            length = dwarf_get_uleb128( &ptr );
            end = ptr + length;
            has_z = 1;
            break;
        case 'L': lsda_encoding = *ptr++; break;
        case 'P':
        {
            unsigned char encoding = *ptr++;
            /* This is a native DWARF personality, not a Windows SEH handler. */
            (void)dwarf_get_ptr( &ptr, encoding, bases );
            break;
        }
        case 'R': info.fde_encoding = *ptr++; break;
        case 'S': info.signal_frame = 1; break;
        default:
            WARN( "unknown RISC-V64 CIE augmentation '%c'\n", *augmentation );
            return STATUS_INVALID_DISPOSITION;
        }
    }
    if (end) ptr = end;
    end = (const unsigned char *)(&cie->length + 1) + cie->length;
    execute_cfa_instructions( ptr, end, old_pc, &info, bases );

    ptr = (const unsigned char *)(fde + 1);
    info.ip = dwarf_get_ptr( &ptr, info.fde_encoding, bases );
    code_end = info.ip + dwarf_get_ptr( &ptr, info.fde_encoding & 0x0f, bases );
    if (old_pc < info.ip || old_pc > code_end) return STATUS_INVALID_DISPOSITION;
    end = NULL;
    if (has_z)
    {
        length = dwarf_get_uleb128( &ptr );
        end = ptr + length;
    }
    if (lsda_encoding != DW_EH_PE_omit)
        dispatch->HandlerData = (void *)dwarf_get_ptr( &ptr, lsda_encoding, bases );
    else dispatch->HandlerData = NULL;
    if (end) ptr = end;
    end = (const unsigned char *)(&fde->length + 1) + fde->length;
    execute_cfa_instructions( ptr, end, old_pc, &info, bases );

    apply_frame_state( &next, &info.state, bases );
    next.Pc = next.Gpr.X[1];
    next.Gpr.X[0] = 0;
    if (!next.Pc || next.Gpr.X[2] <= old_sp) return STATUS_INVALID_DISPOSITION;
    dispatch->LanguageHandler = NULL;
    dispatch->EstablisherFrame = old_sp;
    *context = next;
    TRACE( "RISC-V64 ELF unwind %p/%p -> %p/%p\n",
           (void *)old_pc, (void *)old_sp,
           (void *)(ULONG_PTR)context->Pc, (void *)(ULONG_PTR)context->Gpr.X[2] );
    return STATUS_SUCCESS;
}

NTSTATUS unwind_builtin_dll( void *args )
{
    struct unwind_builtin_dll_params *params = args;
    struct dwarf_eh_bases bases;
    const struct dwarf_fde *fde;

    if (!params || !params->context || !params->dispatch)
        return STATUS_INVALID_PARAMETER;
    if (!params->context->Pc) return STATUS_UNSUCCESSFUL;
    fde = _Unwind_Find_FDE( (void *)(ULONG_PTR)(params->context->Pc - 1), &bases );
    if (!fde) return STATUS_UNSUCCESSFUL;
    return riscv64_dwarf_virtual_unwind( params->context,
                                          (DISPATCHER_CONTEXT *)params->dispatch, fde, &bases );
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

