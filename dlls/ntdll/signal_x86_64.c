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
#include "ntstatus.h"
#define WIN32_NO_STATUS
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

#include <asm/prctl.h>
extern int arch_prctl(int func, void *ptr);

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

#if defined(__NetBSD__)
# include <sys/ucontext.h>
# include <sys/types.h>
# include <signal.h>

typedef ucontext_t SIGCONTEXT;

#define RAX_sig(context)    ((context)->uc_mcontext.__gregs[_REG_RAX])
#define RBX_sig(context)    ((context)->uc_mcontext.__gregs[_REG_RBX])
#define RCX_sig(context)    ((context)->uc_mcontext.__gregs[_REG_RCX])
#define RDX_sig(context)    ((context)->uc_mcontext.__gregs[_REG_RDX])
#define RSI_sig(context)    ((context)->uc_mcontext.__gregs[_REG_RSI])
#define RDI_sig(context)    ((context)->uc_mcontext.__gregs[_REG_RDI])
#define RBP_sig(context)    ((context)->uc_mcontext.__gregs[_REG_RBP])
#define R8_sig(context)     ((context)->uc_mcontext.__gregs[_REG_R8])
#define R9_sig(context)     ((context)->uc_mcontext.__gregs[_REG_R9])
#define R10_sig(context)    ((context)->uc_mcontext.__gregs[_REG_R10])
#define R11_sig(context)    ((context)->uc_mcontext.__gregs[_REG_R11])
#define R12_sig(context)    ((context)->uc_mcontext.__gregs[_REG_R12])
#define R13_sig(context)    ((context)->uc_mcontext.__gregs[_REG_R13])
#define R14_sig(context)    ((context)->uc_mcontext.__gregs[_REG_R14])
#define R15_sig(context)    ((context)->uc_mcontext.__gregs[_REG_R15])

#define CS_sig(context)     ((context)->uc_mcontext.__gregs[_REG_CS])
#define DS_sig(context)     ((context)->uc_mcontext.__gregs[_REG_DS])
#define ES_sig(context)     ((context)->uc_mcontext.__gregs[_REG_ES])
#define FS_sig(context)     ((context)->uc_mcontext.__gregs[_REG_FS])
#define GS_sig(context)     ((context)->uc_mcontext.__gregs[_REG_GS])
#define SS_sig(context)     ((context)->uc_mcontext.__gregs[_REG_SS])

#define EFL_sig(context)    ((context)->uc_mcontext.__gregs[_REG_RFL])

#define RIP_sig(context)    (*((unsigned long*)&(context)->uc_mcontext.__gregs[_REG_RIP]))
#define RSP_sig(context)    (*((unsigned long*)&(context)->uc_mcontext.__gregs[_REG_URSP]))

#define TRAP_sig(context)   ((context)->uc_mcontext.__gregs[_REG_TRAPNO])
#define ERROR_sig(context)  ((context)->uc_mcontext.__gregs[_REG_ERR])

#define FAULT_CODE          (__siginfo->si_code)
#define FAULT_ADDRESS       (__siginfo->si_addr)

#define HANDLER_DEF(name) void name( int __signal, siginfo_t *__siginfo, SIGCONTEXT *__context )
#define HANDLER_CONTEXT (__context)

#define FPU_sig(context)   ((XMM_SAVE_AREA32 *)((context)->uc_mcontext.__fpregs))
#endif /* __NetBSD__ */

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
    context->ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS;
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
    if (FPU_sig(sigcontext))
    {
        context->ContextFlags |= CONTEXT_FLOATING_POINT;
        context->u.FltSave = *FPU_sig(sigcontext);
    }
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
 *		RtlCaptureContext (NTDLL.@)
 */
void WINAPI __regs_RtlCaptureContext( CONTEXT *context, CONTEXT *regs )
{
    *context = *regs;
}
DEFINE_REGS_ENTRYPOINT( RtlCaptureContext, 1 )


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
    flags &= ~CONTEXT_AMD64;  /* get rid of CPU id */
    if (flags & CONTEXT_CONTROL)
    {
        to->Rbp    = from->Rbp;
        to->Rip    = from->Rip;
        to->Rsp    = from->Rsp;
        to->SegCs  = from->SegCs;
        to->SegSs  = from->SegSs;
        to->EFlags = from->EFlags;
        to->MxCsr  = from->MxCsr;
    }
    if (flags & CONTEXT_INTEGER)
    {
        to->Rax = from->Rax;
        to->Rcx = from->Rcx;
        to->Rdx = from->Rdx;
        to->Rbx = from->Rbx;
        to->Rsi = from->Rsi;
        to->Rdi = from->Rdi;
        to->R8  = from->R8;
        to->R9  = from->R9;
        to->R10 = from->R10;
        to->R11 = from->R11;
        to->R12 = from->R12;
        to->R13 = from->R13;
        to->R14 = from->R14;
        to->R15 = from->R15;
    }
    if (flags & CONTEXT_SEGMENTS)
    {
        to->SegDs = from->SegDs;
        to->SegEs = from->SegEs;
        to->SegFs = from->SegFs;
        to->SegGs = from->SegGs;
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
        to->u.FltSave = from->u.FltSave;
    }
    if (flags & CONTEXT_DEBUG_REGISTERS)
    {
        to->Dr0 = from->Dr0;
        to->Dr1 = from->Dr1;
        to->Dr2 = from->Dr2;
        to->Dr3 = from->Dr3;
        to->Dr6 = from->Dr6;
        to->Dr7 = from->Dr7;
    }
}


/***********************************************************************
 *           context_to_server
 *
 * Convert a register context to the server format.
 */
NTSTATUS context_to_server( context_t *to, const CONTEXT *from )
{
    DWORD flags = from->ContextFlags & ~CONTEXT_AMD64;  /* get rid of CPU id */

    memset( to, 0, sizeof(*to) );
    to->cpu = CPU_x86_64;

    if (flags & CONTEXT_CONTROL)
    {
        to->flags |= SERVER_CTX_CONTROL;
        to->ctl.x86_64_regs.rbp   = from->Rbp;
        to->ctl.x86_64_regs.rip   = from->Rip;
        to->ctl.x86_64_regs.rsp   = from->Rsp;
        to->ctl.x86_64_regs.cs    = from->SegCs;
        to->ctl.x86_64_regs.ss    = from->SegSs;
        to->ctl.x86_64_regs.flags = from->EFlags;
        to->ctl.x86_64_regs.mxcsr = from->MxCsr;
    }
    if (flags & CONTEXT_INTEGER)
    {
        to->flags |= SERVER_CTX_INTEGER;
        to->integer.x86_64_regs.rax = from->Rax;
        to->integer.x86_64_regs.rcx = from->Rcx;
        to->integer.x86_64_regs.rdx = from->Rdx;
        to->integer.x86_64_regs.rbx = from->Rbx;
        to->integer.x86_64_regs.rsi = from->Rsi;
        to->integer.x86_64_regs.rdi = from->Rdi;
        to->integer.x86_64_regs.r8  = from->R8;
        to->integer.x86_64_regs.r9  = from->R9;
        to->integer.x86_64_regs.r10 = from->R10;
        to->integer.x86_64_regs.r11 = from->R11;
        to->integer.x86_64_regs.r12 = from->R12;
        to->integer.x86_64_regs.r13 = from->R13;
        to->integer.x86_64_regs.r14 = from->R14;
        to->integer.x86_64_regs.r15 = from->R15;
    }
    if (flags & CONTEXT_SEGMENTS)
    {
        to->flags |= SERVER_CTX_SEGMENTS;
        to->seg.x86_64_regs.ds = from->SegDs;
        to->seg.x86_64_regs.es = from->SegEs;
        to->seg.x86_64_regs.fs = from->SegFs;
        to->seg.x86_64_regs.gs = from->SegGs;
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
        to->flags |= SERVER_CTX_FLOATING_POINT;
        memcpy( to->fp.x86_64_regs.fpregs, &from->u.FltSave, sizeof(to->fp.x86_64_regs.fpregs) );
    }
    if (flags & CONTEXT_DEBUG_REGISTERS)
    {
        to->flags |= SERVER_CTX_DEBUG_REGISTERS;
        to->debug.x86_64_regs.dr0 = from->Dr0;
        to->debug.x86_64_regs.dr1 = from->Dr1;
        to->debug.x86_64_regs.dr2 = from->Dr2;
        to->debug.x86_64_regs.dr3 = from->Dr3;
        to->debug.x86_64_regs.dr6 = from->Dr6;
        to->debug.x86_64_regs.dr7 = from->Dr7;
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
    if (from->cpu != CPU_x86_64) return STATUS_INVALID_PARAMETER;

    to->ContextFlags = CONTEXT_AMD64;
    if (from->flags & SERVER_CTX_CONTROL)
    {
        to->ContextFlags |= CONTEXT_CONTROL;
        to->Rbp    = from->ctl.x86_64_regs.rbp;
        to->Rip    = from->ctl.x86_64_regs.rip;
        to->Rsp    = from->ctl.x86_64_regs.rsp;
        to->SegCs  = from->ctl.x86_64_regs.cs;
        to->SegSs  = from->ctl.x86_64_regs.ss;
        to->EFlags = from->ctl.x86_64_regs.flags;
        to->MxCsr  = from->ctl.x86_64_regs.mxcsr;
    }

    if (from->flags & SERVER_CTX_INTEGER)
    {
        to->ContextFlags |= CONTEXT_INTEGER;
        to->Rax = from->integer.x86_64_regs.rax;
        to->Rcx = from->integer.x86_64_regs.rcx;
        to->Rdx = from->integer.x86_64_regs.rdx;
        to->Rbx = from->integer.x86_64_regs.rbx;
        to->Rsi = from->integer.x86_64_regs.rsi;
        to->Rdi = from->integer.x86_64_regs.rdi;
        to->R8  = from->integer.x86_64_regs.r8;
        to->R9  = from->integer.x86_64_regs.r9;
        to->R10 = from->integer.x86_64_regs.r10;
        to->R11 = from->integer.x86_64_regs.r11;
        to->R12 = from->integer.x86_64_regs.r12;
        to->R13 = from->integer.x86_64_regs.r13;
        to->R14 = from->integer.x86_64_regs.r14;
        to->R15 = from->integer.x86_64_regs.r15;
    }
    if (from->flags & SERVER_CTX_SEGMENTS)
    {
        to->ContextFlags |= CONTEXT_SEGMENTS;
        to->SegDs = from->seg.x86_64_regs.ds;
        to->SegEs = from->seg.x86_64_regs.es;
        to->SegFs = from->seg.x86_64_regs.fs;
        to->SegGs = from->seg.x86_64_regs.gs;
    }
    if (from->flags & SERVER_CTX_FLOATING_POINT)
    {
        to->ContextFlags |= CONTEXT_FLOATING_POINT;
        memcpy( &to->u.FltSave, from->fp.x86_64_regs.fpregs, sizeof(from->fp.x86_64_regs.fpregs) );
    }
    if (from->flags & SERVER_CTX_DEBUG_REGISTERS)
    {
        to->ContextFlags |= CONTEXT_DEBUG_REGISTERS;
        to->Dr0 = from->debug.x86_64_regs.dr0;
        to->Dr1 = from->debug.x86_64_regs.dr1;
        to->Dr2 = from->debug.x86_64_regs.dr2;
        to->Dr3 = from->debug.x86_64_regs.dr3;
        to->Dr6 = from->debug.x86_64_regs.dr6;
        to->Dr7 = from->debug.x86_64_regs.dr7;
    }
    return STATUS_SUCCESS;
}


/**********************************************************************
 *           call_stack_handlers
 *
 * Call the stack handlers chain.
 */
static NTSTATUS call_stack_handlers( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    EXCEPTION_POINTERS ptrs;

    FIXME( "not implemented on x86_64\n" );

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

        TRACE( "code=%x flags=%x addr=%p ip=%lx tid=%04x\n",
               rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
               context->Rip, GetCurrentThreadId() );
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
            TRACE(" rax=%016lx rbx=%016lx rcx=%016lx rdx=%016lx\n",
                  context->Rax, context->Rbx, context->Rcx, context->Rdx );
            TRACE(" rsi=%016lx rdi=%016lx rbp=%016lx rsp=%016lx\n",
                  context->Rsi, context->Rdi, context->Rbp, context->Rsp );
            TRACE("  r8=%016lx  r9=%016lx r10=%016lx r11=%016lx\n",
                  context->R8, context->R9, context->R10, context->R11 );
            TRACE(" r12=%016lx r13=%016lx r14=%016lx r15=%016lx\n",
                  context->R12, context->R13, context->R14, context->R15 );
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


/**********************************************************************
 *		segv_handler
 *
 * Handler for SIGSEGV and related errors.
 */
static HANDLER_DEF(segv_handler)
{
    EXCEPTION_RECORD rec;
    CONTEXT context;
    NTSTATUS status;

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
        if (!(rec.ExceptionCode = virtual_handle_fault( FAULT_ADDRESS, rec.ExceptionInformation[0] )))
            goto done;
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

    status = raise_exception( &rec, &context, TRUE );
    if (status) raise_status( status, &rec );
done:
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
    NTSTATUS status;

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

    status = raise_exception( &rec, &context, TRUE );
    if (status) raise_status( status, &rec );
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
    NTSTATUS status;

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

    status = raise_exception( &rec, &context, TRUE );
    if (status) raise_status( status, &rec );
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
        NTSTATUS status;

        save_context( &context, HANDLER_CONTEXT );
        rec.ExceptionCode    = CONTROL_C_EXIT;
        rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
        rec.ExceptionRecord  = NULL;
        rec.ExceptionAddress = (LPVOID)context.Rip;
        rec.NumberParameters = 0;
        status = raise_exception( &rec, &context, TRUE );
        if (status) raise_status( status, &rec );
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
    NTSTATUS status;

    save_context( &context, HANDLER_CONTEXT );
    rec.ExceptionCode    = EXCEPTION_WINE_ASSERTION;
    rec.ExceptionFlags   = EH_NONCONTINUABLE;
    rec.ExceptionRecord  = NULL;
    rec.ExceptionAddress = (LPVOID)context.Rip;
    rec.NumberParameters = 0;
    status = raise_exception( &rec, &context, TRUE );
    if (status) raise_status( status, &rec );
    restore_context( &context, HANDLER_CONTEXT );
}


/**********************************************************************
 *		quit_handler
 *
 * Handler for SIGQUIT.
 */
static HANDLER_DEF(quit_handler)
{
    abort_thread(0);
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
int CDECL __wine_set_signal_handler(unsigned int sig, wine_signal_handler wsh)
{
    if (sig > sizeof(handlers) / sizeof(handlers[0])) return -1;
    if (handlers[sig] != NULL) return -2;
    handlers[sig] = wsh;
    return 0;
}


/**********************************************************************
 *		signal_init_thread
 */
void signal_init_thread( TEB *teb )
{
#ifdef __linux__
    arch_prctl( ARCH_SET_GS, teb );
#else
# error Please define setting %gs for your architecture
#endif
}

/**********************************************************************
 *		signal_init_process
 */
void signal_init_process(void)
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
    return;

 error:
    perror("sigaction");
    exit(1);
}


/**********************************************************************
 *              RtlLookupFunctionEntry   (NTDLL.@)
 */
PRUNTIME_FUNCTION WINAPI RtlLookupFunctionEntry( ULONG64 pc, ULONG64 *base,
                                                 UNWIND_HISTORY_TABLE *table )
{
    FIXME("stub\n");
    return NULL;
}


/**********************************************************************
 *              RtlVirtualUnwind   (NTDLL.@)
 */
PVOID WINAPI RtlVirtualUnwind ( ULONG type, ULONG64 base, ULONG64 pc,
                                RUNTIME_FUNCTION *function, CONTEXT *context,
                                PVOID *data, ULONG64 *frame,
                                KNONVOLATILE_CONTEXT_POINTERS *ctx_ptr )
{
    FIXME("stub\n");
    return NULL;
}


/*******************************************************************
 *		RtlUnwind (NTDLL.@)
 */
void WINAPI __regs_RtlUnwind( EXCEPTION_REGISTRATION_RECORD* pEndFrame, PVOID targetIp,
                              PEXCEPTION_RECORD pRecord, PVOID retval, CONTEXT *context )
{
    EXCEPTION_RECORD record;

    /* build an exception record, if we do not have one */
    if (!pRecord)
    {
        record.ExceptionCode    = STATUS_UNWIND;
        record.ExceptionFlags   = 0;
        record.ExceptionRecord  = NULL;
        record.ExceptionAddress = (void *)context->Rip;
        record.NumberParameters = 0;
        pRecord = &record;
    }

    pRecord->ExceptionFlags |= EH_UNWINDING | (pEndFrame ? 0 : EH_EXIT_UNWIND);

    FIXME( "code=%x flags=%x not implemented on x86_64\n",
           pRecord->ExceptionCode, pRecord->ExceptionFlags );
    NtTerminateProcess( GetCurrentProcess(), 1 );
}
DEFINE_REGS_ENTRYPOINT( RtlUnwind, 4 )


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
void WINAPI __regs_RtlRaiseException( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    NTSTATUS status;

    rec->ExceptionAddress = (void *)context->Rip;
    status = raise_exception( rec, context, TRUE );
    if (status != STATUS_SUCCESS) raise_status( status, rec );
}
DEFINE_REGS_ENTRYPOINT( RtlRaiseException, 1 )


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
