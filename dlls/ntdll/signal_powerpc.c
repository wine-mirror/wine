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
#ifdef linux
# warning Please define registers for your platform linux ppc some are missing

typedef struct ucontext SIGCONTEXT;

# define HANDLER_DEF(name) void name( int __signal, struct siginfo *__siginfo, SIGCONTEXT *__context )
# define HANDLER_CONTEXT (__context)

/* All Registers access - only for local access */
# define REG_sig(reg_name, context)		((context)->uc_mcontext.regs->reg_name)


/* Gpr Registers access  */
# define GPR_sig(reg_num, context)		REG_sig(r##reg_num, context)

# define IAR_sig(context)			REG_sig(nip, context)	/* Program counter */
# define MSR_sig(context)			REG_sig(msr, context)   /* Machine State Register (Supervisor) */
# define CTR_sig(context)			REG_sig(ctr, context)   /* Count register */

# define XER_sig(context)			XER_sig not defined on your platform /* Link register */
# define LR_sig(context)			LR_sig not defined on your platform  /* User's integer exception register */
# define CR_sig(context)			CR_sig not defined on your platform  /* Condition register */

/* Float Registers access  */
# define FLOAT_sig(reg_num, context)		FLOAT_sig not defined on your platform /* Float registers */

# define FPSCR_sig(reg_num, context)		FPSCR_sig not defined on your platform /* Float registers */

/* Exception Registers access */
# define DAR_sig(context)			DAR_sig not defined on your platform
# define DSISR_sig(context)			DSISR_sig not defined on your platform
# define TRAP_sig(context)			TRAP_sig not defined on your platform

#endif /* linux */

#ifdef __darwin__

# include <sys/ucontext.h>

# include <sys/types.h>
# include <signal.h>
typedef siginfo_t siginfo;

typedef struct ucontext SIGCONTEXT;


# define HANDLER_DEF(name) void name( int __signal, siginfo *__siginfo, SIGCONTEXT *__context )
# define HANDLER_CONTEXT (__context)

/* All Registers access - only for local access */
# define REG_sig(reg_name, context)		((context)->uc_mcontext->ss.reg_name)
# define FLOATREG_sig(reg_name, context)	((context)->uc_mcontext->fs.reg_name)
# define EXCEPREG_sig(reg_name, context)	((context)->uc_mcontext->es.reg_name)
# define VECREG_sig(reg_name, context)		((context)->uc_mcontext->vs.reg_name)

/* Gpr Registers access */
# define GPR_sig(reg_num, context)		REG_sig(r##reg_num, context)

# define IAR_sig(context)			REG_sig(srr0, context)	/* Program counter */
# define MSR_sig(context)			REG_sig(srr1, context)  /* Machine State Register (Supervisor) */
# define CTR_sig(context)			REG_sig(ctr, context)

# define XER_sig(context)			REG_sig(xer, context) /* Link register */
# define LR_sig(context)			REG_sig(lr, context)  /* User's integer exception register */
# define CR_sig(context)			REG_sig(cr, context)  /* Condition register */

/* Float Registers access */
# define FLOAT_sig(reg_num, context)		FLOATREG_sig(fpregs[reg_num], context)

# define FPSCR_sig(context)			((double)FLOATREG_sig(fpscr, context))

/* Exception Registers access */
# define DAR_sig(context)			EXCEPREG_sig(dar, context)     /* Fault registers for coredump */
# define DSISR_sig(context)			EXCEPREG_sig(dsisr, context)
# define TRAP_sig(context)			EXCEPREG_sig(exception, context) /* number of powerpc exception taken */

/* Signal defs : Those are undefined on darwin
SIGBUS
#undef BUS_ADRERR
#undef BUS_OBJERR
SIGILL
#undef ILL_ILLOPN
#undef ILL_ILLTRP
#undef ILL_ILLADR
#undef ILL_COPROC
#undef ILL_PRVREG
#undef ILL_BADSTK
SIGTRAP
#undef TRAP_BRKPT
#undef TRAP_TRACE
SIGFPE
*/

#endif /* __darwin__ */



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

#define C(x) context->Gpr##x = GPR_sig(x,sigcontext)
        /* Save Gpr registers */
	C(0); C(1); C(2); C(3); C(4); C(5); C(6); C(7); C(8); C(9); C(10);
	C(11); C(12); C(13); C(14); C(15); C(16); C(17); C(18); C(19); C(20);
	C(21); C(22); C(23); C(24); C(25); C(26); C(27); C(28); C(29); C(30);
	C(31);
#undef C

	context->Iar = IAR_sig(sigcontext);  /* Program Counter */
	context->Msr = MSR_sig(sigcontext);  /* Machine State Register (Supervisor) */
	context->Ctr = CTR_sig(sigcontext);
        
        context->Xer = XER_sig(sigcontext);
	context->Lr  = LR_sig(sigcontext);
	context->Cr  = CR_sig(sigcontext);
        
        /* Saving Exception regs */
        context->Dar   = DAR_sig(sigcontext);
        context->Dsisr = DSISR_sig(sigcontext);
        context->Trap  = TRAP_sig(sigcontext);
}


/***********************************************************************
 *           restore_context
 *
 * Build a sigcontext from the register values.
 */
static void restore_context( const CONTEXT *context, SIGCONTEXT *sigcontext )
{

#define C(x)  GPR_sig(x,sigcontext) = context->Gpr##x
	C(0); C(1); C(2); C(3); C(4); C(5); C(6); C(7); C(8); C(9); C(10);
	C(11); C(12); C(13); C(14); C(15); C(16); C(17); C(18); C(19); C(20);
	C(21); C(22); C(23); C(24); C(25); C(26); C(27); C(28); C(29); C(30);
	C(31);
#undef C

        IAR_sig(sigcontext) = context->Iar;  /* Program Counter */
        MSR_sig(sigcontext) = context->Msr;  /* Machine State Register (Supervisor) */
        CTR_sig(sigcontext) = context->Ctr;
        
        XER_sig(sigcontext) = context->Xer;
        LR_sig(sigcontext) = context->Lr;
	CR_sig(sigcontext) = context->Cr;
        
        /* Setting Exception regs */
        DAR_sig(sigcontext) = context->Dar;
        DSISR_sig(sigcontext) = context->Dsisr;
        TRAP_sig(sigcontext) = context->Trap;
}


/***********************************************************************
 *           save_fpu
 *
 * Set the FPU context from a sigcontext.
 */
inline static void save_fpu( CONTEXT *context, const SIGCONTEXT *sigcontext )
{
#define C(x)   context->Fpr##x = FLOAT_sig(x,sigcontext)
        C(0); C(1); C(2); C(3); C(4); C(5); C(6); C(7); C(8); C(9); C(10);
	C(11); C(12); C(13); C(14); C(15); C(16); C(17); C(18); C(19); C(20);
	C(21); C(22); C(23); C(24); C(25); C(26); C(27); C(28); C(29); C(30);
	C(31);
#undef C
        context->Fpscr = FPSCR_sig(sigcontext);
}


/***********************************************************************
 *           restore_fpu
 *
 * Restore the FPU context to a sigcontext.
 */
inline static void restore_fpu( CONTEXT *context, const SIGCONTEXT *sigcontext )
{
#define C(x)  FLOAT_sig(x,sigcontext) = context->Fpr##x
        C(0); C(1); C(2); C(3); C(4); C(5); C(6); C(7); C(8); C(9); C(10);
	C(11); C(12); C(13); C(14); C(15); C(16); C(17); C(18); C(19); C(20);
	C(21); C(22); C(23); C(24); C(25); C(26); C(27); C(28); C(29); C(30);
	C(31);
#undef C
        FPSCR_sig(sigcontext) = context->Fpscr;
}


/**********************************************************************
 *		get_fpu_code
 *
 * Get the FPU exception code from the FPU status.
 */
static inline DWORD get_fpu_code( const CONTEXT *context )
{
    DWORD status  = context->Fpscr;

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
 *		do_segv
 *
 * Implementation of SIGSEGV handler.
 */
static void do_segv( CONTEXT *context, int trap, int err, int code, void * addr )
{
    EXCEPTION_RECORD rec;
    DWORD page_fault_code = EXCEPTION_ACCESS_VIOLATION;

    rec.ExceptionRecord  = NULL;
    rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    rec.ExceptionAddress = addr;
    rec.NumberParameters = 0;
    
    switch (trap) {
    case SIGSEGV:
    	switch ( code & 0xffff ) {
	case SEGV_MAPERR:
	case SEGV_ACCERR:
		rec.NumberParameters = 2;
		rec.ExceptionInformation[0] = 0; /* FIXME ? */
		rec.ExceptionInformation[1] = (DWORD)addr;
		if (!(page_fault_code=VIRTUAL_HandleFault(addr)))
			return;
		rec.ExceptionCode = page_fault_code;
		break;
	default:FIXME("Unhandled SIGSEGV/%x\n",code);
		break;
	}
    	break;
    case SIGBUS:
    	switch ( code & 0xffff ) {
	case BUS_ADRALN:
		rec.ExceptionCode = EXCEPTION_DATATYPE_MISALIGNMENT;
		break;
#ifdef BUS_ADRERR
	case BUS_ADRERR:
#endif
#ifdef BUS_OBJERR
	case BUS_OBJERR:
		/* FIXME: correct for all cases ? */
		rec.NumberParameters = 2;
		rec.ExceptionInformation[0] = 0; /* FIXME ? */
		rec.ExceptionInformation[1] = (DWORD)addr;
		if (!(page_fault_code=VIRTUAL_HandleFault(addr)))
			return;
		rec.ExceptionCode = page_fault_code;
		break;
#endif
	default:FIXME("Unhandled SIGBUS/%x\n",code);
		break;
	}
    	break;
    case SIGILL:
    	switch ( code & 0xffff ) {
	case ILL_ILLOPC: /* illegal opcode */
#ifdef ILL_ILLOPN
	case ILL_ILLOPN: /* illegal operand */
#endif
#ifdef ILL_ILLADR
	case ILL_ILLADR: /* illegal addressing mode */
#endif
#ifdef ILL_ILLTRP
	case ILL_ILLTRP: /* illegal trap */
#endif
#ifdef ILL_COPROC
	case ILL_COPROC: /* coprocessor error */
#endif
		rec.ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
		break;
	case ILL_PRVOPC: /* privileged opcode */
#ifdef ILL_PRVREG
	case ILL_PRVREG: /* privileged register */
#endif
		rec.ExceptionCode = EXCEPTION_PRIV_INSTRUCTION;
		break;
#ifdef ILL_BADSTK
	case ILL_BADSTK: /* internal stack error */
        	rec.ExceptionCode = EXCEPTION_STACK_OVERFLOW;
		break;
#endif
	default:FIXME("Unhandled SIGILL/%x\n", code);
		break;
	}
    	break;
    }
    EXC_RtlRaiseException( &rec, context );
}

/**********************************************************************
 *		do_trap
 *
 * Implementation of SIGTRAP handler.
 */
static void do_trap( CONTEXT *context, int code, void * addr )
{
    EXCEPTION_RECORD rec;

    rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    rec.ExceptionRecord  = NULL;
    rec.ExceptionAddress = addr;
    rec.NumberParameters = 0;

    /* FIXME: check if we might need to modify PC */
    switch (code & 0xffff) {
#ifdef TRAP_BRKPT
    case TRAP_BRKPT:
        rec.ExceptionCode = EXCEPTION_BREAKPOINT;
    	break;
#endif
#ifdef TRAP_TRACE
    case TRAP_TRACE:
        rec.ExceptionCode = EXCEPTION_SINGLE_STEP;
    	break;
#endif
    default:FIXME("Unhandled SIGTRAP/%x\n", code);
		break;
    }
    EXC_RtlRaiseException( &rec, context );
}

/**********************************************************************
 *		do_trap
 *
 * Implementation of SIGFPE handler.
 */
static void do_fpe( CONTEXT *context, int code, void * addr )
{
    EXCEPTION_RECORD rec;

    switch ( code  & 0xffff ) {
#ifdef FPE_FLTSUB
    case FPE_FLTSUB:
        rec.ExceptionCode = EXCEPTION_ARRAY_BOUNDS_EXCEEDED;
        break;
#endif
#ifdef FPE_INTDIV
    case FPE_INTDIV:
        rec.ExceptionCode = EXCEPTION_INT_DIVIDE_BY_ZERO;
        break;
#endif
#ifdef FPE_INTOVF
    case FPE_INTOVF:
        rec.ExceptionCode = EXCEPTION_INT_OVERFLOW;
        break;
#endif
#ifdef FPE_FLTDIV
    case FPE_FLTDIV:
        rec.ExceptionCode = EXCEPTION_FLT_DIVIDE_BY_ZERO;
        break;
#endif
#ifdef FPE_FLTOVF
    case FPE_FLTOVF:
        rec.ExceptionCode = EXCEPTION_FLT_OVERFLOW;
        break;
#endif
#ifdef FPE_FLTUND
    case FPE_FLTUND:
        rec.ExceptionCode = EXCEPTION_FLT_UNDERFLOW;
        break;
#endif
#ifdef FPE_FLTRES
    case FPE_FLTRES:
        rec.ExceptionCode = EXCEPTION_FLT_INEXACT_RESULT;
        break;
#endif
#ifdef FPE_FLTINV
    case FPE_FLTINV:
#endif
    default:
        rec.ExceptionCode = EXCEPTION_FLT_INVALID_OPERATION;
        break;
    }
    rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    rec.ExceptionRecord  = NULL;
    rec.ExceptionAddress = addr;
    rec.NumberParameters = 0;
    EXC_RtlRaiseException( &rec, context );
}

/**********************************************************************
 *		segv_handler
 *
 * Handler for SIGSEGV and related errors.
 */
static HANDLER_DEF(segv_handler)
{
    CONTEXT context;
    save_context( &context, HANDLER_CONTEXT );
    do_segv( &context, __siginfo->si_signo, __siginfo->si_errno, __siginfo->si_code, __siginfo->si_addr );
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
    save_context( &context, HANDLER_CONTEXT );
    do_trap( &context, __siginfo->si_code, __siginfo->si_addr );
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
    save_fpu( &context, HANDLER_CONTEXT );
    save_context( &context, HANDLER_CONTEXT );
    do_fpe( &context,  __siginfo->si_code, __siginfo->si_addr );
    restore_context( &context, HANDLER_CONTEXT );
    restore_fpu( &context, HANDLER_CONTEXT );
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
