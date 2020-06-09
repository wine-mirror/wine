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

#if 0
#pragma makedep unix
#endif

#ifdef __x86_64__

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_MACHINE_SYSARCH_H
# include <machine/sysarch.h>
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
#ifdef HAVE_LIBUNWIND
# define UNW_LOCAL_ONLY
# include <libunwind.h>
#endif
#ifdef __APPLE__
# include <mach/mach.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/exception.h"
#include "wine/list.h"
#include "wine/asm.h"
#include "unix_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);

/***********************************************************************
 * signal context platform-specific definitions
 */
#ifdef linux

#include <asm/prctl.h>
static inline int arch_prctl( int func, void *ptr ) { return syscall( __NR_arch_prctl, func, ptr ); }

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

#elif defined(__FreeBSD__) || defined (__FreeBSD_kernel__)

#define RAX_sig(context)     ((context)->uc_mcontext.mc_rax)
#define RBX_sig(context)     ((context)->uc_mcontext.mc_rbx)
#define RCX_sig(context)     ((context)->uc_mcontext.mc_rcx)
#define RDX_sig(context)     ((context)->uc_mcontext.mc_rdx)
#define RSI_sig(context)     ((context)->uc_mcontext.mc_rsi)
#define RDI_sig(context)     ((context)->uc_mcontext.mc_rdi)
#define RBP_sig(context)     ((context)->uc_mcontext.mc_rbp)
#define R8_sig(context)      ((context)->uc_mcontext.mc_r8)
#define R9_sig(context)      ((context)->uc_mcontext.mc_r9)
#define R10_sig(context)     ((context)->uc_mcontext.mc_r10)
#define R11_sig(context)     ((context)->uc_mcontext.mc_r11)
#define R12_sig(context)     ((context)->uc_mcontext.mc_r12)
#define R13_sig(context)     ((context)->uc_mcontext.mc_r13)
#define R14_sig(context)     ((context)->uc_mcontext.mc_r14)
#define R15_sig(context)     ((context)->uc_mcontext.mc_r15)
#define CS_sig(context)      ((context)->uc_mcontext.mc_cs)
#define DS_sig(context)      ((context)->uc_mcontext.mc_ds)
#define ES_sig(context)      ((context)->uc_mcontext.mc_es)
#define FS_sig(context)      ((context)->uc_mcontext.mc_fs)
#define GS_sig(context)      ((context)->uc_mcontext.mc_gs)
#define SS_sig(context)      ((context)->uc_mcontext.mc_ss)
#define EFL_sig(context)     ((context)->uc_mcontext.mc_rflags)
#define RIP_sig(context)     ((context)->uc_mcontext.mc_rip)
#define RSP_sig(context)     ((context)->uc_mcontext.mc_rsp)
#define TRAP_sig(context)    ((context)->uc_mcontext.mc_trapno)
#define ERROR_sig(context)   ((context)->uc_mcontext.mc_err)
#define FPU_sig(context)     ((XMM_SAVE_AREA32 *)((context)->uc_mcontext.mc_fpstate))

#elif defined(__NetBSD__)

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
#define FPU_sig(context)    ((XMM_SAVE_AREA32 *)((context)->uc_mcontext.__fpregs))

#elif defined (__APPLE__)

#define RAX_sig(context)     ((context)->uc_mcontext->__ss.__rax)
#define RBX_sig(context)     ((context)->uc_mcontext->__ss.__rbx)
#define RCX_sig(context)     ((context)->uc_mcontext->__ss.__rcx)
#define RDX_sig(context)     ((context)->uc_mcontext->__ss.__rdx)
#define RSI_sig(context)     ((context)->uc_mcontext->__ss.__rsi)
#define RDI_sig(context)     ((context)->uc_mcontext->__ss.__rdi)
#define RBP_sig(context)     ((context)->uc_mcontext->__ss.__rbp)
#define R8_sig(context)      ((context)->uc_mcontext->__ss.__r8)
#define R9_sig(context)      ((context)->uc_mcontext->__ss.__r9)
#define R10_sig(context)     ((context)->uc_mcontext->__ss.__r10)
#define R11_sig(context)     ((context)->uc_mcontext->__ss.__r11)
#define R12_sig(context)     ((context)->uc_mcontext->__ss.__r12)
#define R13_sig(context)     ((context)->uc_mcontext->__ss.__r13)
#define R14_sig(context)     ((context)->uc_mcontext->__ss.__r14)
#define R15_sig(context)     ((context)->uc_mcontext->__ss.__r15)
#define CS_sig(context)      ((context)->uc_mcontext->__ss.__cs)
#define FS_sig(context)      ((context)->uc_mcontext->__ss.__fs)
#define GS_sig(context)      ((context)->uc_mcontext->__ss.__gs)
#define EFL_sig(context)     ((context)->uc_mcontext->__ss.__rflags)
#define RIP_sig(context)     (*((unsigned long*)&(context)->uc_mcontext->__ss.__rip))
#define RSP_sig(context)     (*((unsigned long*)&(context)->uc_mcontext->__ss.__rsp))
#define TRAP_sig(context)    ((context)->uc_mcontext->__es.__trapno)
#define ERROR_sig(context)   ((context)->uc_mcontext->__es.__err)
#define FPU_sig(context)     ((XMM_SAVE_AREA32 *)&(context)->uc_mcontext->__fs.__fpu_fcw)

#else
#error You must define the signal context functions for your platform
#endif

enum i386_trap_code
{
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

static const size_t teb_size = 0x2000;  /* we reserve two pages for the TEB */

typedef void (*raise_func)( EXCEPTION_RECORD *rec, CONTEXT *context );

/* stack layout when calling an exception raise function */
struct stack_layout
{
    CONTEXT           context;
    EXCEPTION_RECORD  rec;
    ULONG64           rsi;
    ULONG64           rdi;
    ULONG64           rbp;
    ULONG64           rip;
    ULONG64           red_zone[16];
};

struct amd64_thread_data
{
    DWORD_PTR dr0;           /* debug registers */
    DWORD_PTR dr1;
    DWORD_PTR dr2;
    DWORD_PTR dr3;
    DWORD_PTR dr6;
    DWORD_PTR dr7;
    void     *exit_frame;    /* exit frame pointer */
};

C_ASSERT( sizeof(struct amd64_thread_data) <= sizeof(((TEB *)0)->SystemReserved2) );
C_ASSERT( offsetof( TEB, SystemReserved2 ) + offsetof( struct amd64_thread_data, exit_frame ) == 0x330 );

static inline struct amd64_thread_data *amd64_thread_data(void)
{
    return (struct amd64_thread_data *)NtCurrentTeb()->SystemReserved2;
}


static inline void set_sigcontext( const CONTEXT *context, ucontext_t *sigcontext )
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
#ifdef DS_sig
    DS_sig(sigcontext) = context->SegDs;
#endif
#ifdef ES_sig
    ES_sig(sigcontext) = context->SegEs;
#endif
#ifdef SS_sig
    SS_sig(sigcontext) = context->SegSs;
#endif
}


/***********************************************************************
 *           get_signal_stack
 *
 * Get the base of the signal stack for the current thread.
 */
static inline void *get_signal_stack(void)
{
    return (char *)NtCurrentTeb() + teb_size;
}


/***********************************************************************
 *           is_inside_signal_stack
 *
 * Check if pointer is inside the signal stack.
 */
static inline BOOL is_inside_signal_stack( void *ptr )
{
    return ((char *)ptr >= (char *)get_signal_stack() &&
            (char *)ptr < (char *)get_signal_stack() + signal_stack_size);
}


/***********************************************************************
 *           save_context
 *
 * Set the register values from a sigcontext.
 */
static void save_context( CONTEXT *context, const ucontext_t *sigcontext )
{
    context->ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS | CONTEXT_DEBUG_REGISTERS;
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
#ifdef DS_sig
    context->SegDs  = DS_sig(sigcontext);
#else
    __asm__("movw %%ds,%0" : "=m" (context->SegDs));
#endif
#ifdef ES_sig
    context->SegEs  = ES_sig(sigcontext);
#else
    __asm__("movw %%es,%0" : "=m" (context->SegEs));
#endif
#ifdef SS_sig
    context->SegSs  = SS_sig(sigcontext);
#else
    __asm__("movw %%ss,%0" : "=m" (context->SegSs));
#endif
    context->Dr0    = amd64_thread_data()->dr0;
    context->Dr1    = amd64_thread_data()->dr1;
    context->Dr2    = amd64_thread_data()->dr2;
    context->Dr3    = amd64_thread_data()->dr3;
    context->Dr6    = amd64_thread_data()->dr6;
    context->Dr7    = amd64_thread_data()->dr7;
    if (FPU_sig(sigcontext))
    {
        context->ContextFlags |= CONTEXT_FLOATING_POINT;
        context->u.FltSave = *FPU_sig(sigcontext);
        context->MxCsr = context->u.FltSave.MxCsr;
    }
}


/***********************************************************************
 *           restore_context
 *
 * Build a sigcontext from the register values.
 */
static void restore_context( const CONTEXT *context, ucontext_t *sigcontext )
{
    amd64_thread_data()->dr0 = context->Dr0;
    amd64_thread_data()->dr1 = context->Dr1;
    amd64_thread_data()->dr2 = context->Dr2;
    amd64_thread_data()->dr3 = context->Dr3;
    amd64_thread_data()->dr6 = context->Dr6;
    amd64_thread_data()->dr7 = context->Dr7;
    set_sigcontext( context, sigcontext );
    if (FPU_sig(sigcontext)) *FPU_sig(sigcontext) = context->u.FltSave;
}


/***********************************************************************
 *           set_full_cpu_context
 *
 * Set the new CPU context.
 */
extern void set_full_cpu_context( const CONTEXT *context );
__ASM_GLOBAL_FUNC( set_full_cpu_context,
                   "subq $40,%rsp\n\t"
                   __ASM_SEH(".seh_stackalloc 0x40\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   __ASM_CFI(".cfi_adjust_cfa_offset 40\n\t")
                   "ldmxcsr 0x34(%rdi)\n\t"         /* context->MxCsr */
                   "movw 0x38(%rdi),%ax\n\t"        /* context->SegCs */
                   "movq %rax,8(%rsp)\n\t"
                   "movw 0x42(%rdi),%ax\n\t"        /* context->SegSs */
                   "movq %rax,32(%rsp)\n\t"
                   "movq 0x44(%rdi),%rax\n\t"       /* context->Eflags */
                   "movq %rax,16(%rsp)\n\t"
                   "movq 0x80(%rdi),%rcx\n\t"       /* context->Rcx */
                   "movq 0x88(%rdi),%rdx\n\t"       /* context->Rdx */
                   "movq 0x90(%rdi),%rbx\n\t"       /* context->Rbx */
                   "movq 0x98(%rdi),%rax\n\t"       /* context->Rsp */
                   "movq %rax,24(%rsp)\n\t"
                   "movq 0xa0(%rdi),%rbp\n\t"       /* context->Rbp */
                   "movq 0xa8(%rdi),%rsi\n\t"       /* context->Rsi */
                   "movq 0xb8(%rdi),%r8\n\t"        /* context->R8 */
                   "movq 0xc0(%rdi),%r9\n\t"        /* context->R9 */
                   "movq 0xc8(%rdi),%r10\n\t"       /* context->R10 */
                   "movq 0xd0(%rdi),%r11\n\t"       /* context->R11 */
                   "movq 0xd8(%rdi),%r12\n\t"       /* context->R12 */
                   "movq 0xe0(%rdi),%r13\n\t"       /* context->R13 */
                   "movq 0xe8(%rdi),%r14\n\t"       /* context->R14 */
                   "movq 0xf0(%rdi),%r15\n\t"       /* context->R15 */
                   "movq 0xf8(%rdi),%rax\n\t"       /* context->Rip */
                   "movq %rax,(%rsp)\n\t"
                   "fxrstor 0x100(%rdi)\n\t"        /* context->FtlSave */
                   "movdqa 0x1a0(%rdi),%xmm0\n\t"   /* context->Xmm0 */
                   "movdqa 0x1b0(%rdi),%xmm1\n\t"   /* context->Xmm1 */
                   "movdqa 0x1c0(%rdi),%xmm2\n\t"   /* context->Xmm2 */
                   "movdqa 0x1d0(%rdi),%xmm3\n\t"   /* context->Xmm3 */
                   "movdqa 0x1e0(%rdi),%xmm4\n\t"   /* context->Xmm4 */
                   "movdqa 0x1f0(%rdi),%xmm5\n\t"   /* context->Xmm5 */
                   "movdqa 0x200(%rdi),%xmm6\n\t"   /* context->Xmm6 */
                   "movdqa 0x210(%rdi),%xmm7\n\t"   /* context->Xmm7 */
                   "movdqa 0x220(%rdi),%xmm8\n\t"   /* context->Xmm8 */
                   "movdqa 0x230(%rdi),%xmm9\n\t"   /* context->Xmm9 */
                   "movdqa 0x240(%rdi),%xmm10\n\t"  /* context->Xmm10 */
                   "movdqa 0x250(%rdi),%xmm11\n\t"  /* context->Xmm11 */
                   "movdqa 0x260(%rdi),%xmm12\n\t"  /* context->Xmm12 */
                   "movdqa 0x270(%rdi),%xmm13\n\t"  /* context->Xmm13 */
                   "movdqa 0x280(%rdi),%xmm14\n\t"  /* context->Xmm14 */
                   "movdqa 0x290(%rdi),%xmm15\n\t"  /* context->Xmm15 */
                   "movq 0x78(%rdi),%rax\n\t"       /* context->Rax */
                   "movq 0xb0(%rdi),%rdi\n\t"       /* context->Rdi */
                   "iretq" );


/***********************************************************************
 *           get_server_context_flags
 *
 * Convert CPU-specific flags to generic server flags
 */
static unsigned int get_server_context_flags( DWORD flags )
{
    unsigned int ret = 0;

    flags &= ~CONTEXT_AMD64;  /* get rid of CPU id */
    if (flags & CONTEXT_CONTROL) ret |= SERVER_CTX_CONTROL;
    if (flags & CONTEXT_INTEGER) ret |= SERVER_CTX_INTEGER;
    if (flags & CONTEXT_SEGMENTS) ret |= SERVER_CTX_SEGMENTS;
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
    flags &= ~CONTEXT_AMD64;  /* get rid of CPU id */
    if (flags & CONTEXT_CONTROL)
    {
        to->Rbp    = from->Rbp;
        to->Rip    = from->Rip;
        to->Rsp    = from->Rsp;
        to->SegCs  = from->SegCs;
        to->SegSs  = from->SegSs;
        to->EFlags = from->EFlags;
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
        to->MxCsr = from->MxCsr;
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
        to->MxCsr = to->u.FltSave.MxCsr;
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


/***********************************************************************
 *              NtSetContextThread  (NTDLL.@)
 *              ZwSetContextThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetContextThread( HANDLE handle, const CONTEXT *context )
{
    NTSTATUS ret = STATUS_SUCCESS;
    DWORD flags = context->ContextFlags & ~CONTEXT_AMD64;
    BOOL self = (handle == GetCurrentThread());

    /* debug registers require a server call */
    if (self && (flags & CONTEXT_DEBUG_REGISTERS))
        self = (amd64_thread_data()->dr0 == context->Dr0 &&
                amd64_thread_data()->dr1 == context->Dr1 &&
                amd64_thread_data()->dr2 == context->Dr2 &&
                amd64_thread_data()->dr3 == context->Dr3 &&
                amd64_thread_data()->dr6 == context->Dr6 &&
                amd64_thread_data()->dr7 == context->Dr7);

    if (!self)
    {
        context_t server_context;

        context_to_server( &server_context, context );
        ret = set_thread_context( handle, &server_context, &self );
        if (ret || !self) return ret;
        if (flags & CONTEXT_DEBUG_REGISTERS)
        {
            amd64_thread_data()->dr0 = context->Dr0;
            amd64_thread_data()->dr1 = context->Dr1;
            amd64_thread_data()->dr2 = context->Dr2;
            amd64_thread_data()->dr3 = context->Dr3;
            amd64_thread_data()->dr6 = context->Dr6;
            amd64_thread_data()->dr7 = context->Dr7;
        }
    }

    if (flags & CONTEXT_FULL)
    {
        if (!(flags & CONTEXT_CONTROL))
            FIXME( "setting partial context (%x) not supported\n", flags );
        else
            set_full_cpu_context( context );
    }
    return ret;
}


/***********************************************************************
 *              NtGetContextThread  (NTDLL.@)
 *              ZwGetContextThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtGetContextThread( HANDLE handle, CONTEXT *context )
{
    NTSTATUS ret;
    DWORD needed_flags;
    BOOL self = (handle == GetCurrentThread());

    if (!context) return STATUS_INVALID_PARAMETER;

    needed_flags = context->ContextFlags;

    /* debug registers require a server call */
    if (context->ContextFlags & (CONTEXT_DEBUG_REGISTERS & ~CONTEXT_AMD64)) self = FALSE;

    if (!self)
    {
        context_t server_context;
        unsigned int server_flags = get_server_context_flags( context->ContextFlags );

        if ((ret = get_thread_context( handle, &server_context, server_flags, &self ))) return ret;
        if ((ret = context_from_server( context, &server_context ))) return ret;
        needed_flags &= ~context->ContextFlags;
    }

    if (self)
    {
        if (needed_flags)
        {
            CONTEXT ctx;
            RtlCaptureContext( &ctx );
            copy_context( context, &ctx, ctx.ContextFlags & needed_flags );
            context->ContextFlags |= ctx.ContextFlags & needed_flags;
        }
        /* update the cached version of the debug registers */
        if (context->ContextFlags & (CONTEXT_DEBUG_REGISTERS & ~CONTEXT_AMD64))
        {
            amd64_thread_data()->dr0 = context->Dr0;
            amd64_thread_data()->dr1 = context->Dr1;
            amd64_thread_data()->dr2 = context->Dr2;
            amd64_thread_data()->dr3 = context->Dr3;
            amd64_thread_data()->dr6 = context->Dr6;
            amd64_thread_data()->dr7 = context->Dr7;
        }
    }
    return STATUS_SUCCESS;
}


extern void CDECL raise_func_trampoline( EXCEPTION_RECORD *rec, CONTEXT *context, raise_func func );
__ASM_GLOBAL_FUNC( raise_func_trampoline,
                   __ASM_CFI(".cfi_signal_frame\n\t")
                   __ASM_CFI(".cfi_def_cfa %rbp,160\n\t")  /* red zone + rip + rbp + rdi + rsi */
                   __ASM_CFI(".cfi_rel_offset %rip,24\n\t")
                   __ASM_CFI(".cfi_rel_offset %rbp,16\n\t")
                   __ASM_CFI(".cfi_rel_offset %rdi,8\n\t")
                   __ASM_CFI(".cfi_rel_offset %rsi,0\n\t")
                   "call *%r8\n\t"
                   "int $3")

/***********************************************************************
 *           setup_exception
 *
 * Setup a proper stack frame for the raise function, and modify the
 * sigcontext so that the return from the signal handler will call
 * the raise function.
 */
static struct stack_layout *setup_exception( ucontext_t *sigcontext )
{
    struct stack_layout *stack;
    DWORD exception_code = 0;

    stack = (struct stack_layout *)(RSP_sig(sigcontext) & ~15);

    /* stack sanity checks */

    if (is_inside_signal_stack( stack ))
    {
        ERR( "nested exception on signal stack in thread %04x eip %016lx esp %016lx stack %p-%p\n",
             GetCurrentThreadId(), (ULONG_PTR)RIP_sig(sigcontext), (ULONG_PTR)RSP_sig(sigcontext),
             NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
        abort_thread(1);
    }

    if (stack - 1 > stack || /* check for overflow in subtraction */
        (char *)stack <= (char *)NtCurrentTeb()->DeallocationStack ||
        (char *)stack > (char *)NtCurrentTeb()->Tib.StackBase)
    {
        WARN( "exception outside of stack limits in thread %04x eip %016lx esp %016lx stack %p-%p\n",
              GetCurrentThreadId(), (ULONG_PTR)RIP_sig(sigcontext), (ULONG_PTR)RSP_sig(sigcontext),
              NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
    }
    else if ((char *)(stack - 1) < (char *)NtCurrentTeb()->DeallocationStack + 4096)
    {
        /* stack overflow on last page, unrecoverable */
        UINT diff = (char *)NtCurrentTeb()->DeallocationStack + 4096 - (char *)(stack - 1);
        ERR( "stack overflow %u bytes in thread %04x eip %016lx esp %016lx stack %p-%p-%p\n",
             diff, GetCurrentThreadId(), (ULONG_PTR)RIP_sig(sigcontext),
             (ULONG_PTR)RSP_sig(sigcontext), NtCurrentTeb()->DeallocationStack,
             NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
        abort_thread(1);
    }
    else if ((char *)(stack - 1) < (char *)NtCurrentTeb()->Tib.StackLimit)
    {
        /* stack access below stack limit, may be recoverable */
        switch (virtual_handle_stack_fault( stack - 1 ))
        {
        case 0:  /* not handled */
        {
            UINT diff = (char *)NtCurrentTeb()->Tib.StackLimit - (char *)(stack - 1);
            ERR( "stack overflow %u bytes in thread %04x eip %016lx esp %016lx stack %p-%p-%p\n",
                 diff, GetCurrentThreadId(), (ULONG_PTR)RIP_sig(sigcontext),
                 (ULONG_PTR)RSP_sig(sigcontext), NtCurrentTeb()->DeallocationStack,
                 NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
            abort_thread(1);
        }
        case -1:  /* overflow */
            exception_code = EXCEPTION_STACK_OVERFLOW;
            break;
        }
    }

    stack--;  /* push the stack_layout structure */
#if defined(VALGRIND_MAKE_MEM_UNDEFINED)
    VALGRIND_MAKE_MEM_UNDEFINED(stack, sizeof(*stack));
#elif defined(VALGRIND_MAKE_WRITABLE)
    VALGRIND_MAKE_WRITABLE(stack, sizeof(*stack));
#endif
    stack->rec.ExceptionRecord  = NULL;
    stack->rec.ExceptionCode    = exception_code;
    stack->rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    stack->rec.ExceptionAddress = (void *)RIP_sig(sigcontext);
    stack->rec.NumberParameters = 0;
    save_context( &stack->context, sigcontext );

    return stack;
}

static void setup_raise_exception( ucontext_t *sigcontext, struct stack_layout *stack )
{
    ULONG64 *rsp_ptr;
    NTSTATUS status;

    if (stack->rec.ExceptionCode == EXCEPTION_SINGLE_STEP)
    {
        /* when single stepping can't tell whether this is a hw bp or a
         * single step interrupt. try to avoid as much overhead as possible
         * and only do a server call if there is any hw bp enabled. */

        if (!(stack->context.EFlags & 0x100) || (stack->context.Dr7 & 0xff))
        {
            /* (possible) hardware breakpoint, fetch the debug registers */
            DWORD saved_flags = stack->context.ContextFlags;
            stack->context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
            NtGetContextThread(GetCurrentThread(), &stack->context);
            stack->context.ContextFlags |= saved_flags;  /* restore flags */
        }

        stack->context.EFlags &= ~0x100;  /* clear single-step flag */
    }

    status = send_debug_event( &stack->rec, &stack->context, TRUE );
    if (status == DBG_CONTINUE || status == DBG_EXCEPTION_HANDLED)
    {
        restore_context( &stack->context, sigcontext );
        return;
    }

    /* store return address and %rbp without aligning, so that the offset is fixed */
    rsp_ptr = (ULONG64 *)RSP_sig(sigcontext) - 16;
    *(--rsp_ptr) = stack->context.Rip;
    *(--rsp_ptr) = stack->context.Rbp;
    *(--rsp_ptr) = stack->context.Rdi;
    *(--rsp_ptr) = stack->context.Rsi;

    /* now modify the sigcontext to return to the raise function */
    RIP_sig(sigcontext) = (ULONG_PTR)raise_func_trampoline;
    RCX_sig(sigcontext) = (ULONG_PTR)&stack->rec;
    RDX_sig(sigcontext) = (ULONG_PTR)&stack->context;
    R8_sig(sigcontext)  = (ULONG_PTR)KiUserExceptionDispatcher;
    RBP_sig(sigcontext) = (ULONG_PTR)rsp_ptr;
    RSP_sig(sigcontext) = (ULONG_PTR)stack;
    /* clear single-step, direction, and align check flag */
    EFL_sig(sigcontext) &= ~(0x100|0x400|0x40000);
}


/***********************************************************************
 *           is_privileged_instr
 *
 * Check if the fault location is a privileged instruction.
 */
static inline DWORD is_privileged_instr( CONTEXT *context )
{
    BYTE instr[16];
    unsigned int i, prefix_count = 0;
    unsigned int len = virtual_uninterrupted_read_memory( (BYTE *)context->Rip, instr, sizeof(instr) );

    for (i = 0; i < len; i++) switch (instr[i])
    {
    /* instruction prefixes */
    case 0x2e:  /* %cs: */
    case 0x36:  /* %ss: */
    case 0x3e:  /* %ds: */
    case 0x26:  /* %es: */
    case 0x40:  /* rex */
    case 0x41:  /* rex */
    case 0x42:  /* rex */
    case 0x43:  /* rex */
    case 0x44:  /* rex */
    case 0x45:  /* rex */
    case 0x46:  /* rex */
    case 0x47:  /* rex */
    case 0x48:  /* rex */
    case 0x49:  /* rex */
    case 0x4a:  /* rex */
    case 0x4b:  /* rex */
    case 0x4c:  /* rex */
    case 0x4d:  /* rex */
    case 0x4e:  /* rex */
    case 0x4f:  /* rex */
    case 0x64:  /* %fs: */
    case 0x65:  /* %gs: */
    case 0x66:  /* opcode size */
    case 0x67:  /* addr size */
    case 0xf0:  /* lock */
    case 0xf2:  /* repne */
    case 0xf3:  /* repe */
        if (++prefix_count >= 15) return EXCEPTION_ILLEGAL_INSTRUCTION;
        continue;

    case 0x0f: /* extended instruction */
        if (i == len - 1) return 0;
        switch (instr[i + 1])
        {
        case 0x06: /* clts */
        case 0x08: /* invd */
        case 0x09: /* wbinvd */
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
    return 0;
}


/***********************************************************************
 *           handle_interrupt
 *
 * Handle an interrupt.
 */
static inline BOOL handle_interrupt( ucontext_t *sigcontext, struct stack_layout *stack )
{
    switch (ERROR_sig(sigcontext) >> 3)
    {
    case 0x2c:
        stack->rec.ExceptionCode = STATUS_ASSERTION_FAILURE;
        break;
    case 0x2d:
        switch (stack->context.Rax)
        {
            case 1: /* BREAKPOINT_PRINT */
            case 3: /* BREAKPOINT_LOAD_SYMBOLS */
            case 4: /* BREAKPOINT_UNLOAD_SYMBOLS */
            case 5: /* BREAKPOINT_COMMAND_STRING (>= Win2003) */
                RIP_sig(sigcontext) += 3;
                return TRUE;
        }
        stack->context.Rip += 3;
        stack->rec.ExceptionCode = EXCEPTION_BREAKPOINT;
        stack->rec.ExceptionAddress = (void *)stack->context.Rip;
        stack->rec.NumberParameters = 1;
        stack->rec.ExceptionInformation[0] = stack->context.Rax;
        break;
    default:
        return FALSE;
    }
    setup_raise_exception( sigcontext, stack );
    return TRUE;
}


/**********************************************************************
 *		segv_handler
 *
 * Handler for SIGSEGV and related errors.
 */
static void segv_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    struct stack_layout *stack;
    ucontext_t *ucontext = sigcontext;

    stack = (struct stack_layout *)(RSP_sig(ucontext) & ~15);

    /* check for exceptions on the signal stack caused by write watches */
    if (TRAP_sig(ucontext) == TRAP_x86_PAGEFLT && is_inside_signal_stack( stack ) &&
        !virtual_handle_fault( siginfo->si_addr, (ERROR_sig(ucontext) >> 1) & 0x09, TRUE ))
    {
        return;
    }

    /* check for page fault inside the thread stack */
    if (TRAP_sig(ucontext) == TRAP_x86_PAGEFLT)
    {
        switch (virtual_handle_stack_fault( siginfo->si_addr ))
        {
        case 1:  /* handled */
            return;
        case -1:  /* overflow */
            stack = setup_exception( sigcontext );
            stack->rec.ExceptionCode = EXCEPTION_STACK_OVERFLOW;
            goto done;
        }
    }

    stack = setup_exception( sigcontext );
    if (stack->rec.ExceptionCode == EXCEPTION_STACK_OVERFLOW) goto done;

    switch(TRAP_sig(ucontext))
    {
    case TRAP_x86_OFLOW:   /* Overflow exception */
        stack->rec.ExceptionCode = EXCEPTION_INT_OVERFLOW;
        break;
    case TRAP_x86_BOUND:   /* Bound range exception */
        stack->rec.ExceptionCode = EXCEPTION_ARRAY_BOUNDS_EXCEEDED;
        break;
    case TRAP_x86_PRIVINFLT:   /* Invalid opcode exception */
        stack->rec.ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
        break;
    case TRAP_x86_STKFLT:  /* Stack fault */
        stack->rec.ExceptionCode = EXCEPTION_STACK_OVERFLOW;
        break;
    case TRAP_x86_SEGNPFLT:  /* Segment not present exception */
    case TRAP_x86_PROTFLT:   /* General protection fault */
        {
            WORD err = ERROR_sig(ucontext);
            if (!err && (stack->rec.ExceptionCode = is_privileged_instr( &stack->context ))) break;
            if ((err & 7) == 2 && handle_interrupt( ucontext, stack )) return;
            stack->rec.ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
            stack->rec.NumberParameters = 2;
            stack->rec.ExceptionInformation[0] = 0;
            stack->rec.ExceptionInformation[1] = 0xffffffffffffffff;
        }
        break;
    case TRAP_x86_PAGEFLT:  /* Page fault */
        stack->rec.NumberParameters = 2;
        stack->rec.ExceptionInformation[0] = (ERROR_sig(ucontext) >> 1) & 0x09;
        stack->rec.ExceptionInformation[1] = (ULONG_PTR)siginfo->si_addr;
        if (!(stack->rec.ExceptionCode = virtual_handle_fault((void *)stack->rec.ExceptionInformation[1],
                                                              stack->rec.ExceptionInformation[0], FALSE )))
            return;
        break;
    case TRAP_x86_ALIGNFLT:  /* Alignment check exception */
        stack->rec.ExceptionCode = EXCEPTION_DATATYPE_MISALIGNMENT;
        break;
    default:
        ERR( "Got unexpected trap %ld\n", (ULONG_PTR)TRAP_sig(ucontext) );
        /* fall through */
    case TRAP_x86_NMI:       /* NMI interrupt */
    case TRAP_x86_DNA:       /* Device not available exception */
    case TRAP_x86_DOUBLEFLT: /* Double fault exception */
    case TRAP_x86_TSSFLT:    /* Invalid TSS exception */
    case TRAP_x86_MCHK:      /* Machine check exception */
    case TRAP_x86_CACHEFLT:  /* Cache flush exception */
        stack->rec.ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
        break;
    }
done:
    setup_raise_exception( sigcontext, stack );
}


/**********************************************************************
 *		trap_handler
 *
 * Handler for SIGTRAP.
 */
static void trap_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    struct stack_layout *stack = setup_exception( sigcontext );

    switch (siginfo->si_code)
    {
    case TRAP_TRACE:  /* Single-step exception */
    case 4 /* TRAP_HWBKPT */: /* Hardware breakpoint exception */
        stack->rec.ExceptionCode = EXCEPTION_SINGLE_STEP;
        break;
    case TRAP_BRKPT:   /* Breakpoint exception */
#ifdef SI_KERNEL
    case SI_KERNEL:
#endif
        /* Check if this is actually icebp instruction */
        if (((unsigned char *)stack->rec.ExceptionAddress)[-1] == 0xF1)
        {
            stack->rec.ExceptionCode = EXCEPTION_SINGLE_STEP;
            break;
        }
        stack->rec.ExceptionAddress = (char *)stack->rec.ExceptionAddress - 1;  /* back up over the int3 instruction */
        /* fall through */
    default:
        stack->rec.ExceptionCode = EXCEPTION_BREAKPOINT;
        stack->rec.NumberParameters = 1;
        stack->rec.ExceptionInformation[0] = 0;
        break;
    }

    setup_raise_exception( sigcontext, stack );
}


/**********************************************************************
 *		fpe_handler
 *
 * Handler for SIGFPE.
 */
static void fpe_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    struct stack_layout *stack = setup_exception( sigcontext );

    switch (siginfo->si_code)
    {
    case FPE_FLTSUB:
        stack->rec.ExceptionCode = EXCEPTION_ARRAY_BOUNDS_EXCEEDED;
        break;
    case FPE_INTDIV:
        stack->rec.ExceptionCode = EXCEPTION_INT_DIVIDE_BY_ZERO;
        break;
    case FPE_INTOVF:
        stack->rec.ExceptionCode = EXCEPTION_INT_OVERFLOW;
        break;
    case FPE_FLTDIV:
        stack->rec.ExceptionCode = EXCEPTION_FLT_DIVIDE_BY_ZERO;
        break;
    case FPE_FLTOVF:
        stack->rec.ExceptionCode = EXCEPTION_FLT_OVERFLOW;
        break;
    case FPE_FLTUND:
        stack->rec.ExceptionCode = EXCEPTION_FLT_UNDERFLOW;
        break;
    case FPE_FLTRES:
        stack->rec.ExceptionCode = EXCEPTION_FLT_INEXACT_RESULT;
        break;
    case FPE_FLTINV:
    default:
        stack->rec.ExceptionCode = EXCEPTION_FLT_INVALID_OPERATION;
        break;
    }

    setup_raise_exception( sigcontext, stack );
}


/**********************************************************************
 *		int_handler
 *
 * Handler for SIGINT.
 */
static void int_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    struct stack_layout *stack = setup_exception( sigcontext );
    stack->rec.ExceptionCode = CONTROL_C_EXIT;
    setup_raise_exception( sigcontext, stack );
}


/**********************************************************************
 *		abrt_handler
 *
 * Handler for SIGABRT.
 */
static void abrt_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    struct stack_layout *stack = setup_exception( sigcontext );
    stack->rec.ExceptionCode = EXCEPTION_WINE_ASSERTION;
    stack->rec.ExceptionFlags = EH_NONCONTINUABLE;
    setup_raise_exception( sigcontext, stack );
}


/**********************************************************************
 *		quit_handler
 *
 * Handler for SIGQUIT.
 */
static void quit_handler( int signal, siginfo_t *siginfo, void *ucontext )
{
    abort_thread(0);
}


/**********************************************************************
 *		usr1_handler
 *
 * Handler for SIGUSR1, used to signal a thread that it got suspended.
 */
static void usr1_handler( int signal, siginfo_t *siginfo, void *ucontext )
{
    CONTEXT context;

    save_context( &context, ucontext );
    wait_suspend( &context );
    restore_context( &context, ucontext );
}


/**********************************************************************
 *           get_thread_ldt_entry
 */
NTSTATUS CDECL get_thread_ldt_entry( HANDLE handle, void *data, ULONG len, ULONG *ret_len )
{
    return STATUS_NOT_IMPLEMENTED;
}


/******************************************************************************
 *           NtSetLdtEntries   (NTDLL.@)
 *           ZwSetLdtEntries   (NTDLL.@)
 */
NTSTATUS WINAPI NtSetLdtEntries( ULONG sel1, LDT_ENTRY entry1, ULONG sel2, LDT_ENTRY entry2 )
{
    return STATUS_NOT_IMPLEMENTED;
}


/**********************************************************************
 *             signal_init_threading
 */
void signal_init_threading(void)
{
}


/**********************************************************************
 *		signal_alloc_thread
 */
NTSTATUS signal_alloc_thread( TEB *teb )
{
    return STATUS_SUCCESS;
}


/**********************************************************************
 *		signal_free_thread
 */
void signal_free_thread( TEB *teb )
{
}

#ifdef __APPLE__
/**********************************************************************
 *		mac_thread_gsbase
 */
static void *mac_thread_gsbase(void)
{
    struct thread_identifier_info tiinfo;
    unsigned int info_count = THREAD_IDENTIFIER_INFO_COUNT;
    static int gsbase_offset = -1;

    kern_return_t kr = thread_info(mach_thread_self(), THREAD_IDENTIFIER_INFO, (thread_info_t) &tiinfo, &info_count);
    if (kr == KERN_SUCCESS) return (void*)tiinfo.thread_handle;

    if (gsbase_offset < 0)
    {
        /* Search for the array of TLS slots within the pthread data structure.
           That's what the macOS pthread implementation uses for gsbase. */
        const void* const sentinel1 = (const void*)0x2bffb6b4f11228ae;
        const void* const sentinel2 = (const void*)0x0845a7ff6ab76707;
        int rc;
        pthread_key_t key;
        const void** p = (const void**)pthread_self();
        int i;

        gsbase_offset = 0;
        if ((rc = pthread_key_create(&key, NULL))) return NULL;

        pthread_setspecific(key, sentinel1);

        for (i = key + 1; i < 2000; i++) /* arbitrary limit */
        {
            if (p[i] == sentinel1)
            {
                pthread_setspecific(key, sentinel2);

                if (p[i] == sentinel2)
                {
                    gsbase_offset = (i - key) * sizeof(*p);
                    break;
                }

                pthread_setspecific(key, sentinel1);
            }
        }

        pthread_key_delete(key);
    }

    if (gsbase_offset) return (char*)pthread_self() + gsbase_offset;
    return NULL;
}
#endif


/**********************************************************************
 *		signal_init_thread
 */
void signal_init_thread( TEB *teb )
{
    const WORD fpu_cw = 0x27f;
    stack_t ss;

#if defined __linux__
    arch_prctl( ARCH_SET_GS, teb );
#elif defined (__FreeBSD__) || defined (__FreeBSD_kernel__)
    amd64_set_gsbase( teb );
#elif defined(__NetBSD__)
    sysarch( X86_64_SET_GSBASE, &teb );
#elif defined (__APPLE__)
    __asm__ volatile (".byte 0x65\n\tmovq %0,%c1"
                      :
                      : "r" (teb->Tib.Self), "n" (FIELD_OFFSET(TEB, Tib.Self)));
    __asm__ volatile (".byte 0x65\n\tmovq %0,%c1"
                      :
                      : "r" (teb->ThreadLocalStoragePointer), "n" (FIELD_OFFSET(TEB, ThreadLocalStoragePointer)));

    /* alloc_tls_slot() needs to poke a value to an address relative to each
       thread's gsbase.  Have each thread record its gsbase pointer into its
       TEB so alloc_tls_slot() can find it. */
    teb->Reserved5[0] = mac_thread_gsbase();
#else
# error Please define setting %gs for your architecture
#endif

    ss.ss_sp    = (char *)teb + teb_size;
    ss.ss_size  = signal_stack_size;
    ss.ss_flags = 0;
    if (sigaltstack(&ss, NULL) == -1) perror( "sigaltstack" );

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
    sig_act.sa_flags = SA_SIGINFO | SA_RESTART | SA_ONSTACK;

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
    sig_act.sa_sigaction = trap_handler;
    if (sigaction( SIGTRAP, &sig_act, NULL ) == -1) goto error;
    sig_act.sa_sigaction = segv_handler;
    if (sigaction( SIGSEGV, &sig_act, NULL ) == -1) goto error;
    if (sigaction( SIGILL, &sig_act, NULL ) == -1) goto error;
    if (sigaction( SIGBUS, &sig_act, NULL ) == -1) goto error;
    return;

 error:
    perror("sigaction");
    exit(1);
}


/***********************************************************************
 *           init_thread_context
 */
static void init_thread_context( CONTEXT *context, LPTHREAD_START_ROUTINE entry, void *arg, void *relay )
{
    __asm__( "movw %%cs,%0" : "=m" (context->SegCs) );
    __asm__( "movw %%ss,%0" : "=m" (context->SegSs) );
    context->Rcx    = (ULONG_PTR)entry;
    context->Rdx    = (ULONG_PTR)arg;
    context->Rsp    = (ULONG_PTR)NtCurrentTeb()->Tib.StackBase - 0x28;
    context->Rip    = (ULONG_PTR)relay;
    context->EFlags = 0x200;
    context->u.FltSave.ControlWord = 0x27f;
    context->u.FltSave.MxCsr = context->MxCsr = 0x1f80;
}


/***********************************************************************
 *           attach_thread
 */
PCONTEXT DECLSPEC_HIDDEN attach_thread( LPTHREAD_START_ROUTINE entry, void *arg,
                                        BOOL suspend, void *relay )
{
    CONTEXT *ctx;

    if (suspend)
    {
        CONTEXT context = { 0 };

        context.ContextFlags = CONTEXT_ALL;
        init_thread_context( &context, entry, arg, relay );
        wait_suspend( &context );
        ctx = (CONTEXT *)((ULONG_PTR)context.Rsp & ~15) - 1;
        *ctx = context;
    }
    else
    {
        ctx = (CONTEXT *)((char *)NtCurrentTeb()->Tib.StackBase - 0x30) - 1;
        init_thread_context( ctx, entry, arg, relay );
    }
    pthread_sigmask( SIG_UNBLOCK, &server_block_set, NULL );
    ctx->ContextFlags = CONTEXT_FULL;
    LdrInitializeThunk( ctx, (void **)&ctx->Rcx, 0, 0 );
    return ctx;
}


/***********************************************************************
 *           signal_start_thread
 */
__ASM_GLOBAL_FUNC( signal_start_thread,
                   "subq $56,%rsp\n\t"
                   __ASM_SEH(".seh_stackalloc 56\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   __ASM_CFI(".cfi_adjust_cfa_offset 56\n\t")
                   "movq %rbp,48(%rsp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %rbp,48\n\t")
                   "movq %rbx,40(%rsp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %rbx,40\n\t")
                   "movq %r12,32(%rsp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %r12,32\n\t")
                   "movq %r13,24(%rsp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %r13,24\n\t")
                   "movq %r14,16(%rsp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %r14,16\n\t")
                   "movq %r15,8(%rsp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %r15,8\n\t")
                   /* store exit frame */
                   "movq %gs:0x30,%rax\n\t"
                   "movq %rsp,0x330(%rax)\n\t"      /* amd64_thread_data()->exit_frame */
                   /* switch to thread stack */
                   "movq 8(%rax),%rax\n\t"          /* NtCurrentTeb()->Tib.StackBase */
                   "leaq -0x1000(%rax),%rsp\n\t"
                   /* attach dlls */
                   "call " __ASM_NAME("attach_thread") "\n\t"
                   "movq %rax,%rbx\n\t"
                   "leaq -32(%rax),%rsp\n\t"
                   /* clear the stack */
                   "andq $~0xfff,%rax\n\t"  /* round down to page size */
                   "movq %rax,%rdi\n\t"
                   "call " __ASM_NAME("virtual_clear_thread_stack") "\n\t"
                   /* switch to the initial context */
                   "movl $1,%edx\n\t"
                   "movq %rbx,%rcx\n\t"
                   "call " __ASM_NAME("NtContinue") )


/***********************************************************************
 *           signal_exit_thread
 */
__ASM_GLOBAL_FUNC( signal_exit_thread,
                   /* fetch exit frame */
                   "movq %gs:0x30,%rax\n\t"
                   "movq 0x330(%rax),%rdx\n\t"      /* amd64_thread_data()->exit_frame */
                   "testq %rdx,%rdx\n\t"
                   "jnz 1f\n\t"
                   "jmp *%rsi\n"
                   /* switch to exit frame stack */
                   "1:\tmovq $0,0x330(%rax)\n\t"
                   "movq %rdx,%rsp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 56\n\t")
                   __ASM_CFI(".cfi_rel_offset %rbp,48\n\t")
                   __ASM_CFI(".cfi_rel_offset %rbx,40\n\t")
                   __ASM_CFI(".cfi_rel_offset %r12,32\n\t")
                   __ASM_CFI(".cfi_rel_offset %r13,24\n\t")
                   __ASM_CFI(".cfi_rel_offset %r14,16\n\t")
                   __ASM_CFI(".cfi_rel_offset %r15,8\n\t")
                   "call *%rsi" )

#endif  /* __x86_64__ */
