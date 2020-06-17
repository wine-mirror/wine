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

#if 0
#pragma makedep unix
#endif

#ifdef __aarch64__

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
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
#ifdef HAVE_LIBUNWIND
# define UNW_LOCAL_ONLY
# include <libunwind.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winnt.h"
#include "winternl.h"
#include "wine/exception.h"
#include "wine/asm.h"
#include "unix_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);

/***********************************************************************
 * signal context platform-specific definitions
 */
#ifdef linux

/* All Registers access - only for local access */
# define REG_sig(reg_name, context) ((context)->uc_mcontext.reg_name)
# define REGn_sig(reg_num, context) ((context)->uc_mcontext.regs[reg_num])

/* Special Registers access  */
# define SP_sig(context)            REG_sig(sp, context)    /* Stack pointer */
# define PC_sig(context)            REG_sig(pc, context)    /* Program counter */
# define PSTATE_sig(context)        REG_sig(pstate, context) /* Current State Register */
# define FP_sig(context)            REGn_sig(29, context)    /* Frame pointer */
# define LR_sig(context)            REGn_sig(30, context)    /* Link Register */

static struct _aarch64_ctx *get_extended_sigcontext( ucontext_t *sigcontext, unsigned int magic )
{
    struct _aarch64_ctx *ctx = (struct _aarch64_ctx *)sigcontext->uc_mcontext.__reserved;
    while ((char *)ctx < (char *)(&sigcontext->uc_mcontext + 1) && ctx->magic && ctx->size)
    {
        if (ctx->magic == magic) return ctx;
        ctx = (struct _aarch64_ctx *)((char *)ctx + ctx->size);
    }
    return NULL;
}

static struct fpsimd_context *get_fpsimd_context( ucontext_t *sigcontext )
{
    return (struct fpsimd_context *)get_extended_sigcontext( sigcontext, FPSIMD_MAGIC );
}

static DWORD64 get_fault_esr( ucontext_t *sigcontext )
{
    struct esr_context *esr = (struct esr_context *)get_extended_sigcontext( sigcontext, ESR_MAGIC );
    if (esr) return esr->esr;
    return 0;
}

#endif /* linux */

static pthread_key_t teb_key;

static const size_t teb_size = 0x2000;  /* we reserve two pages for the TEB */

typedef void (*raise_func)( EXCEPTION_RECORD *rec, CONTEXT *context );

/* stack layout when calling an exception raise function */
struct stack_layout
{
    CONTEXT           context;
    EXCEPTION_RECORD  rec;
    void             *redzone[2];
};

struct arm64_thread_data
{
    void     *exit_frame;    /* exit frame pointer */
    CONTEXT  *context;       /* context to set with SIGUSR2 */
};

C_ASSERT( sizeof(struct arm64_thread_data) <= sizeof(((TEB *)0)->SystemReserved2) );
C_ASSERT( offsetof( TEB, SystemReserved2 ) + offsetof( struct arm64_thread_data, exit_frame ) == 0x300 );

static inline struct arm64_thread_data *arm64_thread_data(void)
{
    return (struct arm64_thread_data *)NtCurrentTeb()->SystemReserved2;
}


/***********************************************************************
 *           save_context
 *
 * Set the register values from a sigcontext.
 */
static void save_context( CONTEXT *context, const ucontext_t *sigcontext )
{
    DWORD i;

    context->ContextFlags = CONTEXT_FULL;
    context->u.s.Fp = FP_sig(sigcontext);     /* Frame pointer */
    context->u.s.Lr = LR_sig(sigcontext);     /* Link register */
    context->Sp     = SP_sig(sigcontext);     /* Stack pointer */
    context->Pc     = PC_sig(sigcontext);     /* Program Counter */
    context->Cpsr   = PSTATE_sig(sigcontext); /* Current State Register */
    for (i = 0; i <= 28; i++) context->u.X[i] = REGn_sig( i, sigcontext );
}


/***********************************************************************
 *           restore_context
 *
 * Build a sigcontext from the register values.
 */
static void restore_context( const CONTEXT *context, ucontext_t *sigcontext )
{
    DWORD i;

    FP_sig(sigcontext)     = context->u.s.Fp; /* Frame pointer */
    LR_sig(sigcontext)     = context->u.s.Lr; /* Link register */
    SP_sig(sigcontext)     = context->Sp;     /* Stack pointer */
    PC_sig(sigcontext)     = context->Pc;     /* Program Counter */
    PSTATE_sig(sigcontext) = context->Cpsr;   /* Current State Register */
    for (i = 0; i <= 28; i++) REGn_sig( i, sigcontext ) = context->u.X[i];
}


/***********************************************************************
 *           save_fpu
 *
 * Set the FPU context from a sigcontext.
 */
static void save_fpu( CONTEXT *context, ucontext_t *sigcontext )
{
    struct fpsimd_context *fp = get_fpsimd_context( sigcontext );

    if (!fp) return;
    context->ContextFlags |= CONTEXT_FLOATING_POINT;
    context->Fpcr = fp->fpcr;
    context->Fpsr = fp->fpsr;
    memcpy( context->V, fp->vregs, sizeof(context->V) );
}


/***********************************************************************
 *           restore_fpu
 *
 * Restore the FPU context to a sigcontext.
 */
static void restore_fpu( CONTEXT *context, ucontext_t *sigcontext )
{
    struct fpsimd_context *fp = get_fpsimd_context( sigcontext );

    if (!fp) return;
    fp->fpcr = context->Fpcr;
    fp->fpsr = context->Fpsr;
    memcpy( fp->vregs, context->V, sizeof(fp->vregs) );
}


/***********************************************************************
 *           get_server_context_flags
 *
 * Convert CPU-specific flags to generic server flags
 */
static unsigned int get_server_context_flags( DWORD flags )
{
    unsigned int ret = 0;

    flags &= ~CONTEXT_ARM64;  /* get rid of CPU id */
    if (flags & CONTEXT_CONTROL) ret |= SERVER_CTX_CONTROL;
    if (flags & CONTEXT_INTEGER) ret |= SERVER_CTX_INTEGER;
    if (flags & CONTEXT_FLOATING_POINT) ret |= SERVER_CTX_FLOATING_POINT;
    if (flags & CONTEXT_DEBUG_REGISTERS) ret |= SERVER_CTX_DEBUG_REGISTERS;
    return ret;
}


/***********************************************************************
 *           copy_context
 *
 * Copy a register context according to the flags.
 */
static void copy_context( CONTEXT *to, const CONTEXT *from, DWORD flags )
{
    flags &= ~CONTEXT_ARM64;  /* get rid of CPU id */
    if (flags & CONTEXT_CONTROL)
    {
        to->u.s.Fp  = from->u.s.Fp;
        to->u.s.Lr  = from->u.s.Lr;
        to->Sp      = from->Sp;
        to->Pc      = from->Pc;
        to->Cpsr    = from->Cpsr;
    }
    if (flags & CONTEXT_INTEGER)
    {
        memcpy( to->u.X, from->u.X, sizeof(to->u.X) );
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
        memcpy( to->V, from->V, sizeof(to->V) );
        to->Fpcr = from->Fpcr;
        to->Fpsr = from->Fpsr;
    }
    if (flags & CONTEXT_DEBUG_REGISTERS)
    {
        memcpy( to->Bcr, from->Bcr, sizeof(to->Bcr) );
        memcpy( to->Bvr, from->Bvr, sizeof(to->Bvr) );
        memcpy( to->Wcr, from->Wcr, sizeof(to->Wcr) );
        memcpy( to->Wvr, from->Wvr, sizeof(to->Wvr) );
    }
}


/***********************************************************************
 *           context_to_server
 *
 * Convert a register context to the server format.
 */
NTSTATUS context_to_server( context_t *to, const CONTEXT *from )
{
    DWORD i, flags = from->ContextFlags & ~CONTEXT_ARM64;  /* get rid of CPU id */

    memset( to, 0, sizeof(*to) );
    to->cpu = CPU_ARM64;

    if (flags & CONTEXT_CONTROL)
    {
        to->flags |= SERVER_CTX_CONTROL;
        to->integer.arm64_regs.x[29] = from->u.s.Fp;
        to->integer.arm64_regs.x[30] = from->u.s.Lr;
        to->ctl.arm64_regs.sp     = from->Sp;
        to->ctl.arm64_regs.pc     = from->Pc;
        to->ctl.arm64_regs.pstate = from->Cpsr;
    }
    if (flags & CONTEXT_INTEGER)
    {
        to->flags |= SERVER_CTX_INTEGER;
        for (i = 0; i <= 28; i++) to->integer.arm64_regs.x[i] = from->u.X[i];
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
        to->flags |= SERVER_CTX_FLOATING_POINT;
        for (i = 0; i < 32; i++)
        {
            to->fp.arm64_regs.q[i].low = from->V[i].s.Low;
            to->fp.arm64_regs.q[i].high = from->V[i].s.High;
        }
        to->fp.arm64_regs.fpcr = from->Fpcr;
        to->fp.arm64_regs.fpsr = from->Fpsr;
    }
    if (flags & CONTEXT_DEBUG_REGISTERS)
    {
        to->flags |= SERVER_CTX_DEBUG_REGISTERS;
        for (i = 0; i < ARM64_MAX_BREAKPOINTS; i++) to->debug.arm64_regs.bcr[i] = from->Bcr[i];
        for (i = 0; i < ARM64_MAX_BREAKPOINTS; i++) to->debug.arm64_regs.bvr[i] = from->Bvr[i];
        for (i = 0; i < ARM64_MAX_WATCHPOINTS; i++) to->debug.arm64_regs.wcr[i] = from->Wcr[i];
        for (i = 0; i < ARM64_MAX_WATCHPOINTS; i++) to->debug.arm64_regs.wvr[i] = from->Wvr[i];
    }
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           context_from_server
 *
 * Convert a register context from the server format.
 */
NTSTATUS context_from_server( CONTEXT *to, const context_t *from )
{
    DWORD i;

    if (from->cpu != CPU_ARM64) return STATUS_INVALID_PARAMETER;

    to->ContextFlags = CONTEXT_ARM64;
    if (from->flags & SERVER_CTX_CONTROL)
    {
        to->ContextFlags |= CONTEXT_CONTROL;
        to->u.s.Fp = from->integer.arm64_regs.x[29];
        to->u.s.Lr = from->integer.arm64_regs.x[30];
        to->Sp     = from->ctl.arm64_regs.sp;
        to->Pc     = from->ctl.arm64_regs.pc;
        to->Cpsr   = from->ctl.arm64_regs.pstate;
    }
    if (from->flags & SERVER_CTX_INTEGER)
    {
        to->ContextFlags |= CONTEXT_INTEGER;
        for (i = 0; i <= 28; i++) to->u.X[i] = from->integer.arm64_regs.x[i];
    }
    if (from->flags & SERVER_CTX_FLOATING_POINT)
    {
        to->ContextFlags |= CONTEXT_FLOATING_POINT;
        for (i = 0; i < 32; i++)
        {
            to->V[i].s.Low = from->fp.arm64_regs.q[i].low;
            to->V[i].s.High = from->fp.arm64_regs.q[i].high;
        }
        to->Fpcr = from->fp.arm64_regs.fpcr;
        to->Fpsr = from->fp.arm64_regs.fpsr;
    }
    if (from->flags & SERVER_CTX_DEBUG_REGISTERS)
    {
        to->ContextFlags |= CONTEXT_DEBUG_REGISTERS;
        for (i = 0; i < ARM64_MAX_BREAKPOINTS; i++) to->Bcr[i] = from->debug.arm64_regs.bcr[i];
        for (i = 0; i < ARM64_MAX_BREAKPOINTS; i++) to->Bvr[i] = from->debug.arm64_regs.bvr[i];
        for (i = 0; i < ARM64_MAX_WATCHPOINTS; i++) to->Wcr[i] = from->debug.arm64_regs.wcr[i];
        for (i = 0; i < ARM64_MAX_WATCHPOINTS; i++) to->Wvr[i] = from->debug.arm64_regs.wvr[i];
    }
    return STATUS_SUCCESS;
}


/***********************************************************************
 *              NtSetContextThread  (NTDLL.@)
 *              ZwSetContextThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetContextThread( HANDLE handle, const CONTEXT *context )
{
    NTSTATUS ret = STATUS_SUCCESS;
    BOOL self = (handle == GetCurrentThread());

    if (self && (context->ContextFlags & (CONTEXT_DEBUG_REGISTERS & ~CONTEXT_ARM64))) self = FALSE;

    if (!self)
    {
        context_t server_context;
        context_to_server( &server_context, context );
        ret = set_thread_context( handle, &server_context, &self );
    }
    if (self && ret == STATUS_SUCCESS)
    {
        InterlockedExchangePointer( (void **)&arm64_thread_data()->context, (void *)context );
        raise( SIGUSR2 );
    }
    return ret;
}


/***********************************************************************
 *              NtGetContextThread  (NTDLL.@)
 *              ZwGetContextThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtGetContextThread( HANDLE handle, CONTEXT *context )
{
    NTSTATUS ret;
    DWORD needed_flags = context->ContextFlags;
    BOOL self = (handle == GetCurrentThread());

    if (!self)
    {
        context_t server_context;
        unsigned int server_flags = get_server_context_flags( context->ContextFlags );

        if ((ret = get_thread_context( handle, &server_context, server_flags, &self ))) return ret;
        if ((ret = context_from_server( context, &server_context ))) return ret;
        needed_flags &= ~context->ContextFlags;
    }

    if (self && needed_flags)
    {
        CONTEXT ctx;
        RtlCaptureContext( &ctx );
        copy_context( context, &ctx, ctx.ContextFlags & needed_flags );
        context->ContextFlags |= ctx.ContextFlags & needed_flags;
    }
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           setup_exception
 *
 * Setup the exception record and context on the thread stack.
 */
static struct stack_layout *setup_exception( ucontext_t *sigcontext )
{
    struct stack_layout *stack;
    DWORD exception_code = 0;

    /* push the stack_layout structure */
    stack = (struct stack_layout *)((SP_sig(sigcontext) - sizeof(*stack)) & ~15);

    stack->rec.ExceptionRecord  = NULL;
    stack->rec.ExceptionCode    = exception_code;
    stack->rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    stack->rec.ExceptionAddress = (LPVOID)PC_sig(sigcontext);
    stack->rec.NumberParameters = 0;

    save_context( &stack->context, sigcontext );
    save_fpu( &stack->context, sigcontext );
    return stack;
}


extern void raise_func_trampoline( EXCEPTION_RECORD *rec, CONTEXT *context, raise_func func, void *sp );
__ASM_GLOBAL_FUNC( raise_func_trampoline,
                   __ASM_CFI(".cfi_signal_frame\n\t")
                   "stp x29, x30, [sp, #-0x20]!\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset 32\n\t")
                   __ASM_CFI(".cfi_offset 29, -32\n\t")
                   __ASM_CFI(".cfi_offset 30, -24\n\t")
                   "mov x29, sp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register 29\n\t")
                   "str x3, [sp, 0x10]\n\t"
                   __ASM_CFI(".cfi_remember_state\n\t")
                   __ASM_CFI(".cfi_escape 0x0f,0x03,0x8d,0x10,0x06\n\t") /* CFA */
                   __ASM_CFI(".cfi_escape 0x10,0x1d,0x02,0x8d,0x00\n\t") /* x29 */
                   __ASM_CFI(".cfi_escape 0x10,0x1e,0x02,0x8d,0x08\n\t") /* x30 */
                   "blr x2\n\t"
                   __ASM_CFI(".cfi_restore_state\n\t")
                   "brk #1")

/***********************************************************************
 *           setup_raise_exception
 *
 * Modify the signal context to call the exception raise function.
 */
static void setup_raise_exception( ucontext_t *sigcontext, struct stack_layout *stack )
{
    NTSTATUS status = send_debug_event( &stack->rec, &stack->context, TRUE );

    if (status == DBG_CONTINUE || status == DBG_EXCEPTION_HANDLED)
    {
        restore_context( &stack->context, sigcontext );
        return;
    }
    REGn_sig(3, sigcontext) = SP_sig(sigcontext); /* original stack pointer, fourth arg for raise_func_trampoline */
    SP_sig(sigcontext) = (ULONG_PTR)stack;
    LR_sig(sigcontext) = PC_sig(sigcontext);
    PC_sig(sigcontext) = (ULONG_PTR)raise_func_trampoline; /* raise_generic_exception; */
    REGn_sig(0, sigcontext) = (ULONG_PTR)&stack->rec;  /* first arg for raise_generic_exception */
    REGn_sig(1, sigcontext) = (ULONG_PTR)&stack->context; /* second arg for raise_generic_exception */
    REGn_sig(2, sigcontext) = (ULONG_PTR)pKiUserExceptionDispatcher; /* third arg for raise_func_trampoline */
    REGn_sig(18, sigcontext) = (ULONG_PTR)NtCurrentTeb();
}

/**********************************************************************
 *		segv_handler
 *
 * Handler for SIGSEGV and related errors.
 */
static void segv_handler( int signal, siginfo_t *info, void *ucontext )
{
    struct stack_layout *stack;
    ucontext_t *context = ucontext;

    /* check for page fault inside the thread stack */
    if (signal == SIGSEGV)
    {
        switch (virtual_handle_stack_fault( info->si_addr ))
        {
        case 1:  /* handled */
            return;
        case -1:  /* overflow */
            stack = setup_exception( context );
            stack->rec.ExceptionCode = EXCEPTION_STACK_OVERFLOW;
            goto done;
        }
    }

    stack = setup_exception( context );
    if (stack->rec.ExceptionCode == EXCEPTION_STACK_OVERFLOW) goto done;

    switch(signal)
    {
    case SIGILL:   /* Invalid opcode exception */
        stack->rec.ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
        break;
    case SIGSEGV:  /* Segmentation fault */
        stack->rec.NumberParameters = 2;
        stack->rec.ExceptionInformation[0] = (get_fault_esr( context ) & 0x40) != 0;
        stack->rec.ExceptionInformation[1] = (ULONG_PTR)info->si_addr;
        if (!(stack->rec.ExceptionCode = virtual_handle_fault( (void *)stack->rec.ExceptionInformation[1],
                                                         stack->rec.ExceptionInformation[0], FALSE )))
            return;
        break;
    case SIGBUS:  /* Alignment check exception */
        stack->rec.ExceptionCode = EXCEPTION_DATATYPE_MISALIGNMENT;
        break;
    default:
        ERR("Got unexpected signal %i\n", signal);
        stack->rec.ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
        break;
    }
done:
    setup_raise_exception( context, stack );
}


/**********************************************************************
 *		trap_handler
 *
 * Handler for SIGTRAP.
 */
static void trap_handler( int signal, siginfo_t *info, void *ucontext )
{
    ucontext_t *context = ucontext;
    struct stack_layout *stack = setup_exception( context );

    switch (info->si_code)
    {
    case TRAP_TRACE:
        stack->rec.ExceptionCode = EXCEPTION_SINGLE_STEP;
        break;
    case TRAP_BRKPT:
    default:
        stack->rec.ExceptionCode = EXCEPTION_BREAKPOINT;
        stack->context.Pc += 4;
        break;
    }
    setup_raise_exception( context, stack );
}


/**********************************************************************
 *		fpe_handler
 *
 * Handler for SIGFPE.
 */
static void fpe_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    struct stack_layout *stack = setup_exception( sigcontext );

    switch (siginfo->si_code & 0xffff )
    {
#ifdef FPE_FLTSUB
    case FPE_FLTSUB:
        stack->rec.ExceptionCode = EXCEPTION_ARRAY_BOUNDS_EXCEEDED;
        break;
#endif
#ifdef FPE_INTDIV
    case FPE_INTDIV:
        stack->rec.ExceptionCode = EXCEPTION_INT_DIVIDE_BY_ZERO;
        break;
#endif
#ifdef FPE_INTOVF
    case FPE_INTOVF:
        stack->rec.ExceptionCode = EXCEPTION_INT_OVERFLOW;
        break;
#endif
#ifdef FPE_FLTDIV
    case FPE_FLTDIV:
        stack->rec.ExceptionCode = EXCEPTION_FLT_DIVIDE_BY_ZERO;
        break;
#endif
#ifdef FPE_FLTOVF
    case FPE_FLTOVF:
        stack->rec.ExceptionCode = EXCEPTION_FLT_OVERFLOW;
        break;
#endif
#ifdef FPE_FLTUND
    case FPE_FLTUND:
        stack->rec.ExceptionCode = EXCEPTION_FLT_UNDERFLOW;
        break;
#endif
#ifdef FPE_FLTRES
    case FPE_FLTRES:
        stack->rec.ExceptionCode = EXCEPTION_FLT_INEXACT_RESULT;
        break;
#endif
#ifdef FPE_FLTINV
    case FPE_FLTINV:
#endif
    default:
        stack->rec.ExceptionCode = EXCEPTION_FLT_INVALID_OPERATION;
        break;
    }
    setup_raise_exception( sigcontext, stack );
}


/**********************************************************************
 *		int_handler
 *
 * Handler for SIGINT.
 */
static void int_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    struct stack_layout *stack = setup_exception( sigcontext );

    stack->rec.ExceptionCode = CONTROL_C_EXIT;
    setup_raise_exception( sigcontext, stack );
}


/**********************************************************************
 *		abrt_handler
 *
 * Handler for SIGABRT.
 */
static void abrt_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    struct stack_layout *stack = setup_exception( sigcontext );

    stack->rec.ExceptionCode  = EXCEPTION_WINE_ASSERTION;
    stack->rec.ExceptionFlags = EH_NONCONTINUABLE;
    setup_raise_exception( sigcontext, stack );
}


/**********************************************************************
 *		quit_handler
 *
 * Handler for SIGQUIT.
 */
static void quit_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    abort_thread(0);
}


/**********************************************************************
 *		usr1_handler
 *
 * Handler for SIGUSR1, used to signal a thread that it got suspended.
 */
static void usr1_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    CONTEXT context;

    save_context( &context, sigcontext );
    wait_suspend( &context );
    restore_context( &context, sigcontext );
}


/**********************************************************************
 *		usr2_handler
 *
 * Handler for SIGUSR2, used to set a thread context.
 */
static void usr2_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    CONTEXT *context = InterlockedExchangePointer( (void **)&arm64_thread_data()->context, NULL );
    if (!context) return;
    if ((context->ContextFlags & ~CONTEXT_ARM64) & CONTEXT_FLOATING_POINT)
        restore_fpu( context, sigcontext );
    restore_context( context, sigcontext );
}


/**********************************************************************
 *           get_thread_ldt_entry
 */
NTSTATUS CDECL get_thread_ldt_entry( HANDLE handle, void *data, ULONG len, ULONG *ret_len )
{
    return STATUS_NOT_IMPLEMENTED;
}


/******************************************************************************
 *           NtSetLdtEntries   (NTDLL.@)
 *           ZwSetLdtEntries   (NTDLL.@)
 */
NTSTATUS WINAPI NtSetLdtEntries( ULONG sel1, LDT_ENTRY entry1, ULONG sel2, LDT_ENTRY entry2 )
{
    return STATUS_NOT_IMPLEMENTED;
}


/**********************************************************************
 *             signal_init_threading
 */
void signal_init_threading(void)
{
    pthread_key_create( &teb_key, NULL );
}


/**********************************************************************
 *             signal_alloc_thread
 */
NTSTATUS signal_alloc_thread( TEB *teb )
{
    return STATUS_SUCCESS;
}


/**********************************************************************
 *             signal_free_thread
 */
void signal_free_thread( TEB *teb )
{
}


/**********************************************************************
 *		signal_init_thread
 */
void signal_init_thread( TEB *teb )
{
    stack_t ss;

    ss.ss_sp    = (char *)teb + teb_size;
    ss.ss_size  = signal_stack_size;
    ss.ss_flags = 0;
    if (sigaltstack( &ss, NULL ) == -1) perror( "sigaltstack" );

    /* Win64/ARM applications expect the TEB pointer to be in the x18 platform register. */
    __asm__ __volatile__( "mov x18, %0" : : "r" (teb) );

    pthread_setspecific( teb_key, teb );
}


/**********************************************************************
 *		signal_init_process
 */
void signal_init_process(void)
{
    struct sigaction sig_act;

    sig_act.sa_mask = server_block_set;
    sig_act.sa_flags = SA_SIGINFO | SA_RESTART | SA_ONSTACK;

    sig_act.sa_sigaction = int_handler;
    if (sigaction( SIGINT, &sig_act, NULL ) == -1) goto error;
    sig_act.sa_sigaction = fpe_handler;
    if (sigaction( SIGFPE, &sig_act, NULL ) == -1) goto error;
    sig_act.sa_sigaction = abrt_handler;
    if (sigaction( SIGABRT, &sig_act, NULL ) == -1) goto error;
    sig_act.sa_sigaction = quit_handler;
    if (sigaction( SIGQUIT, &sig_act, NULL ) == -1) goto error;
    sig_act.sa_sigaction = usr1_handler;
    if (sigaction( SIGUSR1, &sig_act, NULL ) == -1) goto error;
    sig_act.sa_sigaction = usr2_handler;
    if (sigaction( SIGUSR2, &sig_act, NULL ) == -1) goto error;
    sig_act.sa_sigaction = trap_handler;
    if (sigaction( SIGTRAP, &sig_act, NULL ) == -1) goto error;
    sig_act.sa_sigaction = segv_handler;
    if (sigaction( SIGSEGV, &sig_act, NULL ) == -1) goto error;
    if (sigaction( SIGILL, &sig_act, NULL ) == -1) goto error;
    if (sigaction( SIGBUS, &sig_act, NULL ) == -1) goto error;
    return;

 error:
    perror("sigaction");
    exit(1);
}

/***********************************************************************
 *           init_thread_context
 */
static void init_thread_context( CONTEXT *context, LPTHREAD_START_ROUTINE entry, void *arg, void *relay )
{
    context->u.s.X0  = (DWORD64)entry;
    context->u.s.X1  = (DWORD64)arg;
    context->u.s.X18 = (DWORD64)NtCurrentTeb();
    context->Sp      = (DWORD64)NtCurrentTeb()->Tib.StackBase;
    context->Pc      = (DWORD64)relay;
}


/***********************************************************************
 *           attach_thread
 */
PCONTEXT DECLSPEC_HIDDEN attach_thread( LPTHREAD_START_ROUTINE entry, void *arg,
                                        BOOL suspend, void *relay )
{
    CONTEXT *ctx;

    if (suspend)
    {
        CONTEXT context = { CONTEXT_ALL };

        init_thread_context( &context, entry, arg, relay );
        wait_suspend( &context );
        ctx = (CONTEXT *)((ULONG_PTR)context.Sp & ~15) - 1;
        *ctx = context;
    }
    else
    {
        ctx = (CONTEXT *)NtCurrentTeb()->Tib.StackBase - 1;
        init_thread_context( ctx, entry, arg, relay );
    }
    pthread_sigmask( SIG_UNBLOCK, &server_block_set, NULL );
    ctx->ContextFlags = CONTEXT_FULL;
    pLdrInitializeThunk( ctx, (void **)&ctx->u.s.X0, 0, 0 );
    return ctx;
}


/***********************************************************************
 *           signal_start_thread
 */
__ASM_GLOBAL_FUNC( signal_start_thread,
                   "stp x29, x30, [sp,#-16]!\n\t"
                   "mov x18, x4\n\t"             /* teb */
                   /* store exit frame */
                   "mov x29, sp\n\t"
                   "str x29, [x4, #0x300]\n\t"  /* arm64_thread_data()->exit_frame */
                   /* switch to thread stack */
                   "ldr x5, [x4, #8]\n\t"       /* teb->Tib.StackBase */
                   "sub sp, x5, #0x1000\n\t"
                   /* attach dlls */
                   "bl " __ASM_NAME("attach_thread") "\n\t"
                   "mov sp, x0\n\t"
                   /* clear the stack */
                   "and x0, x0, #~0xfff\n\t"  /* round down to page size */
                   "bl " __ASM_NAME("virtual_clear_thread_stack") "\n\t"
                   /* switch to the initial context */
                   "mov x0, sp\n\t"
                   "ldp q0, q1, [x0, #0x110]\n\t"      /* context->V[0,1] */
                   "ldp q2, q3, [x0, #0x130]\n\t"      /* context->V[2,3] */
                   "ldp q4, q5, [x0, #0x150]\n\t"      /* context->V[4,5] */
                   "ldp q6, q7, [x0, #0x170]\n\t"      /* context->V[6,7] */
                   "ldp q8, q9, [x0, #0x190]\n\t"      /* context->V[8,9] */
                   "ldp q10, q11, [x0, #0x1b0]\n\t"    /* context->V[10,11] */
                   "ldp q12, q13, [x0, #0x1d0]\n\t"    /* context->V[12,13] */
                   "ldp q14, q15, [x0, #0x1f0]\n\t"    /* context->V[14,15] */
                   "ldp q16, q17, [x0, #0x210]\n\t"    /* context->V[16,17] */
                   "ldp q18, q19, [x0, #0x230]\n\t"    /* context->V[18,19] */
                   "ldp q20, q21, [x0, #0x250]\n\t"    /* context->V[20,21] */
                   "ldp q22, q23, [x0, #0x270]\n\t"    /* context->V[22,23] */
                   "ldp q24, q25, [x0, #0x290]\n\t"    /* context->V[24,25] */
                   "ldp q26, q27, [x0, #0x2b0]\n\t"    /* context->V[26,27] */
                   "ldp q28, q29, [x0, #0x2d0]\n\t"    /* context->V[28,29] */
                   "ldp q30, q31, [x0, #0x2f0]\n\t"    /* context->V[30,31] */
                   "ldr w1, [x0, #0x310]\n\t"          /* context->Fpcr */
                   "msr fpcr, x1\n\t"
                   "ldr w1, [x0, #0x314]\n\t"          /* context->Fpsr */
                   "msr fpsr, x1\n\t"
                   "mov x1, #1\n\t"
                   "b " __ASM_NAME("NtContinue") )


extern void DECLSPEC_NORETURN call_thread_exit_func( int status, void (*func)(int), TEB *teb );
__ASM_GLOBAL_FUNC( call_thread_exit_func,
                   "stp x29, x30, [sp,#-16]!\n\t"
                   "ldr x3, [x2, #0x300]\n\t"  /* arm64_thread_data()->exit_frame */
                   "str xzr, [x2, #0x300]\n\t"
                   "cbz x3, 1f\n\t"
                   "mov sp, x3\n"
                   "1:\tldp x29, x30, [sp], #16\n\t"
                   "br x1" )

/***********************************************************************
 *           signal_exit_thread
 */
void signal_exit_thread( int status, void (*func)(int) )
{
    call_thread_exit_func( status, func, NtCurrentTeb() );
}


/**********************************************************************
 *           NtCurrentTeb   (NTDLL.@)
 */
TEB * WINAPI NtCurrentTeb(void)
{
    return pthread_getspecific( teb_key );
}

#endif  /* __aarch64__ */
