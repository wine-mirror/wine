/*
 * i386 signal handling routines
 *
 * Copyright 1999 Alexandre Julliard
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

#ifdef __i386__

#include "config.h"
#include "wine/port.h"

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
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
#ifdef HAVE_SYS_SYSCTL_H
# include <sys/sysctl.h>
#endif

#include "windef.h"
#include "thread.h"
#include "wine/library.h"
#include "ntdll_misc.h"
#include "wine/exception.h"
#include "wine/debug.h"

#ifdef HAVE_VALGRIND_MEMCHECK_H
#include <valgrind/memcheck.h>
#endif

#undef ERR  /* Solaris needs to define this */

/***********************************************************************
 * signal context platform-specific definitions
 */

#ifdef linux

typedef ucontext_t SIGCONTEXT;

#define EAX_sig(context)     ((context)->uc_mcontext.gregs[REG_EAX])
#define EBX_sig(context)     ((context)->uc_mcontext.gregs[REG_EBX])
#define ECX_sig(context)     ((context)->uc_mcontext.gregs[REG_ECX])
#define EDX_sig(context)     ((context)->uc_mcontext.gregs[REG_EDX])
#define ESI_sig(context)     ((context)->uc_mcontext.gregs[REG_ESI])
#define EDI_sig(context)     ((context)->uc_mcontext.gregs[REG_EDI])
#define EBP_sig(context)     ((context)->uc_mcontext.gregs[REG_EBP])
#define ESP_sig(context)     ((context)->uc_mcontext.gregs[REG_ESP])

#define CS_sig(context)      ((context)->uc_mcontext.gregs[REG_CS])
#define DS_sig(context)      ((context)->uc_mcontext.gregs[REG_DS])
#define ES_sig(context)      ((context)->uc_mcontext.gregs[REG_ES])
#define SS_sig(context)      ((context)->uc_mcontext.gregs[REG_SS])
#define FS_sig(context)      ((context)->uc_mcontext.gregs[REG_FS])
#define GS_sig(context)      ((context)->uc_mcontext.gregs[REG_GS])

#define EFL_sig(context)     ((context)->uc_mcontext.gregs[REG_EFL])
#define EIP_sig(context)     ((context)->uc_mcontext.gregs[REG_EIP])
#define TRAP_sig(context)    ((context)->uc_mcontext.gregs[REG_TRAPNO])
#define ERROR_sig(context)   ((context)->uc_mcontext.gregs[REG_ERR])

#define FPU_sig(context)     ((FLOATING_SAVE_AREA*)((context)->uc_mcontext.fpregs))

#define VM86_EAX 0 /* the %eax value while vm86_enter is executing */

int vm86_enter( void **vm86_ptr );
void vm86_return(void);
void vm86_return_end(void);
__ASM_GLOBAL_FUNC(vm86_enter,
                  "pushl %ebp\n\t"
                  "movl %esp, %ebp\n\t"
                  "movl $166,%eax\n\t"  /*SYS_vm86*/
                  "movl 8(%ebp),%ecx\n\t" /* vm86_ptr */
                  "movl (%ecx),%ecx\n\t"
                  "pushl %ebx\n\t"
                  "movl $1,%ebx\n\t"    /*VM86_ENTER*/
                  "pushl %ecx\n\t"      /* put vm86plus_struct ptr somewhere we can find it */
                  "pushl %fs\n\t"
                  "pushl %gs\n\t"
                  "int $0x80\n"
                  ".globl " __ASM_NAME("vm86_return") "\n\t"
                  __ASM_FUNC("vm86_return") "\n"
                  __ASM_NAME("vm86_return") ":\n\t"
                  "popl %gs\n\t"
                  "popl %fs\n\t"
                  "popl %ecx\n\t"
                  "popl %ebx\n\t"
                  "popl %ebp\n\t"
                  "testl %eax,%eax\n\t"
                  "jl 0f\n\t"
                  "cmpb $0,%al\n\t" /* VM86_SIGNAL */
                  "je " __ASM_NAME("vm86_enter") "\n\t"
                  "0:\n\t"
                  "movl 4(%esp),%ecx\n\t"  /* vm86_ptr */
                  "movl $0,(%ecx)\n\t"
                  ".globl " __ASM_NAME("vm86_return_end") "\n\t"
                  __ASM_FUNC("vm86_return_end") "\n"
                  __ASM_NAME("vm86_return_end") ":\n\t"
                  "ret" )

#ifdef HAVE_SYS_VM86_H
# define __HAVE_VM86
#endif

#endif  /* linux */

#ifdef BSDI

#include <machine/frame.h>
typedef struct trapframe SIGCONTEXT;

#define EAX_sig(context)     ((context)->tf_eax)
#define EBX_sig(context)     ((context)->tf_ebx)
#define ECX_sig(context)     ((context)->tf_ecx)
#define EDX_sig(context)     ((context)->tf_edx)
#define ESI_sig(context)     ((context)->tf_esi)
#define EDI_sig(context)     ((context)->tf_edi)
#define EBP_sig(context)     ((context)->tf_ebp)

#define CS_sig(context)      ((context)->tf_cs)
#define DS_sig(context)      ((context)->tf_ds)
#define ES_sig(context)      ((context)->tf_es)
#define SS_sig(context)      ((context)->tf_ss)

#define EFL_sig(context)     ((context)->tf_eflags)

#define EIP_sig(context)     (*((unsigned long*)&(context)->tf_eip))
#define ESP_sig(context)     (*((unsigned long*)&(context)->tf_esp))

#endif /* bsdi */

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__)

typedef struct sigcontext SIGCONTEXT;

#define EAX_sig(context)     ((context)->sc_eax)
#define EBX_sig(context)     ((context)->sc_ebx)
#define ECX_sig(context)     ((context)->sc_ecx)
#define EDX_sig(context)     ((context)->sc_edx)
#define ESI_sig(context)     ((context)->sc_esi)
#define EDI_sig(context)     ((context)->sc_edi)
#define EBP_sig(context)     ((context)->sc_ebp)

#define CS_sig(context)      ((context)->sc_cs)
#define DS_sig(context)      ((context)->sc_ds)
#define ES_sig(context)      ((context)->sc_es)
#define FS_sig(context)      ((context)->sc_fs)
#define GS_sig(context)      ((context)->sc_gs)
#define SS_sig(context)      ((context)->sc_ss)

#define TRAP_sig(context)    ((context)->sc_trapno)
#define ERROR_sig(context)   ((context)->sc_err)
#define EFL_sig(context)     ((context)->sc_eflags)

#define EIP_sig(context)     ((context)->sc_eip)
#define ESP_sig(context)     ((context)->sc_esp)

#endif  /* *BSD */

#if defined(__svr4__) || defined(_SCO_DS) || defined(__sun)

#ifdef _SCO_DS
#include <sys/regset.h>
#endif
#include <sys/ucontext.h>
typedef struct ucontext SIGCONTEXT;

#ifdef _SCO_DS
#define gregs regs
#endif

#define EAX_sig(context)     ((context)->uc_mcontext.gregs[EAX])
#define EBX_sig(context)     ((context)->uc_mcontext.gregs[EBX])
#define ECX_sig(context)     ((context)->uc_mcontext.gregs[ECX])
#define EDX_sig(context)     ((context)->uc_mcontext.gregs[EDX])
#define ESI_sig(context)     ((context)->uc_mcontext.gregs[ESI])
#define EDI_sig(context)     ((context)->uc_mcontext.gregs[EDI])
#define EBP_sig(context)     ((context)->uc_mcontext.gregs[EBP])

#define CS_sig(context)      ((context)->uc_mcontext.gregs[CS])
#define DS_sig(context)      ((context)->uc_mcontext.gregs[DS])
#define ES_sig(context)      ((context)->uc_mcontext.gregs[ES])
#define SS_sig(context)      ((context)->uc_mcontext.gregs[SS])

#define FS_sig(context)      ((context)->uc_mcontext.gregs[FS])
#define GS_sig(context)      ((context)->uc_mcontext.gregs[GS])

#define EFL_sig(context)     ((context)->uc_mcontext.gregs[EFL])

#define EIP_sig(context)     ((context)->uc_mcontext.gregs[EIP])
#ifdef UESP
#define ESP_sig(context)     ((context)->uc_mcontext.gregs[UESP])
#elif defined(R_ESP)
#define ESP_sig(context)     ((context)->uc_mcontext.gregs[R_ESP])
#else
#define ESP_sig(context)     ((context)->uc_mcontext.gregs[ESP])
#endif
#ifdef ERR
#define ERROR_sig(context)   ((context)->uc_mcontext.gregs[ERR])
#endif
#ifdef TRAPNO
#define TRAP_sig(context)     ((context)->uc_mcontext.gregs[TRAPNO])
#endif

#endif  /* svr4 || SCO_DS */

#ifdef __APPLE__
# include <sys/ucontext.h>

typedef ucontext_t SIGCONTEXT;

/* work around silly renaming of struct members in OS X 10.5 */
#if __DARWIN_UNIX03 && defined(_STRUCT_X86_EXCEPTION_STATE32)
#define EAX_sig(context)     ((context)->uc_mcontext->__ss.__eax)
#define EBX_sig(context)     ((context)->uc_mcontext->__ss.__ebx)
#define ECX_sig(context)     ((context)->uc_mcontext->__ss.__ecx)
#define EDX_sig(context)     ((context)->uc_mcontext->__ss.__edx)
#define ESI_sig(context)     ((context)->uc_mcontext->__ss.__esi)
#define EDI_sig(context)     ((context)->uc_mcontext->__ss.__edi)
#define EBP_sig(context)     ((context)->uc_mcontext->__ss.__ebp)
#define CS_sig(context)      ((context)->uc_mcontext->__ss.__cs)
#define DS_sig(context)      ((context)->uc_mcontext->__ss.__ds)
#define ES_sig(context)      ((context)->uc_mcontext->__ss.__es)
#define FS_sig(context)      ((context)->uc_mcontext->__ss.__fs)
#define GS_sig(context)      ((context)->uc_mcontext->__ss.__gs)
#define SS_sig(context)      ((context)->uc_mcontext->__ss.__ss)
#define EFL_sig(context)     ((context)->uc_mcontext->__ss.__eflags)
#define EIP_sig(context)     (*((unsigned long*)&(context)->uc_mcontext->__ss.__eip))
#define ESP_sig(context)     (*((unsigned long*)&(context)->uc_mcontext->__ss.__esp))
#define TRAP_sig(context)    ((context)->uc_mcontext->__es.__trapno)
#define ERROR_sig(context)   ((context)->uc_mcontext->__es.__err)
#else
#define EAX_sig(context)     ((context)->uc_mcontext->ss.eax)
#define EBX_sig(context)     ((context)->uc_mcontext->ss.ebx)
#define ECX_sig(context)     ((context)->uc_mcontext->ss.ecx)
#define EDX_sig(context)     ((context)->uc_mcontext->ss.edx)
#define ESI_sig(context)     ((context)->uc_mcontext->ss.esi)
#define EDI_sig(context)     ((context)->uc_mcontext->ss.edi)
#define EBP_sig(context)     ((context)->uc_mcontext->ss.ebp)
#define CS_sig(context)      ((context)->uc_mcontext->ss.cs)
#define DS_sig(context)      ((context)->uc_mcontext->ss.ds)
#define ES_sig(context)      ((context)->uc_mcontext->ss.es)
#define FS_sig(context)      ((context)->uc_mcontext->ss.fs)
#define GS_sig(context)      ((context)->uc_mcontext->ss.gs)
#define SS_sig(context)      ((context)->uc_mcontext->ss.ss)
#define EFL_sig(context)     ((context)->uc_mcontext->ss.eflags)
#define EIP_sig(context)     (*((unsigned long*)&(context)->uc_mcontext->ss.eip))
#define ESP_sig(context)     (*((unsigned long*)&(context)->uc_mcontext->ss.esp))
#define TRAP_sig(context)    ((context)->uc_mcontext->es.trapno)
#define ERROR_sig(context)   ((context)->uc_mcontext->es.err)
#endif

#endif /* __APPLE__ */

WINE_DEFAULT_DEBUG_CHANNEL(seh);

typedef int (*wine_signal_handler)(unsigned int sig);

static size_t signal_stack_mask;
static size_t signal_stack_size;

static wine_signal_handler handlers[256];

extern void DECLSPEC_NORETURN __wine_call_from_32_restore_regs( const CONTEXT *context );

enum i386_trap_code
{
    TRAP_x86_UNKNOWN    = -1,  /* Unknown fault (TRAP_sig not defined) */
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    TRAP_x86_DIVIDE     = T_DIVIDE,     /* Division by zero exception */
    TRAP_x86_TRCTRAP    = T_TRCTRAP,    /* Single-step exception */
    TRAP_x86_NMI        = T_NMI,        /* NMI interrupt */
    TRAP_x86_BPTFLT     = T_BPTFLT,     /* Breakpoint exception */
    TRAP_x86_OFLOW      = T_OFLOW,      /* Overflow exception */
    TRAP_x86_BOUND      = T_BOUND,      /* Bound range exception */
    TRAP_x86_PRIVINFLT  = T_PRIVINFLT,  /* Invalid opcode exception */
    TRAP_x86_DNA        = T_DNA,        /* Device not available exception */
    TRAP_x86_DOUBLEFLT  = T_DOUBLEFLT,  /* Double fault exception */
    TRAP_x86_FPOPFLT    = T_FPOPFLT,    /* Coprocessor segment overrun */
    TRAP_x86_TSSFLT     = T_TSSFLT,     /* Invalid TSS exception */
    TRAP_x86_SEGNPFLT   = T_SEGNPFLT,   /* Segment not present exception */
    TRAP_x86_STKFLT     = T_STKFLT,     /* Stack fault */
    TRAP_x86_PROTFLT    = T_PROTFLT,    /* General protection fault */
    TRAP_x86_PAGEFLT    = T_PAGEFLT,    /* Page fault */
    TRAP_x86_ARITHTRAP  = T_ARITHTRAP,  /* Floating point exception */
    TRAP_x86_ALIGNFLT   = T_ALIGNFLT,   /* Alignment check exception */
    TRAP_x86_MCHK       = T_MCHK,       /* Machine check exception */
    TRAP_x86_CACHEFLT   = T_XMMFLT      /* Cache flush exception */
#else
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
    TRAP_x86_CACHEFLT   = 19   /* SIMD exception (via SIGFPE) if CPU is SSE capable
                                  otherwise Cache flush exception (via SIGSEV) */
#endif
};


/***********************************************************************
 *           dispatch_signal
 */
static inline int dispatch_signal(unsigned int sig)
{
    if (handlers[sig] == NULL) return 0;
    return handlers[sig](sig);
}


/***********************************************************************
 *           get_trap_code
 *
 * Get the trap code for a signal.
 */
static inline enum i386_trap_code get_trap_code( const SIGCONTEXT *sigcontext )
{
#ifdef TRAP_sig
    return TRAP_sig(sigcontext);
#else
    return TRAP_x86_UNKNOWN;  /* unknown trap code */
#endif
}

/***********************************************************************
 *           get_error_code
 *
 * Get the error code for a signal.
 */
static inline WORD get_error_code( const SIGCONTEXT *sigcontext )
{
#ifdef ERROR_sig
    return ERROR_sig(sigcontext);
#else
    return 0;
#endif
}

/***********************************************************************
 *           get_signal_stack
 *
 * Get the base of the signal stack for the current thread.
 */
static inline void *get_signal_stack(void)
{
    return (char *)NtCurrentTeb() + 4096;
}


/***********************************************************************
 *           get_current_teb
 *
 * Get the current teb based on the stack pointer.
 */
static inline TEB *get_current_teb(void)
{
    unsigned long esp;
    __asm__("movl %%esp,%0" : "=g" (esp) );
    return (TEB *)(esp & ~signal_stack_mask);
}


#ifdef __HAVE_VM86
/***********************************************************************
 *           save_vm86_context
 *
 * Set the register values from a vm86 structure.
 */
static void save_vm86_context( CONTEXT *context, const struct vm86plus_struct *vm86 )
{
    context->ContextFlags = CONTEXT_FULL;
    context->Eax    = vm86->regs.eax;
    context->Ebx    = vm86->regs.ebx;
    context->Ecx    = vm86->regs.ecx;
    context->Edx    = vm86->regs.edx;
    context->Esi    = vm86->regs.esi;
    context->Edi    = vm86->regs.edi;
    context->Esp    = vm86->regs.esp;
    context->Ebp    = vm86->regs.ebp;
    context->Eip    = vm86->regs.eip;
    context->SegCs  = vm86->regs.cs;
    context->SegDs  = vm86->regs.ds;
    context->SegEs  = vm86->regs.es;
    context->SegFs  = vm86->regs.fs;
    context->SegGs  = vm86->regs.gs;
    context->SegSs  = vm86->regs.ss;
    context->EFlags = vm86->regs.eflags;
}


/***********************************************************************
 *           restore_vm86_context
 *
 * Build a vm86 structure from the register values.
 */
static void restore_vm86_context( const CONTEXT *context, struct vm86plus_struct *vm86 )
{
    vm86->regs.eax    = context->Eax;
    vm86->regs.ebx    = context->Ebx;
    vm86->regs.ecx    = context->Ecx;
    vm86->regs.edx    = context->Edx;
    vm86->regs.esi    = context->Esi;
    vm86->regs.edi    = context->Edi;
    vm86->regs.esp    = context->Esp;
    vm86->regs.ebp    = context->Ebp;
    vm86->regs.eip    = context->Eip;
    vm86->regs.cs     = context->SegCs;
    vm86->regs.ds     = context->SegDs;
    vm86->regs.es     = context->SegEs;
    vm86->regs.fs     = context->SegFs;
    vm86->regs.gs     = context->SegGs;
    vm86->regs.ss     = context->SegSs;
    vm86->regs.eflags = context->EFlags;
}


/**********************************************************************
 *		merge_vm86_pending_flags
 *
 * Merges TEB.vm86_ptr and TEB.vm86_pending VIP flags and
 * raises exception if there are pending events and VIF flag
 * has been turned on.
 *
 * Called from __wine_enter_vm86 because vm86_enter
 * doesn't check for pending events. 
 *
 * Called from raise_vm86_sti_exception to check for
 * pending events in a signal safe way.
 */
static void merge_vm86_pending_flags( EXCEPTION_RECORD *rec )
{
    BOOL check_pending = TRUE;
    struct vm86plus_struct *vm86 =
        (struct vm86plus_struct*)(ntdll_get_thread_data()->vm86_ptr);

    /*
     * In order to prevent a race when SIGUSR2 occurs while
     * we are returning from exception handler, pending events
     * will be rechecked after each raised exception.
     */
    while (check_pending && NtCurrentTeb()->vm86_pending)
    {
        check_pending = FALSE;
        ntdll_get_thread_data()->vm86_ptr = NULL;
            
        /*
         * If VIF is set, throw exception.
         * Note that SIGUSR2 may turn VIF flag off so
         * VIF check must occur only when TEB.vm86_ptr is NULL.
         */
        if (vm86->regs.eflags & VIF_MASK)
        {
            CONTEXT vcontext;
            save_vm86_context( &vcontext, vm86 );
            
            rec->ExceptionCode    = EXCEPTION_VM86_STI;
            rec->ExceptionFlags   = EXCEPTION_CONTINUABLE;
            rec->ExceptionRecord  = NULL;
            rec->NumberParameters = 0;
            rec->ExceptionAddress = (LPVOID)vcontext.Eip;

            vcontext.EFlags &= ~VIP_MASK;
            NtCurrentTeb()->vm86_pending = 0;
            __regs_RtlRaiseException( rec, &vcontext );

            restore_vm86_context( &vcontext, vm86 );
            check_pending = TRUE;
        }

        ntdll_get_thread_data()->vm86_ptr = vm86;
    }

    /*
     * Merge VIP flags in a signal safe way. This requires
     * that the following operation compiles into atomic
     * instruction.
     */
    vm86->regs.eflags |= NtCurrentTeb()->vm86_pending;
}
#endif /* __HAVE_VM86 */


typedef void (WINAPI *raise_func)( EXCEPTION_RECORD *rec, CONTEXT *context );


/***********************************************************************
 *           init_handler
 *
 * Handler initialization when the full context is not needed.
 */
static inline void *init_handler( const SIGCONTEXT *sigcontext, WORD *fs, WORD *gs )
{
    void *stack = (void *)(ESP_sig(sigcontext) & ~3);
    TEB *teb = get_current_teb();
    struct ntdll_thread_data *thread_data = (struct ntdll_thread_data *)teb->SystemReserved2;

    /* get %fs and %gs at time of the fault */
#ifdef FS_sig
    *fs = LOWORD(FS_sig(sigcontext));
#else
    *fs = wine_get_fs();
#endif
#ifdef GS_sig
    *gs = LOWORD(GS_sig(sigcontext));
#else
    *gs = wine_get_gs();
#endif

    wine_set_fs( thread_data->fs );

    /* now restore a proper %gs for the fault handler */
    if (!wine_ldt_is_system(CS_sig(sigcontext)) ||
        !wine_ldt_is_system(SS_sig(sigcontext)))  /* 16-bit mode */
    {
        /*
         * Win16 or DOS protected mode. Note that during switch
         * from 16-bit mode to linear mode, CS may be set to system
         * segment before FS is restored. Fortunately, in this case
         * SS is still non-system segment. This is why both CS and SS
         * are checked.
         */
        wine_set_gs( thread_data->gs );
        stack = teb->WOW32Reserved;
    }
#ifdef __HAVE_VM86
    else if ((void *)EIP_sig(sigcontext) == vm86_return)  /* vm86 mode */
    {
        unsigned int *int_stack = stack;
        /* fetch the saved %gs from the stack */
        wine_set_gs( int_stack[0] );
    }
#endif
    else  /* 32-bit mode */
    {
#ifdef GS_sig
        wine_set_gs( GS_sig(sigcontext) );
#endif
    }
    return stack;
}


/***********************************************************************
 *           save_fpu
 *
 * Save the thread FPU context.
 */
static inline void save_fpu( CONTEXT *context )
{
#ifdef __GNUC__
    context->ContextFlags |= CONTEXT_FLOATING_POINT;
    __asm__ __volatile__( "fnsave %0; fwait" : "=m" (context->FloatSave) );
#endif
}


/***********************************************************************
 *           restore_fpu
 *
 * Restore the FPU context to a sigcontext.
 */
static inline void restore_fpu( const CONTEXT *context )
{
    FLOATING_SAVE_AREA float_status = context->FloatSave;
    /* reset the current interrupt status */
    float_status.StatusWord &= float_status.ControlWord | 0xffffff80;
#ifdef __GNUC__
    __asm__ __volatile__( "frstor %0; fwait" : : "m" (float_status) );
#endif  /* __GNUC__ */
}


/***********************************************************************
 *           save_context
 *
 * Build a context structure from the signal info.
 */
static inline void save_context( CONTEXT *context, const SIGCONTEXT *sigcontext, WORD fs, WORD gs )
{
    struct ntdll_thread_regs * const regs = ntdll_get_thread_regs();

    memset(context, 0, sizeof(*context));
    context->ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
    context->Eax          = EAX_sig(sigcontext);
    context->Ebx          = EBX_sig(sigcontext);
    context->Ecx          = ECX_sig(sigcontext);
    context->Edx          = EDX_sig(sigcontext);
    context->Esi          = ESI_sig(sigcontext);
    context->Edi          = EDI_sig(sigcontext);
    context->Ebp          = EBP_sig(sigcontext);
    context->EFlags       = EFL_sig(sigcontext);
    context->Eip          = EIP_sig(sigcontext);
    context->Esp          = ESP_sig(sigcontext);
    context->SegCs        = LOWORD(CS_sig(sigcontext));
    context->SegDs        = LOWORD(DS_sig(sigcontext));
    context->SegEs        = LOWORD(ES_sig(sigcontext));
    context->SegFs        = fs;
    context->SegGs        = gs;
    context->SegSs        = LOWORD(SS_sig(sigcontext));
    context->Dr0          = regs->dr0;
    context->Dr1          = regs->dr1;
    context->Dr2          = regs->dr2;
    context->Dr3          = regs->dr3;
    context->Dr6          = regs->dr6;
    context->Dr7          = regs->dr7;

#ifdef FPU_sig
    if (FPU_sig(sigcontext))
    {
        context->ContextFlags |= CONTEXT_FLOATING_POINT;
        context->FloatSave = *FPU_sig(sigcontext);
    }
    else
#endif
    {
        save_fpu( context );
    }
}


/***********************************************************************
 *           restore_context
 *
 * Restore the signal info from the context.
 */
static inline void restore_context( const CONTEXT *context, SIGCONTEXT *sigcontext )
{
    struct ntdll_thread_regs * const regs = ntdll_get_thread_regs();

    regs->dr0 = context->Dr0;
    regs->dr1 = context->Dr1;
    regs->dr2 = context->Dr2;
    regs->dr3 = context->Dr3;
    regs->dr6 = context->Dr6;
    regs->dr7 = context->Dr7;
    EAX_sig(sigcontext) = context->Eax;
    EBX_sig(sigcontext) = context->Ebx;
    ECX_sig(sigcontext) = context->Ecx;
    EDX_sig(sigcontext) = context->Edx;
    ESI_sig(sigcontext) = context->Esi;
    EDI_sig(sigcontext) = context->Edi;
    EBP_sig(sigcontext) = context->Ebp;
    EFL_sig(sigcontext) = context->EFlags;
    EIP_sig(sigcontext) = context->Eip;
    ESP_sig(sigcontext) = context->Esp;
    CS_sig(sigcontext)  = context->SegCs;
    DS_sig(sigcontext)  = context->SegDs;
    ES_sig(sigcontext)  = context->SegEs;
    SS_sig(sigcontext)  = context->SegSs;
#ifdef GS_sig
    GS_sig(sigcontext)  = context->SegGs;
#else
    wine_set_gs( context->SegGs );
#endif
#ifdef FS_sig
    FS_sig(sigcontext)  = context->SegFs;
#else
    wine_set_fs( context->SegFs );
#endif

#ifdef FPU_sig
    if (FPU_sig(sigcontext))
    {
        *FPU_sig(sigcontext) = context->FloatSave;
    }
    else
#endif
    {
        restore_fpu( context );
    }
}


/***********************************************************************
 *              get_cpu_context
 *
 * Register function to get the context of the current thread.
 */
void WINAPI __regs_get_cpu_context( CONTEXT *context, CONTEXT *regs )
{
    *context = *regs;
    save_fpu( context );
}
DEFINE_REGS_ENTRYPOINT( get_cpu_context, 4, 4 )


/***********************************************************************
 *           set_cpu_context
 *
 * Set the new CPU context. Used by NtSetContextThread.
 */
void set_cpu_context( const CONTEXT *context )
{
    DWORD flags = context->ContextFlags & ~CONTEXT_i386;

    if (flags & CONTEXT_FLOATING_POINT) restore_fpu( context );

    if (flags & CONTEXT_DEBUG_REGISTERS)
    {
        struct ntdll_thread_regs * const regs = ntdll_get_thread_regs();
        regs->dr0 = context->Dr0;
        regs->dr1 = context->Dr1;
        regs->dr2 = context->Dr2;
        regs->dr3 = context->Dr3;
        regs->dr6 = context->Dr6;
        regs->dr7 = context->Dr7;
    }
    if (flags & CONTEXT_FULL)
    {
        if (!(flags & CONTEXT_CONTROL))
            FIXME( "setting partial context (%x) not supported\n", flags );
        else if (flags & CONTEXT_SEGMENTS)
            __wine_call_from_32_restore_regs( context );
        else
        {
            CONTEXT newcontext = *context;
            newcontext.SegDs = wine_get_ds();
            newcontext.SegEs = wine_get_es();
            newcontext.SegFs = wine_get_fs();
            newcontext.SegGs = wine_get_gs();
            __wine_call_from_32_restore_regs( &newcontext );
        }
    }
}


/***********************************************************************
 *           is_privileged_instr
 *
 * Check if the fault location is a privileged instruction.
 * Based on the instruction emulation code in dlls/kernel/instr.c.
 */
static inline DWORD is_privileged_instr( CONTEXT86 *context )
{
    const BYTE *instr;
    unsigned int prefix_count = 0;

    if (!wine_ldt_is_system( context->SegCs )) return 0;
    instr = (BYTE *)context->Eip;

    for (;;) switch(*instr)
    {
    /* instruction prefixes */
    case 0x2e:  /* %cs: */
    case 0x36:  /* %ss: */
    case 0x3e:  /* %ds: */
    case 0x26:  /* %es: */
    case 0x64:  /* %fs: */
    case 0x65:  /* %gs: */
    case 0x66:  /* opcode size */
    case 0x67:  /* addr size */
    case 0xf0:  /* lock */
    case 0xf2:  /* repne */
    case 0xf3:  /* repe */
        if (++prefix_count >= 15) return EXCEPTION_ILLEGAL_INSTRUCTION;
        instr++;
        continue;

    case 0x0f: /* extended instruction */
        switch(instr[1])
        {
        case 0x20: /* mov crX, reg */
        case 0x21: /* mov drX, reg */
        case 0x22: /* mov reg, crX */
        case 0x23: /* mov reg drX */
            return EXCEPTION_PRIV_INSTRUCTION;
        }
        return 0;
    case 0x6c: /* insb (%dx) */
    case 0x6d: /* insl (%dx) */
    case 0x6e: /* outsb (%dx) */
    case 0x6f: /* outsl (%dx) */
    case 0xcd: /* int $xx */
    case 0xe4: /* inb al,XX */
    case 0xe5: /* in (e)ax,XX */
    case 0xe6: /* outb XX,al */
    case 0xe7: /* out XX,(e)ax */
    case 0xec: /* inb (%dx),%al */
    case 0xed: /* inl (%dx),%eax */
    case 0xee: /* outb %al,(%dx) */
    case 0xef: /* outl %eax,(%dx) */
    case 0xf4: /* hlt */
    case 0xfa: /* cli */
    case 0xfb: /* sti */
        return EXCEPTION_PRIV_INSTRUCTION;
    default:
        return 0;
    }
}


#include "pshpack1.h"
struct atl_thunk
{
    DWORD movl;  /* movl this,4(%esp) */
    DWORD this;
    BYTE  jmp;   /* jmp func */
    int   func;
};
#include "poppack.h"

/**********************************************************************
 *		check_atl_thunk
 *
 * Check if code destination is an ATL thunk, and emulate it if so.
 */
static BOOL check_atl_thunk( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    const struct atl_thunk *thunk = (const struct atl_thunk *)rec->ExceptionInformation[1];
    BOOL ret = FALSE;

    __TRY
    {
        if (thunk->movl == 0x042444c7 && thunk->jmp == 0xe9)
        {
            *((DWORD *)context->Esp + 1) = thunk->this;
            context->Eip = (DWORD_PTR)(&thunk->func + 1) + thunk->func;
            TRACE( "emulating ATL thunk at %p, func=%08x arg=%08x\n",
                   thunk, context->Eip, *((DWORD *)context->Esp + 1) );
            ret = TRUE;
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        return FALSE;
    }
    __ENDTRY
    return ret;
}


/***********************************************************************
 *           setup_exception
 *
 * Setup a proper stack frame for the raise function, and modify the
 * sigcontext so that the return from the signal handler will call
 * the raise function.
 */
static EXCEPTION_RECORD *setup_exception( SIGCONTEXT *sigcontext, raise_func func )
{
    struct stack_layout
    {
        void             *ret_addr;      /* return address from raise_func */
        EXCEPTION_RECORD *rec_ptr;       /* first arg for raise_func */
        CONTEXT          *context_ptr;   /* second arg for raise_func */
        CONTEXT           context;
        EXCEPTION_RECORD  rec;
        DWORD             ebp;
        DWORD             eip;
    } *stack;

    WORD fs, gs;

    stack = init_handler( sigcontext, &fs, &gs );

    /* stack sanity checks */

    if ((char *)stack >= (char *)get_signal_stack() &&
        (char *)stack < (char *)get_signal_stack() + signal_stack_size)
    {
        WINE_ERR( "nested exception on signal stack in thread %04x eip %08x esp %08x stack %p-%p\n",
                  GetCurrentThreadId(), (unsigned int) EIP_sig(sigcontext),
                  (unsigned int) ESP_sig(sigcontext), NtCurrentTeb()->Tib.StackLimit,
                  NtCurrentTeb()->Tib.StackBase );
        server_abort_thread(1);
    }

    if (stack - 1 > stack || /* check for overflow in subtraction */
        (char *)(stack - 1) < (char *)NtCurrentTeb()->Tib.StackLimit ||
        (char *)stack > (char *)NtCurrentTeb()->Tib.StackBase)
    {
        UINT diff = (char *)NtCurrentTeb()->Tib.StackLimit - (char *)stack;
        if (diff < 4096)
        {
            WINE_ERR( "stack overflow %u bytes in thread %04x eip %08x esp %08x stack %p-%p\n",
                      diff, GetCurrentThreadId(), (unsigned int) EIP_sig(sigcontext),
                      (unsigned int) ESP_sig(sigcontext), NtCurrentTeb()->Tib.StackLimit,
                      NtCurrentTeb()->Tib.StackBase );
            server_abort_thread(1);
        }
        else WARN( "exception outside of stack limits in thread %04x eip %08x esp %08x stack %p-%p\n",
                   GetCurrentThreadId(), (unsigned int) EIP_sig(sigcontext),
                   (unsigned int) ESP_sig(sigcontext), NtCurrentTeb()->Tib.StackLimit,
                   NtCurrentTeb()->Tib.StackBase );
    }

    stack--;  /* push the stack_layout structure */
#ifdef HAVE_VALGRIND_MEMCHECK_H
    VALGRIND_MAKE_WRITABLE(stack, sizeof(*stack));
#endif
    stack->ret_addr     = (void *)0xdeadbabe;  /* raise_func must not return */
    stack->rec_ptr      = &stack->rec;
    stack->context_ptr  = &stack->context;

    stack->rec.ExceptionRecord  = NULL;
    stack->rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    stack->rec.ExceptionAddress = (LPVOID)EIP_sig(sigcontext);
    stack->rec.NumberParameters = 0;

    save_context( &stack->context, sigcontext, fs, gs );

    /* now modify the sigcontext to return to the raise function */
    ESP_sig(sigcontext) = (DWORD)stack;
    EIP_sig(sigcontext) = (DWORD)func;
    /* clear single-step and align check flag */
    EFL_sig(sigcontext) &= ~(0x100|0x40000); 
    CS_sig(sigcontext)  = wine_get_cs();
    DS_sig(sigcontext)  = wine_get_ds();
    ES_sig(sigcontext)  = wine_get_es();
    FS_sig(sigcontext)  = wine_get_fs();
    GS_sig(sigcontext)  = wine_get_gs();
    SS_sig(sigcontext)  = wine_get_ss();

    return stack->rec_ptr;
}


/***********************************************************************
 *           get_exception_context
 *
 * Get a pointer to the context built by setup_exception.
 */
static inline CONTEXT *get_exception_context( EXCEPTION_RECORD *rec )
{
    return (CONTEXT *)rec - 1;  /* cf. stack_layout structure */
}


/**********************************************************************
 *		get_fpu_code
 *
 * Get the FPU exception code from the FPU status.
 */
static inline DWORD get_fpu_code( const CONTEXT *context )
{
    DWORD status = context->FloatSave.StatusWord;

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
 *		raise_segv_exception
 */
static void WINAPI raise_segv_exception( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    switch(rec->ExceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION:
        if (rec->NumberParameters == 2)
        {
            if (rec->ExceptionInformation[0] == EXCEPTION_EXECUTE_FAULT && check_atl_thunk( rec, context ))
                goto done;
            rec->ExceptionCode = VIRTUAL_HandleFault( (void *)rec->ExceptionInformation[1] );
        }
        break;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        /* FIXME: pass through exception handler first? */
        if (context->EFlags & 0x00040000)
        {
            /* Disable AC flag, return */
            context->EFlags &= ~0x00040000;
            goto done;
        }
        break;
    }
    __regs_RtlRaiseException( rec, context );
done:
    NtSetContextThread( GetCurrentThread(), context );
}


/**********************************************************************
 *		raise_trap_exception
 */
static void WINAPI raise_trap_exception( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    if (rec->ExceptionCode == EXCEPTION_SINGLE_STEP)
    {
        if (context->EFlags & 0x100)
        {
            context->EFlags &= ~0x100;  /* clear single-step flag */
        }
        else  /* hardware breakpoint, fetch the debug registers */
        {
            context->ContextFlags = CONTEXT_DEBUG_REGISTERS;
            NtGetContextThread(GetCurrentThread(), context);
            /* we have either:
             *    - a bp from a debug register
             *    - a single step interrupt at popf instruction, which just has
             *      removed the TF.
             *    - someone did a kill(SIGTRAP) on us, and we shall return
             *      a breakpoint, not a single step exception
             */
            if ( !(context->Dr6 & 0xf) && !(context->Dr6 & 0x4000) )
                rec->ExceptionCode = EXCEPTION_BREAKPOINT;
            context->ContextFlags |= CONTEXT_FULL;  /* restore flags */
        }
    }

    __regs_RtlRaiseException( rec, context );
    NtSetContextThread( GetCurrentThread(), context );
}


/**********************************************************************
 *		raise_exception
 *
 * Generic raise function for exceptions that don't need special treatment.
 */
static void WINAPI raise_exception( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    __regs_RtlRaiseException( rec, context );
    NtSetContextThread( GetCurrentThread(), context );
}


#ifdef __HAVE_VM86
/**********************************************************************
 *		raise_vm86_sti_exception
 */
static void WINAPI raise_vm86_sti_exception( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    /* merge_vm86_pending_flags merges the vm86_pending flag in safely */
    NtCurrentTeb()->vm86_pending |= VIP_MASK;

    if (ntdll_get_thread_data()->vm86_ptr)
    {
        if (((char*)context->Eip >= (char*)vm86_return) &&
            ((char*)context->Eip <= (char*)vm86_return_end) &&
            (VM86_TYPE(context->Eax) != VM86_SIGNAL)) {
            /* exiting from VM86, can't throw */
            goto done;
        }
        merge_vm86_pending_flags( rec );
    }
    else if (NtCurrentTeb()->dpmi_vif &&
             !wine_ldt_is_system(context->SegCs) &&
             !wine_ldt_is_system(context->SegSs))
    {
        /* Executing DPMI code and virtual interrupts are enabled. */
        NtCurrentTeb()->vm86_pending = 0;
        __regs_RtlRaiseException( rec, context );
    }
done:
    NtSetContextThread( GetCurrentThread(), context );
}


/**********************************************************************
 *		usr2_handler
 *
 * Handler for SIGUSR2.
 * We use it to signal that the running __wine_enter_vm86() should
 * immediately set VIP_MASK, causing pending events to be handled
 * as early as possible.
 */
static void usr2_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    EXCEPTION_RECORD *rec = setup_exception( sigcontext, raise_vm86_sti_exception );
    rec->ExceptionCode = EXCEPTION_VM86_STI;
}
#endif /* __HAVE_VM86 */


/**********************************************************************
 *		segv_handler
 *
 * Handler for SIGSEGV and related errors.
 */
static void segv_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    SIGCONTEXT *context = sigcontext;
    EXCEPTION_RECORD *rec = setup_exception( context, raise_segv_exception );

    switch(get_trap_code(context))
    {
    case TRAP_x86_OFLOW:   /* Overflow exception */
        rec->ExceptionCode = EXCEPTION_INT_OVERFLOW;
        break;
    case TRAP_x86_BOUND:   /* Bound range exception */
        rec->ExceptionCode = EXCEPTION_ARRAY_BOUNDS_EXCEEDED;
        break;
    case TRAP_x86_PRIVINFLT:   /* Invalid opcode exception */
        rec->ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
        break;
    case TRAP_x86_STKFLT:  /* Stack fault */
        rec->ExceptionCode = EXCEPTION_STACK_OVERFLOW;
        break;
    case TRAP_x86_SEGNPFLT:  /* Segment not present exception */
    case TRAP_x86_PROTFLT:   /* General protection fault */
    case TRAP_x86_UNKNOWN:   /* Unknown fault code */
        {
            WORD err = get_error_code(context);
            if (!err && (rec->ExceptionCode = is_privileged_instr( get_exception_context(rec) ))) break;
            rec->ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
            rec->NumberParameters = 2;
            rec->ExceptionInformation[0] = 0;
            /* if error contains a LDT selector, use that as fault address */
            if ((err & 7) == 4 && !wine_ldt_is_system( err | 7 ))
                rec->ExceptionInformation[1] = err & ~7;
            else
                rec->ExceptionInformation[1] = 0xffffffff;
        }
        break;
    case TRAP_x86_PAGEFLT:  /* Page fault */
        rec->ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
        rec->NumberParameters = 2;
        rec->ExceptionInformation[0] = (get_error_code(context) >> 1) & 0x09;
        rec->ExceptionInformation[1] = (ULONG_PTR)siginfo->si_addr;
        break;
    case TRAP_x86_ALIGNFLT:  /* Alignment check exception */
        rec->ExceptionCode = EXCEPTION_DATATYPE_MISALIGNMENT;
        break;
    default:
        WINE_ERR( "Got unexpected trap %d\n", get_trap_code(context) );
        /* fall through */
    case TRAP_x86_NMI:       /* NMI interrupt */
    case TRAP_x86_DNA:       /* Device not available exception */
    case TRAP_x86_DOUBLEFLT: /* Double fault exception */
    case TRAP_x86_TSSFLT:    /* Invalid TSS exception */
    case TRAP_x86_MCHK:      /* Machine check exception */
    case TRAP_x86_CACHEFLT:  /* Cache flush exception */
        rec->ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
        break;
    }
}


/**********************************************************************
 *		trap_handler
 *
 * Handler for SIGTRAP.
 */
static void trap_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    SIGCONTEXT *context = sigcontext;
    EXCEPTION_RECORD *rec = setup_exception( context, raise_trap_exception );

    switch(get_trap_code(context))
    {
    case TRAP_x86_TRCTRAP:  /* Single-step exception */
        rec->ExceptionCode = EXCEPTION_SINGLE_STEP;
        break;
    case TRAP_x86_BPTFLT:   /* Breakpoint exception */
        rec->ExceptionAddress = (char *)rec->ExceptionAddress - 1;  /* back up over the int3 instruction */
        /* fall through */
    default:
        rec->ExceptionCode = EXCEPTION_BREAKPOINT;
        break;
    }
}


/**********************************************************************
 *		fpe_handler
 *
 * Handler for SIGFPE.
 */
static void fpe_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    CONTEXT *win_context;
    SIGCONTEXT *context = sigcontext;
    EXCEPTION_RECORD *rec = setup_exception( context, raise_exception );

    win_context = get_exception_context( rec );

    switch(get_trap_code(context))
    {
    case TRAP_x86_DIVIDE:   /* Division by zero exception */
        rec->ExceptionCode = EXCEPTION_INT_DIVIDE_BY_ZERO;
        break;
    case TRAP_x86_FPOPFLT:   /* Coprocessor segment overrun */
        rec->ExceptionCode = EXCEPTION_FLT_INVALID_OPERATION;
        break;
    case TRAP_x86_ARITHTRAP:  /* Floating point exception */
    case TRAP_x86_UNKNOWN:    /* Unknown fault code */
        rec->ExceptionCode = get_fpu_code( win_context );
        break;
    case TRAP_x86_CACHEFLT:  /* SIMD exception */
        /* TODO:
         * Behaviour only tested for divide-by-zero exceptions
         * Check for other SIMD exceptions as well */
        if(siginfo->si_code != FPE_FLTDIV)
            FIXME("untested SIMD exception: %#x. Might not work correctly\n",
                  siginfo->si_code);

        rec->ExceptionCode = STATUS_FLOAT_MULTIPLE_TRAPS;
        rec->NumberParameters = 1;
        /* no idea what meaning is actually behind this but thats what native does */
        rec->ExceptionInformation[0] = 0;
        break;
    default:
        WINE_ERR( "Got unexpected trap %d\n", get_trap_code(context) );
        rec->ExceptionCode = EXCEPTION_FLT_INVALID_OPERATION;
        break;
    }
}


/**********************************************************************
 *		int_handler
 *
 * Handler for SIGINT.
 *
 * FIXME: should not be calling external functions on the signal stack.
 */
static void int_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    WORD fs, gs;
    init_handler( sigcontext, &fs, &gs );
    if (!dispatch_signal(SIGINT))
    {
        EXCEPTION_RECORD *rec = setup_exception( sigcontext, raise_exception );
        rec->ExceptionCode = CONTROL_C_EXIT;
    }
}

/**********************************************************************
 *		abrt_handler
 *
 * Handler for SIGABRT.
 */
static void abrt_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    EXCEPTION_RECORD *rec = setup_exception( sigcontext, raise_exception );
    rec->ExceptionCode  = EXCEPTION_WINE_ASSERTION;
    rec->ExceptionFlags = EH_NONCONTINUABLE;
}


/**********************************************************************
 *		term_handler
 *
 * Handler for SIGTERM.
 */
static void term_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    WORD fs, gs;
    init_handler( sigcontext, &fs, &gs );
    server_abort_thread(0);
}


/**********************************************************************
 *		usr1_handler
 *
 * Handler for SIGUSR1, used to signal a thread that it got suspended.
 */
static void usr1_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    CONTEXT context;
    WORD fs, gs;

    init_handler( sigcontext, &fs, &gs );
    save_context( &context, sigcontext, fs, gs );
    wait_suspend( &context );
    restore_context( &context, sigcontext );
}


/**********************************************************************
 *		get_signal_stack_total_size
 *
 * Retrieve the size to allocate for the signal stack, including the TEB at the bottom.
 * Must be a power of two.
 */
size_t get_signal_stack_total_size(void)
{
    static const size_t teb_size = 4096;  /* we reserve one page for the TEB */

    if (!signal_stack_size)
    {
        size_t size = 4096, min_size = teb_size + max( MINSIGSTKSZ, 4096 );
        /* find the first power of two not smaller than min_size */
        while (size < min_size) size *= 2;
        signal_stack_mask = size - 1;
        signal_stack_size = size - teb_size;
    }
    return signal_stack_size + teb_size;
}


/***********************************************************************
 *           __wine_set_signal_handler   (NTDLL.@)
 */
int __wine_set_signal_handler(unsigned int sig, wine_signal_handler wsh)
{
    if (sig >= sizeof(handlers) / sizeof(handlers[0])) return -1;
    if (handlers[sig] != NULL) return -2;
    handlers[sig] = wsh;
    return 0;
}


/**********************************************************************
 *		SIGNAL_Init
 */
BOOL SIGNAL_Init(void)
{
    struct sigaction sig_act;

#ifdef HAVE_SIGALTSTACK
    stack_t ss;

#ifdef __APPLE__
    int mib[2], val = 1;

    mib[0] = CTL_KERN;
    mib[1] = KERN_THALTSTACK;
    sysctl( mib, 2, NULL, NULL, &val, sizeof(val) );
#endif

    ss.ss_sp    = get_signal_stack();
    ss.ss_size  = signal_stack_size;
    ss.ss_flags = 0;
    if (sigaltstack(&ss, NULL) == -1)
    {
        perror( "sigaltstack" );
        return FALSE;
    }
#endif  /* HAVE_SIGALTSTACK */

    sig_act.sa_mask = server_block_set;
    sig_act.sa_flags = SA_SIGINFO | SA_RESTART;
#ifdef SA_ONSTACK
    sig_act.sa_flags |= SA_ONSTACK;
#endif

    sig_act.sa_sigaction = int_handler;
    if (sigaction( SIGINT, &sig_act, NULL ) == -1) goto error;
    sig_act.sa_sigaction = fpe_handler;
    if (sigaction( SIGFPE, &sig_act, NULL ) == -1) goto error;
    sig_act.sa_sigaction = abrt_handler;
    if (sigaction( SIGABRT, &sig_act, NULL ) == -1) goto error;
    sig_act.sa_sigaction = term_handler;
    if (sigaction( SIGTERM, &sig_act, NULL ) == -1) goto error;
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

#ifdef __HAVE_VM86
    sig_act.sa_sigaction = usr2_handler;
    if (sigaction( SIGUSR2, &sig_act, NULL ) == -1) goto error;
#endif

    return TRUE;

 error:
    perror("sigaction");
    return FALSE;
}


#ifdef __HAVE_VM86
/**********************************************************************
 *		__wine_enter_vm86   (NTDLL.@)
 *
 * Enter vm86 mode with the specified register context.
 */
void __wine_enter_vm86( CONTEXT *context )
{
    EXCEPTION_RECORD rec;
    int res;
    struct vm86plus_struct vm86;

    memset( &vm86, 0, sizeof(vm86) );
    for (;;)
    {
        restore_vm86_context( context, &vm86 );

        ntdll_get_thread_data()->vm86_ptr = &vm86;
        merge_vm86_pending_flags( &rec );

        res = vm86_enter( &ntdll_get_thread_data()->vm86_ptr ); /* uses and clears teb->vm86_ptr */
        if (res < 0)
        {
            errno = -res;
            return;
        }

        save_vm86_context( context, &vm86 );

        rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
        rec.ExceptionRecord  = NULL;
        rec.ExceptionAddress = (LPVOID)context->Eip;
        rec.NumberParameters = 0;

        switch(VM86_TYPE(res))
        {
        case VM86_UNKNOWN: /* unhandled GP fault - IO-instruction or similar */
            rec.ExceptionCode = EXCEPTION_PRIV_INSTRUCTION;
            break;
        case VM86_TRAP: /* return due to DOS-debugger request */
            switch(VM86_ARG(res))
            {
            case TRAP_x86_TRCTRAP:  /* Single-step exception */
                rec.ExceptionCode = EXCEPTION_SINGLE_STEP;
                break;
            case TRAP_x86_BPTFLT:   /* Breakpoint exception */
                rec.ExceptionAddress = (char *)rec.ExceptionAddress - 1;  /* back up over the int3 instruction */
                /* fall through */
            default:
                rec.ExceptionCode = EXCEPTION_BREAKPOINT;
                break;
            }
            break;
        case VM86_INTx: /* int3/int x instruction (ARG = x) */
            rec.ExceptionCode = EXCEPTION_VM86_INTx;
            rec.NumberParameters = 1;
            rec.ExceptionInformation[0] = VM86_ARG(res);
            break;
        case VM86_STI: /* sti/popf/iret instruction enabled virtual interrupts */
            context->EFlags |= VIF_MASK;
            context->EFlags &= ~VIP_MASK;
            NtCurrentTeb()->vm86_pending = 0;
            rec.ExceptionCode = EXCEPTION_VM86_STI;
            break;
        case VM86_PICRETURN: /* return due to pending PIC request */
            rec.ExceptionCode = EXCEPTION_VM86_PICRETURN;
            break;
        case VM86_SIGNAL: /* cannot happen because vm86_enter handles this case */
        default:
            WINE_ERR( "unhandled result from vm86 mode %x\n", res );
            continue;
        }
        __regs_RtlRaiseException( &rec, context );
    }
}

#else /* __HAVE_VM86 */
/**********************************************************************
 *		__wine_enter_vm86   (NTDLL.@)
 */
void __wine_enter_vm86( CONTEXT *context )
{
    MESSAGE("vm86 mode not supported on this platform\n");
}
#endif /* __HAVE_VM86 */

/**********************************************************************
 *		DbgBreakPoint   (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( DbgBreakPoint, "int $3; ret")

/**********************************************************************
 *		DbgUserBreakPoint   (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( DbgUserBreakPoint, "int $3; ret")


/**********************************************************************
 *		EXC_CallHandler   (internal)
 *
 * Some exception handlers depend on EBP to have a fixed position relative to
 * the exception frame.
 * Shrinker depends on (*1) doing what it does,
 * (*2) being the exact instruction it is and (*3) beginning with 0x64
 * (i.e. the %fs prefix to the movl instruction). It also depends on the
 * function calling the handler having only 5 parameters (*4).
 */
__ASM_GLOBAL_FUNC( EXC_CallHandler,
"	pushl	%ebp\n"
"	movl	%esp, %ebp\n"
"	pushl	%ebx\n"
"	movl	28(%ebp), %edx\n" /* ugly hack to pass the 6th param needed because of Shrinker */
"	pushl	24(%ebp)\n"
"	pushl	20(%ebp)\n"
"	pushl	16(%ebp)\n"
"	pushl	12(%ebp)\n"
"	pushl	8(%ebp)\n"
"	call	" __ASM_NAME("call_exception_handler") "\n"
"	popl	%ebx\n"
"	leave\n"
"	ret\n"
)
__ASM_GLOBAL_FUNC(call_exception_handler,
"	pushl	%ebp\n"
"	movl	%esp, %ebp\n"
"	subl    $12,%esp\n"
"	pushl	12(%ebp)\n"       /* make any exceptions in this... */
"	pushl	%edx\n"           /* handler be handled by... */
"	.byte	0x64\n"
"	pushl	(0)\n"            /* nested_handler (passed in edx). */
"	.byte	0x64\n"
"	movl	%esp,(0)\n"       /* push the new exception frame onto the exception stack. */
"	pushl	20(%ebp)\n"
"	pushl	16(%ebp)\n"
"	pushl	12(%ebp)\n"
"	pushl	8(%ebp)\n"
"	movl	24(%ebp), %ecx\n" /* (*1) */
"	call	*%ecx\n"          /* call handler. (*2) */
"	.byte	0x64\n"
"	movl	(0), %esp\n"      /* restore previous... (*3) */
"	.byte	0x64\n"
"	popl	(0)\n"            /* exception frame. */
"	movl	%ebp, %esp\n"     /* restore saved stack, in case it was corrupted */
"	popl	%ebp\n"
"	ret	$20\n"            /* (*4) */
)
#endif  /* __i386__ */
