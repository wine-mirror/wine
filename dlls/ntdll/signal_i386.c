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
#ifdef HAVE_SYS_UCONTEXT_H
# include <sys/ucontext.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "wine/library.h"
#include "ntdll_misc.h"
#include "wine/exception.h"
#include "wine/debug.h"

#ifdef HAVE_VALGRIND_MEMCHECK_H
#include <valgrind/memcheck.h>
#endif

#undef ERR  /* Solaris needs to define this */

/* not defined for x86, so copy the x86_64 definition */
typedef struct DECLSPEC_ALIGN(16) _M128A
{
    ULONGLONG Low;
    LONGLONG High;
} M128A;

typedef struct
{
    WORD ControlWord;
    WORD StatusWord;
    BYTE TagWord;
    BYTE Reserved1;
    WORD ErrorOpcode;
    DWORD ErrorOffset;
    WORD ErrorSelector;
    WORD Reserved2;
    DWORD DataOffset;
    WORD DataSelector;
    WORD Reserved3;
    DWORD MxCsr;
    DWORD MxCsr_Mask;
    M128A FloatRegisters[8];
    M128A XmmRegisters[16];
    BYTE Reserved4[96];
} XMM_SAVE_AREA32;

/***********************************************************************
 * signal context platform-specific definitions
 */

#ifdef __linux__

#ifndef HAVE_SYS_UCONTEXT_H

enum
{
    REG_GS, REG_FS, REG_ES, REG_DS, REG_EDI, REG_ESI, REG_EBP, REG_ESP,
    REG_EBX, REG_EDX, REG_ECX, REG_EAX, REG_TRAPNO, REG_ERR, REG_EIP,
    REG_CS, REG_EFL, REG_UESP, REG_SS, NGREG
};

typedef int greg_t;
typedef greg_t gregset_t[NGREG];

struct _libc_fpreg
{
    unsigned short significand[4];
    unsigned short exponent;
};

struct _libc_fpstate
{
    unsigned long cw;
    unsigned long sw;
    unsigned long tag;
    unsigned long ipoff;
    unsigned long cssel;
    unsigned long dataoff;
    unsigned long datasel;
    struct _libc_fpreg _st[8];
    unsigned long status;
};

typedef struct _libc_fpstate* fpregset_t;

typedef struct
{
    gregset_t     gregs;
    fpregset_t    fpregs;
    unsigned long oldmask;
    unsigned long cr2;
} mcontext_t;

typedef struct ucontext
{
    unsigned long     uc_flags;
    struct ucontext  *uc_link;
    stack_t           uc_stack;
    mcontext_t        uc_mcontext;
    sigset_t          uc_sigmask;
} ucontext_t;
#endif /* HAVE_SYS_UCONTEXT_H */

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
#define FPUX_sig(context)    (FPU_sig(context) && !((context)->uc_mcontext.fpregs->status >> 16) ? (XMM_SAVE_AREA32 *)(FPU_sig(context) + 1) : NULL)

#define VM86_EAX 0 /* the %eax value while vm86_enter is executing */
#define VIF_FLAG 0x00080000
#define VIP_FLAG 0x00100000

int vm86_enter( void **vm86_ptr );
void vm86_return(void);
void vm86_return_end(void);
__ASM_GLOBAL_FUNC(vm86_enter,
                  "pushl %ebp\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                  __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                  "movl %esp,%ebp\n\t"
                  __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                  "pushl %ebx\n\t"
                  __ASM_CFI(".cfi_rel_offset %ebx,-4\n\t")
                  "movl $166,%eax\n\t"  /*SYS_vm86*/
                  "movl 8(%ebp),%ecx\n\t" /* vm86_ptr */
                  "movl (%ecx),%ecx\n\t"
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
                  __ASM_CFI(".cfi_same_value %ebx\n\t")
                  "popl %ebp\n\t"
                  __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                  __ASM_CFI(".cfi_same_value %ebp\n\t")
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

#ifdef __ANDROID__
/* custom signal restorer since we may have unmapped the one in vdso, and bionic doesn't check for that */
void rt_sigreturn(void);
__ASM_GLOBAL_FUNC( rt_sigreturn,
                   "movl $173,%eax\n\t"  /* NR_rt_sigreturn */
                   "int $0x80" );
#endif

#elif defined (__BSDI__)

#include <machine/frame.h>
typedef struct trapframe ucontext_t;

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

#define FPU_sig(context)     NULL  /* FIXME */
#define FPUX_sig(context)    NULL  /* FIXME */

#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)

#include <machine/trap.h>

#define EAX_sig(context)     ((context)->uc_mcontext.mc_eax)
#define EBX_sig(context)     ((context)->uc_mcontext.mc_ebx)
#define ECX_sig(context)     ((context)->uc_mcontext.mc_ecx)
#define EDX_sig(context)     ((context)->uc_mcontext.mc_edx)
#define ESI_sig(context)     ((context)->uc_mcontext.mc_esi)
#define EDI_sig(context)     ((context)->uc_mcontext.mc_edi)
#define EBP_sig(context)     ((context)->uc_mcontext.mc_ebp)

#define CS_sig(context)      ((context)->uc_mcontext.mc_cs)
#define DS_sig(context)      ((context)->uc_mcontext.mc_ds)
#define ES_sig(context)      ((context)->uc_mcontext.mc_es)
#define FS_sig(context)      ((context)->uc_mcontext.mc_fs)
#define GS_sig(context)      ((context)->uc_mcontext.mc_gs)
#define SS_sig(context)      ((context)->uc_mcontext.mc_ss)

#define TRAP_sig(context)    ((context)->uc_mcontext.mc_trapno)
#define ERROR_sig(context)   ((context)->uc_mcontext.mc_err)
#define EFL_sig(context)     ((context)->uc_mcontext.mc_eflags)

#define EIP_sig(context)     ((context)->uc_mcontext.mc_eip)
#define ESP_sig(context)     ((context)->uc_mcontext.mc_esp)

#define FPU_sig(context)     NULL  /* FIXME */
#define FPUX_sig(context)    NULL  /* FIXME */

#elif defined (__OpenBSD__)

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

#define FPU_sig(context)     NULL  /* FIXME */
#define FPUX_sig(context)    NULL  /* FIXME */

#define T_MCHK T_MACHK
#define T_XMMFLT T_XFTRAP

#elif defined(__svr4__) || defined(_SCO_DS) || defined(__sun)

#ifdef _SCO_DS
#include <sys/regset.h>
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

#define FPU_sig(context)     NULL  /* FIXME */
#define FPUX_sig(context)    NULL  /* FIXME */

#elif defined (__APPLE__)

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
#define FPU_sig(context)     NULL
#define FPUX_sig(context)    ((XMM_SAVE_AREA32 *)&(context)->uc_mcontext->__fs.__fpu_fcw)
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
#define FPU_sig(context)     NULL
#define FPUX_sig(context)    ((XMM_SAVE_AREA32 *)&(context)->uc_mcontext->fs.fpu_fcw)
#endif

#elif defined(__NetBSD__)

#define EAX_sig(context)       ((context)->uc_mcontext.__gregs[_REG_EAX])
#define EBX_sig(context)       ((context)->uc_mcontext.__gregs[_REG_EBX])
#define ECX_sig(context)       ((context)->uc_mcontext.__gregs[_REG_ECX])
#define EDX_sig(context)       ((context)->uc_mcontext.__gregs[_REG_EDX])
#define ESI_sig(context)       ((context)->uc_mcontext.__gregs[_REG_ESI])
#define EDI_sig(context)       ((context)->uc_mcontext.__gregs[_REG_EDI])
#define EBP_sig(context)       ((context)->uc_mcontext.__gregs[_REG_EBP])
#define ESP_sig(context)       _UC_MACHINE_SP(context)

#define CS_sig(context)        ((context)->uc_mcontext.__gregs[_REG_CS])
#define DS_sig(context)        ((context)->uc_mcontext.__gregs[_REG_DS])
#define ES_sig(context)        ((context)->uc_mcontext.__gregs[_REG_ES])
#define SS_sig(context)        ((context)->uc_mcontext.__gregs[_REG_SS])
#define FS_sig(context)        ((context)->uc_mcontext.__gregs[_REG_FS])
#define GS_sig(context)        ((context)->uc_mcontext.__gregs[_REG_GS])

#define EFL_sig(context)       ((context)->uc_mcontext.__gregs[_REG_EFL])
#define EIP_sig(context)       _UC_MACHINE_PC(context)
#define TRAP_sig(context)      ((context)->uc_mcontext.__gregs[_REG_TRAPNO])
#define ERROR_sig(context)     ((context)->uc_mcontext.__gregs[_REG_ERR])

#define FPU_sig(context)     NULL
#define FPUX_sig(context)    ((XMM_SAVE_AREA32 *)&((context)->uc_mcontext.__fpregs))

#define T_MCHK T_MCA
#define T_XMMFLT T_XMM

#elif defined(__GNU__)

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

#define FPU_sig(context)     ((FLOATING_SAVE_AREA *)&(context)->uc_mcontext.fpregs.fp_reg_set.fpchip_state)
#define FPUX_sig(context)    NULL

#else
#error You must define the signal context functions for your platform
#endif /* linux */

WINE_DEFAULT_DEBUG_CHANNEL(seh);

typedef int (*wine_signal_handler)(unsigned int sig);

static const size_t teb_size = 4096;  /* we reserve one page for the TEB */
static size_t signal_stack_mask;
static size_t signal_stack_size;

static wine_signal_handler handlers[256];

static BOOL fpux_support;  /* whether the CPU supports extended fpu context */

extern void DECLSPEC_NORETURN __wine_restore_regs( const CONTEXT *context );

enum i386_trap_code
{
    TRAP_x86_UNKNOWN    = -1,  /* Unknown fault (TRAP_sig not defined) */
#if defined(__FreeBSD__) || defined (__FreeBSD_kernel__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
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

/* Exception record for handling exceptions happening inside exception handlers */
typedef struct
{
    EXCEPTION_REGISTRATION_RECORD frame;
    EXCEPTION_REGISTRATION_RECORD *prevFrame;
} EXC_NESTED_FRAME;

extern DWORD EXC_CallHandler( EXCEPTION_RECORD *record, EXCEPTION_REGISTRATION_RECORD *frame,
                              CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher,
                              PEXCEPTION_HANDLER handler, PEXCEPTION_HANDLER nested_handler );

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
static inline enum i386_trap_code get_trap_code( const ucontext_t *sigcontext )
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
static inline WORD get_error_code( const ucontext_t *sigcontext )
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


/*******************************************************************
 *         is_valid_frame
 */
static inline BOOL is_valid_frame( void *frame )
{
    if ((ULONG_PTR)frame & 3) return FALSE;
    return (frame >= NtCurrentTeb()->Tib.StackLimit &&
            (void **)frame < (void **)NtCurrentTeb()->Tib.StackBase - 1);
}

/*******************************************************************
 *         raise_handler
 *
 * Handler for exceptions happening inside a handler.
 */
static DWORD raise_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                            CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    if (rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
        return ExceptionContinueSearch;
    /* We shouldn't get here so we store faulty frame in dispatcher */
    *dispatcher = ((EXC_NESTED_FRAME*)frame)->prevFrame;
    return ExceptionNestedException;
}


/*******************************************************************
 *         unwind_handler
 *
 * Handler for exceptions happening inside an unwind handler.
 */
static DWORD unwind_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                             CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    if (!(rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND)))
        return ExceptionContinueSearch;
    /* We shouldn't get here so we store faulty frame in dispatcher */
    *dispatcher = ((EXC_NESTED_FRAME*)frame)->prevFrame;
    return ExceptionCollidedUnwind;
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
        res = EXC_CallHandler( rec, frame, context, &dispatch, frame->Handler, raise_handler );
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

        TRACE( "code=%x flags=%x addr=%p ip=%08x tid=%04x\n",
               rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
               context->Eip, GetCurrentThreadId() );
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
            TRACE(" eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x\n",
                  context->Eax, context->Ebx, context->Ecx,
                  context->Edx, context->Esi, context->Edi );
            TRACE(" ebp=%08x esp=%08x cs=%04x ds=%04x es=%04x fs=%04x gs=%04x flags=%08x\n",
                  context->Ebp, context->Esp, context->SegCs, context->SegDs,
                  context->SegEs, context->SegFs, context->SegGs, context->EFlags );
        }
        status = send_debug_event( rec, TRUE, context );
        if (status == DBG_CONTINUE || status == DBG_EXCEPTION_HANDLED)
            return STATUS_SUCCESS;

        /* fix up instruction pointer in context for EXCEPTION_BREAKPOINT */
        if (rec->ExceptionCode == EXCEPTION_BREAKPOINT) context->Eip--;

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
            WINE_ERR("Exception frame is not in stack limits => unable to dispatch exception.\n");
        else if (rec->ExceptionCode == STATUS_NONCONTINUABLE_EXCEPTION)
            WINE_ERR("Process attempted to continue execution after noncontinuable exception.\n");
        else
            WINE_ERR("Unhandled exception code %x flags %x addr %p\n",
                     rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress );
        NtTerminateProcess( NtCurrentProcess(), rec->ExceptionCode );
    }
    return STATUS_SUCCESS;
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
    while (check_pending && get_vm86_teb_info()->vm86_pending)
    {
        check_pending = FALSE;
        ntdll_get_thread_data()->vm86_ptr = NULL;
            
        /*
         * If VIF is set, throw exception.
         * Note that SIGUSR2 may turn VIF flag off so
         * VIF check must occur only when TEB.vm86_ptr is NULL.
         */
        if (vm86->regs.eflags & VIF_FLAG)
        {
            CONTEXT vcontext;
            save_vm86_context( &vcontext, vm86 );
            
            rec->ExceptionCode    = EXCEPTION_VM86_STI;
            rec->ExceptionFlags   = EXCEPTION_CONTINUABLE;
            rec->ExceptionRecord  = NULL;
            rec->NumberParameters = 0;
            rec->ExceptionAddress = (LPVOID)vcontext.Eip;

            vcontext.EFlags &= ~VIP_FLAG;
            get_vm86_teb_info()->vm86_pending = 0;
            raise_exception( rec, &vcontext, TRUE );

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
    vm86->regs.eflags |= get_vm86_teb_info()->vm86_pending;
}
#endif /* __HAVE_VM86 */


#ifdef __sun

/* We have to workaround two Solaris breakages:
 * - Solaris doesn't restore %ds and %es before calling the signal handler so exceptions in 16-bit
 *   code crash badly.
 * - Solaris inserts a libc trampoline to call our handler, but the trampoline expects that registers
 *   are setup correctly. So we need to insert our own trampoline below the libc trampoline to set %gs.
 */

extern int sigaction_syscall( int sig, const struct sigaction *new, struct sigaction *old );
__ASM_GLOBAL_FUNC( sigaction_syscall,
                  "movl $0x62,%eax\n\t"
                  "int $0x91\n\t"
                  "ret" )

/* assume the same libc handler is used for all signals */
static void (*libc_sigacthandler)( int signal, siginfo_t *siginfo, void *context );

static void wine_sigacthandler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    struct ntdll_thread_data *thread_data;

    __asm__ __volatile__("mov %ss,%ax; mov %ax,%ds; mov %ax,%es");

    thread_data = (struct ntdll_thread_data *)get_current_teb()->SpareBytes1;
    wine_set_fs( thread_data->fs );
    wine_set_gs( thread_data->gs );

    libc_sigacthandler( signal, siginfo, sigcontext );
}

static int solaris_sigaction( int sig, const struct sigaction *new, struct sigaction *old )
{
    struct sigaction real_act;

    if (sigaction( sig, new, old ) == -1) return -1;

    /* retrieve the real handler and flags with a direct syscall */
    sigaction_syscall( sig, NULL, &real_act );
    libc_sigacthandler = real_act.sa_sigaction;
    real_act.sa_sigaction = wine_sigacthandler;
    sigaction_syscall( sig, &real_act, NULL );
    return 0;
}
#define sigaction(sig,new,old) solaris_sigaction(sig,new,old)

#endif

typedef void (WINAPI *raise_func)( EXCEPTION_RECORD *rec, CONTEXT *context );

extern void clear_alignment_flag(void);
__ASM_GLOBAL_FUNC( clear_alignment_flag,
                   "pushfl\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "andl $~0x40000,(%esp)\n\t"
                   "popfl\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset -4\n\t")
                   "ret" )


/***********************************************************************
 *           init_handler
 *
 * Handler initialization when the full context is not needed.
 * Return the stack pointer to use for pushing the exception data.
 */
static inline void *init_handler( const ucontext_t *sigcontext, WORD *fs, WORD *gs )
{
    TEB *teb = get_current_teb();

    clear_alignment_flag();

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

#ifndef __sun  /* see above for Solaris handling */
    {
        struct ntdll_thread_data *thread_data = (struct ntdll_thread_data *)teb->SpareBytes1;
        wine_set_fs( thread_data->fs );
        wine_set_gs( thread_data->gs );
    }
#endif

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
        return teb->WOW32Reserved;
    }
    return (void *)(ESP_sig(sigcontext) & ~3);
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
 *           save_fpux
 *
 * Save the thread FPU extended context.
 */
static inline void save_fpux( CONTEXT *context )
{
#ifdef __GNUC__
    /* we have to enforce alignment by hand */
    char buffer[sizeof(XMM_SAVE_AREA32) + 16];
    XMM_SAVE_AREA32 *state = (XMM_SAVE_AREA32 *)(((ULONG_PTR)buffer + 15) & ~15);

    __asm__ __volatile__( "fxsave %0" : "=m" (*state) );
    context->ContextFlags |= CONTEXT_EXTENDED_REGISTERS;
    memcpy( context->ExtendedRegisters, state, sizeof(*state) );
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
 *           restore_fpux
 *
 * Restore the FPU extended context to a sigcontext.
 */
static inline void restore_fpux( const CONTEXT *context )
{
#ifdef __GNUC__
    /* we have to enforce alignment by hand */
    char buffer[sizeof(XMM_SAVE_AREA32) + 16];
    XMM_SAVE_AREA32 *state = (XMM_SAVE_AREA32 *)(((ULONG_PTR)buffer + 15) & ~15);

    memcpy( state, context->ExtendedRegisters, sizeof(*state) );
    /* reset the current interrupt status */
    state->StatusWord &= state->ControlWord | 0xff80;
    __asm__ __volatile__( "fxrstor %0" : : "m" (*state) );
#endif
}


/***********************************************************************
 *           fpux_to_fpu
 *
 * Build a standard FPU context from an extended one.
 */
static void fpux_to_fpu( FLOATING_SAVE_AREA *fpu, const XMM_SAVE_AREA32 *fpux )
{
    unsigned int i, tag, stack_top;

    fpu->ControlWord   = fpux->ControlWord | 0xffff0000;
    fpu->StatusWord    = fpux->StatusWord | 0xffff0000;
    fpu->ErrorOffset   = fpux->ErrorOffset;
    fpu->ErrorSelector = fpux->ErrorSelector | (fpux->ErrorOpcode << 16);
    fpu->DataOffset    = fpux->DataOffset;
    fpu->DataSelector  = fpux->DataSelector;
    fpu->Cr0NpxState   = fpux->StatusWord | 0xffff0000;

    stack_top = (fpux->StatusWord >> 11) & 7;
    fpu->TagWord = 0xffff0000;
    for (i = 0; i < 8; i++)
    {
        memcpy( &fpu->RegisterArea[10 * i], &fpux->FloatRegisters[i], 10 );
        if (!(fpux->TagWord & (1 << i))) tag = 3;  /* empty */
        else
        {
            const M128A *reg = &fpux->FloatRegisters[(i - stack_top) & 7];
            if ((reg->High & 0x7fff) == 0x7fff)  /* exponent all ones */
            {
                tag = 2;  /* special */
            }
            else if (!(reg->High & 0x7fff))  /* exponent all zeroes */
            {
                if (reg->Low) tag = 2;  /* special */
                else tag = 1;  /* zero */
            }
            else
            {
                if (reg->Low >> 63) tag = 0;  /* valid */
                else tag = 2;  /* special */
            }
        }
        fpu->TagWord |= tag << (2 * i);
    }
}


/***********************************************************************
 *           save_context
 *
 * Build a context structure from the signal info.
 */
static inline void save_context( CONTEXT *context, const ucontext_t *sigcontext, WORD fs, WORD gs )
{
    struct ntdll_thread_data * const regs = ntdll_get_thread_data();
    FLOATING_SAVE_AREA *fpu = FPU_sig(sigcontext);
    XMM_SAVE_AREA32 *fpux = FPUX_sig(sigcontext);

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

    if (fpu)
    {
        context->ContextFlags |= CONTEXT_FLOATING_POINT;
        context->FloatSave = *fpu;
    }
    if (fpux)
    {
        context->ContextFlags |= CONTEXT_FLOATING_POINT | CONTEXT_EXTENDED_REGISTERS;
        memcpy( context->ExtendedRegisters, fpux, sizeof(*fpux) );
        fpux_support = TRUE;
        if (!fpu) fpux_to_fpu( &context->FloatSave, fpux );
    }
    if (!fpu && !fpux) save_fpu( context );
}


/***********************************************************************
 *           restore_context
 *
 * Restore the signal info from the context.
 */
static inline void restore_context( const CONTEXT *context, ucontext_t *sigcontext )
{
    struct ntdll_thread_data * const regs = ntdll_get_thread_data();
    FLOATING_SAVE_AREA *fpu = FPU_sig(sigcontext);
    XMM_SAVE_AREA32 *fpux = FPUX_sig(sigcontext);

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

    if (fpu) *fpu = context->FloatSave;
    if (fpux) memcpy( fpux, context->ExtendedRegisters, sizeof(*fpux) );
    if (!fpu && !fpux) restore_fpu( context );
}


/***********************************************************************
 *		RtlCaptureContext (NTDLL.@)
 */
__ASM_STDCALL_FUNC( RtlCaptureContext, 4,
                    "pushl %eax\n\t"
                    __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                    "movl 8(%esp),%eax\n\t"    /* context */
                    "movl $0x10007,(%eax)\n\t" /* context->ContextFlags */
                    "movw %gs,0x8c(%eax)\n\t"  /* context->SegGs */
                    "movw %fs,0x90(%eax)\n\t"  /* context->SegFs */
                    "movw %es,0x94(%eax)\n\t"  /* context->SegEs */
                    "movw %ds,0x98(%eax)\n\t"  /* context->SegDs */
                    "movl %edi,0x9c(%eax)\n\t" /* context->Edi */
                    "movl %esi,0xa0(%eax)\n\t" /* context->Esi */
                    "movl %ebx,0xa4(%eax)\n\t" /* context->Ebx */
                    "movl %edx,0xa8(%eax)\n\t" /* context->Edx */
                    "movl %ecx,0xac(%eax)\n\t" /* context->Ecx */
                    "movl %ebp,0xb4(%eax)\n\t" /* context->Ebp */
                    "movl 4(%esp),%edx\n\t"
                    "movl %edx,0xb8(%eax)\n\t" /* context->Eip */
                    "movw %cs,0xbc(%eax)\n\t"  /* context->SegCs */
                    "pushfl\n\t"
                    __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                    "popl 0xc0(%eax)\n\t"      /* context->EFlags */
                    __ASM_CFI(".cfi_adjust_cfa_offset -4\n\t")
                    "leal 8(%esp),%edx\n\t"
                    "movl %edx,0xc4(%eax)\n\t" /* context->Esp */
                    "movw %ss,0xc8(%eax)\n\t"  /* context->SegSs */
                    "popl 0xb0(%eax)\n\t"      /* context->Eax */
                    __ASM_CFI(".cfi_adjust_cfa_offset -4\n\t")
                    "ret $4" )


/***********************************************************************
 *           set_cpu_context
 *
 * Set the new CPU context. Used by NtSetContextThread.
 */
void set_cpu_context( const CONTEXT *context )
{
    DWORD flags = context->ContextFlags & ~CONTEXT_i386;

    if ((flags & CONTEXT_EXTENDED_REGISTERS) && fpux_support) restore_fpux( context );
    else if (flags & CONTEXT_FLOATING_POINT) restore_fpu( context );

    if (flags & CONTEXT_DEBUG_REGISTERS)
    {
        ntdll_get_thread_data()->dr0 = context->Dr0;
        ntdll_get_thread_data()->dr1 = context->Dr1;
        ntdll_get_thread_data()->dr2 = context->Dr2;
        ntdll_get_thread_data()->dr3 = context->Dr3;
        ntdll_get_thread_data()->dr6 = context->Dr6;
        ntdll_get_thread_data()->dr7 = context->Dr7;
    }
    if (flags & CONTEXT_FULL)
    {
        if (!(flags & CONTEXT_CONTROL))
            FIXME( "setting partial context (%x) not supported\n", flags );
        else if (flags & CONTEXT_SEGMENTS)
            __wine_restore_regs( context );
        else
        {
            CONTEXT newcontext = *context;
            newcontext.SegDs = wine_get_ds();
            newcontext.SegEs = wine_get_es();
            newcontext.SegFs = wine_get_fs();
            newcontext.SegGs = wine_get_gs();
            __wine_restore_regs( &newcontext );
        }
    }
}


/***********************************************************************
 *           set_debug_registers
 */
static void set_debug_registers( const CONTEXT *context )
{
    DWORD flags = context->ContextFlags & ~CONTEXT_i386;
    context_t server_context;

    if (!(flags & CONTEXT_DEBUG_REGISTERS)) return;
    if (ntdll_get_thread_data()->dr0 == context->Dr0 &&
        ntdll_get_thread_data()->dr1 == context->Dr1 &&
        ntdll_get_thread_data()->dr2 == context->Dr2 &&
        ntdll_get_thread_data()->dr3 == context->Dr3 &&
        ntdll_get_thread_data()->dr6 == context->Dr6 &&
        ntdll_get_thread_data()->dr7 == context->Dr7) return;

    context_to_server( &server_context, context );

    SERVER_START_REQ( set_thread_context )
    {
        req->handle  = wine_server_obj_handle( GetCurrentThread() );
        req->suspend = 0;
        wine_server_add_data( req, &server_context, sizeof(server_context) );
        wine_server_call( req );
    }
    SERVER_END_REQ;
}


/***********************************************************************
 *           copy_context
 *
 * Copy a register context according to the flags.
 */
void copy_context( CONTEXT *to, const CONTEXT *from, DWORD flags )
{
    flags &= ~CONTEXT_i386;  /* get rid of CPU id */
    if (flags & CONTEXT_INTEGER)
    {
        to->Eax = from->Eax;
        to->Ebx = from->Ebx;
        to->Ecx = from->Ecx;
        to->Edx = from->Edx;
        to->Esi = from->Esi;
        to->Edi = from->Edi;
    }
    if (flags & CONTEXT_CONTROL)
    {
        to->Ebp    = from->Ebp;
        to->Esp    = from->Esp;
        to->Eip    = from->Eip;
        to->SegCs  = from->SegCs;
        to->SegSs  = from->SegSs;
        to->EFlags = from->EFlags;
    }
    if (flags & CONTEXT_SEGMENTS)
    {
        to->SegDs = from->SegDs;
        to->SegEs = from->SegEs;
        to->SegFs = from->SegFs;
        to->SegGs = from->SegGs;
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
    if (flags & CONTEXT_FLOATING_POINT)
    {
        to->FloatSave = from->FloatSave;
    }
    if (flags & CONTEXT_EXTENDED_REGISTERS)
    {
        memcpy( to->ExtendedRegisters, from->ExtendedRegisters, sizeof(to->ExtendedRegisters) );
    }
}


/***********************************************************************
 *           context_to_server
 *
 * Convert a register context to the server format.
 */
NTSTATUS context_to_server( context_t *to, const CONTEXT *from )
{
    DWORD flags = from->ContextFlags & ~CONTEXT_i386;  /* get rid of CPU id */

    memset( to, 0, sizeof(*to) );
    to->cpu = CPU_x86;

    if (flags & CONTEXT_CONTROL)
    {
        to->flags |= SERVER_CTX_CONTROL;
        to->ctl.i386_regs.ebp    = from->Ebp;
        to->ctl.i386_regs.esp    = from->Esp;
        to->ctl.i386_regs.eip    = from->Eip;
        to->ctl.i386_regs.cs     = from->SegCs;
        to->ctl.i386_regs.ss     = from->SegSs;
        to->ctl.i386_regs.eflags = from->EFlags;
    }
    if (flags & CONTEXT_INTEGER)
    {
        to->flags |= SERVER_CTX_INTEGER;
        to->integer.i386_regs.eax = from->Eax;
        to->integer.i386_regs.ebx = from->Ebx;
        to->integer.i386_regs.ecx = from->Ecx;
        to->integer.i386_regs.edx = from->Edx;
        to->integer.i386_regs.esi = from->Esi;
        to->integer.i386_regs.edi = from->Edi;
    }
    if (flags & CONTEXT_SEGMENTS)
    {
        to->flags |= SERVER_CTX_SEGMENTS;
        to->seg.i386_regs.ds = from->SegDs;
        to->seg.i386_regs.es = from->SegEs;
        to->seg.i386_regs.fs = from->SegFs;
        to->seg.i386_regs.gs = from->SegGs;
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
        to->flags |= SERVER_CTX_FLOATING_POINT;
        to->fp.i386_regs.ctrl     = from->FloatSave.ControlWord;
        to->fp.i386_regs.status   = from->FloatSave.StatusWord;
        to->fp.i386_regs.tag      = from->FloatSave.TagWord;
        to->fp.i386_regs.err_off  = from->FloatSave.ErrorOffset;
        to->fp.i386_regs.err_sel  = from->FloatSave.ErrorSelector;
        to->fp.i386_regs.data_off = from->FloatSave.DataOffset;
        to->fp.i386_regs.data_sel = from->FloatSave.DataSelector;
        to->fp.i386_regs.cr0npx   = from->FloatSave.Cr0NpxState;
        memcpy( to->fp.i386_regs.regs, from->FloatSave.RegisterArea, sizeof(to->fp.i386_regs.regs) );
    }
    if (flags & CONTEXT_DEBUG_REGISTERS)
    {
        to->flags |= SERVER_CTX_DEBUG_REGISTERS;
        to->debug.i386_regs.dr0 = from->Dr0;
        to->debug.i386_regs.dr1 = from->Dr1;
        to->debug.i386_regs.dr2 = from->Dr2;
        to->debug.i386_regs.dr3 = from->Dr3;
        to->debug.i386_regs.dr6 = from->Dr6;
        to->debug.i386_regs.dr7 = from->Dr7;
    }
    if (flags & CONTEXT_EXTENDED_REGISTERS)
    {
        to->flags |= SERVER_CTX_EXTENDED_REGISTERS;
        memcpy( to->ext.i386_regs, from->ExtendedRegisters, sizeof(to->ext.i386_regs) );
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
    if (from->cpu != CPU_x86) return STATUS_INVALID_PARAMETER;

    to->ContextFlags = CONTEXT_i386;
    if (from->flags & SERVER_CTX_CONTROL)
    {
        to->ContextFlags |= CONTEXT_CONTROL;
        to->Ebp    = from->ctl.i386_regs.ebp;
        to->Esp    = from->ctl.i386_regs.esp;
        to->Eip    = from->ctl.i386_regs.eip;
        to->SegCs  = from->ctl.i386_regs.cs;
        to->SegSs  = from->ctl.i386_regs.ss;
        to->EFlags = from->ctl.i386_regs.eflags;
    }
    if (from->flags & SERVER_CTX_INTEGER)
    {
        to->ContextFlags |= CONTEXT_INTEGER;
        to->Eax = from->integer.i386_regs.eax;
        to->Ebx = from->integer.i386_regs.ebx;
        to->Ecx = from->integer.i386_regs.ecx;
        to->Edx = from->integer.i386_regs.edx;
        to->Esi = from->integer.i386_regs.esi;
        to->Edi = from->integer.i386_regs.edi;
    }
    if (from->flags & SERVER_CTX_SEGMENTS)
    {
        to->ContextFlags |= CONTEXT_SEGMENTS;
        to->SegDs = from->seg.i386_regs.ds;
        to->SegEs = from->seg.i386_regs.es;
        to->SegFs = from->seg.i386_regs.fs;
        to->SegGs = from->seg.i386_regs.gs;
    }
    if (from->flags & SERVER_CTX_FLOATING_POINT)
    {
        to->ContextFlags |= CONTEXT_FLOATING_POINT;
        to->FloatSave.ControlWord   = from->fp.i386_regs.ctrl;
        to->FloatSave.StatusWord    = from->fp.i386_regs.status;
        to->FloatSave.TagWord       = from->fp.i386_regs.tag;
        to->FloatSave.ErrorOffset   = from->fp.i386_regs.err_off;
        to->FloatSave.ErrorSelector = from->fp.i386_regs.err_sel;
        to->FloatSave.DataOffset    = from->fp.i386_regs.data_off;
        to->FloatSave.DataSelector  = from->fp.i386_regs.data_sel;
        to->FloatSave.Cr0NpxState   = from->fp.i386_regs.cr0npx;
        memcpy( to->FloatSave.RegisterArea, from->fp.i386_regs.regs, sizeof(to->FloatSave.RegisterArea) );
    }
    if (from->flags & SERVER_CTX_DEBUG_REGISTERS)
    {
        to->ContextFlags |= CONTEXT_DEBUG_REGISTERS;
        to->Dr0 = from->debug.i386_regs.dr0;
        to->Dr1 = from->debug.i386_regs.dr1;
        to->Dr2 = from->debug.i386_regs.dr2;
        to->Dr3 = from->debug.i386_regs.dr3;
        to->Dr6 = from->debug.i386_regs.dr6;
        to->Dr7 = from->debug.i386_regs.dr7;
    }
    if (from->flags & SERVER_CTX_EXTENDED_REGISTERS)
    {
        to->ContextFlags |= CONTEXT_EXTENDED_REGISTERS;
        memcpy( to->ExtendedRegisters, from->ext.i386_regs, sizeof(to->ExtendedRegisters) );
    }
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           is_privileged_instr
 *
 * Check if the fault location is a privileged instruction.
 * Based on the instruction emulation code in dlls/kernel/instr.c.
 */
static inline DWORD is_privileged_instr( CONTEXT *context )
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

/***********************************************************************
 *           check_invalid_gs
 *
 * Check for fault caused by invalid %gs value (some copy protection schemes mess with it).
 */
static inline BOOL check_invalid_gs( CONTEXT *context )
{
    unsigned int prefix_count = 0;
    const BYTE *instr = (BYTE *)context->Eip;
    WORD system_gs = ntdll_get_thread_data()->gs;

    if (context->SegGs == system_gs) return FALSE;
    if (!wine_ldt_is_system( context->SegCs )) return FALSE;
    /* only handle faults in system libraries */
    if (virtual_is_valid_code_address( instr, 1 )) return FALSE;

    for (;;) switch(*instr)
    {
    /* instruction prefixes */
    case 0x2e:  /* %cs: */
    case 0x36:  /* %ss: */
    case 0x3e:  /* %ds: */
    case 0x26:  /* %es: */
    case 0x64:  /* %fs: */
    case 0x66:  /* opcode size */
    case 0x67:  /* addr size */
    case 0xf0:  /* lock */
    case 0xf2:  /* repne */
    case 0xf3:  /* repe */
        if (++prefix_count >= 15) return FALSE;
        instr++;
        continue;
    case 0x65:  /* %gs: */
        TRACE( "%04x/%04x at %p, fixing up\n", context->SegGs, system_gs, instr );
        context->SegGs = system_gs;
        return TRUE;
    default:
        return FALSE;
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

    if (!virtual_is_valid_code_address( thunk, sizeof(*thunk) )) return FALSE;

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
 *           setup_exception_record
 *
 * Setup the exception record and context on the thread stack.
 */
static EXCEPTION_RECORD *setup_exception_record( ucontext_t *sigcontext, void *stack_ptr,
                                                 WORD fs, WORD gs, raise_func func )
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
    } *stack = stack_ptr;
    DWORD exception_code = 0;

    /* stack sanity checks */

    if ((char *)stack >= (char *)get_signal_stack() &&
        (char *)stack < (char *)get_signal_stack() + signal_stack_size)
    {
        WINE_ERR( "nested exception on signal stack in thread %04x eip %08x esp %08x stack %p-%p\n",
                  GetCurrentThreadId(), (unsigned int) EIP_sig(sigcontext),
                  (unsigned int) ESP_sig(sigcontext), NtCurrentTeb()->Tib.StackLimit,
                  NtCurrentTeb()->Tib.StackBase );
        abort_thread(1);
    }

    if (stack - 1 > stack || /* check for overflow in subtraction */
        (char *)stack <= (char *)NtCurrentTeb()->DeallocationStack ||
        (char *)stack > (char *)NtCurrentTeb()->Tib.StackBase)
    {
        WARN( "exception outside of stack limits in thread %04x eip %08x esp %08x stack %p-%p\n",
              GetCurrentThreadId(), (unsigned int) EIP_sig(sigcontext),
              (unsigned int) ESP_sig(sigcontext), NtCurrentTeb()->Tib.StackLimit,
              NtCurrentTeb()->Tib.StackBase );
    }
    else if ((char *)(stack - 1) < (char *)NtCurrentTeb()->DeallocationStack + 4096)
    {
        /* stack overflow on last page, unrecoverable */
        UINT diff = (char *)NtCurrentTeb()->DeallocationStack + 4096 - (char *)(stack - 1);
        WINE_ERR( "stack overflow %u bytes in thread %04x eip %08x esp %08x stack %p-%p-%p\n",
                  diff, GetCurrentThreadId(), (unsigned int) EIP_sig(sigcontext),
                  (unsigned int) ESP_sig(sigcontext), NtCurrentTeb()->DeallocationStack,
                  NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
        abort_thread(1);
    }
    else if ((char *)(stack - 1) < (char *)NtCurrentTeb()->Tib.StackLimit)
    {
        /* stack access below stack limit, may be recoverable */
        if (virtual_handle_stack_fault( stack - 1 )) exception_code = EXCEPTION_STACK_OVERFLOW;
        else
        {
            UINT diff = (char *)NtCurrentTeb()->Tib.StackLimit - (char *)(stack - 1);
            WINE_ERR( "stack overflow %u bytes in thread %04x eip %08x esp %08x stack %p-%p-%p\n",
                      diff, GetCurrentThreadId(), (unsigned int) EIP_sig(sigcontext),
                      (unsigned int) ESP_sig(sigcontext), NtCurrentTeb()->DeallocationStack,
                      NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
            abort_thread(1);
        }
    }

    stack--;  /* push the stack_layout structure */
#if defined(VALGRIND_MAKE_MEM_UNDEFINED)
    VALGRIND_MAKE_MEM_UNDEFINED(stack, sizeof(*stack));
#elif defined(VALGRIND_MAKE_WRITABLE)
    VALGRIND_MAKE_WRITABLE(stack, sizeof(*stack));
#endif
    stack->ret_addr     = (void *)0xdeadbabe;  /* raise_func must not return */
    stack->rec_ptr      = &stack->rec;
    stack->context_ptr  = &stack->context;

    stack->rec.ExceptionRecord  = NULL;
    stack->rec.ExceptionCode    = exception_code;
    stack->rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    stack->rec.ExceptionAddress = (LPVOID)EIP_sig(sigcontext);
    stack->rec.NumberParameters = 0;

    save_context( &stack->context, sigcontext, fs, gs );

    /* now modify the sigcontext to return to the raise function */
    ESP_sig(sigcontext) = (DWORD)stack;
    EIP_sig(sigcontext) = (DWORD)func;
    /* clear single-step, direction, and align check flag */
    EFL_sig(sigcontext) &= ~(0x100|0x400|0x40000);
    CS_sig(sigcontext)  = wine_get_cs();
    DS_sig(sigcontext)  = wine_get_ds();
    ES_sig(sigcontext)  = wine_get_es();
    FS_sig(sigcontext)  = wine_get_fs();
    GS_sig(sigcontext)  = wine_get_gs();
    SS_sig(sigcontext)  = wine_get_ss();

    return stack->rec_ptr;
}


/***********************************************************************
 *           setup_exception
 *
 * Setup a proper stack frame for the raise function, and modify the
 * sigcontext so that the return from the signal handler will call
 * the raise function.
 */
static EXCEPTION_RECORD *setup_exception( ucontext_t *sigcontext, raise_func func )
{
    WORD fs, gs;
    void *stack = init_handler( sigcontext, &fs, &gs );

    return setup_exception_record( sigcontext, stack, fs, gs, func );
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
    DWORD status = context->FloatSave.StatusWord & ~(context->FloatSave.ControlWord & 0x3f);

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
    NTSTATUS status;

    switch(rec->ExceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION:
        if (rec->NumberParameters == 2)
        {
            if (rec->ExceptionInformation[0] == EXCEPTION_EXECUTE_FAULT && check_atl_thunk( rec, context ))
                goto done;
            if (rec->ExceptionInformation[1] == 0xffffffff && check_invalid_gs( context ))
                goto done;
            if (!(rec->ExceptionCode = virtual_handle_fault( (void *)rec->ExceptionInformation[1],
                                                             rec->ExceptionInformation[0] )))
                goto done;
            /* send EXCEPTION_EXECUTE_FAULT only if data execution prevention is enabled */
            if (rec->ExceptionInformation[0] == EXCEPTION_EXECUTE_FAULT)
            {
                ULONG flags;
                NtQueryInformationProcess( GetCurrentProcess(), ProcessExecuteFlags,
                                           &flags, sizeof(flags), NULL );
                if (!(flags & MEM_EXECUTE_OPTION_DISABLE))
                    rec->ExceptionInformation[0] = EXCEPTION_READ_FAULT;
            }
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
    status = NtRaiseException( rec, context, TRUE );
    raise_status( status, rec );
done:
    set_cpu_context( context );
}


/**********************************************************************
 *		raise_trap_exception
 */
static void WINAPI raise_trap_exception( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    NTSTATUS status;

    if (rec->ExceptionCode == EXCEPTION_SINGLE_STEP)
    {
        /* when single stepping can't tell whether this is a hw bp or a
         * single step interrupt. try to avoid as much overhead as possible
         * and only do a server call if there is any hw bp enabled. */

        if( !(context->EFlags & 0x100) || (ntdll_get_thread_data()->dr7 & 0xff) )
        {
            /* (possible) hardware breakpoint, fetch the debug registers */
            DWORD saved_flags = context->ContextFlags;
            context->ContextFlags = CONTEXT_DEBUG_REGISTERS;
            NtGetContextThread(GetCurrentThread(), context);
            context->ContextFlags |= saved_flags;  /* restore flags */
        }

        context->EFlags &= ~0x100;  /* clear single-step flag */
    }

    status = NtRaiseException( rec, context, TRUE );
    raise_status( status, rec );
}


/**********************************************************************
 *		raise_generic_exception
 *
 * Generic raise function for exceptions that don't need special treatment.
 */
static void WINAPI raise_generic_exception( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    NTSTATUS status;

    status = NtRaiseException( rec, context, TRUE );
    raise_status( status, rec );
}


#ifdef __HAVE_VM86
/**********************************************************************
 *		raise_vm86_sti_exception
 */
static void WINAPI raise_vm86_sti_exception( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    /* merge_vm86_pending_flags merges the vm86_pending flag in safely */
    get_vm86_teb_info()->vm86_pending |= VIP_FLAG;

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
    else if (get_vm86_teb_info()->dpmi_vif &&
             !wine_ldt_is_system(context->SegCs) &&
             !wine_ldt_is_system(context->SegSs))
    {
        /* Executing DPMI code and virtual interrupts are enabled. */
        get_vm86_teb_info()->vm86_pending = 0;
        NtRaiseException( rec, context, TRUE );
    }
done:
    set_cpu_context( context );
}


/**********************************************************************
 *		usr2_handler
 *
 * Handler for SIGUSR2.
 * We use it to signal that the running __wine_enter_vm86() should
 * immediately set VIP_FLAG, causing pending events to be handled
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
    WORD fs, gs;
    EXCEPTION_RECORD *rec;
    ucontext_t *context = sigcontext;
    void *stack = init_handler( sigcontext, &fs, &gs );

    /* check for page fault inside the thread stack */
    if (get_trap_code(context) == TRAP_x86_PAGEFLT &&
        (char *)siginfo->si_addr >= (char *)NtCurrentTeb()->DeallocationStack &&
        (char *)siginfo->si_addr < (char *)NtCurrentTeb()->Tib.StackBase &&
        virtual_handle_stack_fault( siginfo->si_addr ))
    {
        /* check if this was the last guard page */
        if ((char *)siginfo->si_addr < (char *)NtCurrentTeb()->DeallocationStack + 2*4096)
        {
            rec = setup_exception_record( context, stack, fs, gs, raise_segv_exception );
            rec->ExceptionCode = EXCEPTION_STACK_OVERFLOW;
        }
        return;
    }

    rec = setup_exception_record( context, stack, fs, gs, raise_segv_exception );
    if (rec->ExceptionCode == EXCEPTION_STACK_OVERFLOW) return;

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
    ucontext_t *context = sigcontext;
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
    ucontext_t *context = sigcontext;
    EXCEPTION_RECORD *rec = setup_exception( context, raise_generic_exception );

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
        rec->ExceptionAddress = (LPVOID)win_context->FloatSave.ErrorOffset;
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
        /* no idea what meaning is actually behind this but that's what native does */
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
        EXCEPTION_RECORD *rec = setup_exception( sigcontext, raise_generic_exception );
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
    EXCEPTION_RECORD *rec = setup_exception( sigcontext, raise_generic_exception );
    rec->ExceptionCode  = EXCEPTION_WINE_ASSERTION;
    rec->ExceptionFlags = EH_NONCONTINUABLE;
}


/**********************************************************************
 *		quit_handler
 *
 * Handler for SIGQUIT.
 */
static void quit_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    WORD fs, gs;
    init_handler( sigcontext, &fs, &gs );
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
    WORD fs, gs;

    init_handler( sigcontext, &fs, &gs );
    save_context( &context, sigcontext, fs, gs );
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


/***********************************************************************
 *           locking for LDT routines
 */
static RTL_CRITICAL_SECTION ldt_section;
static RTL_CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &ldt_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": ldt_section") }
};
static RTL_CRITICAL_SECTION ldt_section = { &critsect_debug, -1, 0, 0, 0, 0 };
static sigset_t ldt_sigset;

static void ldt_lock(void)
{
    sigset_t sigset;

    pthread_sigmask( SIG_BLOCK, &server_block_set, &sigset );
    RtlEnterCriticalSection( &ldt_section );
    if (ldt_section.RecursionCount == 1) ldt_sigset = sigset;
}

static void ldt_unlock(void)
{
    if (ldt_section.RecursionCount == 1)
    {
        sigset_t sigset = ldt_sigset;
        RtlLeaveCriticalSection( &ldt_section );
        pthread_sigmask( SIG_SETMASK, &sigset, NULL );
    }
    else RtlLeaveCriticalSection( &ldt_section );
}


/**********************************************************************
 *		signal_alloc_thread
 */
NTSTATUS signal_alloc_thread( TEB **teb )
{
    static size_t sigstack_zero_bits;
    struct ntdll_thread_data *thread_data;
    struct ntdll_thread_data *parent_data = NULL;
    SIZE_T size;
    void *addr = NULL;
    NTSTATUS status;

    if (!sigstack_zero_bits)
    {
        size_t min_size = teb_size + max( MINSIGSTKSZ, 8192 );
        /* find the first power of two not smaller than min_size */
        sigstack_zero_bits = 12;
        while ((1u << sigstack_zero_bits) < min_size) sigstack_zero_bits++;
        signal_stack_mask = (1 << sigstack_zero_bits) - 1;
        signal_stack_size = (1 << sigstack_zero_bits) - teb_size;
    }
    else parent_data = ntdll_get_thread_data();

    size = signal_stack_mask + 1;
    if (!(status = NtAllocateVirtualMemory( NtCurrentProcess(), &addr, sigstack_zero_bits,
                                            &size, MEM_COMMIT | MEM_TOP_DOWN, PAGE_READWRITE )))
    {
        *teb = addr;
        (*teb)->Tib.Self = &(*teb)->Tib;
        (*teb)->Tib.ExceptionList = (void *)~0UL;
        thread_data = (struct ntdll_thread_data *)(*teb)->SpareBytes1;
        if (!(thread_data->fs = wine_ldt_alloc_fs()))
        {
            size = 0;
            NtFreeVirtualMemory( NtCurrentProcess(), &addr, &size, MEM_RELEASE );
            status = STATUS_TOO_MANY_THREADS;
        }
        if (parent_data)
        {
            /* inherit debug registers from parent thread */
            thread_data->dr0 = parent_data->dr0;
            thread_data->dr1 = parent_data->dr1;
            thread_data->dr2 = parent_data->dr2;
            thread_data->dr3 = parent_data->dr3;
            thread_data->dr6 = parent_data->dr6;
            thread_data->dr7 = parent_data->dr7;
        }

    }
    return status;
}


/**********************************************************************
 *		signal_free_thread
 */
void signal_free_thread( TEB *teb )
{
    SIZE_T size;
    struct ntdll_thread_data *thread_data = (struct ntdll_thread_data *)teb->SpareBytes1;

    if (thread_data) wine_ldt_free_fs( thread_data->fs );
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
    const WORD fpu_cw = 0x27f;
    struct ntdll_thread_data *thread_data = (struct ntdll_thread_data *)teb->SpareBytes1;
    LDT_ENTRY fs_entry;
    stack_t ss;

#ifdef __APPLE__
    int mib[2], val = 1;

    mib[0] = CTL_KERN;
    mib[1] = KERN_THALTSTACK;
    sysctl( mib, 2, NULL, NULL, &val, sizeof(val) );
#endif

    ss.ss_sp    = (char *)teb + teb_size;
    ss.ss_size  = signal_stack_size;
    ss.ss_flags = 0;
    if (sigaltstack(&ss, NULL) == -1) perror( "sigaltstack" );

    wine_ldt_set_base( &fs_entry, teb );
    wine_ldt_set_limit( &fs_entry, teb_size - 1 );
    wine_ldt_set_flags( &fs_entry, WINE_LDT_FLAGS_DATA|WINE_LDT_FLAGS_32BIT );
    wine_ldt_init_fs( thread_data->fs, &fs_entry );
    thread_data->gs = wine_get_gs();

#ifdef __GNUC__
    __asm__ volatile ("fninit; fldcw %0" : : "m" (fpu_cw));
#else
    FIXME("FPU setup not implemented for this platform.\n");
#endif
}

/**********************************************************************
 *		signal_init_process
 */
void signal_init_process(void)
{
    struct sigaction sig_act;

    sig_act.sa_mask = server_block_set;
    sig_act.sa_flags = SA_SIGINFO | SA_RESTART;
#ifdef SA_ONSTACK
    sig_act.sa_flags |= SA_ONSTACK;
#endif
#ifdef __ANDROID__
    sig_act.sa_flags |= SA_RESTORER;
    sig_act.sa_restorer = rt_sigreturn;
#endif
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

#ifdef __HAVE_VM86
    sig_act.sa_sigaction = usr2_handler;
    if (sigaction( SIGUSR2, &sig_act, NULL ) == -1) goto error;
#endif

    wine_ldt_init_locking( ldt_lock, ldt_unlock );
    return;

 error:
    perror("sigaction");
    exit(1);
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
            context->EFlags |= VIF_FLAG;
            context->EFlags &= ~VIP_FLAG;
            get_vm86_teb_info()->vm86_pending = 0;
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
        raise_exception( &rec, context, TRUE );
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


/*******************************************************************
 *		RtlUnwind (NTDLL.@)
 */
void WINAPI __regs_RtlUnwind( EXCEPTION_REGISTRATION_RECORD* pEndFrame, PVOID targetIp,
                              PEXCEPTION_RECORD pRecord, PVOID retval, CONTEXT *context )
{
    EXCEPTION_RECORD record;
    EXCEPTION_REGISTRATION_RECORD *frame, *dispatch;
    DWORD res;

    context->Eax = (DWORD)retval;

    /* build an exception record, if we do not have one */
    if (!pRecord)
    {
        record.ExceptionCode    = STATUS_UNWIND;
        record.ExceptionFlags   = 0;
        record.ExceptionRecord  = NULL;
        record.ExceptionAddress = (void *)context->Eip;
        record.NumberParameters = 0;
        pRecord = &record;
    }

    pRecord->ExceptionFlags |= EH_UNWINDING | (pEndFrame ? 0 : EH_EXIT_UNWIND);

    TRACE( "code=%x flags=%x\n", pRecord->ExceptionCode, pRecord->ExceptionFlags );

    /* get chain of exception frames */
    frame = NtCurrentTeb()->Tib.ExceptionList;
    while ((frame != (EXCEPTION_REGISTRATION_RECORD*)~0UL) && (frame != pEndFrame))
    {
        /* Check frame address */
        if (pEndFrame && (frame > pEndFrame))
            raise_status( STATUS_INVALID_UNWIND_TARGET, pRecord );

        if (!is_valid_frame( frame )) raise_status( STATUS_BAD_STACK, pRecord );

        /* Call handler */
        TRACE( "calling handler at %p code=%x flags=%x\n",
               frame->Handler, pRecord->ExceptionCode, pRecord->ExceptionFlags );
        res = EXC_CallHandler( pRecord, frame, context, &dispatch, frame->Handler, unwind_handler );
        TRACE( "handler at %p returned %x\n", frame->Handler, res );

        switch(res)
        {
        case ExceptionContinueSearch:
            break;
        case ExceptionCollidedUnwind:
            frame = dispatch;
            break;
        default:
            raise_status( STATUS_INVALID_DISPOSITION, pRecord );
            break;
        }
        frame = __wine_pop_frame( frame );
    }
}
DEFINE_REGS_ENTRYPOINT( RtlUnwind, 4 )


/*******************************************************************
 *		NtRaiseException (NTDLL.@)
 */
NTSTATUS WINAPI NtRaiseException( EXCEPTION_RECORD *rec, CONTEXT *context, BOOL first_chance )
{
    NTSTATUS status = raise_exception( rec, context, first_chance );
    if (status == STATUS_SUCCESS)
    {
        set_debug_registers( context );
        set_cpu_context( context );
    }
    return status;
}


/***********************************************************************
 *		RtlRaiseException (NTDLL.@)
 */
void WINAPI __regs_RtlRaiseException( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    NTSTATUS status;

    rec->ExceptionAddress = (void *)context->Eip;
    status = raise_exception( rec, context, TRUE );
    if (status != STATUS_SUCCESS) raise_status( status, rec );
}
DEFINE_REGS_ENTRYPOINT( RtlRaiseException, 1 )


/*************************************************************************
 *		RtlCaptureStackBackTrace (NTDLL.@)
 */
USHORT WINAPI RtlCaptureStackBackTrace( ULONG skip, ULONG count, PVOID *buffer, ULONG *hash )
{
    CONTEXT context;
    ULONG i;
    ULONG *frame;

    RtlCaptureContext( &context );
    if (hash) *hash = 0;
    frame = (ULONG *)context.Ebp;

    while (skip--)
    {
        if (!is_valid_frame( frame )) return 0;
        frame = (ULONG *)*frame;
    }

    for (i = 0; i < count; i++)
    {
        if (!is_valid_frame( frame )) break;
        buffer[i] = (void *)frame[1];
        if (hash) *hash += frame[1];
        frame = (ULONG *)*frame;
    }
    return i;
}


extern void DECLSPEC_NORETURN call_thread_entry_point( LPTHREAD_START_ROUTINE entry, void *arg );
__ASM_GLOBAL_FUNC( call_thread_entry_point,
                   "pushl %ebp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                   "movl %esp,%ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                   "pushl %ebx\n\t"
                   __ASM_CFI(".cfi_rel_offset %ebx,-4\n\t")
                   "pushl %esi\n\t"
                   __ASM_CFI(".cfi_rel_offset %esi,-8\n\t")
                   "pushl %edi\n\t"
                   __ASM_CFI(".cfi_rel_offset %edi,-12\n\t")
                   "pushl %ebp\n\t"
                   "pushl 12(%ebp)\n\t"
                   "pushl 8(%ebp)\n\t"
                   "call " __ASM_NAME("call_thread_func") );

extern void DECLSPEC_NORETURN call_thread_exit_func( int status, void (*func)(int), void *frame );
__ASM_GLOBAL_FUNC( call_thread_exit_func,
                   "movl 4(%esp),%eax\n\t"
                   "movl 8(%esp),%ecx\n\t"
                   "movl 12(%esp),%ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa %ebp,4\n\t")
                   __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                   __ASM_CFI(".cfi_rel_offset %ebx,-4\n\t")
                   __ASM_CFI(".cfi_rel_offset %esi,-8\n\t")
                   __ASM_CFI(".cfi_rel_offset %edi,-12\n\t")
                   "leal -20(%ebp),%esp\n\t"
                   "pushl %eax\n\t"
                   "call *%ecx" );

/* wrapper for apps that don't declare the thread function correctly */
extern void DECLSPEC_NORETURN call_thread_func_wrapper( LPTHREAD_START_ROUTINE entry, void *arg );
__ASM_GLOBAL_FUNC(call_thread_func_wrapper,
                  "pushl %ebp\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                  __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                  "movl %esp,%ebp\n\t"
                  __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                  "subl $4,%esp\n\t"
                  "pushl 12(%ebp)\n\t"
                  "call *8(%ebp)\n\t"
                  "leal -4(%ebp),%esp\n\t"
                  "pushl %eax\n\t"
                  "call " __ASM_NAME("exit_thread") "\n\t"
                  "int $3" )

/***********************************************************************
 *           call_thread_func
 */
void call_thread_func( LPTHREAD_START_ROUTINE entry, void *arg, void *frame )
{
    ntdll_get_thread_data()->exit_frame = frame;
    __TRY
    {
        call_thread_func_wrapper( entry, arg );
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
    if (!ntdll_get_thread_data()->exit_frame) exit_thread( status );
    call_thread_exit_func( status, exit_thread, ntdll_get_thread_data()->exit_frame );
}

/***********************************************************************
 *           abort_thread
 */
void abort_thread( int status )
{
    if (!ntdll_get_thread_data()->exit_frame) terminate_thread( status );
    call_thread_exit_func( status, terminate_thread, ntdll_get_thread_data()->exit_frame );
}

/**********************************************************************
 *		DbgBreakPoint   (NTDLL.@)
 */
__ASM_STDCALL_FUNC( DbgBreakPoint, 0, "int $3; ret")

/**********************************************************************
 *		DbgUserBreakPoint   (NTDLL.@)
 */
__ASM_STDCALL_FUNC( DbgUserBreakPoint, 0, "int $3; ret")

/**********************************************************************
 *           NtCurrentTeb   (NTDLL.@)
 */
__ASM_STDCALL_FUNC( NtCurrentTeb, 0, ".byte 0x64\n\tmovl 0x18,%eax\n\tret" )


/**************************************************************************
 *           _chkstk   (NTDLL.@)
 */
__ASM_STDCALL_FUNC( _chkstk, 0,
                   "negl %eax\n\t"
                   "addl %esp,%eax\n\t"
                   "xchgl %esp,%eax\n\t"
                   "movl 0(%eax),%eax\n\t"  /* copy return address from old location */
                   "movl %eax,0(%esp)\n\t"
                   "ret" )

/**************************************************************************
 *           _alloca_probe   (NTDLL.@)
 */
__ASM_STDCALL_FUNC( _alloca_probe, 0,
                   "negl %eax\n\t"
                   "addl %esp,%eax\n\t"
                   "xchgl %esp,%eax\n\t"
                   "movl 0(%eax),%eax\n\t"  /* copy return address from old location */
                   "movl %eax,0(%esp)\n\t"
                   "ret" )


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
                  "pushl %ebp\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                  __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                  "movl %esp,%ebp\n\t"
                  __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                   "pushl %ebx\n\t"
                   __ASM_CFI(".cfi_rel_offset %ebx,-4\n\t")
                   "movl 28(%ebp), %edx\n\t" /* ugly hack to pass the 6th param needed because of Shrinker */
                   "pushl 24(%ebp)\n\t"
                   "pushl 20(%ebp)\n\t"
                   "pushl 16(%ebp)\n\t"
                   "pushl 12(%ebp)\n\t"
                   "pushl 8(%ebp)\n\t"
                   "call " __ASM_NAME("call_exception_handler") "\n\t"
                   "popl %ebx\n\t"
                   __ASM_CFI(".cfi_same_value %ebx\n\t")
                   "leave\n"
                   __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                   __ASM_CFI(".cfi_same_value %ebp\n\t")
                   "ret" )
__ASM_GLOBAL_FUNC(call_exception_handler,
                  "pushl %ebp\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                  __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                  "movl %esp,%ebp\n\t"
                  __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                  "subl $12,%esp\n\t"
                  "pushl 12(%ebp)\n\t"      /* make any exceptions in this... */
                  "pushl %edx\n\t"          /* handler be handled by... */
                  ".byte 0x64\n\t"
                  "pushl (0)\n\t"           /* nested_handler (passed in edx). */
                  ".byte 0x64\n\t"
                  "movl %esp,(0)\n\t"       /* push the new exception frame onto the exception stack. */
                  "pushl 20(%ebp)\n\t"
                  "pushl 16(%ebp)\n\t"
                  "pushl 12(%ebp)\n\t"
                  "pushl 8(%ebp)\n\t"
                  "movl 24(%ebp), %ecx\n\t" /* (*1) */
                  "call *%ecx\n\t"          /* call handler. (*2) */
                  ".byte 0x64\n\t"
                  "movl (0), %esp\n\t"      /* restore previous... (*3) */
                  ".byte 0x64\n\t"
                  "popl (0)\n\t"            /* exception frame. */
                  "movl %ebp, %esp\n\t"     /* restore saved stack, in case it was corrupted */
                  "popl %ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                   __ASM_CFI(".cfi_same_value %ebp\n\t")
                  "ret $20" )            /* (*4) */

#endif  /* __i386__ */
