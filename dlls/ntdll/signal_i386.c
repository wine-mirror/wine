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

#include "selectors.h"

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
} SIGCONTEXT;

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
                          : "0" (SYS_sigaction), "r" (sig), "c" (new), "d" (old) );
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
                          : "0" (SYS_sigaltstack), "r" (new), "c" (old) );
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
                  "int $0x80\n"
                  ".globl " __ASM_NAME("vm86_return") "\n\t"
                  __ASM_FUNC("vm86_return") "\n"
                  __ASM_NAME("vm86_return") ":\n\t"
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

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)

typedef struct sigcontext SIGCONTEXT;

#define HANDLER_DEF(name) void name( int __signal, int code, SIGCONTEXT *__context )
#define HANDLER_CONTEXT __context

#endif  /* FreeBSD */

#if defined(__svr4__) || defined(_SCO_DS) || defined(__sun)

#ifdef _SCO_DS
#include <sys/regset.h>
#endif
/* Solaris kludge */
#undef ERR
#include <sys/ucontext.h>
#undef ERR
typedef struct ucontext SIGCONTEXT;

#define HANDLER_DEF(name) void name( int __signal, void *__siginfo, SIGCONTEXT *__context )
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

#if defined(linux) || defined(__NetBSD__) || defined(__FreeBSD__) ||\
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
#define SS_sig(context)      ((context)->sc_ss)

/* FS and GS are now in the sigcontext struct of FreeBSD, but not
 * saved by the exception handling. duh.
 * Actually they are in -current (have been for a while), and that
 * patch now finally has been MFC'd to -stable too (Nov 15 1999).
 * If you're running a system from the -stable branch older than that,
 * like a 3.3-RELEASE, grab the patch from the ports tree:
 * ftp://ftp.freebsd.org/pub/FreeBSD/FreeBSD-current/ports/emulators/wine/files/patch-3.3-sys-fsgs
 * (If its not yet there when you look, go here:
 * http://www.jelal.kn-bremen.de/freebsd/ports/emulators/wine/files/ )
 */
#ifdef __FreeBSD__
#define FS_sig(context)      ((context)->sc_fs)
#define GS_sig(context)      ((context)->sc_gs)
#endif

#ifdef linux
#define FS_sig(context)      ((context)->sc_fs)
#define GS_sig(context)      ((context)->sc_gs)
#define CR2_sig(context)     ((context)->cr2)
#define TRAP_sig(context)    ((context)->sc_trapno)
#define ERROR_sig(context)   ((context)->sc_err)
#define FPU_sig(context)     ((FLOATING_SAVE_AREA*)((context)->i387))
#endif

#ifndef __FreeBSD__
#define EFL_sig(context)     ((context)->sc_eflags)
#else
#define EFL_sig(context)     ((context)->sc_efl)
/* FreeBSD, see i386/i386/traps.c::trap_pfault va->err kludge  */
#define CR2_sig(context)     ((context)->sc_err)
#define TRAP_sig(context)    ((context)->sc_trapno)
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
#ifdef R_ESP
#define ESP_sig(context)     ((context)->uc_mcontext.gregs[R_ESP])
#else
#define ESP_sig(context)     ((context)->uc_mcontext.gregs[ESP])
#endif
#ifdef TRAPNO
#define TRAP_sig(context)     ((context)->uc_mcontext.gregs[TRAPNO])
#endif

#endif  /* svr4 || SCO_DS */


/* exception code definitions (already defined by FreeBSD/NetBSD) */
#if !defined(__FreeBSD__) && !defined(__NetBSD__) /* FIXME: other BSDs? */
#define T_DIVIDE        0   /* Division by zero exception */
#define T_TRCTRAP       1   /* Single-step exception */
#define T_NMI           2   /* NMI interrupt */
#define T_BPTFLT        3   /* Breakpoint exception */
#define T_OFLOW         4   /* Overflow exception */
#define T_BOUND         5   /* Bound range exception */
#define T_PRIVINFLT     6   /* Invalid opcode exception */
#define T_DNA           7   /* Device not available exception */
#define T_DOUBLEFLT     8   /* Double fault exception */
#define T_FPOPFLT       9   /* Coprocessor segment overrun */
#define T_TSSFLT        10  /* Invalid TSS exception */
#define T_SEGNPFLT      11  /* Segment not present exception */
#define T_STKFLT        12  /* Stack fault */
#define T_PROTFLT       13  /* General protection fault */
#define T_PAGEFLT       14  /* Page fault */
#define T_RESERVED      15  /* Unknown exception */
#define T_ARITHTRAP     16  /* Floating point exception */
#define T_ALIGNFLT      17  /* Alignment check exception */
#define T_MCHK          18  /* Machine check exception */
#define T_CACHEFLT      19  /* Cache flush exception */
#endif
#if defined(__NetBSD__)
#define T_MCHK          19  /* Machine check exception */
#endif

#define T_UNKNOWN     (-1)  /* Unknown fault (TRAP_sig not defined) */

#include "wine/exception.h"
#include "stackframe.h"
#include "global.h"
#include "miscemu.h"
#include "syslevel.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);

typedef int (*wine_signal_handler)(unsigned int sig);

static wine_signal_handler handlers[256];

static sigset_t all_sigs;

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
 *           get_trap_code
 *
 * Get the trap code for a signal.
 */
static inline int get_trap_code( const SIGCONTEXT *sigcontext )
{
#ifdef TRAP_sig
    return TRAP_sig(sigcontext);
#else
    return T_UNKNOWN;  /* unknown trap code */
#endif
}

/***********************************************************************
 *           get_error_code
 *
 * Get the error code for a signal.
 */
static inline int get_error_code( const SIGCONTEXT *sigcontext )
{
#ifdef ERROR_sig
    return ERROR_sig(sigcontext);
#else
    return 0;
#endif
}

/***********************************************************************
 *           get_cr2_value
 *
 * Get the CR2 value for a signal.
 */
static inline void *get_cr2_value( const SIGCONTEXT *sigcontext )
{
#ifdef CR2_sig
    return (void *)CR2_sig(sigcontext);
#else
    return NULL;
#endif
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
#endif /* __HAVE_VM86 */


/***********************************************************************
 *           save_context
 *
 * Set the register values from a sigcontext.
 */
static void save_context( CONTEXT *context, const SIGCONTEXT *sigcontext )
{
    WORD fs;
    /* get %fs at time of the fault */
#ifdef FS_sig
    fs = FS_sig(sigcontext);
#else
    fs = wine_get_fs();
#endif
    context->SegFs = fs;

    /* now restore a proper %fs for the fault handler */
    if (!IS_SELECTOR_SYSTEM(CS_sig(sigcontext)) ||
        !IS_SELECTOR_SYSTEM(SS_sig(sigcontext)))  /* 16-bit mode */
    {
        /*
         * Win16 or DOS protected mode. Note that during switch 
         * from 16-bit mode to linear mode, CS may be set to system 
         * segment before FS is restored. Fortunately, in this case 
         * SS is still non-system segment. This is why both CS and SS
         * are checked.
         */
        fs = SYSLEVEL_Win16CurrentTeb;
    }
#ifdef __HAVE_VM86
    else if ((void *)EIP_sig(sigcontext) == vm86_return)  /* vm86 mode */
    {
        /* fetch the saved %fs on the stack */
        fs = *(unsigned int *)ESP_sig(sigcontext);
        if (EAX_sig(sigcontext) == VM86_EAX) {
            struct vm86plus_struct *vm86;
            wine_set_fs(fs);
            /* retrieve pointer to vm86plus struct that was stored in vm86_enter
             * (but we could also get if from teb->vm86_ptr) */
            vm86 = *(struct vm86plus_struct **)(ESP_sig(sigcontext) + sizeof(int));
            /* get context from vm86 struct */
            save_vm86_context( context, vm86 );
            wine_set_gs( NtCurrentTeb()->gs_sel );
            return;
        }
    }
#endif  /* __HAVE_VM86 */

    wine_set_fs(fs);

    context->Eax    = EAX_sig(sigcontext);
    context->Ebx    = EBX_sig(sigcontext);
    context->Ecx    = ECX_sig(sigcontext);
    context->Edx    = EDX_sig(sigcontext);
    context->Esi    = ESI_sig(sigcontext);
    context->Edi    = EDI_sig(sigcontext);
    context->Ebp    = EBP_sig(sigcontext);
    context->EFlags = EFL_sig(sigcontext);
    context->Eip    = EIP_sig(sigcontext);
    context->Esp    = ESP_sig(sigcontext);
    context->SegCs  = LOWORD(CS_sig(sigcontext));
    context->SegDs  = LOWORD(DS_sig(sigcontext));
    context->SegEs  = LOWORD(ES_sig(sigcontext));
    context->SegSs  = LOWORD(SS_sig(sigcontext));
#ifdef GS_sig
    context->SegGs  = LOWORD(GS_sig(sigcontext));
#else
    context->SegGs  = wine_get_gs();
#endif
    wine_set_gs( NtCurrentTeb()->gs_sel );
}


/***********************************************************************
 *           restore_context
 *
 * Build a sigcontext from the register values.
 */
static void restore_context( const CONTEXT *context, SIGCONTEXT *sigcontext )
{
#ifdef __HAVE_VM86
    /* check if exception occurred in vm86 mode */
    if ((void *)EIP_sig(sigcontext) == vm86_return &&
        IS_SELECTOR_SYSTEM(CS_sig(sigcontext)) &&
        EAX_sig(sigcontext) == VM86_EAX)
    {
        /* retrieve pointer to vm86plus struct that was stored in vm86_enter
         * (but we could also get it from teb->vm86_ptr) */
        struct vm86plus_struct *vm86 = *(struct vm86plus_struct **)(ESP_sig(sigcontext) + sizeof(int));
        restore_vm86_context( context, vm86 );
        return;
    }
#endif /* __HAVE_VM86 */

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
#ifdef FS_sig
    FS_sig(sigcontext)  = context->SegFs;
#else
    wine_set_fs( context->SegFs );
#endif
#ifdef GS_sig
    GS_sig(sigcontext)  = context->SegGs;
#else
    wine_set_gs( context->SegGs );
#endif
}


/***********************************************************************
 *           init_handler
 *
 * Handler initialization when the full context is not needed.
 */
static void init_handler( const SIGCONTEXT *sigcontext )
{
    /* restore a proper %fs for the fault handler */
    if (!IS_SELECTOR_SYSTEM(CS_sig(sigcontext)) ||
        !IS_SELECTOR_SYSTEM(SS_sig(sigcontext)))  /* 16-bit mode */
    {
        wine_set_fs( SYSLEVEL_Win16CurrentTeb );
    }
#ifdef __HAVE_VM86
    else if ((void *)EIP_sig(sigcontext) == vm86_return)  /* vm86 mode */
    {
        /* fetch the saved %fs on the stack */
        wine_set_fs( *(unsigned int *)ESP_sig(sigcontext) );
    }
#endif  /* __HAVE_VM86 */
#ifdef FS_sig
    else  /* 32-bit mode, get %fs at time of the fault */
    {
        wine_set_fs( FS_sig(sigcontext) );
    }
#endif  /* FS_sig */
    wine_set_gs( NtCurrentTeb()->gs_sel );
}


/***********************************************************************
 *           save_fpu
 *
 * Set the FPU context from a sigcontext.
 */
inline static void save_fpu( CONTEXT *context, const SIGCONTEXT *sigcontext )
{
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
inline static void restore_fpu( CONTEXT *context, const SIGCONTEXT *sigcontext )
{
    /* reset the current interrupt status */
    context->FloatSave.StatusWord &= context->FloatSave.ControlWord | 0xffffff80;
#ifdef FPU_sig
    if (FPU_sig(sigcontext))
    {
        *FPU_sig(sigcontext) = context->FloatSave;
        return;
    }
#endif  /* FPU_sig */
#ifdef __GNUC__
    /* avoid nested exceptions */
    __asm__ __volatile__( "frstor %0; fwait" : : "m" (context->FloatSave) );
#endif  /* __GNUC__ */
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


/***********************************************************************
 *           SIGNAL_Unblock
 *
 * Unblock signals. Called from EXC_RtlRaiseException.
 */
void SIGNAL_Unblock( void )
{
    sigprocmask( SIG_UNBLOCK, &all_sigs, NULL );
}


/**********************************************************************
 *		do_segv
 *
 * Implementation of SIGSEGV handler.
 */
static void do_segv( CONTEXT *context, int trap_code, void *cr2, int err_code )
{
    EXCEPTION_RECORD rec;
    DWORD page_fault_code = EXCEPTION_ACCESS_VIOLATION;

#ifdef CR2_sig
    /* we want the page-fault case to be fast */
    if (trap_code == T_PAGEFLT)
        if (!(page_fault_code = VIRTUAL_HandleFault( cr2 ))) return;
#endif

    rec.ExceptionRecord  = NULL;
    rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    rec.ExceptionAddress = (LPVOID)context->Eip;
    rec.NumberParameters = 0;

    switch(trap_code)
    {
    case T_OFLOW:   /* Overflow exception */
        rec.ExceptionCode = EXCEPTION_INT_OVERFLOW;
        break;
    case T_BOUND:   /* Bound range exception */
        rec.ExceptionCode = EXCEPTION_ARRAY_BOUNDS_EXCEEDED;
        break;
    case T_PRIVINFLT:   /* Invalid opcode exception */
        rec.ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
        break;
    case T_STKFLT:  /* Stack fault */
        rec.ExceptionCode = EXCEPTION_STACK_OVERFLOW;
        break;
    case T_SEGNPFLT:  /* Segment not present exception */
    case T_PROTFLT:   /* General protection fault */
    case T_UNKNOWN:   /* Unknown fault code */
        if (INSTR_EmulateInstruction( context )) return;
        rec.ExceptionCode = EXCEPTION_PRIV_INSTRUCTION;
        break;
    case T_PAGEFLT:  /* Page fault */
#ifdef CR2_sig
        rec.NumberParameters = 2;
        rec.ExceptionInformation[0] = (err_code & 2) != 0;
        rec.ExceptionInformation[1] = (DWORD)cr2;
#endif /* CR2_sig */
        rec.ExceptionCode = page_fault_code;
        break;
    case T_ALIGNFLT:  /* Alignment check exception */
        /* FIXME: pass through exception handler first? */
        if (context->EFlags & 0x00040000)
        {
            /* Disable AC flag, return */
            context->EFlags &= ~0x00040000;
            return;
        }
        rec.ExceptionCode = EXCEPTION_DATATYPE_MISALIGNMENT;
        break;
    default:
        ERR( "Got unexpected trap %d\n", trap_code );
        /* fall through */
    case T_NMI:       /* NMI interrupt */
    case T_DNA:       /* Device not available exception */
    case T_DOUBLEFLT: /* Double fault exception */
    case T_TSSFLT:    /* Invalid TSS exception */
    case T_RESERVED:  /* Unknown exception */
    case T_MCHK:      /* Machine check exception */
#ifdef T_CACHEFLT
    case T_CACHEFLT:  /* Cache flush exception */
#endif
        rec.ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
        break;
    }
    EXC_RtlRaiseException( &rec, context );
}


/**********************************************************************
 *		do_trap
 *
 * Implementation of SIGTRAP handler.
 */
static void do_trap( CONTEXT *context, int trap_code )
{
    EXCEPTION_RECORD rec;

    rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    rec.ExceptionRecord  = NULL;
    rec.ExceptionAddress = (LPVOID)context->Eip;
    rec.NumberParameters = 0;

    switch(trap_code)
    {
    case T_TRCTRAP:  /* Single-step exception */
        if (context->EFlags & 0x100)
        {
            rec.ExceptionCode = EXCEPTION_SINGLE_STEP;
            context->EFlags &= ~0x100;  /* clear single-step flag */
        }
        else
        {
            /* likely we get this because of a kill(SIGTRAP) on ourself,
             * so send a bp exception instead of a single step exception
             */
            TRACE("Spurious single step trap => breakpoint simulation\n");
            rec.ExceptionCode = EXCEPTION_BREAKPOINT;
        }
        break;
    case T_BPTFLT:   /* Breakpoint exception */
        rec.ExceptionAddress = (char *)rec.ExceptionAddress - 1;  /* back up over the int3 instruction */
        /* fall through */
    default:
        rec.ExceptionCode = EXCEPTION_BREAKPOINT;
        break;
    }
    EXC_RtlRaiseException( &rec, context );
}


/**********************************************************************
 *		do_fpe
 *
 * Implementation of SIGFPE handler
 */
static void do_fpe( CONTEXT *context, int trap_code )
{
    EXCEPTION_RECORD rec;

    switch(trap_code)
    {
    case T_DIVIDE:   /* Division by zero exception */
        rec.ExceptionCode = EXCEPTION_INT_DIVIDE_BY_ZERO;
        break;
    case T_FPOPFLT:   /* Coprocessor segment overrun */
        rec.ExceptionCode = EXCEPTION_FLT_INVALID_OPERATION;
        break;
    case T_ARITHTRAP:  /* Floating point exception */
    case T_UNKNOWN:    /* Unknown fault code */
        rec.ExceptionCode = get_fpu_code( context );
        break;
    default:
        ERR( "Got unexpected trap %d\n", trap_code );
        rec.ExceptionCode = EXCEPTION_FLT_INVALID_OPERATION;
        break;
    }
    rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    rec.ExceptionRecord  = NULL;
    rec.ExceptionAddress = (LPVOID)context->Eip;
    rec.NumberParameters = 0;
    EXC_RtlRaiseException( &rec, context );
}


#ifdef __HAVE_VM86
/**********************************************************************
 *		set_vm86_pend
 *
 * Handler for SIGUSR2, which we use to set the vm86 pending flag.
 */
static void set_vm86_pend( CONTEXT *context )
{
    EXCEPTION_RECORD rec;
    TEB *teb = NtCurrentTeb();
    struct vm86plus_struct *vm86 = (struct vm86plus_struct*)(teb->vm86_ptr);

    rec.ExceptionCode           = EXCEPTION_VM86_STI;
    rec.ExceptionFlags          = EXCEPTION_CONTINUABLE;
    rec.ExceptionRecord         = NULL;
    rec.NumberParameters        = 1;
    rec.ExceptionInformation[0] = 0;

    /* __wine_enter_vm86() merges the vm86_pending flag in safely */
    teb->vm86_pending |= VIP_MASK;
    /* see if we were in VM86 mode */
    if (context->EFlags & 0x00020000)
    {
        /* seems so, also set flag in signal context */
        if (context->EFlags & VIP_MASK) return;
        context->EFlags |= VIP_MASK;
        vm86->regs.eflags |= VIP_MASK; /* no exception recursion */
        if (context->EFlags & VIF_MASK) {
            /* VIF is set, throw exception */
            teb->vm86_pending = 0;
            teb->vm86_ptr = NULL;
            rec.ExceptionAddress = (LPVOID)context->Eip;
            EXC_RtlRaiseException( &rec, context );
            teb->vm86_ptr = vm86;
        }
    }
    else if (vm86)
    {
        /* not in VM86, but possibly setting up for it */
        if (vm86->regs.eflags & VIP_MASK) return;
        vm86->regs.eflags |= VIP_MASK;
        if (((char*)context->Eip >= (char*)vm86_return) &&
            ((char*)context->Eip <= (char*)vm86_return_end) &&
            (VM86_TYPE(context->Eax) != VM86_SIGNAL)) {
            /* exiting from VM86, can't throw */
            return;
        }
        if (vm86->regs.eflags & VIF_MASK) {
            /* VIF is set, throw exception */
            CONTEXT vcontext;
            teb->vm86_pending = 0;
            teb->vm86_ptr = NULL;
            save_vm86_context( &vcontext, vm86 );
            rec.ExceptionAddress = (LPVOID)vcontext.Eip;
            EXC_RtlRaiseException( &rec, &vcontext );
            teb->vm86_ptr = vm86;
            restore_vm86_context( &vcontext, vm86 );
        }
    }
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
    CONTEXT context;

    save_context( &context, HANDLER_CONTEXT );
    set_vm86_pend( &context );
    restore_context( &context, HANDLER_CONTEXT );
}


/**********************************************************************
 *		alrm_handler
 *
 * Handler for SIGALRM.
 * Increases the alarm counter and sets the vm86 pending flag.
 */
static HANDLER_DEF(alrm_handler)
{
    CONTEXT context;

    save_context( &context, HANDLER_CONTEXT );
    NtCurrentTeb()->alarms++;
    set_vm86_pend( &context );
    restore_context( &context, HANDLER_CONTEXT );
}
#endif /* __HAVE_VM86 */


/**********************************************************************
 *		segv_handler
 *
 * Handler for SIGSEGV and related errors.
 */
static HANDLER_DEF(segv_handler)
{
    CONTEXT context;
    save_context( &context, HANDLER_CONTEXT );
    do_segv( &context, get_trap_code(HANDLER_CONTEXT),
             get_cr2_value(HANDLER_CONTEXT), get_error_code(HANDLER_CONTEXT) );
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
    do_trap( &context, get_trap_code(HANDLER_CONTEXT) );
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
    do_fpe( &context, get_trap_code(HANDLER_CONTEXT) );
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
    init_handler( HANDLER_CONTEXT );
    if (!dispatch_signal(SIGINT))
    {
        EXCEPTION_RECORD rec;
        CONTEXT context;

        save_context( &context, HANDLER_CONTEXT );
        rec.ExceptionCode    = CONTROL_C_EXIT;
        rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
        rec.ExceptionRecord  = NULL;
        rec.ExceptionAddress = (LPVOID)context.Eip;
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
    rec.ExceptionAddress = (LPVOID)context.Eip;
    rec.NumberParameters = 0;
    EXC_RtlRaiseException( &rec, &context ); /* Should never return.. */
    restore_context( &context, HANDLER_CONTEXT );
}


/**********************************************************************
 *		usr1_handler
 *
 * Handler for SIGUSR1, used to signal a thread that it got suspended.
 */
static HANDLER_DEF(usr1_handler)
{
    init_handler( HANDLER_CONTEXT );
    /* wait with 0 timeout, will only return once the thread is no longer suspended */
    WaitForMultipleObjectsEx( 0, NULL, FALSE, 0, FALSE );
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
    if (!have_sigaltstack && NtCurrentTeb()->signal_stack)
    {
        struct kernel_sigaction sig_act;
        sig_act.ksa_handler = func;
        sig_act.ksa_flags   = SA_RESTART;
        sig_act.ksa_mask    = (1 << (SIGINT-1)) |
                              (1 << (SIGUSR2-1)) |
                              (1 << (SIGALRM-1));
        /* point to the top of the stack */
        sig_act.ksa_restorer = (char *)NtCurrentTeb()->signal_stack + SIGNAL_STACK_SIZE;
        return wine_sigaction( sig, &sig_act, NULL );
    }
#endif  /* linux */
    sig_act.sa_handler = func;
    sigemptyset( &sig_act.sa_mask );
    sigaddset( &sig_act.sa_mask, SIGINT );
    sigaddset( &sig_act.sa_mask, SIGUSR2 );
    sigaddset( &sig_act.sa_mask, SIGALRM );

#ifdef linux
    sig_act.sa_flags = SA_RESTART;
#elif defined (__svr4__) || defined(_SCO_DS)
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
    if ((ss.ss_sp = NtCurrentTeb()->signal_stack))
    {
        ss.ss_size  = SIGNAL_STACK_SIZE;
        ss.ss_flags = 0;
        if (!sigaltstack(&ss, NULL)) have_sigaltstack = 1;
#ifdef linux
        /* sigaltstack may fail because the kernel is too old, or
           because glibc is brain-dead. In the latter case a
           direct system call should succeed. */
        else if (!wine_sigaltstack(&ss, NULL)) have_sigaltstack = 1;
#endif  /* linux */
    }
#endif  /* HAVE_SIGALTSTACK */

    sigfillset( &all_sigs );

    if (set_handler( SIGINT,  have_sigaltstack, (void (*)())int_handler ) == -1) goto error;
    if (set_handler( SIGFPE,  have_sigaltstack, (void (*)())fpe_handler ) == -1) goto error;
    if (set_handler( SIGSEGV, have_sigaltstack, (void (*)())segv_handler ) == -1) goto error;
    if (set_handler( SIGILL,  have_sigaltstack, (void (*)())segv_handler ) == -1) goto error;
    if (set_handler( SIGABRT, have_sigaltstack, (void (*)())abrt_handler ) == -1) goto error;
    if (set_handler( SIGUSR1, have_sigaltstack, (void (*)())usr1_handler ) == -1) goto error;
#ifdef SIGBUS
    if (set_handler( SIGBUS,  have_sigaltstack, (void (*)())segv_handler ) == -1) goto error;
#endif
#ifdef SIGTRAP
    if (set_handler( SIGTRAP, have_sigaltstack, (void (*)())trap_handler ) == -1) goto error;
#endif

#ifdef __HAVE_VM86
    if (set_handler( SIGALRM, have_sigaltstack, (void (*)())alrm_handler ) == -1) goto error;
    if (set_handler( SIGUSR2, have_sigaltstack, (void (*)())usr2_handler ) == -1) goto error;
#endif

    return TRUE;

 error:
    perror("sigaction");
    return FALSE;
}


/**********************************************************************
 *		SIGNAL_Reset
 */
void SIGNAL_Reset(void)
{
    sigset_t block_set;

    /* block the async signals */
    sigemptyset( &block_set );
    sigaddset( &block_set, SIGALRM );
    sigaddset( &block_set, SIGIO );
    sigaddset( &block_set, SIGHUP );
    sigaddset( &block_set, SIGUSR1 );
    sigaddset( &block_set, SIGUSR2 );
    sigprocmask( SIG_BLOCK, &block_set, NULL );

    /* restore default handlers */
    signal( SIGINT, SIG_DFL );
    signal( SIGFPE, SIG_DFL );
    signal( SIGSEGV, SIG_DFL );
    signal( SIGILL, SIG_DFL );
#ifdef SIGBUS
    signal( SIGBUS, SIG_DFL );
#endif
#ifdef SIGTRAP
    signal( SIGTRAP, SIG_DFL );
#endif
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
    TEB *teb = NtCurrentTeb();
    int res;
    struct vm86plus_struct vm86;

    memset( &vm86, 0, sizeof(vm86) );
    for (;;)
    {
        restore_vm86_context( context, &vm86 );
        /* Linux doesn't preserve pending flag (VIP_MASK) on return,
         * so save it on entry, just in case */
        teb->vm86_pending |= (context->EFlags & VIP_MASK);
        /* Work around race conditions with signal handler
         * (avoiding sigprocmask for performance reasons) */
        teb->vm86_ptr = &vm86;
        vm86.regs.eflags |= teb->vm86_pending;
        /* Check for VIF|VIP here, since vm86_enter doesn't */
        if ((vm86.regs.eflags & (VIF_MASK|VIP_MASK)) == (VIF_MASK|VIP_MASK)) {
            teb->vm86_ptr = NULL;
            teb->vm86_pending = 0;
            context->EFlags |= VIP_MASK;
            rec.ExceptionCode = EXCEPTION_VM86_STI;
            rec.ExceptionInformation[0] = 0;
            goto cancel_vm86;
        }

        do
        {
            res = vm86_enter( &teb->vm86_ptr ); /* uses and clears teb->vm86_ptr */
            if (res < 0)
            {
                errno = -res;
                return;
            }
        } while (VM86_TYPE(res) == VM86_SIGNAL);

        save_vm86_context( context, &vm86 );
        context->EFlags |= teb->vm86_pending;

        switch(VM86_TYPE(res))
        {
        case VM86_UNKNOWN: /* unhandled GP fault - IO-instruction or similar */
            do_segv( context, T_PROTFLT, 0, 0 );
            continue;
        case VM86_TRAP: /* return due to DOS-debugger request */
            do_trap( context, VM86_ARG(res) );
            continue;
        case VM86_INTx: /* int3/int x instruction (ARG = x) */
            rec.ExceptionCode = EXCEPTION_VM86_INTx;
            break;
        case VM86_STI: /* sti/popf/iret instruction enabled virtual interrupts */
            teb->vm86_pending = 0;
            rec.ExceptionCode = EXCEPTION_VM86_STI;
            break;
        case VM86_PICRETURN: /* return due to pending PIC request */
            rec.ExceptionCode = EXCEPTION_VM86_PICRETURN;
            break;
        default:
            ERR( "unhandled result from vm86 mode %x\n", res );
            continue;
        }
        rec.ExceptionInformation[0] = VM86_ARG(res);
cancel_vm86:
        rec.ExceptionFlags          = EXCEPTION_CONTINUABLE;
        rec.ExceptionRecord         = NULL;
        rec.ExceptionAddress        = (LPVOID)context->Eip;
        rec.NumberParameters        = 1;
        EXC_RtlRaiseException( &rec, context );
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

#endif  /* __i386__ */
