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

#ifdef __aarch64__

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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/library.h"
#include "wine/exception.h"
#include "ntdll_misc.h"
#include "wine/debug.h"
#include "winnt.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);
WINE_DECLARE_DEBUG_CHANNEL(relay);

static pthread_key_t teb_key;

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

/* Exceptions */
# define FAULT_sig(context)         REG_sig(fault_address, context)

#endif /* linux */

static const size_t teb_size = 0x2000;  /* we reserve two pages for the TEB */
static size_t signal_stack_size;

typedef void (WINAPI *raise_func)( EXCEPTION_RECORD *rec, CONTEXT *context );
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

/*******************************************************************
 *         is_valid_frame
 */
static inline BOOL is_valid_frame( void *frame )
{
    if ((ULONG_PTR)frame & 3) return FALSE;
    return (frame >= NtCurrentTeb()->Tib.StackLimit &&
            (void **)frame < (void **)NtCurrentTeb()->Tib.StackBase - 1);
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
static inline void save_fpu( CONTEXT *context, const ucontext_t *sigcontext )
{
    FIXME( "Not implemented on ARM64\n" );
}


/***********************************************************************
 *           restore_fpu
 *
 * Restore the FPU context to a sigcontext.
 */
static inline void restore_fpu( CONTEXT *context, const ucontext_t *sigcontext )
{
    FIXME( "Not implemented on ARM64\n" );
}

/***********************************************************************
 *		RtlCaptureContext (NTDLL.@)
 */
/* FIXME: Use the Stack instead of the actual register values? */
__ASM_STDCALL_FUNC( RtlCaptureContext, 8,
                    "stp x0, x1, [sp, #-32]!\n\t"
                    "mov w1, #0x400000\n\t"         /* CONTEXT_ARM64 */
                    "add w1, w1, #0x3\n\t"          /* CONTEXT_FULL */
                    "str w1, [x0]\n\t"              /* context->ContextFlags */ /* 32-bit, look at cpsr */
                    "mrs x1, NZCV\n\t"
                    "str w1, [x0, #0x4]\n\t"        /* context->Cpsr */
                    "ldp x0, x1, [sp], #32\n\t"
                    "str x0, [x0, #0x8]\n\t"        /* context->X0 */
                    "str x1, [x0, #0x10]\n\t"       /* context->X1 */
                    "str x2, [x0, #0x18]\n\t"       /* context->X2 */
                    "str x3, [x0, #0x20]\n\t"       /* context->X3 */
                    "str x4, [x0, #0x28]\n\t"       /* context->X4 */
                    "str x5, [x0, #0x30]\n\t"       /* context->X5 */
                    "str x6, [x0, #0x38]\n\t"       /* context->X6 */
                    "str x7, [x0, #0x40]\n\t"       /* context->X7 */
                    "str x8, [x0, #0x48]\n\t"       /* context->X8 */
                    "str x9, [x0, #0x50]\n\t"       /* context->X9 */
                    "str x10, [x0, #0x58]\n\t"      /* context->X10 */
                    "str x11, [x0, #0x60]\n\t"      /* context->X11 */
                    "str x12, [x0, #0x68]\n\t"      /* context->X12 */
                    "str x13, [x0, #0x70]\n\t"      /* context->X13 */
                    "str x14, [x0, #0x78]\n\t"      /* context->X14 */
                    "str x15, [x0, #0x80]\n\t"      /* context->X15 */
                    "str x16, [x0, #0x88]\n\t"      /* context->X16 */
                    "str x17, [x0, #0x90]\n\t"      /* context->X17 */
                    "str x18, [x0, #0x98]\n\t"      /* context->X18 */
                    "str x19, [x0, #0xa0]\n\t"      /* context->X19 */
                    "str x20, [x0, #0xa8]\n\t"      /* context->X20 */
                    "str x21, [x0, #0xb0]\n\t"      /* context->X21 */
                    "str x22, [x0, #0xb8]\n\t"      /* context->X22 */
                    "str x23, [x0, #0xc0]\n\t"      /* context->X23 */
                    "str x24, [x0, #0xc8]\n\t"      /* context->X24 */
                    "str x25, [x0, #0xd0]\n\t"      /* context->X25 */
                    "str x26, [x0, #0xd8]\n\t"      /* context->X26 */
                    "str x27, [x0, #0xe0]\n\t"      /* context->X27 */
                    "str x28, [x0, #0xe8]\n\t"      /* context->X28 */
                    "str x29, [x0, #0xf0]\n\t"      /* context->Fp */
                    "str x30, [x0, #0xf8]\n\t"      /* context->Lr */
                    "mov x1, sp\n\t"
                    "str x1, [x0, #0x100]\n\t"      /* context->Sp */
                    "adr x1, 1f\n\t"
                    "1: str x1, [x0, #0x108]\n\t"   /* context->Pc */
                    "ret"
                    )

/***********************************************************************
 *           set_cpu_context
 *
 * Set the new CPU context.
 */
static void set_cpu_context( const CONTEXT *context )
{
    FIXME( "Not implemented on ARM64\n" );
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
        for (i = 0; i < 64; i++) to->fp.arm64_regs.d[i] = from->V[i / 2].D[i % 2];
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
        for (i = 0; i < 64; i++) to->V[i / 2].D[i % 2] = from->fp.arm64_regs.d[i];
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
    NTSTATUS ret;
    BOOL self;
    context_t server_context;

    context_to_server( &server_context, context );
    ret = set_thread_context( handle, &server_context, &self );
    if (self && ret == STATUS_SUCCESS) set_cpu_context( context );
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
 *           setup_exception_record
 *
 * Setup the exception record and context on the thread stack.
 */
static EXCEPTION_RECORD *setup_exception( ucontext_t *sigcontext, raise_func func )
{
    struct stack_layout
    {
        CONTEXT           context;
        EXCEPTION_RECORD  rec;
    } *stack;
    DWORD exception_code = 0;

    /* push the stack_layout structure */
    stack = (struct stack_layout *)((SP_sig(sigcontext) - sizeof(*stack)) & ~15);

    stack->rec.ExceptionRecord  = NULL;
    stack->rec.ExceptionCode    = exception_code;
    stack->rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    stack->rec.ExceptionAddress = (LPVOID)PC_sig(sigcontext);
    stack->rec.NumberParameters = 0;

    save_context( &stack->context, sigcontext );

    /* now modify the sigcontext to return to the raise function */
    SP_sig(sigcontext) = (ULONG_PTR)stack;
    PC_sig(sigcontext) = (ULONG_PTR)func;
    REGn_sig(0, sigcontext) = (ULONG_PTR)&stack->rec;  /* first arg for raise_func */
    REGn_sig(1, sigcontext) = (ULONG_PTR)&stack->context; /* second arg for raise_func */

    return &stack->rec;
}

/**********************************************************************
 *		raise_segv_exception
 */
static void WINAPI raise_segv_exception( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    NTSTATUS status;

    switch(rec->ExceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION:
        if (rec->NumberParameters == 2)
        {
            if (!(rec->ExceptionCode = virtual_handle_fault( (void *)rec->ExceptionInformation[1],
                                                             rec->ExceptionInformation[0], FALSE )))
                goto done;
        }
        break;
    }
    status = NtRaiseException( rec, context, TRUE );
    if (status) raise_status( status, rec );
done:
    set_cpu_context( context );
}

/**********************************************************************
 *           call_stack_handlers
 *
 * Call the stack handlers chain.
 */
static NTSTATUS call_stack_handlers( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    EXCEPTION_REGISTRATION_RECORD *frame, *dispatch, *nested_frame;
    DWORD res;

    frame = NtCurrentTeb()->Tib.ExceptionList;
    nested_frame = NULL;
    while (frame != (EXCEPTION_REGISTRATION_RECORD*)~0UL)
    {
        /* Check frame address */
        if (!is_valid_frame( frame ))
        {
            rec->ExceptionFlags |= EH_STACK_INVALID;
            break;
        }

        /* Call handler */
        TRACE( "calling handler at %p code=%x flags=%x\n",
               frame->Handler, rec->ExceptionCode, rec->ExceptionFlags );
        res = frame->Handler( rec, frame, context, &dispatch );
        TRACE( "handler at %p returned %x\n", frame->Handler, res );

        if (frame == nested_frame)
        {
            /* no longer nested */
            nested_frame = NULL;
            rec->ExceptionFlags &= ~EH_NESTED_CALL;
        }

        switch(res)
        {
        case ExceptionContinueExecution:
            if (!(rec->ExceptionFlags & EH_NONCONTINUABLE)) return STATUS_SUCCESS;
            return STATUS_NONCONTINUABLE_EXCEPTION;
        case ExceptionContinueSearch:
            break;
        case ExceptionNestedException:
            if (nested_frame < dispatch) nested_frame = dispatch;
            rec->ExceptionFlags |= EH_NESTED_CALL;
            break;
        default:
            return STATUS_INVALID_DISPOSITION;
        }
        frame = frame->Prev;
    }
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

        for (c = 0; c < rec->NumberParameters; c++)
            TRACE( " info[%d]=%016lx\n", c, rec->ExceptionInformation[c] );
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

static inline DWORD is_write_fault( ucontext_t *context )
{
    DWORD inst = *(DWORD *)PC_sig(context);
    if ((inst & 0xbfff0000) == 0x0c000000   /* C3.3.1 */ ||
        (inst & 0xbfe00000) == 0x0c800000   /* C3.3.2 */ ||
        (inst & 0xbfdf0000) == 0x0d000000   /* C3.3.3 */ ||
        (inst & 0xbfc00000) == 0x0d800000   /* C3.3.4 */ ||
        (inst & 0x3f400000) == 0x08000000   /* C3.3.6 */ ||
        (inst & 0x3bc00000) == 0x38000000   /* C3.3.8-12 */ ||
        (inst & 0x3fe00000) == 0x3c800000   /* C3.3.8-12 128bit */ ||
        (inst & 0x3bc00000) == 0x39000000   /* C3.3.13 */ ||
        (inst & 0x3fc00000) == 0x3d800000   /* C3.3.13 128bit */ ||
        (inst & 0x3a400000) == 0x28000000)  /* C3.3.7,14-16 */
        return EXCEPTION_WRITE_FAULT;
    return EXCEPTION_READ_FAULT;
}

/**********************************************************************
 *		segv_handler
 *
 * Handler for SIGSEGV and related errors.
 */
static void segv_handler( int signal, siginfo_t *info, void *ucontext )
{
    EXCEPTION_RECORD *rec;
    ucontext_t *context = ucontext;

    /* check for page fault inside the thread stack */
    if (signal == SIGSEGV &&
        (char *)info->si_addr >= (char *)NtCurrentTeb()->DeallocationStack &&
        (char *)info->si_addr < (char *)NtCurrentTeb()->Tib.StackBase &&
        virtual_handle_stack_fault( info->si_addr ))
    {
        /* check if this was the last guard page */
        if ((char *)info->si_addr < (char *)NtCurrentTeb()->DeallocationStack + 2*4096)
        {
            rec = setup_exception( context, raise_segv_exception );
            rec->ExceptionCode = EXCEPTION_STACK_OVERFLOW;
        }
        return;
    }

    rec = setup_exception( context, raise_segv_exception );
    if (rec->ExceptionCode == EXCEPTION_STACK_OVERFLOW) return;

    switch(signal)
    {
    case SIGILL:   /* Invalid opcode exception */
        rec->ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
        break;
    case SIGSEGV:  /* Segmentation fault */
        rec->ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
        rec->NumberParameters = 2;
        rec->ExceptionInformation[0] = is_write_fault(context);
        rec->ExceptionInformation[1] = (ULONG_PTR)info->si_addr;
        break;
    case SIGBUS:  /* Alignment check exception */
        rec->ExceptionCode = EXCEPTION_DATATYPE_MISALIGNMENT;
        break;
    default:
        ERR("Got unexpected signal %i\n", signal);
        rec->ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
        break;
    }
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
    rec.ExceptionAddress = (LPVOID)context.Pc;
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
static void fpe_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    EXCEPTION_RECORD rec;
    CONTEXT context;
    NTSTATUS status;

    save_fpu( &context, sigcontext );
    save_context( &context, sigcontext );

    switch (siginfo->si_code & 0xffff )
    {
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
    rec.ExceptionAddress = (LPVOID)context.Pc;
    rec.NumberParameters = 0;
    status = raise_exception( &rec, &context, TRUE );
    if (status) raise_status( status, &rec );

    restore_context( &context, sigcontext );
    restore_fpu( &context, sigcontext );
}

/**********************************************************************
 *		int_handler
 *
 * Handler for SIGINT.
 */
static void int_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    if (!dispatch_signal(SIGINT))
    {
        EXCEPTION_RECORD rec;
        CONTEXT context;
        NTSTATUS status;

        save_context( &context, sigcontext );
        rec.ExceptionCode    = CONTROL_C_EXIT;
        rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
        rec.ExceptionRecord  = NULL;
        rec.ExceptionAddress = (LPVOID)context.Pc;
        rec.NumberParameters = 0;
        status = raise_exception( &rec, &context, TRUE );
        if (status) raise_status( status, &rec );
        restore_context( &context, sigcontext );
    }
}


/**********************************************************************
 *		abrt_handler
 *
 * Handler for SIGABRT.
 */
static void abrt_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    EXCEPTION_RECORD rec;
    CONTEXT context;
    NTSTATUS status;

    save_context( &context, sigcontext );
    rec.ExceptionCode    = EXCEPTION_WINE_ASSERTION;
    rec.ExceptionFlags   = EH_NONCONTINUABLE;
    rec.ExceptionRecord  = NULL;
    rec.ExceptionAddress = (LPVOID)context.Pc;
    rec.NumberParameters = 0;
    status = raise_exception( &rec, &context, TRUE );
    if (status) raise_status( status, &rec );
    restore_context( &context, sigcontext );
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


/***********************************************************************
 *           __wine_set_signal_handler   (NTDLL.@)
 */
int CDECL __wine_set_signal_handler(unsigned int sig, wine_signal_handler wsh)
{
    if (sig >= sizeof(handlers) / sizeof(handlers[0])) return -1;
    if (handlers[sig] != NULL) return -2;
    handlers[sig] = wsh;
    return 0;
}


/**********************************************************************
 *             signal_alloc_thread
 */
NTSTATUS signal_alloc_thread( TEB **teb )
{
    static size_t sigstack_zero_bits;
    SIZE_T size;
    NTSTATUS status;

    if (!sigstack_zero_bits)
    {
        size_t min_size = teb_size + max( MINSIGSTKSZ, 8192 );
        /* find the first power of two not smaller than min_size */
        sigstack_zero_bits = 12;
        while ((1u << sigstack_zero_bits) < min_size) sigstack_zero_bits++;
        signal_stack_size = (1 << sigstack_zero_bits) - teb_size;
        assert( sizeof(TEB) <= teb_size );
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
 *             signal_free_thread
 */
void signal_free_thread( TEB *teb )
{
    SIZE_T size = 0;

    NtFreeVirtualMemory( NtCurrentProcess(), (void **)&teb, &size, MEM_RELEASE );
}


/**********************************************************************
 *		signal_init_thread
 */
void signal_init_thread( TEB *teb )
{
    static BOOL init_done;

    if (!init_done)
    {
        pthread_key_create( &teb_key, NULL );
        init_done = TRUE;
    }

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
    if (sigaction( SIGILL, &sig_act, NULL ) == -1) goto error;
#ifdef SIGBUS
    if (sigaction( SIGBUS, &sig_act, NULL ) == -1) goto error;
#endif

#ifdef SIGTRAP
    sig_act.sa_sigaction = trap_handler;
    if (sigaction( SIGTRAP, &sig_act, NULL ) == -1) goto error;
#endif
    return;

 error:
    perror("sigaction");
    exit(1);
}


/***********************************************************************
 *            RtlUnwind  (NTDLL.@)
 */
void WINAPI RtlUnwind( PVOID pEndFrame, PVOID targetIp, PEXCEPTION_RECORD pRecord, PVOID retval )
{
    FIXME( "Not implemented on ARM64\n" );
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
    rec->ExceptionAddress = (LPVOID)context.Pc;
    status = raise_exception( rec, &context, TRUE );
    if (status) raise_status( status, rec );
}

/*************************************************************************
 *             RtlCaptureStackBackTrace (NTDLL.@)
 */
USHORT WINAPI RtlCaptureStackBackTrace( ULONG skip, ULONG count, PVOID *buffer, ULONG *hash )
{
    FIXME( "(%d, %d, %p, %p) stub!\n", skip, count, buffer, hash );
    return 0;
}

/***********************************************************************
 *           call_thread_entry_point
 */
static void WINAPI call_thread_entry_point( LPTHREAD_START_ROUTINE entry, void *arg )
{
    __TRY
    {
        TRACE_(relay)( "\1Starting thread proc %p (arg=%p)\n", entry, arg );
        RtlExitUserThread( entry( arg ));
    }
    __EXCEPT(unhandled_exception_filter)
    {
        NtTerminateThread( GetCurrentThread(), GetExceptionCode() );
    }
    __ENDTRY
    abort();  /* should not be reached */
}

typedef void (WINAPI *thread_start_func)(LPTHREAD_START_ROUTINE,void *);

struct startup_info
{
    thread_start_func      start;
    LPTHREAD_START_ROUTINE entry;
    void                  *arg;
    BOOL                   suspend;
};

/***********************************************************************
 *           thread_startup
 */
static void thread_startup( void *param )
{
    CONTEXT context = { 0 };
    struct startup_info *info = param;

    /* build the initial context */
    context.ContextFlags = CONTEXT_FULL;
    context.u.s.X0 = (DWORD_PTR)info->entry;
    context.u.s.X1 = (DWORD_PTR)info->arg;
    context.Sp = (DWORD_PTR)NtCurrentTeb()->Tib.StackBase;
    context.Pc = (DWORD_PTR)info->start;

    if (info->suspend) wait_suspend( &context );
    attach_dlls( &context, (void **)&context.u.s.X0 );

    ((thread_start_func)context.Pc)( (LPTHREAD_START_ROUTINE)context.u.s.X0, (void *)context.u.s.X1 );
}

/***********************************************************************
 *           signal_start_thread
 *
 * Thread startup sequence:
 * signal_start_thread()
 *   -> thread_startup()
 *     -> call_thread_entry_point()
 */
void signal_start_thread( LPTHREAD_START_ROUTINE entry, void *arg, BOOL suspend )
{
    struct startup_info info = { call_thread_entry_point, entry, arg, suspend };
    wine_switch_to_stack( thread_startup, &info, NtCurrentTeb()->Tib.StackBase );
}

/**********************************************************************
 *		signal_start_process
 *
 * Process startup sequence:
 * signal_start_process()
 *   -> thread_startup()
 *     -> kernel32_start_process()
 */
void signal_start_process( LPTHREAD_START_ROUTINE entry, BOOL suspend )
{
    struct startup_info info = { kernel32_start_process, entry, NtCurrentTeb()->Peb, suspend };
    wine_switch_to_stack( thread_startup, &info, NtCurrentTeb()->Tib.StackBase );
}

/***********************************************************************
 *           signal_exit_thread
 */
void signal_exit_thread( int status )
{
    exit_thread( status );
}

/***********************************************************************
 *           signal_exit_process
 */
void signal_exit_process( int status )
{
    exit( status );
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

#endif  /* __aarch64__ */
