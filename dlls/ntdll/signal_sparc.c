/*
 * Sparc signal handling routines
 * 
 * Copyright 1999 Ulrich Weigand
 */

#ifdef __sparc__

#include "config.h"

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <sys/ucontext.h>

#include "wine/exception.h"
#include "winnt.h"
#include "winbase.h"
#include "global.h"
#include "stackframe.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(seh)


/*
 * FIXME:  All this works only on Solaris for now
 */

/**********************************************************************
 *		save_context
 */
static void save_context( CONTEXT *context, ucontext_t *ucontext )
{
    /* Special registers */
    context->psr = ucontext->uc_mcontext.gregs[REG_PSR];
    context->pc  = ucontext->uc_mcontext.gregs[REG_PC];
    context->npc = ucontext->uc_mcontext.gregs[REG_nPC];
    context->y   = ucontext->uc_mcontext.gregs[REG_Y];
    context->wim = 0;  /* FIXME */
    context->tbr = 0;  /* FIXME */

    /* Global registers */
    context->g0 = 0;  /* always */
    context->g1 = ucontext->uc_mcontext.gregs[REG_G1];
    context->g2 = ucontext->uc_mcontext.gregs[REG_G2];
    context->g3 = ucontext->uc_mcontext.gregs[REG_G3];
    context->g4 = ucontext->uc_mcontext.gregs[REG_G4];
    context->g5 = ucontext->uc_mcontext.gregs[REG_G5];
    context->g6 = ucontext->uc_mcontext.gregs[REG_G6];
    context->g7 = ucontext->uc_mcontext.gregs[REG_G7];

    /* Current 'out' registers */
    context->o0 = ucontext->uc_mcontext.gregs[REG_O0];
    context->o1 = ucontext->uc_mcontext.gregs[REG_O1];
    context->o2 = ucontext->uc_mcontext.gregs[REG_O2];
    context->o3 = ucontext->uc_mcontext.gregs[REG_O3];
    context->o4 = ucontext->uc_mcontext.gregs[REG_O4];
    context->o5 = ucontext->uc_mcontext.gregs[REG_O5];
    context->o6 = ucontext->uc_mcontext.gregs[REG_O6];
    context->o7 = ucontext->uc_mcontext.gregs[REG_O7];

    /* FIXME: what if the current register window isn't saved? */
    if ( ucontext->uc_mcontext.gwins && ucontext->uc_mcontext.gwins->wbcnt > 0 )
    {
        /* Current 'local' registers from first register window */
        context->l0 = ucontext->uc_mcontext.gwins->wbuf[0].rw_local[0];
        context->l1 = ucontext->uc_mcontext.gwins->wbuf[0].rw_local[1];
        context->l2 = ucontext->uc_mcontext.gwins->wbuf[0].rw_local[2];
        context->l3 = ucontext->uc_mcontext.gwins->wbuf[0].rw_local[3];
        context->l4 = ucontext->uc_mcontext.gwins->wbuf[0].rw_local[4];
        context->l5 = ucontext->uc_mcontext.gwins->wbuf[0].rw_local[5];
        context->l6 = ucontext->uc_mcontext.gwins->wbuf[0].rw_local[6];
        context->l7 = ucontext->uc_mcontext.gwins->wbuf[0].rw_local[7];

        /* Current 'in' registers from first register window */
        context->i0 = ucontext->uc_mcontext.gwins->wbuf[0].rw_in[0];
        context->i1 = ucontext->uc_mcontext.gwins->wbuf[0].rw_in[1];
        context->i2 = ucontext->uc_mcontext.gwins->wbuf[0].rw_in[2];
        context->i3 = ucontext->uc_mcontext.gwins->wbuf[0].rw_in[3];
        context->i4 = ucontext->uc_mcontext.gwins->wbuf[0].rw_in[4];
        context->i5 = ucontext->uc_mcontext.gwins->wbuf[0].rw_in[5];
        context->i6 = ucontext->uc_mcontext.gwins->wbuf[0].rw_in[6];
        context->i7 = ucontext->uc_mcontext.gwins->wbuf[0].rw_in[7];
    }
}

/**********************************************************************
 *		restore_context
 */
static void restore_context( CONTEXT *context, ucontext_t *ucontext )
{
   /* FIXME */
}

/**********************************************************************
 *		save_fpu
 */
static void save_fpu( CONTEXT *context, ucontext_t *ucontext )
{
   /* FIXME */
}

/**********************************************************************
 *		restore_fpu
 */
static void restore_fpu( CONTEXT *context, ucontext_t *ucontext )
{
   /* FIXME */
}


/**********************************************************************
 *		segv_handler
 *
 * Handler for SIGSEGV.
 */
static void segv_handler( int signal, siginfo_t *info, ucontext_t *ucontext )
{
    EXCEPTION_RECORD rec;
    CONTEXT context;

    /* we want the page-fault case to be fast */
    if ( info->si_code == SEGV_ACCERR )
        if (VIRTUAL_HandleFault( (LPVOID)info->si_addr )) return;

    save_context( &context, ucontext );
    rec.ExceptionCode    = EXCEPTION_ACCESS_VIOLATION;
    rec.ExceptionRecord  = NULL;
    rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    rec.ExceptionAddress = (LPVOID)context.pc;
    rec.NumberParameters = 2;
    rec.ExceptionInformation[0] = 0;  /* FIXME: read/write access ? */
    rec.ExceptionInformation[1] = (DWORD)info->si_addr;
    
    EXC_RtlRaiseException( &rec, &context );
    restore_context( &context, ucontext );
}

/**********************************************************************
 *		bus_handler
 *
 * Handler for SIGBUS.
 */
static void bus_handler( int signal, siginfo_t *info, ucontext_t *ucontext )
{
    EXCEPTION_RECORD rec;
    CONTEXT context;

    save_context( &context, ucontext );
    rec.ExceptionRecord  = NULL;
    rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    rec.ExceptionAddress = (LPVOID)context.pc;
    rec.NumberParameters = 0;

    if ( info->si_code == BUS_ADRALN )
        rec.ExceptionCode = EXCEPTION_DATATYPE_MISALIGNMENT;
    else
        rec.ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
    
    EXC_RtlRaiseException( &rec, &context );
    restore_context( &context, ucontext );
}

/**********************************************************************
 *		ill_handler
 *
 * Handler for SIGILL.
 */
static void ill_handler( int signal, siginfo_t *info, ucontext_t *ucontext )
{
    EXCEPTION_RECORD rec;
    CONTEXT context;

    switch ( info->si_code )
    {
    default:
    case ILL_ILLOPC:
    case ILL_ILLOPN:
    case ILL_ILLADR:
    case ILL_ILLTRP:
        rec.ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
        break;

    case ILL_PRVOPC:
    case ILL_PRVREG:
        rec.ExceptionCode = EXCEPTION_PRIV_INSTRUCTION;
        break;

    case ILL_BADSTK:
        rec.ExceptionCode = EXCEPTION_STACK_OVERFLOW;
        break;
    }
    
    save_context( &context, ucontext );
    rec.ExceptionRecord  = NULL;
    rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    rec.ExceptionAddress = (LPVOID)context.pc;
    rec.NumberParameters = 0;
    EXC_RtlRaiseException( &rec, &context );
    restore_context( &context, ucontext );
}


/**********************************************************************
 *		trap_handler
 *
 * Handler for SIGTRAP.
 */
static void trap_handler( int signal, siginfo_t *info, ucontext_t *ucontext )
{
    EXCEPTION_RECORD rec;
    CONTEXT context;

    switch ( info->si_code )
    {
    case TRAP_TRACE:
        rec.ExceptionCode = EXCEPTION_SINGLE_STEP;
        break;
    case TRAP_BRKPT:
    default:
        rec.ExceptionCode = EXCEPTION_BREAKPOINT;
        break;
    }

    save_context( &context, ucontext );
    rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    rec.ExceptionRecord  = NULL;
    rec.ExceptionAddress = (LPVOID)context.pc;
    rec.NumberParameters = 0;
    EXC_RtlRaiseException( &rec, &context );
    restore_context( &context, ucontext );
}


/**********************************************************************
 *		fpe_handler
 *
 * Handler for SIGFPE.
 */
static void fpe_handler( int signal, siginfo_t *info, ucontext_t *ucontext )
{
    EXCEPTION_RECORD rec;
    CONTEXT context;

    switch ( info->si_code )
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

    save_context( &context, ucontext );
    save_fpu( &context, ucontext );
    rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    rec.ExceptionRecord  = NULL;
    rec.ExceptionAddress = (LPVOID)context.pc;
    rec.NumberParameters = 0;
    EXC_RtlRaiseException( &rec, &context );
    restore_context( &context, ucontext );
    restore_fpu( &context, ucontext );
}


/**********************************************************************
 *		int_handler
 *
 * Handler for SIGINT.
 */
static void int_handler( int signal, siginfo_t *info, ucontext_t *ucontext )
{
    EXCEPTION_RECORD rec;
    CONTEXT context;

    save_context( &context, ucontext );
    rec.ExceptionCode    = CONTROL_C_EXIT;
    rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    rec.ExceptionRecord  = NULL;
    rec.ExceptionAddress = (LPVOID)context.pc;
    rec.NumberParameters = 0;
    EXC_RtlRaiseException( &rec, &context );
    restore_context( &context, ucontext );
}


/***********************************************************************
 *           set_handler
 *
 * Set a signal handler
 */
static int set_handler( int sig, void (*func)() )
{
    struct sigaction sig_act;

    sig_act.sa_handler = NULL;
    sig_act.sa_sigaction = func;
    sigemptyset( &sig_act.sa_mask );
    sig_act.sa_flags = SA_SIGINFO;

    return sigaction( sig, &sig_act, NULL );
}


/**********************************************************************
 *		SIGNAL_Init
 */
BOOL SIGNAL_Init(void)
{
    /* ignore SIGPIPE so that WINSOCK can get a EPIPE error instead  */
    signal( SIGPIPE, SIG_IGN );
    /* automatic child reaping to avoid zombies */
    signal( SIGCHLD, SIG_IGN );

    if (set_handler( SIGINT,  (void (*)())int_handler  ) == -1) goto error;
    if (set_handler( SIGFPE,  (void (*)())fpe_handler  ) == -1) goto error;
    if (set_handler( SIGSEGV, (void (*)())segv_handler ) == -1) goto error;
    if (set_handler( SIGILL,  (void (*)())ill_handler  ) == -1) goto error;
    if (set_handler( SIGBUS,  (void (*)())bus_handler  ) == -1) goto error;
    if (set_handler( SIGTRAP, (void (*)())trap_handler ) == -1) goto error;
    return TRUE;

 error:
    perror("sigaction");
    return FALSE;
}

/**********************************************************************
 *              DbgBreakPoint   (NTDLL)
 */
void WINAPI DbgBreakPoint(void)
{
    /* FIXME */
}

/**********************************************************************
 *              DbgUserBreakPoint   (NTDLL)
 */
void WINAPI DbgUserBreakPoint(void)
{
    /* FIXME */
}

#endif  /* __sparc__ */
