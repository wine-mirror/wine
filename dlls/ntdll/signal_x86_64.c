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

#ifdef __x86_64__

#include "config.h"
#include "wine/port.h"

#include <assert.h>
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
#ifdef HAVE_SYS_SIGNAL_H
# include <sys/signal.h>
#endif

#define NONAMELESSUNION
#include "windef.h"
#include "winternl.h"
#include "wine/library.h"
#include "wine/exception.h"
#include "ntdll_misc.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);


/***********************************************************************
 * signal context platform-specific definitions
 */
#ifdef linux

typedef struct ucontext SIGCONTEXT;

# define HANDLER_DEF(name) void name( int __signal, struct siginfo *__siginfo, SIGCONTEXT *__context )
# define HANDLER_CONTEXT (__context)

#define RAX_sig(context)     ((context)->uc_mcontext.gregs[REG_RAX])
#define RBX_sig(context)     ((context)->uc_mcontext.gregs[REG_RBX])
#define RCX_sig(context)     ((context)->uc_mcontext.gregs[REG_RCX])
#define RDX_sig(context)     ((context)->uc_mcontext.gregs[REG_RDX])
#define RSI_sig(context)     ((context)->uc_mcontext.gregs[REG_RSI])
#define RDI_sig(context)     ((context)->uc_mcontext.gregs[REG_RDI])
#define RBP_sig(context)     ((context)->uc_mcontext.gregs[REG_RBP])
#define R8_sig(context)      ((context)->uc_mcontext.gregs[REG_R8])
#define R9_sig(context)      ((context)->uc_mcontext.gregs[REG_R9])
#define R10_sig(context)     ((context)->uc_mcontext.gregs[REG_R10])
#define R11_sig(context)     ((context)->uc_mcontext.gregs[REG_R11])
#define R12_sig(context)     ((context)->uc_mcontext.gregs[REG_R12])
#define R13_sig(context)     ((context)->uc_mcontext.gregs[REG_R13])
#define R14_sig(context)     ((context)->uc_mcontext.gregs[REG_R14])
#define R15_sig(context)     ((context)->uc_mcontext.gregs[REG_R15])

#define CS_sig(context)      (*((WORD *)&(context)->uc_mcontext.gregs[REG_CSGSFS] + 0))
#define GS_sig(context)      (*((WORD *)&(context)->uc_mcontext.gregs[REG_CSGSFS] + 1))
#define FS_sig(context)      (*((WORD *)&(context)->uc_mcontext.gregs[REG_CSGSFS] + 2))

#define RSP_sig(context)     ((context)->uc_mcontext.gregs[REG_RSP])
#define RIP_sig(context)     ((context)->uc_mcontext.gregs[REG_RIP])
#define EFL_sig(context)     ((context)->uc_mcontext.gregs[REG_EFL])
#define TRAP_sig(context)    ((context)->uc_mcontext.gregs[REG_TRAPNO])
#define ERROR_sig(context)   ((context)->uc_mcontext.gregs[REG_ERR])

#define FPU_sig(context)     ((XMM_SAVE_AREA32 *)((context)->uc_mcontext.fpregs))

#define FAULT_CODE     (__siginfo->si_code)
#define FAULT_ADDRESS  (__siginfo->si_addr)

#endif /* linux */

enum i386_trap_code
{
    TRAP_x86_UNKNOWN    = -1,  /* Unknown fault (TRAP_sig not defined) */
    TRAP_x86_DIVIDE     = 0,   /* Division by zero exception */
    TRAP_x86_TRCTRAP    = 1,   /* Single-step exception */
    TRAP_x86_NMI        = 2,   /* NMI interrupt */
    TRAP_x86_BPTFLT     = 3,   /* Breakpoint exception */
    TRAP_x86_OFLOW      = 4,   /* Overflow exception */
    TRAP_x86_BOUND      = 5,   /* Bound range exception */
    TRAP_x86_PRIVINFLT  = 6,   /* Invalid opcode exception */
    TRAP_x86_DNA        = 7,   /* Device not available exception */
    TRAP_x86_DOUBLEFLT  = 8,   /* Double fault exception */
    TRAP_x86_FPOPFLT    = 9,   /* Coprocessor segment overrun */
    TRAP_x86_TSSFLT     = 10,  /* Invalid TSS exception */
    TRAP_x86_SEGNPFLT   = 11,  /* Segment not present exception */
    TRAP_x86_STKFLT     = 12,  /* Stack fault */
    TRAP_x86_PROTFLT    = 13,  /* General protection fault */
    TRAP_x86_PAGEFLT    = 14,  /* Page fault */
    TRAP_x86_ARITHTRAP  = 16,  /* Floating point exception */
    TRAP_x86_ALIGNFLT   = 17,  /* Alignment check exception */
    TRAP_x86_MCHK       = 18,  /* Machine check exception */
    TRAP_x86_CACHEFLT   = 19   /* Cache flush exception */
};

typedef int (*wine_signal_handler)(unsigned int sig);

static wine_signal_handler handlers[256];

/***********************************************************************
 *           dispatch_signal
 */
static inline int dispatch_signal(unsigned int sig)
{
    if (handlers[sig] == NULL) return 0;
    return handlers[sig](sig);
}

/***********************************************************************
 *           save_context
 *
 * Set the register values from a sigcontext.
 */
static void save_context( CONTEXT *context, const SIGCONTEXT *sigcontext )
{
    context->Rax    = RAX_sig(sigcontext);
    context->Rcx    = RCX_sig(sigcontext);
    context->Rdx    = RDX_sig(sigcontext);
    context->Rbx    = RBX_sig(sigcontext);
    context->Rsp    = RSP_sig(sigcontext);
    context->Rbp    = RBP_sig(sigcontext);
    context->Rsi    = RSI_sig(sigcontext);
    context->Rdi    = RDI_sig(sigcontext);
    context->R8     = R8_sig(sigcontext);
    context->R9     = R9_sig(sigcontext);
    context->R10    = R10_sig(sigcontext);
    context->R11    = R11_sig(sigcontext);
    context->R12    = R12_sig(sigcontext);
    context->R13    = R13_sig(sigcontext);
    context->R14    = R14_sig(sigcontext);
    context->R15    = R15_sig(sigcontext);
    context->Rip    = RIP_sig(sigcontext);
    context->SegCs  = CS_sig(sigcontext);
    context->SegFs  = FS_sig(sigcontext);
    context->SegGs  = GS_sig(sigcontext);
    context->EFlags = EFL_sig(sigcontext);
    context->SegDs  = 0;  /* FIXME */
    context->SegEs  = 0;  /* FIXME */
    context->SegSs  = 0;  /* FIXME */
    context->MxCsr  = 0;  /* FIXME */
    if (FPU_sig(sigcontext)) context->u.FltSave = *FPU_sig(sigcontext);
}


/***********************************************************************
 *           restore_context
 *
 * Build a sigcontext from the register values.
 */
static void restore_context( const CONTEXT *context, SIGCONTEXT *sigcontext )
{
    RAX_sig(sigcontext) = context->Rax;
    RCX_sig(sigcontext) = context->Rcx;
    RDX_sig(sigcontext) = context->Rdx;
    RBX_sig(sigcontext) = context->Rbx;
    RSP_sig(sigcontext) = context->Rsp;
    RBP_sig(sigcontext) = context->Rbp;
    RSI_sig(sigcontext) = context->Rsi;
    RDI_sig(sigcontext) = context->Rdi;
    R8_sig(sigcontext)  = context->R8;
    R9_sig(sigcontext)  = context->R9;
    R10_sig(sigcontext) = context->R10;
    R11_sig(sigcontext) = context->R11;
    R12_sig(sigcontext) = context->R12;
    R13_sig(sigcontext) = context->R13;
    R14_sig(sigcontext) = context->R14;
    R15_sig(sigcontext) = context->R15;
    RIP_sig(sigcontext) = context->Rip;
    CS_sig(sigcontext)  = context->SegCs;
    FS_sig(sigcontext)  = context->SegFs;
    GS_sig(sigcontext)  = context->SegGs;
    EFL_sig(sigcontext) = context->EFlags;
    if (FPU_sig(sigcontext)) *FPU_sig(sigcontext) = context->u.FltSave;
}


/***********************************************************************
 *              get_cpu_context
 *
 * Get the context of the current thread.
 */
void get_cpu_context( CONTEXT *context )
{
    FIXME("not implemented\n");
}


/***********************************************************************
 *           set_cpu_context
 *
 * Set the new CPU context.
 */
void set_cpu_context( const CONTEXT *context )
{
    FIXME("not implemented\n");
}


/**********************************************************************
 *		segv_handler
 *
 * Handler for SIGSEGV and related errors.
 */
static HANDLER_DEF(segv_handler)
{
    EXCEPTION_RECORD rec;
    CONTEXT context;

    save_context( &context, HANDLER_CONTEXT );

    rec.ExceptionRecord  = NULL;
    rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    rec.ExceptionAddress = (LPVOID)context.Rip;
    rec.NumberParameters = 0;

    switch(TRAP_sig(HANDLER_CONTEXT))
    {
    case TRAP_x86_OFLOW:   /* Overflow exception */
        rec.ExceptionCode = EXCEPTION_INT_OVERFLOW;
        break;
    case TRAP_x86_BOUND:   /* Bound range exception */
        rec.ExceptionCode = EXCEPTION_ARRAY_BOUNDS_EXCEEDED;
        break;
    case TRAP_x86_PRIVINFLT:   /* Invalid opcode exception */
        rec.ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
        break;
    case TRAP_x86_STKFLT:  /* Stack fault */
        rec.ExceptionCode = EXCEPTION_STACK_OVERFLOW;
        break;
    case TRAP_x86_SEGNPFLT:  /* Segment not present exception */
    case TRAP_x86_PROTFLT:   /* General protection fault */
    case TRAP_x86_UNKNOWN:   /* Unknown fault code */
        rec.ExceptionCode = ERROR_sig(HANDLER_CONTEXT) ? EXCEPTION_ACCESS_VIOLATION
                                                       : EXCEPTION_PRIV_INSTRUCTION;
        break;
    case TRAP_x86_PAGEFLT:  /* Page fault */
        rec.ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
#ifdef FAULT_ADDRESS
        rec.NumberParameters = 2;
        rec.ExceptionInformation[0] = (ERROR_sig(HANDLER_CONTEXT) & 2) != 0;
        rec.ExceptionInformation[1] = (ULONG_PTR)FAULT_ADDRESS;
#endif
        break;
    case TRAP_x86_ALIGNFLT:  /* Alignment check exception */
        rec.ExceptionCode = EXCEPTION_DATATYPE_MISALIGNMENT;
        break;
    default:
        ERR( "Got unexpected trap %ld\n", TRAP_sig(HANDLER_CONTEXT) );
        /* fall through */
    case TRAP_x86_NMI:       /* NMI interrupt */
    case TRAP_x86_DNA:       /* Device not available exception */
    case TRAP_x86_DOUBLEFLT: /* Double fault exception */
    case TRAP_x86_TSSFLT:    /* Invalid TSS exception */
    case TRAP_x86_MCHK:      /* Machine check exception */
    case TRAP_x86_CACHEFLT:  /* Cache flush exception */
        rec.ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
        break;
    }

    __regs_RtlRaiseException( &rec, &context );
    restore_context( &context, HANDLER_CONTEXT );
}

/**********************************************************************
 *		trap_handler
 *
 * Handler for SIGTRAP.
 */
static HANDLER_DEF(trap_handler)
{
    EXCEPTION_RECORD rec;
    CONTEXT context;

    save_context( &context, HANDLER_CONTEXT );
    rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    rec.ExceptionRecord  = NULL;
    rec.ExceptionAddress = (LPVOID)context.Rip;
    rec.NumberParameters = 0;

    switch(FAULT_CODE)
    {
    case TRAP_TRACE:  /* Single-step exception */
        rec.ExceptionCode = EXCEPTION_SINGLE_STEP;
        EFL_sig(HANDLER_CONTEXT) &= ~0x100;  /* clear single-step flag */
        break;
    case TRAP_BRKPT:   /* Breakpoint exception */
        rec.ExceptionAddress = (char *)rec.ExceptionAddress - 1;  /* back up over the int3 instruction */
        /* fall through */
    default:
        rec.ExceptionCode = EXCEPTION_BREAKPOINT;
        break;
    }

    __regs_RtlRaiseException( &rec, &context );
    restore_context( &context, HANDLER_CONTEXT );
}

/**********************************************************************
 *		fpe_handler
 *
 * Handler for SIGFPE.
 */
static HANDLER_DEF(fpe_handler)
{
    EXCEPTION_RECORD rec;
    CONTEXT context;

    save_context( &context, HANDLER_CONTEXT );
    rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    rec.ExceptionRecord  = NULL;
    rec.ExceptionAddress = (LPVOID)context.Rip;
    rec.NumberParameters = 0;

    switch (FAULT_CODE)
    {
    case FPE_FLTSUB:
        rec.ExceptionCode = EXCEPTION_ARRAY_BOUNDS_EXCEEDED;
        break;
    case FPE_INTDIV:
        rec.ExceptionCode = EXCEPTION_INT_DIVIDE_BY_ZERO;
        break;
    case FPE_INTOVF:
        rec.ExceptionCode = EXCEPTION_INT_OVERFLOW;
        break;
    case FPE_FLTDIV:
        rec.ExceptionCode = EXCEPTION_FLT_DIVIDE_BY_ZERO;
        break;
    case FPE_FLTOVF:
        rec.ExceptionCode = EXCEPTION_FLT_OVERFLOW;
        break;
    case FPE_FLTUND:
        rec.ExceptionCode = EXCEPTION_FLT_UNDERFLOW;
        break;
    case FPE_FLTRES:
        rec.ExceptionCode = EXCEPTION_FLT_INEXACT_RESULT;
        break;
    case FPE_FLTINV:
    default:
        rec.ExceptionCode = EXCEPTION_FLT_INVALID_OPERATION;
        break;
    }

    __regs_RtlRaiseException( &rec, &context );
    restore_context( &context, HANDLER_CONTEXT );
}

/**********************************************************************
 *		int_handler
 *
 * Handler for SIGINT.
 */
static HANDLER_DEF(int_handler)
{
    if (!dispatch_signal(SIGINT))
    {
        EXCEPTION_RECORD rec;
        CONTEXT context;

        save_context( &context, HANDLER_CONTEXT );
        rec.ExceptionCode    = CONTROL_C_EXIT;
        rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
        rec.ExceptionRecord  = NULL;
        rec.ExceptionAddress = (LPVOID)context.Rip;
        rec.NumberParameters = 0;
        __regs_RtlRaiseException( &rec, &context );
        restore_context( &context, HANDLER_CONTEXT );
    }
}


/**********************************************************************
 *		abrt_handler
 *
 * Handler for SIGABRT.
 */
static HANDLER_DEF(abrt_handler)
{
    EXCEPTION_RECORD rec;
    CONTEXT context;

    save_context( &context, HANDLER_CONTEXT );
    rec.ExceptionCode    = EXCEPTION_WINE_ASSERTION;
    rec.ExceptionFlags   = EH_NONCONTINUABLE;
    rec.ExceptionRecord  = NULL;
    rec.ExceptionAddress = (LPVOID)context.Rip;
    rec.NumberParameters = 0;
    __regs_RtlRaiseException( &rec, &context ); /* Should never return.. */
    restore_context( &context, HANDLER_CONTEXT );
}


/**********************************************************************
 *		quit_handler
 *
 * Handler for SIGQUIT.
 */
static HANDLER_DEF(quit_handler)
{
    server_abort_thread(0);
}


/**********************************************************************
 *		usr1_handler
 *
 * Handler for SIGUSR1, used to signal a thread that it got suspended.
 */
static HANDLER_DEF(usr1_handler)
{
    CONTEXT context;

    save_context( &context, HANDLER_CONTEXT );
    wait_suspend( &context );
    restore_context( &context, HANDLER_CONTEXT );
}


/**********************************************************************
 *		get_signal_stack_total_size
 *
 * Retrieve the size to allocate for the signal stack, including the TEB at the bottom.
 * Must be a power of two.
 */
size_t get_signal_stack_total_size(void)
{
    assert( sizeof(TEB) <= 2*getpagesize() );
    return 2*getpagesize();  /* this is just for the TEB, we don't need a signal stack */
}


/***********************************************************************
 *           set_handler
 *
 * Set a signal handler
 */
static int set_handler( int sig, void (*func)() )
{
    struct sigaction sig_act;

    sig_act.sa_sigaction = func;
    sig_act.sa_mask = server_block_set;
    sig_act.sa_flags = SA_RESTART | SA_SIGINFO | SA_ONSTACK;
    return sigaction( sig, &sig_act, NULL );
}


/***********************************************************************
 *           __wine_set_signal_handler   (NTDLL.@)
 */
int __wine_set_signal_handler(unsigned int sig, wine_signal_handler wsh)
{
    if (sig > sizeof(handlers) / sizeof(handlers[0])) return -1;
    if (handlers[sig] != NULL) return -2;
    handlers[sig] = wsh;
    return 0;
}


/**********************************************************************
 *		SIGNAL_Init
 */
BOOL SIGNAL_Init(void)
{
    if (set_handler( SIGINT,  (void (*)())int_handler ) == -1) goto error;
    if (set_handler( SIGFPE,  (void (*)())fpe_handler ) == -1) goto error;
    if (set_handler( SIGSEGV, (void (*)())segv_handler ) == -1) goto error;
    if (set_handler( SIGILL,  (void (*)())segv_handler ) == -1) goto error;
    if (set_handler( SIGABRT, (void (*)())abrt_handler ) == -1) goto error;
    if (set_handler( SIGQUIT, (void (*)())quit_handler ) == -1) goto error;
    if (set_handler( SIGUSR1, (void (*)())usr1_handler ) == -1) goto error;
#ifdef SIGBUS
    if (set_handler( SIGBUS,  (void (*)())segv_handler ) == -1) goto error;
#endif
#ifdef SIGTRAP
    if (set_handler( SIGTRAP, (void (*)())trap_handler ) == -1) goto error;
#endif
    return TRUE;

 error:
    perror("sigaction");
    return FALSE;
}


/**********************************************************************
 *              __wine_enter_vm86   (NTDLL.@)
 */
void __wine_enter_vm86( CONTEXT *context )
{
    MESSAGE("vm86 mode not supported on this platform\n");
}

/**********************************************************************
 *		DbgBreakPoint   (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( DbgBreakPoint, "int $3; ret")

/**********************************************************************
 *		DbgUserBreakPoint   (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( DbgUserBreakPoint, "int $3; ret")

#endif  /* __x86_64__ */
