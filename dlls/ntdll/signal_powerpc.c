/*
 * PowerPC signal handling routines
 *
 * Copyright 2002 Marcus Meissner, SuSE Linux AG
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef __powerpc__

#include "config.h"
#include "wine/port.h"

#include <signal.h>
#include <stdlib.h>
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

#ifdef HAVE_SYS_VM86_H
# include <sys/vm86.h>
#endif

#ifdef HAVE_SYS_SIGNAL_H
# include <sys/signal.h>
#endif

#include "windef.h"
#include "winternl.h"
#include "wine/library.h"
#include "wine/exception.h"
#include "selectors.h"
#include "stackframe.h"
#include "global.h"
#include "miscemu.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);


/***********************************************************************
 * signal context platform-specific definitions
 */

typedef struct ucontext SIGCONTEXT;

#define HANDLER_DEF(name) void name( int __signal, struct siginfo *__siginfo, SIGCONTEXT *__context )
#define HANDLER_CONTEXT (__context)

typedef int (*wine_signal_handler)(unsigned int sig);

static wine_signal_handler handlers[256];

extern void WINAPI EXC_RtlRaiseException( PEXCEPTION_RECORD, PCONTEXT );

/***********************************************************************
 *           dispatch_signal
 */
inline static int dispatch_signal(unsigned int sig)
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
#define CX(x,y) context->x = sigcontext->uc_mcontext.regs->y
#define C(x) CX(Gpr##x,gpr[x])
	C(0); C(1); C(2); C(3); C(4); C(5); C(6); C(7); C(8); C(9); C(10);
	C(11); C(12); C(13); C(14); C(15); C(16); C(17); C(18); C(19); C(20);
	C(21); C(22); C(23); C(24); C(25); C(26); C(27); C(28); C(29); C(30);
	C(31);

	CX(Iar,nip);
	CX(Msr,msr);
	CX(Ctr,ctr);
#undef CX
	/* FIXME: fp regs? */

	/* FIXME: missing pt_regs ...
	unsigned long link;
	unsigned long xer;
	unsigned long ccr;
	unsigned long mq;

	unsigned long trap;
	unsigned long dar;
	unsigned long dsisr;

	*/
}


/***********************************************************************
 *           restore_context
 *
 * Build a sigcontext from the register values.
 */
static void restore_context( const CONTEXT *context, SIGCONTEXT *sigcontext )
{
#define CX(x,y) sigcontext->uc_mcontext.regs->y = context->x
	C(0); C(1); C(2); C(3); C(4); C(5); C(6); C(7); C(8); C(9); C(10);
	C(11); C(12); C(13); C(14); C(15); C(16); C(17); C(18); C(19); C(20);
	C(21); C(22); C(23); C(24); C(25); C(26); C(27); C(28); C(29); C(30);
	C(31);

	CX(Iar,nip);
	CX(Msr,msr);
	CX(Ctr,ctr);
#undef CX
}


/***********************************************************************
 *           save_fpu
 *
 * Set the FPU context from a sigcontext.
 */
inline static void save_fpu( CONTEXT *context, const SIGCONTEXT *sigcontext )
{
	/* FIXME? */
}


/***********************************************************************
 *           restore_fpu
 *
 * Restore the FPU context to a sigcontext.
 */
inline static void restore_fpu( CONTEXT *context, const SIGCONTEXT *sigcontext )
{
	/* FIXME? */
}


/**********************************************************************
 *		get_fpu_code
 *
 * Get the FPU exception code from the FPU status.
 */
static inline DWORD get_fpu_code( const CONTEXT *context )
{
    DWORD status  = 0 /* FIXME */;

    if (status & 0x01)  /* IE */
    {
        if (status & 0x40)  /* SF */
            return EXCEPTION_FLT_STACK_CHECK;
        else
            return EXCEPTION_FLT_INVALID_OPERATION;
    }
    if (status & 0x02) return EXCEPTION_FLT_DENORMAL_OPERAND;  /* DE flag */
    if (status & 0x04) return EXCEPTION_FLT_DIVIDE_BY_ZERO;    /* ZE flag */
    if (status & 0x08) return EXCEPTION_FLT_OVERFLOW;          /* OE flag */
    if (status & 0x10) return EXCEPTION_FLT_UNDERFLOW;         /* UE flag */
    if (status & 0x20) return EXCEPTION_FLT_INEXACT_RESULT;    /* PE flag */
    return EXCEPTION_FLT_INVALID_OPERATION;  /* generic error */
}


/**********************************************************************
 *		segv_handler
 *
 * Handler for SIGSEGV and related errors.
 */
static HANDLER_DEF(segv_handler)
{
    CONTEXT context;
    EXCEPTION_RECORD rec;
    DWORD page_fault_code = EXCEPTION_ACCESS_VIOLATION;

    save_context( &context, HANDLER_CONTEXT );

    rec.ExceptionRecord  = NULL;
    rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    rec.ExceptionAddress = (LPVOID)HANDLER_CONTEXT->uc_mcontext.regs->nip;
    rec.NumberParameters = 0;
    switch (__siginfo->si_signo) {
    case SIGSEGV:
    	switch ( __siginfo->si_code & 0xffff ) {
	case SEGV_MAPERR:
	case SEGV_ACCERR:
		rec.NumberParameters = 2;
		rec.ExceptionInformation[0] = 0; /* FIXME ? */
		rec.ExceptionInformation[1] = (DWORD)__siginfo->si_addr;
		if (!(page_fault_code=VIRTUAL_HandleFault(__siginfo->si_addr)))
			return;
		rec.ExceptionCode = page_fault_code;
		break;
	default:FIXME("Unhandled SIGSEGV/%x\n",__siginfo->si_code);
		break;
	}
    	break;
    case SIGBUS:
    	switch ( __siginfo->si_code & 0xffff ) {
	case BUS_ADRALN:
		rec.ExceptionCode = EXCEPTION_DATATYPE_MISALIGNMENT;
		break;
	case BUS_ADRERR:
	case BUS_OBJERR:
		/* FIXME: correct for all cases ? */
		rec.NumberParameters = 2;
		rec.ExceptionInformation[0] = 0; /* FIXME ? */
		rec.ExceptionInformation[1] = (DWORD)__siginfo->si_addr;
		if (!(page_fault_code=VIRTUAL_HandleFault(__siginfo->si_addr)))
			return;
		rec.ExceptionCode = page_fault_code;
		break;
	default:FIXME("Unhandled SIGBUS/%x\n",__siginfo->si_code);
		break;
	}
    	break;
    case SIGILL:
    	switch ( __siginfo->si_code & 0xffff ) {
	case ILL_ILLOPC: /* illegal opcode */
	case ILL_ILLOPN: /* illegal operand */
	case ILL_ILLADR: /* illegal addressing mode */
	case ILL_ILLTRP: /* illegal trap */
	case ILL_COPROC: /* coprocessor error */
		rec.ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
		break;
	case ILL_PRVOPC: /* privileged opcode */
	case ILL_PRVREG: /* privileged register */
		rec.ExceptionCode = EXCEPTION_PRIV_INSTRUCTION;
		break;
	case ILL_BADSTK: /* internal stack error */
        	rec.ExceptionCode = EXCEPTION_STACK_OVERFLOW;
		break;
	default:FIXME("Unhandled SIGILL/%x\n",__siginfo->si_code);
		break;
	}
    	break;
    }
    EXC_RtlRaiseException( &rec, &context );
    restore_context( &context, HANDLER_CONTEXT );
}

/**********************************************************************
 *		trap_handler
 *
 * Handler for SIGTRAP.
 */
static HANDLER_DEF(trap_handler)
{
    CONTEXT context;
    EXCEPTION_RECORD rec;

    save_context( &context, HANDLER_CONTEXT );

    rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    rec.ExceptionRecord  = NULL;
    rec.ExceptionAddress = (LPVOID)__context->uc_mcontext.regs->nip;
    rec.NumberParameters = 0;

    /* FIXME: check if we might need to modify PC */
    switch (__siginfo->si_code & 0xffff) {
    case TRAP_BRKPT:
        rec.ExceptionCode = EXCEPTION_BREAKPOINT;
    	break;
    case TRAP_TRACE:
        rec.ExceptionCode = EXCEPTION_SINGLE_STEP;
    	break;
    }
    EXC_RtlRaiseException( &rec, &context );
    restore_context( &context, HANDLER_CONTEXT );
}


/**********************************************************************
 *		fpe_handler
 *
 * Handler for SIGFPE.
 */
static HANDLER_DEF(fpe_handler)
{
    CONTEXT context;
    EXCEPTION_RECORD rec;

    /*save_fpu( &context, HANDLER_CONTEXT );*/
    save_context( &context, HANDLER_CONTEXT );

    switch ( __siginfo->si_code  & 0xffff ) {
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
    rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    rec.ExceptionRecord  = NULL;
    rec.ExceptionAddress = (LPVOID)__context->uc_mcontext.regs->nip;
    rec.NumberParameters = 0;
    EXC_RtlRaiseException( &rec, &context );
    restore_context( &context, HANDLER_CONTEXT );
    /*restore_fpu( &context, HANDLER_CONTEXT );*/
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
        rec.ExceptionAddress = (LPVOID)context.Iar;
        rec.NumberParameters = 0;
        EXC_RtlRaiseException( &rec, &context );
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
    rec.ExceptionAddress = (LPVOID)context.Iar;
    rec.NumberParameters = 0;
    EXC_RtlRaiseException( &rec, &context ); /* Should never return.. */
    restore_context( &context, HANDLER_CONTEXT );
}


/**********************************************************************
 *		term_handler
 *
 * Handler for SIGTERM.
 */
static HANDLER_DEF(term_handler)
{
    SYSDEPS_AbortThread(0);
}


/**********************************************************************
 *		usr1_handler
 *
 * Handler for SIGUSR1, used to signal a thread that it got suspended.
 */
static HANDLER_DEF(usr1_handler)
{
    LARGE_INTEGER timeout;

    /* wait with 0 timeout, will only return once the thread is no longer suspended */
    timeout.QuadPart = 0;
    NtWaitForMultipleObjects( 0, NULL, FALSE, FALSE, &timeout );
}


/***********************************************************************
 *           set_handler
 *
 * Set a signal handler
 */
static int set_handler( int sig, int have_sigaltstack, void (*func)() )
{
    struct sigaction sig_act;

    sig_act.sa_sigaction = func;
    sigemptyset( &sig_act.sa_mask );
    sigaddset( &sig_act.sa_mask, SIGINT );
    sigaddset( &sig_act.sa_mask, SIGALRM );

    sig_act.sa_flags = SA_RESTART | SA_SIGINFO;

#ifdef SA_ONSTACK
    if (have_sigaltstack) sig_act.sa_flags |= SA_ONSTACK;
#endif
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
    int have_sigaltstack = 0;

#ifdef HAVE_SIGALTSTACK
    struct sigaltstack ss;
    if ((ss.ss_sp = NtCurrentTeb()->signal_stack))
    {
        ss.ss_size  = SIGNAL_STACK_SIZE;
        ss.ss_flags = 0;
        if (!sigaltstack(&ss, NULL)) have_sigaltstack = 1;
    }
#endif  /* HAVE_SIGALTSTACK */

    if (set_handler( SIGINT,  have_sigaltstack, (void (*)())int_handler ) == -1) goto error;
    if (set_handler( SIGFPE,  have_sigaltstack, (void (*)())fpe_handler ) == -1) goto error;
    if (set_handler( SIGSEGV, have_sigaltstack, (void (*)())segv_handler ) == -1) goto error;
    if (set_handler( SIGILL,  have_sigaltstack, (void (*)())segv_handler ) == -1) goto error;
    if (set_handler( SIGABRT, have_sigaltstack, (void (*)())abrt_handler ) == -1) goto error;
    if (set_handler( SIGTERM, have_sigaltstack, (void (*)())term_handler ) == -1) goto error;
    if (set_handler( SIGUSR1, have_sigaltstack, (void (*)())usr1_handler ) == -1) goto error;
#ifdef SIGBUS
    if (set_handler( SIGBUS,  have_sigaltstack, (void (*)())segv_handler ) == -1) goto error;
#endif
#ifdef SIGTRAP
    if (set_handler( SIGTRAP, have_sigaltstack, (void (*)())trap_handler ) == -1) goto error;
#endif

    return TRUE;

 error:
    perror("sigaction");
    return FALSE;
}


/**********************************************************************
 *		SIGNAL_Block
 *
 * Block the async signals.
 */
void SIGNAL_Block(void)
{
    sigset_t block_set;

    sigemptyset( &block_set );
    sigaddset( &block_set, SIGALRM );
    sigaddset( &block_set, SIGIO );
    sigaddset( &block_set, SIGHUP );
    sigaddset( &block_set, SIGUSR1 );
    sigaddset( &block_set, SIGUSR2 );
    sigprocmask( SIG_BLOCK, &block_set, NULL );
}


/***********************************************************************
 *           SIGNAL_Unblock
 *
 * Unblock signals. Called from EXC_RtlRaiseException.
 */
void SIGNAL_Unblock(void)
{
    sigset_t all_sigs;

    sigfillset( &all_sigs );
    sigprocmask( SIG_UNBLOCK, &all_sigs, NULL );
}


/**********************************************************************
 *		SIGNAL_Reset
 *
 * Restore the default handlers.
 */
void SIGNAL_Reset(void)
{
    signal( SIGINT, SIG_DFL );
    signal( SIGFPE, SIG_DFL );
    signal( SIGSEGV, SIG_DFL );
    signal( SIGILL, SIG_DFL );
    signal( SIGABRT, SIG_DFL );
    signal( SIGTERM, SIG_DFL );
#ifdef SIGBUS
    signal( SIGBUS, SIG_DFL );
#endif
#ifdef SIGTRAP
    signal( SIGTRAP, SIG_DFL );
#endif
}


/**********************************************************************
 *              __wine_enter_vm86   (NTDLL.@)
 */
void __wine_enter_vm86( CONTEXT *context )
{
    MESSAGE("vm86 mode not supported on this platform\n");
}

/**********************************************************************
 *              DbgBreakPoint   (NTDLL.@)
 */
void WINAPI DbgBreakPoint(void)
{
     kill(getpid(), SIGTRAP);
}

/**********************************************************************
 *              DbgUserBreakPoint   (NTDLL.@)
 */
void WINAPI DbgUserBreakPoint(void)
{
     kill(getpid(), SIGTRAP);
}

#endif  /* __powerpc__ */
