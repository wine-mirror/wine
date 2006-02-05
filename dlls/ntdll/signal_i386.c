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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef __i386__

#include "config.h"
#include "wine/port.h"

#include <errno.h>
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

#ifdef HAVE_SYS_VM86_H
# include <sys/vm86.h>
#endif

#ifdef HAVE_SYS_SIGNAL_H
# include <sys/signal.h>
#endif

#include "windef.h"
#include "thread.h"
#include "wine/library.h"
#include "ntdll_misc.h"

/***********************************************************************
 * signal context platform-specific definitions
 */

#ifdef linux
typedef struct
{
    unsigned short sc_gs, __gsh;
    unsigned short sc_fs, __fsh;
    unsigned short sc_es, __esh;
    unsigned short sc_ds, __dsh;
    unsigned long sc_edi;
    unsigned long sc_esi;
    unsigned long sc_ebp;
    unsigned long sc_esp;
    unsigned long sc_ebx;
    unsigned long sc_edx;
    unsigned long sc_ecx;
    unsigned long sc_eax;
    unsigned long sc_trapno;
    unsigned long sc_err;
    unsigned long sc_eip;
    unsigned short sc_cs, __csh;
    unsigned long sc_eflags;
    unsigned long esp_at_signal;
    unsigned short sc_ss, __ssh;
    unsigned long i387;
    unsigned long oldmask;
    unsigned long cr2;
} volatile SIGCONTEXT;

#define HANDLER_DEF(name) void name( int __signal, SIGCONTEXT __context )
#define HANDLER_CONTEXT (&__context)

/* this is the sigaction structure from the Linux 2.1.20 kernel.  */
struct kernel_sigaction
{
    void (*ksa_handler)();
    unsigned long ksa_mask;
    unsigned long ksa_flags;
    void *ksa_restorer;
};

/* Similar to the sigaction function in libc, except it leaves alone the
   restorer field, which is used to specify the signal stack address */
static inline int wine_sigaction( int sig, struct kernel_sigaction *new,
                                  struct kernel_sigaction *old )
{
    __asm__ __volatile__( "pushl %%ebx\n\t"
                          "movl %2,%%ebx\n\t"
                          "int $0x80\n\t"
                          "popl %%ebx"
                          : "=a" (sig)
                          : "0" (SYS_sigaction), "S" (sig), "c" (new), "d" (old) );
    if (sig>=0) return 0;
    errno = -sig;
    return -1;
}

#ifdef HAVE_SIGALTSTACK
/* direct syscall for sigaltstack to work around glibc 2.0 brain-damage */
static inline int wine_sigaltstack( const struct sigaltstack *new,
                                    struct sigaltstack *old )
{
    int ret;
    __asm__ __volatile__( "pushl %%ebx\n\t"
                          "movl %2,%%ebx\n\t"
                          "int $0x80\n\t"
                          "popl %%ebx"
                          : "=a" (ret)
                          : "0" (SYS_sigaltstack), "q" (new), "c" (old) );
    if (ret >= 0) return 0;
    errno = -ret;
    return -1;
}
#endif

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
                  "ret" );

#ifdef HAVE_SYS_VM86_H
# define __HAVE_VM86
#endif

#endif  /* linux */

#ifdef BSDI

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

#include <machine/frame.h>
typedef struct trapframe SIGCONTEXT;

#define HANDLER_DEF(name) void name( int __signal, int code, SIGCONTEXT *__context )
#define HANDLER_CONTEXT __context

#define EFL_sig(context)     ((context)->tf_eflags)

#define EIP_sig(context)     (*((unsigned long*)&(context)->tf_eip))
#define ESP_sig(context)     (*((unsigned long*)&(context)->tf_esp))

#endif /* bsdi */

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__)

typedef struct sigcontext SIGCONTEXT;

#define HANDLER_DEF(name) void name( int __signal, int code, SIGCONTEXT *__context )
#define HANDLER_CONTEXT __context

#endif  /* *BSD */

#if defined(__svr4__) || defined(_SCO_DS) || defined(__sun)

#ifdef _SCO_DS
#include <sys/regset.h>
#endif
/* Solaris kludge */
#undef ERR
#include <sys/ucontext.h>
#undef ERR
typedef struct ucontext SIGCONTEXT;

#define HANDLER_DEF(name) void name( int __signal, siginfo_t *__siginfo, SIGCONTEXT *__context )
#define HANDLER_CONTEXT __context

#endif  /* svr4 || SCO_DS */

#ifdef __EMX__

typedef struct
{
    unsigned long ContextFlags;
    FLOATING_SAVE_AREA sc_float;
    unsigned long sc_gs;
    unsigned long sc_fs;
    unsigned long sc_es;
    unsigned long sc_ds;
    unsigned long sc_edi;
    unsigned long sc_esi;
    unsigned long sc_eax;
    unsigned long sc_ebx;
    unsigned long sc_ecx;
    unsigned long sc_edx;
    unsigned long sc_ebp;
    unsigned long sc_eip;
    unsigned long sc_cs;
    unsigned long sc_eflags;
    unsigned long sc_esp;
    unsigned long sc_ss;
} SIGCONTEXT;

#endif  /* __EMX__ */

#ifdef __CYGWIN__

/* FIXME: This section is just here so it can compile, it's most likely
 * completely wrong. */

typedef struct
{
    unsigned short sc_gs, __gsh;
    unsigned short sc_fs, __fsh;
    unsigned short sc_es, __esh;
    unsigned short sc_ds, __dsh;
    unsigned long sc_edi;
    unsigned long sc_esi;
    unsigned long sc_ebp;
    unsigned long sc_esp;
    unsigned long sc_ebx;
    unsigned long sc_edx;
    unsigned long sc_ecx;
    unsigned long sc_eax;
    unsigned long sc_trapno;
    unsigned long sc_err;
    unsigned long sc_eip;
    unsigned short sc_cs, __csh;
    unsigned long sc_eflags;
    unsigned long esp_at_signal;
    unsigned short sc_ss, __ssh;
    unsigned long i387;
    unsigned long oldmask;
    unsigned long cr2;
} SIGCONTEXT;

#define HANDLER_DEF(name) void name( int __signal, SIGCONTEXT __context )
#define HANDLER_CONTEXT (&__context)

#endif /* __CYGWIN__ */

#ifdef __APPLE__
# include <sys/ucontext.h>
# include <sys/types.h>
# include <signal.h>

typedef ucontext_t SIGCONTEXT;

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

#define HANDLER_DEF(name) void name( int __signal, siginfo_t *__siginfo, SIGCONTEXT *__context )
#define HANDLER_CONTEXT (__context)

#define TRAP_sig(context)    ((context)->uc_mcontext->es.trapno)
#define ERROR_sig(context)   ((context)->uc_mcontext->es.err)

#define FAULT_ADDRESS        (__siginfo->si_addr)

#endif /* __APPLE__ */

#if defined(linux) || defined(__NetBSD__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) ||\
    defined(__OpenBSD__) || defined(__EMX__) || defined(__CYGWIN__)

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

#ifdef __NetBSD__
#define ERROR_sig(context)   ((context)->sc_err)
#endif

#ifdef linux
#define ERROR_sig(context)   ((context)->sc_err)
#define FPU_sig(context)     ((FLOATING_SAVE_AREA*)((context)->i387))
#define FAULT_ADDRESS        ((void *)HANDLER_CONTEXT->cr2)
#endif

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#define EFL_sig(context)     ((context)->sc_efl)
/* FreeBSD, see i386/i386/traps.c::trap_pfault va->err kludge  */
#define FAULT_ADDRESS        ((void *)HANDLER_CONTEXT->sc_err)
#else
#define EFL_sig(context)     ((context)->sc_eflags)
#endif

#define EIP_sig(context)     (*((unsigned long*)&(context)->sc_eip))
#define ESP_sig(context)     (*((unsigned long*)&(context)->sc_esp))

#endif  /* linux || __NetBSD__ || __FreeBSD__ || __OpenBSD__ */

#if defined(__svr4__) || defined(_SCO_DS) || defined(__sun)

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
#ifdef TRAPNO
#define TRAP_sig(context)     ((context)->uc_mcontext.gregs[TRAPNO])
#endif

#define FAULT_ADDRESS         (__siginfo->si_addr)

#endif  /* svr4 || SCO_DS */

#include "wine/exception.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);

typedef int (*wine_signal_handler)(unsigned int sig);

static size_t signal_stack_mask;
static size_t signal_stack_size;

static wine_signal_handler handlers[256];

extern void DECLSPEC_NORETURN __wine_call_from_32_restore_regs( const CONTEXT *context );

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


/***********************************************************************
 *           dispatch_signal
 */
inline static int dispatch_signal(unsigned int sig)
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
inline static void *init_handler( const SIGCONTEXT *sigcontext, WORD *fs, WORD *gs )
{
    void *stack = (void *)ESP_sig(sigcontext);
    TEB *teb = get_current_teb();
    struct ntdll_thread_regs *thread_regs = (struct ntdll_thread_regs *)teb->SpareBytes1;

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

    wine_set_fs( thread_regs->fs );

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
        wine_set_gs( thread_regs->gs );
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
 * Set the FPU context from a sigcontext.
 */
inline static void save_fpu( CONTEXT *context, const SIGCONTEXT *sigcontext )
{
    context->ContextFlags |= CONTEXT86_FLOATING_POINT;
#ifdef FPU_sig
    if (FPU_sig(sigcontext))
    {
        context->FloatSave = *FPU_sig(sigcontext);
        return;
    }
#endif  /* FPU_sig */
#ifdef __GNUC__
    __asm__ __volatile__( "fnsave %0; fwait" : "=m" (context->FloatSave) );
#endif  /* __GNUC__ */
}


/***********************************************************************
 *           restore_fpu
 *
 * Restore the FPU context to a sigcontext.
 */
inline static void restore_fpu( const CONTEXT *context )
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
inline static void save_context( CONTEXT *context, const SIGCONTEXT *sigcontext, WORD fs, WORD gs )
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
}


/***********************************************************************
 *           restore_context
 *
 * Restore the signal info from the context.
 */
inline static void restore_context( const CONTEXT *context, SIGCONTEXT *sigcontext )
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
}


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
        if ((flags & CONTEXT_FULL) != (CONTEXT_FULL & ~CONTEXT_i386))
            FIXME( "setting partial context (%lx) not supported\n", flags );
        else
            __wine_call_from_32_restore_regs( context );
    }
}


/***********************************************************************
 *           is_privileged_instr
 *
 * Check if the fault location is a privileged instruction.
 * Based on the instruction emulation code in dlls/kernel/instr.c.
 */
static inline int is_privileged_instr( CONTEXT86 *context )
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
        if (++prefix_count >= 15) return 0;
        instr++;
        continue;

    case 0x0f: /* extended instruction */
        switch(instr[1])
        {
        case 0x20: /* mov crX, reg */
        case 0x21: /* mov drX, reg */
        case 0x22: /* mov reg, crX */
        case 0x23: /* mov reg drX */
            return 1;
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
        return 1;
    default:
        return 0;
    }
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
        ERR( "nested exception on signal stack in thread %04lx eip %08lx esp %08lx stack %p-%p\n",
             GetCurrentThreadId(), EIP_sig(sigcontext), ESP_sig(sigcontext),
             NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
        server_abort_thread(1);
    }

    if (stack - 1 > stack || /* check for overflow in subtraction */
        (char *)(stack - 1) < (char *)NtCurrentTeb()->Tib.StackLimit ||
        (char *)stack > (char *)NtCurrentTeb()->Tib.StackBase)
    {
        UINT diff = (char *)NtCurrentTeb()->Tib.StackLimit - (char *)stack;
        if (diff < 4096)
        {
            ERR( "stack overflow %u bytes in thread %04lx eip %08lx esp %08lx stack %p-%p\n",
                 diff, GetCurrentThreadId(), EIP_sig(sigcontext), ESP_sig(sigcontext),
                 NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
            server_abort_thread(1);
        }
        else WARN( "exception outside of stack limits in thread %04lx eip %08lx esp %08lx stack %p-%p\n",
                   GetCurrentThreadId(), EIP_sig(sigcontext), ESP_sig(sigcontext),
                   NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
    }

    stack--;  /* push the stack_layout structure */
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
    EFL_sig(sigcontext) &= ~0x100;  /* clear single-step flag */
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
            rec->ExceptionCode = VIRTUAL_HandleFault( (void *)rec->ExceptionInformation[1] );
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
            /* do we really have a bp from a debug register ?
             * if not, then someone did a kill(SIGTRAP) on us, and we
             * shall return a breakpoint, not a single step exception
             */
            if (!(context->Dr6 & 0xf)) rec->ExceptionCode = EXCEPTION_BREAKPOINT;
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
static HANDLER_DEF(usr2_handler)
{
    EXCEPTION_RECORD *rec = setup_exception( HANDLER_CONTEXT, raise_vm86_sti_exception );
    rec->ExceptionCode = EXCEPTION_VM86_STI;
}
#endif /* __HAVE_VM86 */


/**********************************************************************
 *		segv_handler
 *
 * Handler for SIGSEGV and related errors.
 */
static HANDLER_DEF(segv_handler)
{
    EXCEPTION_RECORD *rec = setup_exception( HANDLER_CONTEXT, raise_segv_exception );

    switch(get_trap_code(HANDLER_CONTEXT))
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
        if (!get_error_code(HANDLER_CONTEXT) && is_privileged_instr( get_exception_context(rec) ))
            rec->ExceptionCode = EXCEPTION_PRIV_INSTRUCTION;
        else
        {
            WORD err = get_error_code(HANDLER_CONTEXT);
            rec->ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
            rec->NumberParameters = 2;
            rec->ExceptionInformation[0] = 0;
            /* if error contains a LDT selector, use that as fault address */
            rec->ExceptionInformation[1] = (err & 7) == 4 ? (err & ~7) : 0xffffffff;
        }
        break;
    case TRAP_x86_PAGEFLT:  /* Page fault */
        rec->ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
#ifdef FAULT_ADDRESS
        rec->NumberParameters = 2;
        rec->ExceptionInformation[0] = (get_error_code(HANDLER_CONTEXT) & 2) != 0;
        rec->ExceptionInformation[1] = (ULONG_PTR)FAULT_ADDRESS;
#endif
        break;
    case TRAP_x86_ALIGNFLT:  /* Alignment check exception */
        rec->ExceptionCode = EXCEPTION_DATATYPE_MISALIGNMENT;
        break;
    default:
        ERR( "Got unexpected trap %d\n", get_trap_code(HANDLER_CONTEXT) );
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
static HANDLER_DEF(trap_handler)
{
    EXCEPTION_RECORD *rec = setup_exception( HANDLER_CONTEXT, raise_trap_exception );

    switch(get_trap_code(HANDLER_CONTEXT))
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
static HANDLER_DEF(fpe_handler)
{
    EXCEPTION_RECORD *rec = setup_exception( HANDLER_CONTEXT, raise_exception );
    CONTEXT *context;

    context = get_exception_context( rec );
    save_fpu( context, HANDLER_CONTEXT );

    switch(get_trap_code(HANDLER_CONTEXT))
    {
    case TRAP_x86_DIVIDE:   /* Division by zero exception */
        rec->ExceptionCode = EXCEPTION_INT_DIVIDE_BY_ZERO;
        break;
    case TRAP_x86_FPOPFLT:   /* Coprocessor segment overrun */
        rec->ExceptionCode = EXCEPTION_FLT_INVALID_OPERATION;
        break;
    case TRAP_x86_ARITHTRAP:  /* Floating point exception */
    case TRAP_x86_UNKNOWN:    /* Unknown fault code */
        rec->ExceptionCode = get_fpu_code( context );
        break;
    default:
        ERR( "Got unexpected trap %d\n", get_trap_code(HANDLER_CONTEXT) );
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
static HANDLER_DEF(int_handler)
{
    WORD fs, gs;
    init_handler( HANDLER_CONTEXT, &fs, &gs );
    if (!dispatch_signal(SIGINT))
    {
        EXCEPTION_RECORD *rec = setup_exception( HANDLER_CONTEXT, raise_exception );
        rec->ExceptionCode = CONTROL_C_EXIT;
    }
}

/**********************************************************************
 *		abrt_handler
 *
 * Handler for SIGABRT.
 */
static HANDLER_DEF(abrt_handler)
{
    EXCEPTION_RECORD *rec = setup_exception( HANDLER_CONTEXT, raise_exception );
    rec->ExceptionCode  = EXCEPTION_WINE_ASSERTION;
    rec->ExceptionFlags = EH_NONCONTINUABLE;
}


/**********************************************************************
 *		term_handler
 *
 * Handler for SIGTERM.
 */
static HANDLER_DEF(term_handler)
{
    WORD fs, gs;
    init_handler( HANDLER_CONTEXT, &fs, &gs );
    server_abort_thread(0);
}


/**********************************************************************
 *		usr1_handler
 *
 * Handler for SIGUSR1, used to signal a thread that it got suspended.
 */
static HANDLER_DEF(usr1_handler)
{
    WORD fs, gs;
    CONTEXT context;

    init_handler( HANDLER_CONTEXT, &fs, &gs );
    save_context( &context, HANDLER_CONTEXT, fs, gs );
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
 *           set_handler
 *
 * Set a signal handler
 */
static int set_handler( int sig, int have_sigaltstack, void (*func)() )
{
    struct sigaction sig_act;

#ifdef linux
    if (!have_sigaltstack)
    {
        struct kernel_sigaction sig_act;
        sig_act.ksa_handler = func;
        sig_act.ksa_flags   = SA_RESTART;
        sig_act.ksa_mask    = (1 << (SIGINT-1)) |
                              (1 << (SIGUSR1-1)) |
                              (1 << (SIGUSR2-1));
        /* point to the top of the signal stack */
        sig_act.ksa_restorer = (char *)get_signal_stack() + signal_stack_size;
        return wine_sigaction( sig, &sig_act, NULL );
    }
#endif  /* linux */
    sig_act.sa_handler = func;
    sigemptyset( &sig_act.sa_mask );
    sigaddset( &sig_act.sa_mask, SIGINT );
    sigaddset( &sig_act.sa_mask, SIGUSR1 );
    sigaddset( &sig_act.sa_mask, SIGUSR2 );

#if defined(linux) || defined(__NetBSD__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__)
    sig_act.sa_flags = SA_RESTART;
#elif defined (__svr4__) || defined(_SCO_DS) || defined(__APPLE__)
    sig_act.sa_flags = SA_SIGINFO | SA_RESTART;
#else
    sig_act.sa_flags = 0;
#endif

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
    ss.ss_sp    = get_signal_stack();
    ss.ss_size  = signal_stack_size;
    ss.ss_flags = 0;
    if (!sigaltstack(&ss, NULL)) have_sigaltstack = 1;
#ifdef linux
    /* sigaltstack may fail because the kernel is too old, or
       because glibc is brain-dead. In the latter case a
       direct system call should succeed. */
    else if (!wine_sigaltstack(&ss, NULL)) have_sigaltstack = 1;
#endif  /* linux */
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

#ifdef __HAVE_VM86
    if (set_handler( SIGUSR2, have_sigaltstack, (void (*)())usr2_handler ) == -1) goto error;
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
            raise_segv_exception( &rec, context );
            continue;
        case VM86_TRAP: /* return due to DOS-debugger request */
            switch(VM86_ARG(res))
            {
            case TRAP_x86_TRCTRAP:  /* Single-step exception, single step flag is cleared by raise_trap_exception */
                rec.ExceptionCode = EXCEPTION_SINGLE_STEP;
                break;
            case TRAP_x86_BPTFLT:   /* Breakpoint exception */
                rec.ExceptionAddress = (char *)rec.ExceptionAddress - 1;  /* back up over the int3 instruction */
                /* fall through */
            default:
                rec.ExceptionCode = EXCEPTION_BREAKPOINT;
                break;
            }
            raise_trap_exception( &rec, context );
            continue;
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
            ERR( "unhandled result from vm86 mode %x\n", res );
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
__ASM_GLOBAL_FUNC( DbgBreakPoint, "int $3; ret");

/**********************************************************************
 *		DbgUserBreakPoint   (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( DbgUserBreakPoint, "int $3; ret");


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
"	subl    $4,%esp\n"
"	movl	28(%ebp), %edx\n" /* ugly hack to pass the 6th param needed because of Shrinker */
"	pushl	24(%ebp)\n"
"	pushl	20(%ebp)\n"
"	pushl	16(%ebp)\n"
"	pushl	12(%ebp)\n"
"	pushl	8(%ebp)\n"
"	call	" __ASM_NAME("call_exception_handler") "\n"
"	leave\n"
"	ret\n"
);
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
);
#endif  /* __i386__ */
