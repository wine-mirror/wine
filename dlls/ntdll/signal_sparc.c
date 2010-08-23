/*
 * Sparc signal handling routines
 *
 * Copyright 1999 Ulrich Weigand
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

#ifdef __sparc__

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <sys/ucontext.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "winnt.h"

#include "wine/exception.h"
#include "ntdll_misc.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);

static pthread_key_t teb_key;

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
 *           call_stack_handlers
 *
 * Call the stack handlers chain.
 */
static NTSTATUS call_stack_handlers( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    EXCEPTION_POINTERS ptrs;

    FIXME( "not implemented on Sparc\n" );

    /* hack: call unhandled exception filter directly */
    ptrs.ExceptionRecord = rec;
    ptrs.ContextRecord = context;
    unhandled_exception_filter( &ptrs );
    return STATUS_UNHANDLED_EXCEPTION;
}


/*******************************************************************
 *		raise_exception
 *
 * Implementation of NtRaiseException.
 */
static NTSTATUS raise_exception( EXCEPTION_RECORD *rec, CONTEXT *context, BOOL first_chance )
{
    NTSTATUS status;

    if (first_chance)
    {
        DWORD c;

        TRACE( "code=%x flags=%x addr=%p ip=%x tid=%04x\n",
               rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
               context->pc, GetCurrentThreadId() );
        for (c = 0; c < rec->NumberParameters; c++)
            TRACE( " info[%d]=%08lx\n", c, rec->ExceptionInformation[c] );
        if (rec->ExceptionCode == EXCEPTION_WINE_STUB)
        {
            if (rec->ExceptionInformation[1] >> 16)
                MESSAGE( "wine: Call from %p to unimplemented function %s.%s, aborting\n",
                         rec->ExceptionAddress,
                         (char*)rec->ExceptionInformation[0], (char*)rec->ExceptionInformation[1] );
            else
                MESSAGE( "wine: Call from %p to unimplemented function %s.%ld, aborting\n",
                         rec->ExceptionAddress,
                         (char*)rec->ExceptionInformation[0], rec->ExceptionInformation[1] );
        }
        else
        {
            /* FIXME: dump context */
        }

        status = send_debug_event( rec, TRUE, context );
        if (status == DBG_CONTINUE || status == DBG_EXCEPTION_HANDLED)
            return STATUS_SUCCESS;

        if (call_vectored_handlers( rec, context ) == EXCEPTION_CONTINUE_EXECUTION)
            return STATUS_SUCCESS;

        if ((status = call_stack_handlers( rec, context )) != STATUS_UNHANDLED_EXCEPTION)
            return status;
    }

    /* last chance exception */

    status = send_debug_event( rec, FALSE, context );
    if (status != DBG_CONTINUE)
    {
        if (rec->ExceptionFlags & EH_STACK_INVALID)
            ERR("Exception frame is not in stack limits => unable to dispatch exception.\n");
        else if (rec->ExceptionCode == STATUS_NONCONTINUABLE_EXCEPTION)
            ERR("Process attempted to continue execution after noncontinuable exception.\n");
        else
            ERR("Unhandled exception code %x flags %x addr %p\n",
                rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress );
        NtTerminateProcess( NtCurrentProcess(), rec->ExceptionCode );
    }
    return STATUS_SUCCESS;
}


/***********************************************************************
 *		RtlCaptureContext (NTDLL.@)
 */
void WINAPI RtlCaptureContext( CONTEXT *context )
{
    FIXME("not implemented\n");
    memset( context, 0, sizeof(*context) );
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


/***********************************************************************
 *           copy_context
 *
 * Copy a register context according to the flags.
 */
void copy_context( CONTEXT *to, const CONTEXT *from, DWORD flags )
{
    flags &= ~CONTEXT_SPARC;  /* get rid of CPU id */
    if (flags & CONTEXT_CONTROL)
    {
        to->psr = from->psr;
        to->pc  = from->pc;
        to->npc = from->npc;
        to->y   = from->y;
        to->wim = from->wim;
        to->tbr = from->tbr;
    }
    if (flags & CONTEXT_INTEGER)
    {
        to->g0 = from->g0;
        to->g1 = from->g1;
        to->g2 = from->g2;
        to->g3 = from->g3;
        to->g4 = from->g4;
        to->g5 = from->g5;
        to->g6 = from->g6;
        to->g7 = from->g7;
        to->o0 = from->o0;
        to->o1 = from->o1;
        to->o2 = from->o2;
        to->o3 = from->o3;
        to->o4 = from->o4;
        to->o5 = from->o5;
        to->o6 = from->o6;
        to->o7 = from->o7;
        to->l0 = from->l0;
        to->l1 = from->l1;
        to->l2 = from->l2;
        to->l3 = from->l3;
        to->l4 = from->l4;
        to->l5 = from->l5;
        to->l6 = from->l6;
        to->l7 = from->l7;
        to->i0 = from->i0;
        to->i1 = from->i1;
        to->i2 = from->i2;
        to->i3 = from->i3;
        to->i4 = from->i4;
        to->i5 = from->i5;
        to->i6 = from->i6;
        to->i7 = from->i7;
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
        /* FIXME */
    }
}


/***********************************************************************
 *           context_to_server
 *
 * Convert a register context to the server format.
 */
NTSTATUS context_to_server( context_t *to, const CONTEXT *from )
{
    DWORD flags = from->ContextFlags & ~CONTEXT_SPARC;  /* get rid of CPU id */

    memset( to, 0, sizeof(*to) );
    to->cpu = CPU_SPARC;

    if (flags & CONTEXT_CONTROL)
    {
        to->flags |= SERVER_CTX_CONTROL;
        to->ctl.sparc_regs.psr = from->psr;
        to->ctl.sparc_regs.pc  = from->pc;
        to->ctl.sparc_regs.npc = from->npc;
        to->ctl.sparc_regs.y   = from->y;
        to->ctl.sparc_regs.wim = from->wim;
        to->ctl.sparc_regs.tbr = from->tbr;
    }
    if (flags & CONTEXT_INTEGER)
    {
        to->flags |= SERVER_CTX_INTEGER;
        to->integer.sparc_regs.g[0] = from->g0;
        to->integer.sparc_regs.g[1] = from->g1;
        to->integer.sparc_regs.g[2] = from->g2;
        to->integer.sparc_regs.g[3] = from->g3;
        to->integer.sparc_regs.g[4] = from->g4;
        to->integer.sparc_regs.g[5] = from->g5;
        to->integer.sparc_regs.g[6] = from->g6;
        to->integer.sparc_regs.g[7] = from->g7;
        to->integer.sparc_regs.o[0] = from->o0;
        to->integer.sparc_regs.o[1] = from->o1;
        to->integer.sparc_regs.o[2] = from->o2;
        to->integer.sparc_regs.o[3] = from->o3;
        to->integer.sparc_regs.o[4] = from->o4;
        to->integer.sparc_regs.o[5] = from->o5;
        to->integer.sparc_regs.o[6] = from->o6;
        to->integer.sparc_regs.o[7] = from->o7;
        to->integer.sparc_regs.l[0] = from->l0;
        to->integer.sparc_regs.l[1] = from->l1;
        to->integer.sparc_regs.l[2] = from->l2;
        to->integer.sparc_regs.l[3] = from->l3;
        to->integer.sparc_regs.l[4] = from->l4;
        to->integer.sparc_regs.l[5] = from->l5;
        to->integer.sparc_regs.l[6] = from->l6;
        to->integer.sparc_regs.l[7] = from->l7;
        to->integer.sparc_regs.i[0] = from->i0;
        to->integer.sparc_regs.i[1] = from->i1;
        to->integer.sparc_regs.i[2] = from->i2;
        to->integer.sparc_regs.i[3] = from->i3;
        to->integer.sparc_regs.i[4] = from->i4;
        to->integer.sparc_regs.i[5] = from->i5;
        to->integer.sparc_regs.i[6] = from->i6;
        to->integer.sparc_regs.i[7] = from->i7;
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
        /* FIXME */
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
    if (from->cpu != CPU_SPARC) return STATUS_INVALID_PARAMETER;

    to->ContextFlags = CONTEXT_SPARC;
    if (from->flags & SERVER_CTX_CONTROL)
    {
        to->ContextFlags |= CONTEXT_CONTROL;
        to->psr = from->ctl.sparc_regs.psr;
        to->pc  = from->ctl.sparc_regs.pc;
        to->npc = from->ctl.sparc_regs.npc;
        to->y   = from->ctl.sparc_regs.y;
        to->wim = from->ctl.sparc_regs.wim;
        to->tbr = from->ctl.sparc_regs.tbr;
    }
    if (from->flags & SERVER_CTX_INTEGER)
    {
        to->ContextFlags |= CONTEXT_INTEGER;
        to->g0 = from->integer.sparc_regs.g[0];
        to->g1 = from->integer.sparc_regs.g[1];
        to->g2 = from->integer.sparc_regs.g[2];
        to->g3 = from->integer.sparc_regs.g[3];
        to->g4 = from->integer.sparc_regs.g[4];
        to->g5 = from->integer.sparc_regs.g[5];
        to->g6 = from->integer.sparc_regs.g[6];
        to->g7 = from->integer.sparc_regs.g[7];
        to->o0 = from->integer.sparc_regs.o[0];
        to->o1 = from->integer.sparc_regs.o[1];
        to->o2 = from->integer.sparc_regs.o[2];
        to->o3 = from->integer.sparc_regs.o[3];
        to->o4 = from->integer.sparc_regs.o[4];
        to->o5 = from->integer.sparc_regs.o[5];
        to->o6 = from->integer.sparc_regs.o[6];
        to->o7 = from->integer.sparc_regs.o[7];
        to->l0 = from->integer.sparc_regs.l[0];
        to->l1 = from->integer.sparc_regs.l[1];
        to->l2 = from->integer.sparc_regs.l[2];
        to->l3 = from->integer.sparc_regs.l[3];
        to->l4 = from->integer.sparc_regs.l[4];
        to->l5 = from->integer.sparc_regs.l[5];
        to->l6 = from->integer.sparc_regs.l[6];
        to->l7 = from->integer.sparc_regs.l[7];
        to->i0 = from->integer.sparc_regs.i[0];
        to->i1 = from->integer.sparc_regs.i[1];
        to->i2 = from->integer.sparc_regs.i[2];
        to->i3 = from->integer.sparc_regs.i[3];
        to->i4 = from->integer.sparc_regs.i[4];
        to->i5 = from->integer.sparc_regs.i[5];
        to->i6 = from->integer.sparc_regs.i[6];
        to->i7 = from->integer.sparc_regs.i[7];
    }
    if (from->flags & SERVER_CTX_FLOATING_POINT)
    {
        /* FIXME */
    }
    return STATUS_SUCCESS;
}


/**********************************************************************
 *		segv_handler
 *
 * Handler for SIGSEGV.
 */
static void segv_handler( int signal, siginfo_t *info, void *ucontext )
{
    EXCEPTION_RECORD rec;
    CONTEXT context;
    NTSTATUS status;

    rec.ExceptionCode = EXCEPTION_ACCESS_VIOLATION;

    /* we want the page-fault case to be fast */
    if ( info->si_code == SEGV_ACCERR )
        if (!(rec.ExceptionCode = virtual_handle_fault( info->si_addr, 0 ))) return;

    save_context( &context, ucontext );
    rec.ExceptionRecord  = NULL;
    rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    rec.ExceptionAddress = (LPVOID)context.pc;
    rec.NumberParameters = 2;
    rec.ExceptionInformation[0] = 0;  /* FIXME: read/write access ? */
    rec.ExceptionInformation[1] = (ULONG_PTR)info->si_addr;

    status = raise_exception( &rec, &context, TRUE );
    if (status) raise_status( status, &rec );
    restore_context( &context, ucontext );
}

/**********************************************************************
 *		bus_handler
 *
 * Handler for SIGBUS.
 */
static void bus_handler( int signal, siginfo_t *info, void *ucontext )
{
    EXCEPTION_RECORD rec;
    CONTEXT context;
    NTSTATUS status;

    save_context( &context, ucontext );
    rec.ExceptionRecord  = NULL;
    rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    rec.ExceptionAddress = (LPVOID)context.pc;
    rec.NumberParameters = 0;

    if ( info->si_code == BUS_ADRALN )
        rec.ExceptionCode = EXCEPTION_DATATYPE_MISALIGNMENT;
    else
        rec.ExceptionCode = EXCEPTION_ACCESS_VIOLATION;

    status = raise_exception( &rec, &context, TRUE );
    if (status) raise_status( status, &rec );
    restore_context( &context, ucontext );
}

/**********************************************************************
 *		ill_handler
 *
 * Handler for SIGILL.
 */
static void ill_handler( int signal, siginfo_t *info, void *ucontext )
{
    EXCEPTION_RECORD rec;
    CONTEXT context;
    NTSTATUS status;

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
    status = raise_exception( &rec, &context, TRUE );
    if (status) raise_status( status, &rec );
    restore_context( &context, ucontext );
}


/**********************************************************************
 *		trap_handler
 *
 * Handler for SIGTRAP.
 */
static void trap_handler( int signal, siginfo_t *info, void *ucontext )
{
    EXCEPTION_RECORD rec;
    CONTEXT context;
    NTSTATUS status;

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
    status = raise_exception( &rec, &context, TRUE );
    if (status) raise_status( status, &rec );
    restore_context( &context, ucontext );
}


/**********************************************************************
 *		fpe_handler
 *
 * Handler for SIGFPE.
 */
static void fpe_handler( int signal, siginfo_t *info, void *ucontext )
{
    EXCEPTION_RECORD rec;
    CONTEXT context;
    NTSTATUS status;

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
    status = raise_exception( &rec, &context, TRUE );
    if (status) raise_status( status, &rec );
    restore_context( &context, ucontext );
    restore_fpu( &context, ucontext );
}


/**********************************************************************
 *		int_handler
 *
 * Handler for SIGINT.
 */
static void int_handler( int signal, siginfo_t *info, void *ucontext )
{
    if (!dispatch_signal(SIGINT))
    {
        EXCEPTION_RECORD rec;
        CONTEXT context;
        NTSTATUS status;

        save_context( &context, ucontext );
        rec.ExceptionCode    = CONTROL_C_EXIT;
        rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
        rec.ExceptionRecord  = NULL;
        rec.ExceptionAddress = (LPVOID)context.pc;
        rec.NumberParameters = 0;
        status = raise_exception( &rec, &context, TRUE );
        if (status) raise_status( status, &rec );
        restore_context( &context, ucontext );
    }
}

/**********************************************************************
 *		abrt_handler
 *
 * Handler for SIGABRT.
 */
static void abrt_handler( int signal, struct siginfo *info, void *ucontext )
{
    EXCEPTION_RECORD rec;
    CONTEXT context;
    NTSTATUS status;

    save_context( &context, ucontext );
    rec.ExceptionCode    = EXCEPTION_WINE_ASSERTION;
    rec.ExceptionFlags   = EH_NONCONTINUABLE;
    rec.ExceptionRecord  = NULL;
    rec.ExceptionAddress = (LPVOID)context.pc;
    rec.NumberParameters = 0;
    status = raise_exception( &rec, &context, TRUE );
    if (status) raise_status( status, &rec );
    restore_context( &context, ucontext );
}


/**********************************************************************
 *		quit_handler
 *
 * Handler for SIGQUIT.
 */
static void quit_handler( int signal, struct siginfo *info, void *ucontext )
{
    abort_thread(0);
}


/**********************************************************************
 *		usr1_handler
 *
 * Handler for SIGUSR1, used to signal a thread that it got suspended.
 */
static void usr1_handler( int signal, struct siginfo *info, void *ucontext )
{
    CONTEXT context;

    save_context( &context, ucontext );
    wait_suspend( &context );
    restore_context( &context, ucontext );
}


/***********************************************************************
 *           __wine_set_signal_handler   (NTDLL.@)
 */
int CDECL __wine_set_signal_handler(unsigned int sig, wine_signal_handler wsh)
{
    if (sig > sizeof(handlers) / sizeof(handlers[0])) return -1;
    if (handlers[sig] != NULL) return -2;
    handlers[sig] = wsh;
    return 0;
}


/**********************************************************************
 *		signal_alloc_thread
 */
NTSTATUS signal_alloc_thread( TEB **teb )
{
    static size_t sigstack_zero_bits;
    SIZE_T size;
    NTSTATUS status;

    if (!sigstack_zero_bits)
    {
        size_t min_size = getpagesize();  /* this is just for the TEB, we don't use a signal stack yet */
        /* find the first power of two not smaller than min_size */
        while ((1u << sigstack_zero_bits) < min_size) sigstack_zero_bits++;
        assert( sizeof(TEB) <= min_size );
    }

    size = 1 << sigstack_zero_bits;
    *teb = NULL;
    if (!(status = NtAllocateVirtualMemory( NtCurrentProcess(), (void **)teb, sigstack_zero_bits,
                                            &size, MEM_COMMIT | MEM_TOP_DOWN, PAGE_READWRITE )))
    {
        (*teb)->Tib.Self = &(*teb)->Tib;
        (*teb)->Tib.ExceptionList = (void *)~0UL;
    }
    return status;
}


/**********************************************************************
 *		signal_free_thread
 */
void signal_free_thread( TEB *teb )
{
    SIZE_T size;

    if (teb->DeallocationStack)
    {
        size = 0;
        NtFreeVirtualMemory( GetCurrentProcess(), &teb->DeallocationStack, &size, MEM_RELEASE );
    }
    size = 0;
    NtFreeVirtualMemory( NtCurrentProcess(), (void **)&teb, &size, MEM_RELEASE );
}


/**********************************************************************
 *		signal_init_thread
 */
void signal_init_thread( TEB *teb )
{
    static int init_done;

    if (!init_done)
    {
        pthread_key_create( &teb_key, NULL );
        init_done = 1;
    }
    pthread_setspecific( teb_key, teb );
}


/**********************************************************************
 *		signal_init_process
 */
void signal_init_process(void)
{
    struct sigaction sig_act;

    sig_act.sa_mask = server_block_set;
    sig_act.sa_flags = SA_RESTART | SA_SIGINFO;

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

    sig_act.sa_sigaction = segv_handler;
    if (sigaction( SIGSEGV, &sig_act, NULL ) == -1) goto error;
    sig_act.sa_sigaction = ill_handler;
    if (sigaction( SIGILL, &sig_act, NULL ) == -1) goto error;
#ifdef SIGBUS
    sig_act.sa_sigaction = bus_handler;
    if (sigaction( SIGBUS, &sig_act, NULL ) == -1) goto error;
#endif

#ifdef SIGTRAP
    sig_act.sa_sigaction = trap_handler;
    if (sigaction( SIGTRAP, &sig_act, NULL ) == -1) goto error;
#endif

    /* 'ta 6' tells the kernel to synthesize any unaligned accesses this 
       process makes, instead of just signalling an error and terminating
       the process.  wine-devel did not reach a conclusion on whether
       this is correct, because that is what x86 does, or it is harmful 
       because it could obscure problems in user code */
    __asm__("ta 6"); /* 6 == ST_FIX_ALIGN defined in sys/trap.h */
    return;

 error:
    perror("sigaction");
    exit(1);
}


/**********************************************************************
 *		__wine_enter_vm86
 */
void __wine_enter_vm86( CONTEXT *context )
{
    MESSAGE("vm86 mode not supported on this platform\n");
}

/***********************************************************************
 *            RtlUnwind  (NTDLL.@)
 */
void WINAPI RtlUnwind( PVOID pEndFrame, PVOID targetIp, PEXCEPTION_RECORD pRecord, PVOID retval )
{
    FIXME( "Not implemented on Sparc\n" );
}

/*******************************************************************
 *		NtRaiseException (NTDLL.@)
 */
NTSTATUS WINAPI NtRaiseException( EXCEPTION_RECORD *rec, CONTEXT *context, BOOL first_chance )
{
    NTSTATUS status = raise_exception( rec, context, first_chance );
    if (status == STATUS_SUCCESS) NtSetContextThread( GetCurrentThread(), context );
    return status;
}

/***********************************************************************
 *		RtlRaiseException (NTDLL.@)
 */
void WINAPI RtlRaiseException( EXCEPTION_RECORD *rec )
{
    CONTEXT context;
    NTSTATUS status;

    RtlCaptureContext( &context );
    rec->ExceptionAddress = (void *)context.pc;
    status = raise_exception( rec, &context, TRUE );
    if (status) raise_status( status, rec );
}

/*************************************************************************
 *		RtlCaptureStackBackTrace (NTDLL.@)
 */
USHORT WINAPI RtlCaptureStackBackTrace( ULONG skip, ULONG count, PVOID *buffer, ULONG *hash )
{
    FIXME( "(%d, %d, %p, %p) stub!\n", skip, count, buffer, hash );
    return 0;
}

/***********************************************************************
 *           call_thread_entry_point
 */
void call_thread_entry_point( LPTHREAD_START_ROUTINE entry, void *arg )
{
    __TRY
    {
        exit_thread( entry( arg ));
    }
    __EXCEPT(unhandled_exception_filter)
    {
        NtTerminateThread( GetCurrentThread(), GetExceptionCode() );
    }
    __ENDTRY
    abort();  /* should not be reached */
}

/***********************************************************************
 *           RtlExitUserThread  (NTDLL.@)
 */
void WINAPI RtlExitUserThread( ULONG status )
{
    exit_thread( status );
}

/***********************************************************************
 *           abort_thread
 */
void abort_thread( int status )
{
    terminate_thread( status );
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

/**********************************************************************
 *           NtCurrentTeb   (NTDLL.@)
 */
TEB * WINAPI NtCurrentTeb(void)
{
    return pthread_getspecific( teb_key );
}

#endif  /* __sparc__ */
