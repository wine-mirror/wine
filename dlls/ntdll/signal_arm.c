/*
 * ARM signal handling routines
 *
 * Copyright 2002 Marcus Meissner, SuSE Linux AG
 * Copyright 2010, 2011 André Hentschel
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

#ifdef __arm__

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

static pthread_key_t teb_key;

/***********************************************************************
 * signal context platform-specific definitions
 */
#ifdef linux

#if defined(__ANDROID__) && !defined(HAVE_SYS_UCONTEXT_H)
typedef struct ucontext
{
    unsigned long uc_flags;
    struct ucontext *uc_link;
    stack_t uc_stack;
    struct sigcontext uc_mcontext;
    sigset_t uc_sigmask;
    unsigned long uc_regspace[128] __attribute__((__aligned__(8)));
} ucontext_t;
#endif

/* All Registers access - only for local access */
# define REG_sig(reg_name, context) ((context)->uc_mcontext.reg_name)
# define REGn_sig(reg_num, context) ((context)->uc_mcontext.arm_r##reg_num)

/* Special Registers access  */
# define SP_sig(context)            REG_sig(arm_sp, context)    /* Stack pointer */
# define LR_sig(context)            REG_sig(arm_lr, context)    /* Link register */
# define PC_sig(context)            REG_sig(arm_pc, context)    /* Program counter */
# define CPSR_sig(context)          REG_sig(arm_cpsr, context)  /* Current State Register */
# define IP_sig(context)            REG_sig(arm_ip, context)    /* Intra-Procedure-call scratch register */
# define FP_sig(context)            REG_sig(arm_fp, context)    /* Frame pointer */

/* Exceptions */
# define ERROR_sig(context)         REG_sig(error_code, context)
# define FAULT_sig(context)         REG_sig(fault_address, context)
# define TRAP_sig(context)          REG_sig(trap_no, context)

#endif /* linux */

enum arm_trap_code
{
    TRAP_ARM_UNKNOWN    = -1,  /* Unknown fault (TRAP_sig not defined) */
    TRAP_ARM_PRIVINFLT  =  6,  /* Invalid opcode exception */
    TRAP_ARM_PAGEFLT    = 14,  /* Page fault */
    TRAP_ARM_ALIGNFLT   = 17,  /* Alignment check exception */
};

typedef void (WINAPI *raise_func)( EXCEPTION_RECORD *rec, CONTEXT *context );
typedef int (*wine_signal_handler)(unsigned int sig);

static wine_signal_handler handlers[256];


struct UNWIND_INFO
{
    WORD function_length;
    WORD unknown1 : 7;
    WORD count : 5;
    WORD unknown2 : 4;
};

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
#define C(x) context->R##x = REGn_sig(x,sigcontext)
    /* Save normal registers */
    C(0); C(1); C(2); C(3); C(4); C(5); C(6); C(7); C(8); C(9); C(10);
#undef C

    context->ContextFlags = CONTEXT_FULL;
    context->Sp   = SP_sig(sigcontext);   /* Stack pointer */
    context->Lr   = LR_sig(sigcontext);   /* Link register */
    context->Pc   = PC_sig(sigcontext);   /* Program Counter */
    context->Cpsr = CPSR_sig(sigcontext); /* Current State Register */
    context->Ip   = IP_sig(sigcontext);   /* Intra-Procedure-call scratch register */
    context->Fp   = FP_sig(sigcontext);   /* Frame pointer */
}


/***********************************************************************
 *           restore_context
 *
 * Build a sigcontext from the register values.
 */
static void restore_context( const CONTEXT *context, ucontext_t *sigcontext )
{
#define C(x)  REGn_sig(x,sigcontext) = context->R##x
    /* Restore normal registers */
    C(0); C(1); C(2); C(3); C(4); C(5); C(6); C(7); C(8); C(9); C(10);
#undef C

    SP_sig(sigcontext)   = context->Sp;   /* Stack pointer */
    LR_sig(sigcontext)   = context->Lr ;  /* Link register */
    PC_sig(sigcontext)   = context->Pc;   /* Program Counter */
    CPSR_sig(sigcontext) = context->Cpsr; /* Current State Register */
    IP_sig(sigcontext)   = context->Ip;   /* Intra-Procedure-call scratch register */
    FP_sig(sigcontext)   = context->Fp;   /* Frame pointer */
}


/***********************************************************************
 *           save_fpu
 *
 * Set the FPU context from a sigcontext.
 */
static inline void save_fpu( CONTEXT *context, const ucontext_t *sigcontext )
{
    FIXME("not implemented\n");
}


/***********************************************************************
 *           restore_fpu
 *
 * Restore the FPU context to a sigcontext.
 */
static inline void restore_fpu( CONTEXT *context, const ucontext_t *sigcontext )
{
    FIXME("not implemented\n");
}


/***********************************************************************
 *		RtlCaptureContext (NTDLL.@)
 */
/* FIXME: Use the Stack instead of the actual register values */
__ASM_STDCALL_FUNC( RtlCaptureContext, 4,
                    ".arm\n\t"
                    "stmfd SP!, {r1}\n\t"
                    "mov r1, #0x40\n\t"     /* CONTEXT_ARM */
                    "add r1, r1, #0x3\n\t"  /* CONTEXT_FULL */
                    "str r1, [r0]\n\t"      /* context->ContextFlags */
                    "ldmfd SP!, {r1}\n\t"
                    "str r0, [r0, #0x4]\n\t"   /* context->R0 */
                    "str r1, [r0, #0x8]\n\t"   /* context->R1 */
                    "str r2, [r0, #0xc]\n\t"   /* context->R2 */
                    "str r3, [r0, #0x10]\n\t"  /* context->R3 */
                    "str r4, [r0, #0x14]\n\t"  /* context->R4 */
                    "str r5, [r0, #0x18]\n\t"  /* context->R5 */
                    "str r6, [r0, #0x1c]\n\t"  /* context->R6 */
                    "str r7, [r0, #0x20]\n\t"  /* context->R7 */
                    "str r8, [r0, #0x24]\n\t"  /* context->R8 */
                    "str r9, [r0, #0x28]\n\t"  /* context->R9 */
                    "str r10, [r0, #0x2c]\n\t" /* context->R10 */
                    "str r11, [r0, #0x30]\n\t" /* context->Fp */
                    "str IP, [r0, #0x34]\n\t"  /* context->Ip */
                    "str SP, [r0, #0x38]\n\t"  /* context->Sp */
                    "str LR, [r0, #0x3c]\n\t"  /* context->Lr */
                    "str PC, [r0, #0x40]\n\t"  /* context->Pc */
                    "mrs r1, CPSR\n\t"
                    "str r1, [r0, #0x44]\n\t"  /* context->Cpsr */
                    "mov PC, LR\n"
                    )


/***********************************************************************
 *           set_cpu_context
 *
 * Set the new CPU context.
 */
/* FIXME: What about the CPSR? */
__ASM_GLOBAL_FUNC( set_cpu_context,
                   "mov IP, r0\n\t"
                   "ldr r0,  [IP, #0x4]\n\t"  /* context->R0 */
                   "ldr r1,  [IP, #0x8]\n\t"  /* context->R1 */
                   "ldr r2,  [IP, #0xc]\n\t"  /* context->R2 */
                   "ldr r3,  [IP, #0x10]\n\t" /* context->R3 */
                   "ldr r4,  [IP, #0x14]\n\t" /* context->R4 */
                   "ldr r5,  [IP, #0x18]\n\t" /* context->R5 */
                   "ldr r6,  [IP, #0x1c]\n\t" /* context->R6 */
                   "ldr r7,  [IP, #0x20]\n\t" /* context->R7 */
                   "ldr r8,  [IP, #0x24]\n\t" /* context->R8 */
                   "ldr r9,  [IP, #0x28]\n\t" /* context->R9 */
                   "ldr r10, [IP, #0x2c]\n\t" /* context->R10 */
                   "ldr r11, [IP, #0x30]\n\t" /* context->Fp */
                   "ldr SP,  [IP, #0x38]\n\t" /* context->Sp */
                   "ldr LR,  [IP, #0x3c]\n\t" /* context->Lr */
                   "ldr PC,  [IP, #0x40]\n\t" /* context->Pc */
                   )


/***********************************************************************
 *           copy_context
 *
 * Copy a register context according to the flags.
 */
void copy_context( CONTEXT *to, const CONTEXT *from, DWORD flags )
{
    flags &= ~CONTEXT_ARM;  /* get rid of CPU id */
    if (flags & CONTEXT_CONTROL)
    {
        to->Sp      = from->Sp;
        to->Lr      = from->Lr;
        to->Pc      = from->Pc;
        to->Cpsr    = from->Cpsr;
    }
    if (flags & CONTEXT_INTEGER)
    {
        to->R0  = from->R0;
        to->R1  = from->R1;
        to->R2  = from->R2;
        to->R3  = from->R3;
        to->R4  = from->R4;
        to->R5  = from->R5;
        to->R6  = from->R6;
        to->R7  = from->R7;
        to->R8  = from->R8;
        to->R9  = from->R9;
        to->R10 = from->R10;
        to->Ip  = from->Ip;
        to->Fp  = from->Fp;
    }
}


/***********************************************************************
 *           context_to_server
 *
 * Convert a register context to the server format.
 */
NTSTATUS context_to_server( context_t *to, const CONTEXT *from )
{
    DWORD flags = from->ContextFlags & ~CONTEXT_ARM;  /* get rid of CPU id */

    memset( to, 0, sizeof(*to) );
    to->cpu = CPU_ARM;

    if (flags & CONTEXT_CONTROL)
    {
        to->flags |= SERVER_CTX_CONTROL;
        to->ctl.arm_regs.sp   = from->Sp;
        to->ctl.arm_regs.lr   = from->Lr;
        to->ctl.arm_regs.pc   = from->Pc;
        to->ctl.arm_regs.cpsr = from->Cpsr;
    }
    if (flags & CONTEXT_INTEGER)
    {
        to->flags |= SERVER_CTX_INTEGER;
        to->integer.arm_regs.r[0]  = from->R0;
        to->integer.arm_regs.r[1]  = from->R1;
        to->integer.arm_regs.r[2]  = from->R2;
        to->integer.arm_regs.r[3]  = from->R3;
        to->integer.arm_regs.r[4]  = from->R4;
        to->integer.arm_regs.r[5]  = from->R5;
        to->integer.arm_regs.r[6]  = from->R6;
        to->integer.arm_regs.r[7]  = from->R7;
        to->integer.arm_regs.r[8]  = from->R8;
        to->integer.arm_regs.r[9]  = from->R9;
        to->integer.arm_regs.r[10] = from->R10;
        to->integer.arm_regs.r[11] = from->Fp;
        to->integer.arm_regs.r[12] = from->Ip;
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
    if (from->cpu != CPU_ARM) return STATUS_INVALID_PARAMETER;

    to->ContextFlags = CONTEXT_ARM;
    if (from->flags & SERVER_CTX_CONTROL)
    {
        to->ContextFlags |= CONTEXT_CONTROL;
        to->Sp   = from->ctl.arm_regs.sp;
        to->Lr   = from->ctl.arm_regs.lr;
        to->Pc   = from->ctl.arm_regs.pc;
        to->Cpsr = from->ctl.arm_regs.cpsr;
    }
    if (from->flags & SERVER_CTX_INTEGER)
    {
        to->ContextFlags |= CONTEXT_INTEGER;
        to->R0  = from->integer.arm_regs.r[0];
        to->R1  = from->integer.arm_regs.r[1];
        to->R2  = from->integer.arm_regs.r[2];
        to->R3  = from->integer.arm_regs.r[3];
        to->R4  = from->integer.arm_regs.r[4];
        to->R5  = from->integer.arm_regs.r[5];
        to->R6  = from->integer.arm_regs.r[6];
        to->R7  = from->integer.arm_regs.r[7];
        to->R8  = from->integer.arm_regs.r[8];
        to->R9  = from->integer.arm_regs.r[9];
        to->R10 = from->integer.arm_regs.r[10];
        to->Fp  = from->integer.arm_regs.r[11];
        to->Ip  = from->integer.arm_regs.r[12];
     }
    return STATUS_SUCCESS;
}

extern void raise_func_trampoline_thumb( EXCEPTION_RECORD *rec, CONTEXT *context, raise_func func );
__ASM_GLOBAL_FUNC( raise_func_trampoline_thumb,
                   ".thumb\n\t"
                   "blx r2\n\t"
                   "bkpt")

extern void raise_func_trampoline_arm( EXCEPTION_RECORD *rec, CONTEXT *context, raise_func func );
__ASM_GLOBAL_FUNC( raise_func_trampoline_arm,
                   ".arm\n\t"
                   "blx r2\n\t"
                   "bkpt")

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

    stack = (struct stack_layout *)(SP_sig(sigcontext) & ~3);
    stack--;  /* push the stack_layout structure */

    stack->rec.ExceptionRecord  = NULL;
    stack->rec.ExceptionCode    = exception_code;
    stack->rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    stack->rec.ExceptionAddress = (LPVOID)PC_sig(sigcontext);
    stack->rec.NumberParameters = 0;

    save_context( &stack->context, sigcontext );

    /* now modify the sigcontext to return to the raise function */
    SP_sig(sigcontext) = (DWORD)stack;
    if (CPSR_sig(sigcontext) & 0x20)
        PC_sig(sigcontext) = (DWORD)raise_func_trampoline_thumb;
    else
        PC_sig(sigcontext) = (DWORD)raise_func_trampoline_arm;
    REGn_sig(0, sigcontext) = (DWORD)&stack->rec;  /* first arg for raise_func */
    REGn_sig(1, sigcontext) = (DWORD)&stack->context; /* second arg for raise_func */
    REGn_sig(2, sigcontext) = (DWORD)func; /* the raise_func as third arg for the trampoline */


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
                                                             rec->ExceptionInformation[0] )))
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

        TRACE( "code=%x flags=%x addr=%p pc=%08x tid=%04x\n",
               rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
               context->Pc, GetCurrentThreadId() );
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
            TRACE( " r0=%08x r1=%08x r2=%08x r3=%08x r4=%08x r5=%08x\n",
                   context->R0, context->R1, context->R2, context->R3, context->R4, context->R5 );
            TRACE( " r6=%08x r7=%08x r8=%08x r9=%08x r10=%08x fp=%08x\n",
                   context->R6, context->R7, context->R8, context->R9, context->R10, context->Fp );
            TRACE( " ip=%08x sp=%08x lr=%08x pc=%08x cpsr=%08x\n",
                   context->Ip, context->Sp, context->Lr, context->Pc, context->Cpsr );
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
static void segv_handler( int signal, siginfo_t *info, void *ucontext )
{
    EXCEPTION_RECORD *rec;
    ucontext_t *context = ucontext;

    /* check for page fault inside the thread stack */
    if (TRAP_sig(context) == TRAP_ARM_PAGEFLT &&
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

    switch(TRAP_sig(context))
    {
    case TRAP_ARM_PRIVINFLT:   /* Invalid opcode exception */
        rec->ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
        break;
    case TRAP_ARM_PAGEFLT:  /* Page fault */
        rec->ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
        rec->NumberParameters = 2;
        rec->ExceptionInformation[0] = (ERROR_sig(context) & 0x800) != 0;
        rec->ExceptionInformation[1] = (ULONG_PTR)info->si_addr;
        break;
    case TRAP_ARM_ALIGNFLT:  /* Alignment check exception */
        rec->ExceptionCode = EXCEPTION_DATATYPE_MISALIGNMENT;
        break;
    default:
        ERR("Got unexpected trap %ld\n", TRAP_sig(context));
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
    if (sig > sizeof(handlers) / sizeof(handlers[0])) return -1;
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
        size_t min_size = page_size;
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
 *             signal_free_thread
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
 *      set_tpidrurw
 *
 * Win32/ARM applications expect the TEB pointer to be in the TPIDRURW register.
 */
#ifdef __ARM_ARCH_7A__
extern void set_tpidrurw( TEB *teb );
__ASM_GLOBAL_FUNC( set_tpidrurw,
                   "mcr p15, 0, r0, c13, c0, 2\n\t" /* TEB -> TPIDRURW */
                   "blx lr" )
#else
void set_tpidrurw( TEB *teb )
{
}
#endif

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
    set_tpidrurw( teb );
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


/**********************************************************************
 *              __wine_enter_vm86   (NTDLL.@)
 */
void __wine_enter_vm86( CONTEXT *context )
{
    MESSAGE("vm86 mode not supported on this platform\n");
}


/**********************************************************************
 *              RtlAddFunctionTable   (NTDLL.@)
 */
BOOLEAN CDECL RtlAddFunctionTable( RUNTIME_FUNCTION *table, DWORD count, DWORD addr )
{
    FIXME( "%p %u %x: stub\n", table, count, addr );
    return TRUE;
}


/**********************************************************************
 *              RtlDeleteFunctionTable   (NTDLL.@)
 */
BOOLEAN CDECL RtlDeleteFunctionTable( RUNTIME_FUNCTION *table )
{
    FIXME( "%p: stub\n", table );
    return TRUE;
}

/**********************************************************************
 *              find_function_info
 */
static RUNTIME_FUNCTION *find_function_info( ULONG_PTR pc, HMODULE module,
                                             RUNTIME_FUNCTION *func, ULONG size )
{
    int min = 0;
    int max = size/sizeof(*func) - 1;

    while (min <= max)
    {
        int pos = (min + max) / 2;
        DWORD begin = (func[pos].BeginAddress & ~1), end;
        if (func[pos].u.s.Flag)
            end = begin + func[pos].u.s.FunctionLength * 2;
        else
        {
            struct UNWIND_INFO *info;
            info = (struct UNWIND_INFO *)((char *)module + func[pos].u.UnwindData);
            end = begin + info->function_length * 2;
        }

        if ((char *)pc < (char *)module + begin) max = pos - 1;
        else if ((char *)pc >= (char *)module + end) min = pos + 1;
        else return func + pos;
    }
    return NULL;
}

/**********************************************************************
 *              RtlLookupFunctionEntry   (NTDLL.@)
 */
PRUNTIME_FUNCTION WINAPI RtlLookupFunctionEntry( ULONG_PTR pc, DWORD *base,
                                                 UNWIND_HISTORY_TABLE *table )
{
    LDR_MODULE *module;
    RUNTIME_FUNCTION *func;
    ULONG size;

    /* FIXME: should use the history table to make things faster */

    if (LdrFindEntryForAddress( (void *)pc, &module ))
    {
        WARN( "module not found for %lx\n", pc );
        return NULL;
    }
    if (!(func = RtlImageDirectoryEntryToData( module->BaseAddress, TRUE,
                                               IMAGE_DIRECTORY_ENTRY_EXCEPTION, &size )))
    {
        WARN( "no exception table found in module %p pc %lx\n", module->BaseAddress, pc );
        return NULL;
    }
    func = find_function_info( pc, module->BaseAddress, func, size );
    if (func) *base = (DWORD)module->BaseAddress;
    return func;
}

/***********************************************************************
 *            RtlUnwind  (NTDLL.@)
 */
void WINAPI RtlUnwind( void *endframe, void *target_ip, EXCEPTION_RECORD *rec, void *retval )
{
    CONTEXT context;
    EXCEPTION_RECORD record;
    EXCEPTION_REGISTRATION_RECORD *frame, *dispatch;
    DWORD res;

    RtlCaptureContext( &context );
    context.R0 = (DWORD)retval;

    /* build an exception record, if we do not have one */
    if (!rec)
    {
        record.ExceptionCode    = STATUS_UNWIND;
        record.ExceptionFlags   = 0;
        record.ExceptionRecord  = NULL;
        record.ExceptionAddress = (void *)context.Pc;
        record.NumberParameters = 0;
        rec = &record;
    }

    rec->ExceptionFlags |= EH_UNWINDING | (endframe ? 0 : EH_EXIT_UNWIND);

    TRACE( "code=%x flags=%x\n", rec->ExceptionCode, rec->ExceptionFlags );

    /* get chain of exception frames */
    frame = NtCurrentTeb()->Tib.ExceptionList;
    while ((frame != (EXCEPTION_REGISTRATION_RECORD*)~0UL) && (frame != endframe))
    {
        /* Check frame address */
        if (endframe && ((void*)frame > endframe))
            raise_status( STATUS_INVALID_UNWIND_TARGET, rec );

        if (!is_valid_frame( frame )) raise_status( STATUS_BAD_STACK, rec );

        /* Call handler */
        TRACE( "calling handler at %p code=%x flags=%x\n",
               frame->Handler, rec->ExceptionCode, rec->ExceptionFlags );
        res = frame->Handler(rec, frame, &context, &dispatch);
        TRACE( "handler at %p returned %x\n", frame->Handler, res );

        switch(res)
        {
        case ExceptionContinueSearch:
            break;
        case ExceptionCollidedUnwind:
            frame = dispatch;
            break;
        default:
            raise_status( STATUS_INVALID_DISPOSITION, rec );
            break;
        }
        frame = __wine_pop_frame( frame );
    }
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

#endif  /* __arm__ */
