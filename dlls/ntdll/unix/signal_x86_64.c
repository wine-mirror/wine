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

#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#ifdef HAVE_MACHINE_SYSARCH_H
# include <machine/sysarch.h>
#endif
#ifdef HAVE_SYS_AUXV_H
# include <sys/auxv.h>
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
#include "ddk/wdm.h"
#include "wine/list.h"
#include "wine/asm.h"
#include "unix_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(unwind);
WINE_DECLARE_DEBUG_CHANNEL(seh);

#include "dwarf.h"

/***********************************************************************
 * signal context platform-specific definitions
 */
#ifdef linux

#include <asm/prctl.h>
static inline int arch_prctl( int func, void *ptr ) { return syscall( __NR_arch_prctl, func, ptr ); }

extern int CDECL alloc_fs_sel( int sel, void *base ) DECLSPEC_HIDDEN;
__ASM_GLOBAL_FUNC( alloc_fs_sel,
                   /* switch to 32-bit stack */
                   "pushq %rbx\n\t"
                   "pushq %rdi\n\t"
                   "movq %rsp,%rdi\n\t"
                   "movl 0x4(%rdx),%esp\n\t"  /* Tib.StackBase */
                   "subl $0x10,%esp\n\t"
                   /* setup modify_ldt struct on 32-bit stack */
                   "movl %ecx,(%rsp)\n\t"     /* entry_number */
                   "movl %edx,4(%rsp)\n\t"    /* base */
                   "movl $~0,8(%rsp)\n\t"     /* limit */
                   "movl $0x41,12(%rsp)\n\t"  /* seg_32bit | usable */
                   /* invoke 32-bit syscall */
                   "movl %esp,%ebx\n\t"
                   "movl $0xf3,%eax\n\t"      /* SYS_set_thread_area */
                   "int $0x80\n\t"
                   /* restore stack */
                   "movl (%rsp),%eax\n\t"     /* entry_number */
                   "movq %rdi,%rsp\n\t"
                   "popq %rdi\n\t"
                   "popq %rbx\n\t"
                   "ret" );

#ifndef FP_XSTATE_MAGIC1
#define FP_XSTATE_MAGIC1 0x46505853
#endif

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
#define XState_sig(fpu)      (((unsigned int *)fpu->Reserved4)[12] == FP_XSTATE_MAGIC1 ? (XSTATE *)(fpu + 1) : NULL)

#elif defined(__FreeBSD__) || defined (__FreeBSD_kernel__)

#include <machine/trap.h>

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
#define XState_sig(context)  NULL

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
#define XState_sig(context) NULL

#elif defined (__APPLE__)

#include <i386/user_ldt.h>

/* The 'full' structs were added in the macOS 10.14.6 SDK/Xcode 10.3. */
#ifndef _STRUCT_X86_THREAD_FULL_STATE64
#define _STRUCT_X86_THREAD_FULL_STATE64 struct __darwin_x86_thread_full_state64
_STRUCT_X86_THREAD_FULL_STATE64
{
        _STRUCT_X86_THREAD_STATE64      __ss64;
        __uint64_t                      __ds;
        __uint64_t                      __es;
        __uint64_t                      __ss;
        __uint64_t                      __gsbase;
};
#endif

#ifndef _STRUCT_MCONTEXT64_FULL
#define _STRUCT_MCONTEXT64_FULL      struct __darwin_mcontext64_full
_STRUCT_MCONTEXT64_FULL
{
        _STRUCT_X86_EXCEPTION_STATE64   __es;
        _STRUCT_X86_THREAD_FULL_STATE64 __ss;
        _STRUCT_X86_FLOAT_STATE64       __fs;
};
#endif

#ifndef _STRUCT_MCONTEXT_AVX64_FULL
#define _STRUCT_MCONTEXT_AVX64_FULL  struct __darwin_mcontext_avx64_full
_STRUCT_MCONTEXT_AVX64_FULL
{
        _STRUCT_X86_EXCEPTION_STATE64   __es;
        _STRUCT_X86_THREAD_FULL_STATE64 __ss;
        _STRUCT_X86_AVX_STATE64         __fs;
};
#endif

/* AVX512 structs were added in the macOS 10.13 SDK/Xcode 9. */
#ifdef _STRUCT_MCONTEXT_AVX512_64_FULL
#define SIZEOF_STRUCT_MCONTEXT_AVX512_64_FULL sizeof(_STRUCT_MCONTEXT_AVX512_64_FULL)
#else
#define SIZEOF_STRUCT_MCONTEXT_AVX512_64_FULL 2664
#endif

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
#define RIP_sig(context)     ((context)->uc_mcontext->__ss.__rip)
#define RSP_sig(context)     ((context)->uc_mcontext->__ss.__rsp)
#define TRAP_sig(context)    ((context)->uc_mcontext->__es.__trapno)
#define ERROR_sig(context)   ((context)->uc_mcontext->__es.__err)

/* Once a custom LDT is installed (i.e. Wow64), mcontext will point to a
 * larger '_full' struct which includes DS, ES, SS, and GSbase.
 * This changes the offset of the FPU state.
 * Checking mcsize is the only way to determine which mcontext is in use.
 */
static inline XMM_SAVE_AREA32 *FPU_sig( const ucontext_t *context )
{
    if (context->uc_mcsize == sizeof(_STRUCT_MCONTEXT64_FULL) ||
        context->uc_mcsize == sizeof(_STRUCT_MCONTEXT_AVX64_FULL) ||
        context->uc_mcsize == SIZEOF_STRUCT_MCONTEXT_AVX512_64_FULL)
    {
        return (XMM_SAVE_AREA32 *)&((_STRUCT_MCONTEXT64_FULL *)context->uc_mcontext)->__fs.__fpu_fcw;
    }
    return (XMM_SAVE_AREA32 *)&(context)->uc_mcontext->__fs.__fpu_fcw;
}

#define XState_sig(context)  NULL

#else
#error You must define the signal context functions for your platform
#endif

enum i386_trap_code
{
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
    TRAP_x86_CACHEFLT   = 19   /* Cache flush exception */
#endif
};

/* stack layout when calling an exception raise function */
struct stack_layout
{
    CONTEXT           context;
    CONTEXT_EX        context_ex;
    EXCEPTION_RECORD  rec;
    ULONG64           align;
    char              xstate[0]; /* If xstate is present it is allocated
                                  * dynamically to provide 64 byte alignment. */
};

C_ASSERT((offsetof(struct stack_layout, xstate) == sizeof(struct stack_layout)));

C_ASSERT( sizeof(XSTATE) == 0x140 );
C_ASSERT( sizeof(struct stack_layout) == 0x590 ); /* Should match the size in call_user_exception_dispatcher(). */

/* flags to control the behavior of the syscall dispatcher */
#define SYSCALL_HAVE_XSAVE       1
#define SYSCALL_HAVE_XSAVEC      2
#define SYSCALL_HAVE_PTHREAD_TEB 4
#define SYSCALL_HAVE_WRFSGSBASE  8

static unsigned int syscall_flags;

/* stack layout when calling an user apc function.
 * FIXME: match Windows ABI. */
struct apc_stack_layout
{
    ULONG64  save_regs[4];
    void            *func;
    ULONG64         align;
    CONTEXT       context;
    ULONG64           rbp;
    ULONG64           rip;
};

struct syscall_frame
{
    ULONG64               rax;           /* 0000 */
    ULONG64               rbx;           /* 0008 */
    ULONG64               rcx;           /* 0010 */
    ULONG64               rdx;           /* 0018 */
    ULONG64               rsi;           /* 0020 */
    ULONG64               rdi;           /* 0028 */
    ULONG64               r8;            /* 0030 */
    ULONG64               r9;            /* 0038 */
    ULONG64               r10;           /* 0040 */
    ULONG64               r11;           /* 0048 */
    ULONG64               r12;           /* 0050 */
    ULONG64               r13;           /* 0058 */
    ULONG64               r14;           /* 0060 */
    ULONG64               r15;           /* 0068 */
    ULONG64               rip;           /* 0070 */
    WORD                  cs;            /* 0078 */
    WORD                  ds;            /* 007a */
    WORD                  es;            /* 007c */
    WORD                  fs;            /* 007e */
    ULONG64               eflags;        /* 0080 */
    ULONG64               rsp;           /* 0088 */
    WORD                  ss;            /* 0090 */
    WORD                  gs;            /* 0092 */
    DWORD                 restore_flags; /* 0094 */
    ULONG64               rbp;           /* 0098 */
    struct syscall_frame *prev_frame;    /* 00a0 */
    SYSTEM_SERVICE_TABLE *syscall_table; /* 00a8 */
    DWORD                 syscall_flags; /* 00b0 */
    DWORD                 align[3];      /* 00b4 */
    XMM_SAVE_AREA32       xsave;         /* 00c0 */
    DECLSPEC_ALIGN(64) XSTATE xstate;    /* 02c0 */
};

C_ASSERT( sizeof( struct syscall_frame ) == 0x400);

struct amd64_thread_data
{
    DWORD_PTR             dr0;           /* 02f0 debug registers */
    DWORD_PTR             dr1;           /* 02f8 */
    DWORD_PTR             dr2;           /* 0300 */
    DWORD_PTR             dr3;           /* 0308 */
    DWORD_PTR             dr6;           /* 0310 */
    DWORD_PTR             dr7;           /* 0318 */
    void                 *exit_frame;    /* 0320 exit frame pointer */
    struct syscall_frame *syscall_frame; /* 0328 syscall frame pointer */
    void                 *pthread_teb;   /* 0330 thread data for pthread */
    DWORD                 fs;            /* 0338 WOW TEB selector */
};

C_ASSERT( sizeof(struct amd64_thread_data) <= sizeof(((struct ntdll_thread_data *)0)->cpu_data) );
C_ASSERT( offsetof( TEB, GdiTebBatch ) + offsetof( struct amd64_thread_data, exit_frame ) == 0x320 );
C_ASSERT( offsetof( TEB, GdiTebBatch ) + offsetof( struct amd64_thread_data, syscall_frame ) == 0x328 );
C_ASSERT( offsetof( TEB, GdiTebBatch ) + offsetof( struct amd64_thread_data, pthread_teb ) == 0x330 );

static inline struct amd64_thread_data *amd64_thread_data(void)
{
    return (struct amd64_thread_data *)ntdll_get_thread_data()->cpu_data;
}

#ifdef __linux__
static inline TEB *get_current_teb(void)
{
    unsigned long rsp;
    __asm__( "movq %%rsp,%0" : "=r" (rsp) );
    return (TEB *)(rsp & ~signal_stack_mask);
}
#endif

static BOOL is_inside_syscall( const ucontext_t *sigcontext )
{
    return ((char *)RSP_sig(sigcontext) >= (char *)ntdll_get_thread_data()->kernel_stack &&
            (char *)RSP_sig(sigcontext) <= (char *)amd64_thread_data()->syscall_frame);
}


struct xcontext
{
    CONTEXT c;
    CONTEXT_EX c_ex;
    ULONG64 host_compaction_mask;
};

extern BOOL xstate_compaction_enabled DECLSPEC_HIDDEN;

static inline XSTATE *xstate_from_context( const CONTEXT *context )
{
    CONTEXT_EX *xctx = (CONTEXT_EX *)(context + 1);

    if ((context->ContextFlags & CONTEXT_XSTATE) != CONTEXT_XSTATE) return NULL;
    return (XSTATE *)((char *)xctx + xctx->XState.Offset);
}

static inline void context_init_xstate( CONTEXT *context, void *xstate_buffer )
{
    CONTEXT_EX *xctx;

    xctx = (CONTEXT_EX *)(context + 1);
    xctx->Legacy.Length = sizeof(CONTEXT);
    xctx->Legacy.Offset = -(LONG)sizeof(CONTEXT);

    if (xstate_buffer)
    {
        xctx->XState.Length = sizeof(XSTATE);
        xctx->XState.Offset = (BYTE *)xstate_buffer - (BYTE *)xctx;
        context->ContextFlags |= CONTEXT_XSTATE;

        xctx->All.Length = sizeof(CONTEXT) + xctx->XState.Offset + xctx->XState.Length;
    }
    else
    {
        xctx->XState.Length = 25;
        xctx->XState.Offset = 0;

        xctx->All.Length = sizeof(CONTEXT) + 24; /* sizeof(CONTEXT_EX) minus 8 alignment bytes on x64. */
    }

    xctx->All.Offset = -(LONG)sizeof(CONTEXT);
}

static USHORT cs32_sel;  /* selector for %cs in 32-bit mode */
static USHORT cs64_sel;  /* selector for %cs in 64-bit mode */
static USHORT ds64_sel;  /* selector for %ds/%es/%ss in 64-bit mode */
static USHORT fs32_sel;  /* selector for %fs in 32-bit mode */


/***********************************************************************
 *           dwarf_virtual_unwind
 *
 * Equivalent of RtlVirtualUnwind for builtin modules.
 */
static NTSTATUS dwarf_virtual_unwind( ULONG64 ip, ULONG64 *frame,CONTEXT *context,
                                      const struct dwarf_fde *fde, const struct dwarf_eh_bases *bases,
                                      PEXCEPTION_ROUTINE *handler, void **handler_data )
{
    const struct dwarf_cie *cie;
    const unsigned char *ptr, *augmentation, *end;
    ULONG_PTR len, code_end;
    struct frame_info info;
    struct frame_state state_stack[MAX_SAVED_STATES];
    int aug_z_format = 0;
    unsigned char lsda_encoding = DW_EH_PE_omit;

    memset( &info, 0, sizeof(info) );
    info.state_stack = state_stack;
    info.ip = (ULONG_PTR)bases->func;
    *handler = NULL;

    cie = (const struct dwarf_cie *)((const char *)&fde->cie_offset - fde->cie_offset);

    /* parse the CIE first */

    if (cie->version != 1 && cie->version != 3)
    {
        FIXME( "unknown CIE version %u at %p\n", cie->version, cie );
        return STATUS_INVALID_DISPOSITION;
    }
    ptr = cie->augmentation + strlen((const char *)cie->augmentation) + 1;

    info.code_align = dwarf_get_uleb128( &ptr );
    info.data_align = dwarf_get_sleb128( &ptr );
    if (cie->version == 1)
        info.retaddr_reg = *ptr++;
    else
        info.retaddr_reg = dwarf_get_uleb128( &ptr );
    info.state.cfa_rule = RULE_CFA_OFFSET;

    TRACE( "function %lx base %p cie %p len %x id %x version %x aug '%s' code_align %lu data_align %ld retaddr %s\n",
           ip, bases->func, cie, cie->length, cie->id, cie->version, cie->augmentation,
           info.code_align, info.data_align, dwarf_reg_names[info.retaddr_reg] );

    end = NULL;
    for (augmentation = cie->augmentation; *augmentation; augmentation++)
    {
        switch (*augmentation)
        {
        case 'z':
            len = dwarf_get_uleb128( &ptr );
            end = ptr + len;
            aug_z_format = 1;
            continue;
        case 'L':
            lsda_encoding = *ptr++;
            continue;
        case 'P':
        {
            unsigned char encoding = *ptr++;
            *handler = (void *)dwarf_get_ptr( &ptr, encoding, bases );
            continue;
        }
        case 'R':
            info.fde_encoding = *ptr++;
            continue;
        case 'S':
            info.signal_frame = 1;
            continue;
        }
        FIXME( "unknown augmentation '%c'\n", *augmentation );
        if (!end) return STATUS_INVALID_DISPOSITION;  /* cannot continue */
        break;
    }
    if (end) ptr = end;

    end = (const unsigned char *)(&cie->length + 1) + cie->length;
    execute_cfa_instructions( ptr, end, ip, &info, bases );

    ptr = (const unsigned char *)(fde + 1);
    info.ip = dwarf_get_ptr( &ptr, info.fde_encoding, bases );  /* fde code start */
    code_end = info.ip + dwarf_get_ptr( &ptr, info.fde_encoding & 0x0f, bases );  /* fde code length */

    if (aug_z_format)  /* get length of augmentation data */
    {
        len = dwarf_get_uleb128( &ptr );
        end = ptr + len;
    }
    else end = NULL;

    *handler_data = (void *)dwarf_get_ptr( &ptr, lsda_encoding, bases );
    if (end) ptr = end;

    end = (const unsigned char *)(&fde->length + 1) + fde->length;
    TRACE( "fde %p len %x personality %p lsda %p code %lx-%lx\n",
           fde, fde->length, *handler, *handler_data, info.ip, code_end );
    execute_cfa_instructions( ptr, end, ip, &info, bases );
    *frame = context->Rsp;
    apply_frame_state( context, &info.state, bases );

    TRACE( "next function rip=%016lx\n", context->Rip );
    TRACE( "  rax=%016lx rbx=%016lx rcx=%016lx rdx=%016lx\n",
           context->Rax, context->Rbx, context->Rcx, context->Rdx );
    TRACE( "  rsi=%016lx rdi=%016lx rbp=%016lx rsp=%016lx\n",
           context->Rsi, context->Rdi, context->Rbp, context->Rsp );
    TRACE( "   r8=%016lx  r9=%016lx r10=%016lx r11=%016lx\n",
           context->R8, context->R9, context->R10, context->R11 );
    TRACE( "  r12=%016lx r13=%016lx r14=%016lx r15=%016lx\n",
           context->R12, context->R13, context->R14, context->R15 );

    return STATUS_SUCCESS;
}


#ifdef HAVE_LIBUNWIND
/***********************************************************************
 *           libunwind_virtual_unwind
 *
 * Equivalent of RtlVirtualUnwind for builtin modules.
 */
static NTSTATUS libunwind_virtual_unwind( ULONG64 ip, ULONG64 *frame, CONTEXT *context,
                                          PEXCEPTION_ROUTINE *handler, void **handler_data )
{
    unw_context_t unw_context;
    unw_cursor_t cursor;
    unw_proc_info_t info;
    int rc;

#ifdef __APPLE__
    rc = unw_getcontext( &unw_context );
    if (rc == UNW_ESUCCESS)
        rc = unw_init_local( &cursor, &unw_context );
    if (rc == UNW_ESUCCESS)
    {
        unw_set_reg( &cursor, UNW_REG_IP,     context->Rip );
        unw_set_reg( &cursor, UNW_REG_SP,     context->Rsp );
        unw_set_reg( &cursor, UNW_X86_64_RAX, context->Rax );
        unw_set_reg( &cursor, UNW_X86_64_RDX, context->Rdx );
        unw_set_reg( &cursor, UNW_X86_64_RCX, context->Rcx );
        unw_set_reg( &cursor, UNW_X86_64_RBX, context->Rbx );
        unw_set_reg( &cursor, UNW_X86_64_RSI, context->Rsi );
        unw_set_reg( &cursor, UNW_X86_64_RDI, context->Rdi );
        unw_set_reg( &cursor, UNW_X86_64_RBP, context->Rbp );
        unw_set_reg( &cursor, UNW_X86_64_R8,  context->R8 );
        unw_set_reg( &cursor, UNW_X86_64_R9,  context->R9 );
        unw_set_reg( &cursor, UNW_X86_64_R10, context->R10 );
        unw_set_reg( &cursor, UNW_X86_64_R11, context->R11 );
        unw_set_reg( &cursor, UNW_X86_64_R12, context->R12 );
        unw_set_reg( &cursor, UNW_X86_64_R13, context->R13 );
        unw_set_reg( &cursor, UNW_X86_64_R14, context->R14 );
        unw_set_reg( &cursor, UNW_X86_64_R15, context->R15 );
    }
#else
    RAX_sig(&unw_context) = context->Rax;
    RCX_sig(&unw_context) = context->Rcx;
    RDX_sig(&unw_context) = context->Rdx;
    RBX_sig(&unw_context) = context->Rbx;
    RSP_sig(&unw_context) = context->Rsp;
    RBP_sig(&unw_context) = context->Rbp;
    RSI_sig(&unw_context) = context->Rsi;
    RDI_sig(&unw_context) = context->Rdi;
    R8_sig(&unw_context)  = context->R8;
    R9_sig(&unw_context)  = context->R9;
    R10_sig(&unw_context) = context->R10;
    R11_sig(&unw_context) = context->R11;
    R12_sig(&unw_context) = context->R12;
    R13_sig(&unw_context) = context->R13;
    R14_sig(&unw_context) = context->R14;
    R15_sig(&unw_context) = context->R15;
    RIP_sig(&unw_context) = context->Rip;
    CS_sig(&unw_context)  = context->SegCs;
    FS_sig(&unw_context)  = context->SegFs;
    GS_sig(&unw_context)  = context->SegGs;
    EFL_sig(&unw_context) = context->EFlags;
    rc = unw_init_local( &cursor, &unw_context );
#endif
    if (rc != UNW_ESUCCESS)
    {
        WARN( "setup failed: %d\n", rc );
        return STATUS_INVALID_DISPOSITION;
    }

    *handler = NULL;
    *frame = context->Rsp;

    rc = unw_get_proc_info(&cursor, &info);
    if (UNW_ENOINFO < 0) rc = -rc;  /* LLVM libunwind has negative error codes */
    if (rc != UNW_ESUCCESS && rc != -UNW_ENOINFO)
    {
        WARN( "failed to get info: %d\n", rc );
        return STATUS_INVALID_DISPOSITION;
    }
    if (rc == -UNW_ENOINFO || ip < info.start_ip || ip > info.end_ip || info.end_ip == info.start_ip + 1)
        return STATUS_UNSUCCESSFUL;

    TRACE( "ip %#lx function %#lx-%#lx personality %#lx lsda %#lx fde %#lx\n",
           ip, (unsigned long)info.start_ip, (unsigned long)info.end_ip, (unsigned long)info.handler,
           (unsigned long)info.lsda, (unsigned long)info.unwind_info );

    if (!(rc = unw_step( &cursor )))
    {
        WARN( "last frame\n" );
        return STATUS_UNSUCCESSFUL;
    }
    if (rc < 0)
    {
        WARN( "failed to unwind: %d\n", rc );
        return STATUS_INVALID_DISPOSITION;
    }

    unw_get_reg( &cursor, UNW_REG_IP,     (unw_word_t *)&context->Rip );
    unw_get_reg( &cursor, UNW_REG_SP,     (unw_word_t *)&context->Rsp );
    unw_get_reg( &cursor, UNW_X86_64_RAX, (unw_word_t *)&context->Rax );
    unw_get_reg( &cursor, UNW_X86_64_RDX, (unw_word_t *)&context->Rdx );
    unw_get_reg( &cursor, UNW_X86_64_RCX, (unw_word_t *)&context->Rcx );
    unw_get_reg( &cursor, UNW_X86_64_RBX, (unw_word_t *)&context->Rbx );
    unw_get_reg( &cursor, UNW_X86_64_RSI, (unw_word_t *)&context->Rsi );
    unw_get_reg( &cursor, UNW_X86_64_RDI, (unw_word_t *)&context->Rdi );
    unw_get_reg( &cursor, UNW_X86_64_RBP, (unw_word_t *)&context->Rbp );
    unw_get_reg( &cursor, UNW_X86_64_R8,  (unw_word_t *)&context->R8 );
    unw_get_reg( &cursor, UNW_X86_64_R9,  (unw_word_t *)&context->R9 );
    unw_get_reg( &cursor, UNW_X86_64_R10, (unw_word_t *)&context->R10 );
    unw_get_reg( &cursor, UNW_X86_64_R11, (unw_word_t *)&context->R11 );
    unw_get_reg( &cursor, UNW_X86_64_R12, (unw_word_t *)&context->R12 );
    unw_get_reg( &cursor, UNW_X86_64_R13, (unw_word_t *)&context->R13 );
    unw_get_reg( &cursor, UNW_X86_64_R14, (unw_word_t *)&context->R14 );
    unw_get_reg( &cursor, UNW_X86_64_R15, (unw_word_t *)&context->R15 );
    *handler = (void*)info.handler;
    *handler_data = (void*)info.lsda;

    TRACE( "next function rip=%016lx\n", context->Rip );
    TRACE( "  rax=%016lx rbx=%016lx rcx=%016lx rdx=%016lx\n",
           context->Rax, context->Rbx, context->Rcx, context->Rdx );
    TRACE( "  rsi=%016lx rdi=%016lx rbp=%016lx rsp=%016lx\n",
           context->Rsi, context->Rdi, context->Rbp, context->Rsp );
    TRACE( "   r8=%016lx  r9=%016lx r10=%016lx r11=%016lx\n",
           context->R8, context->R9, context->R10, context->R11 );
    TRACE( "  r12=%016lx r13=%016lx r14=%016lx r15=%016lx\n",
           context->R12, context->R13, context->R14, context->R15 );

    return STATUS_SUCCESS;
}
#endif


/***********************************************************************
 *           unwind_builtin_dll
 */
NTSTATUS unwind_builtin_dll( void *args )
{
    struct unwind_builtin_dll_params *params = args;
    DISPATCHER_CONTEXT *dispatch = params->dispatch;
    CONTEXT *context = params->context;
    struct dwarf_eh_bases bases;
    const struct dwarf_fde *fde = _Unwind_Find_FDE( (void *)(context->Rip - 1), &bases );

    if (fde)
        return dwarf_virtual_unwind( context->Rip, &dispatch->EstablisherFrame, context, fde,
                                     &bases, &dispatch->LanguageHandler, &dispatch->HandlerData );
#ifdef HAVE_LIBUNWIND
    return libunwind_virtual_unwind( context->Rip, &dispatch->EstablisherFrame, context,
                                     &dispatch->LanguageHandler, &dispatch->HandlerData );
#endif
    return STATUS_UNSUCCESSFUL;
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
 *           init_handler
 */
static inline void init_handler( const ucontext_t *sigcontext )
{
#ifdef __linux__
    if (fs32_sel)
    {
        struct ntdll_thread_data *thread_data = (struct ntdll_thread_data *)&get_current_teb()->GdiTebBatch;
        arch_prctl( ARCH_SET_FS, ((struct amd64_thread_data *)thread_data->cpu_data)->pthread_teb );
    }
#endif
}


/***********************************************************************
 *           leave_handler
 */
static inline void leave_handler( const ucontext_t *sigcontext )
{
#ifdef __linux__
    if (fs32_sel && !is_inside_signal_stack( (void *)RSP_sig(sigcontext )) && !is_inside_syscall(sigcontext))
        __asm__ volatile( "movw %0,%%fs" :: "r" (fs32_sel) );
#endif
}


/***********************************************************************
 *           save_context
 *
 * Set the register values from a sigcontext.
 */
static void save_context( struct xcontext *xcontext, const ucontext_t *sigcontext )
{
    CONTEXT *context = &xcontext->c;

    init_handler( sigcontext );

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
        XSTATE *xs;

        context->ContextFlags |= CONTEXT_FLOATING_POINT;
        context->u.FltSave = *FPU_sig(sigcontext);
        context->MxCsr = context->u.FltSave.MxCsr;
        if ((cpu_info.ProcessorFeatureBits & CPU_FEATURE_AVX) && (xs = XState_sig(FPU_sig(sigcontext))))
        {
            /* xcontext and sigcontext are both on the signal stack, so we can
             * just reference sigcontext without overflowing 32 bit XState.Offset */
            context_init_xstate( context, xs );
            assert( xcontext->c_ex.XState.Offset == (BYTE *)xs - (BYTE *)&xcontext->c_ex );
            xcontext->host_compaction_mask = xs->CompactionMask;
        }
    }
}


/***********************************************************************
 *           restore_context
 *
 * Build a sigcontext from the register values.
 */
static void restore_context( const struct xcontext *xcontext, ucontext_t *sigcontext )
{
    const CONTEXT *context = &xcontext->c;
    XSTATE *xs;

    amd64_thread_data()->dr0 = context->Dr0;
    amd64_thread_data()->dr1 = context->Dr1;
    amd64_thread_data()->dr2 = context->Dr2;
    amd64_thread_data()->dr3 = context->Dr3;
    amd64_thread_data()->dr6 = context->Dr6;
    amd64_thread_data()->dr7 = context->Dr7;
    set_sigcontext( context, sigcontext );
    if (FPU_sig(sigcontext)) *FPU_sig(sigcontext) = context->u.FltSave;
    if ((cpu_info.ProcessorFeatureBits & CPU_FEATURE_AVX) && (xs = XState_sig(FPU_sig(sigcontext))))
        xs->CompactionMask = xcontext->host_compaction_mask;
    leave_handler( sigcontext );
}


/***********************************************************************
 *           signal_set_full_context
 */
NTSTATUS signal_set_full_context( CONTEXT *context )
{
    NTSTATUS status = NtSetContextThread( GetCurrentThread(), context );

    if (!status && (context->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER)
        amd64_thread_data()->syscall_frame->restore_flags |= CONTEXT_INTEGER;
    return status;
}


/***********************************************************************
 *              get_native_context
 */
void *get_native_context( CONTEXT *context )
{
    return context;
}


/***********************************************************************
 *              get_wow_context
 */
void *get_wow_context( CONTEXT *context )
{
    if (context->SegCs != cs64_sel) return NULL;
    return get_cpu_area( IMAGE_FILE_MACHINE_I386 );
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
    struct syscall_frame *frame = amd64_thread_data()->syscall_frame;

    if ((flags & CONTEXT_XSTATE) && (cpu_info.ProcessorFeatureBits & CPU_FEATURE_AVX))
    {
        CONTEXT_EX *context_ex = (CONTEXT_EX *)(context + 1);
        XSTATE *xs = (XSTATE *)((char *)context_ex + context_ex->XState.Offset);

        if (context_ex->XState.Length < offsetof(XSTATE, YmmContext) ||
            context_ex->XState.Length > sizeof(XSTATE))
            return STATUS_INVALID_PARAMETER;
        if ((xs->Mask & XSTATE_MASK_GSSE) && (context_ex->XState.Length < sizeof(XSTATE)))
            return STATUS_BUFFER_OVERFLOW;
    }
    else flags &= ~CONTEXT_XSTATE;

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
        ret = set_thread_context( handle, context, &self, IMAGE_FILE_MACHINE_AMD64 );
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

    if (flags & CONTEXT_INTEGER)
    {
        frame->rax = context->Rax;
        frame->rbx = context->Rbx;
        frame->rcx = context->Rcx;
        frame->rdx = context->Rdx;
        frame->rsi = context->Rsi;
        frame->rdi = context->Rdi;
        frame->r8  = context->R8;
        frame->r9  = context->R9;
        frame->r10 = context->R10;
        frame->r11 = context->R11;
        frame->r12 = context->R12;
        frame->r13 = context->R13;
        frame->r14 = context->R14;
        frame->r15 = context->R15;
    }
    if (flags & CONTEXT_CONTROL)
    {
        frame->rsp    = context->Rsp;
        frame->rbp    = context->Rbp;
        frame->rip    = context->Rip;
        frame->eflags = context->EFlags;
        frame->cs     = context->SegCs;
        frame->ss     = context->SegSs;
    }
    if (flags & CONTEXT_SEGMENTS)
    {
        frame->ds = context->SegDs;
        frame->es = context->SegEs;
        frame->fs = context->SegFs;
        frame->gs = context->SegGs;
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
        frame->xsave = context->u.FltSave;
        frame->xstate.Mask |= XSTATE_MASK_LEGACY;
    }
    if (flags & CONTEXT_XSTATE)
    {
        CONTEXT_EX *context_ex = (CONTEXT_EX *)(context + 1);
        XSTATE *xs = (XSTATE *)((char *)context_ex + context_ex->XState.Offset);

        if (xs->Mask & XSTATE_MASK_GSSE)
        {
            frame->xstate.Mask |= XSTATE_MASK_GSSE;
            memcpy( &frame->xstate.YmmContext, &xs->YmmContext, sizeof(xs->YmmContext) );
        }
        else frame->xstate.Mask &= ~XSTATE_MASK_GSSE;
    }

    frame->restore_flags |= flags & ~CONTEXT_INTEGER;
    return STATUS_SUCCESS;
}


/***********************************************************************
 *              NtGetContextThread  (NTDLL.@)
 *              ZwGetContextThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtGetContextThread( HANDLE handle, CONTEXT *context )
{
    struct syscall_frame *frame = amd64_thread_data()->syscall_frame;
    DWORD needed_flags = context->ContextFlags & ~CONTEXT_AMD64;
    BOOL self = (handle == GetCurrentThread());

    /* debug registers require a server call */
    if (needed_flags & CONTEXT_DEBUG_REGISTERS) self = FALSE;

    if (!self)
    {
        NTSTATUS ret = get_thread_context( handle, context, &self, IMAGE_FILE_MACHINE_AMD64 );
        if (ret || !self) return ret;
    }

    if (needed_flags & CONTEXT_INTEGER)
    {
        context->Rax = frame->rax;
        context->Rbx = frame->rbx;
        context->Rcx = frame->rcx;
        context->Rdx = frame->rdx;
        context->Rsi = frame->rsi;
        context->Rdi = frame->rdi;
        context->R8  = frame->r8;
        context->R9  = frame->r9;
        context->R10 = frame->r10;
        context->R11 = frame->r11;
        context->R12 = frame->r12;
        context->R13 = frame->r13;
        context->R14 = frame->r14;
        context->R15 = frame->r15;
        context->ContextFlags |= CONTEXT_INTEGER;
    }
    if (needed_flags & CONTEXT_CONTROL)
    {
        context->Rsp    = frame->rsp;
        context->Rbp    = frame->rbp;
        context->Rip    = frame->rip;
        context->EFlags = frame->eflags;
        context->SegCs  = frame->cs;
        context->SegSs  = frame->ss;
        context->ContextFlags |= CONTEXT_CONTROL;
    }
    if (needed_flags & CONTEXT_SEGMENTS)
    {
        context->SegDs  = frame->ds;
        context->SegEs  = frame->es;
        context->SegFs  = frame->fs;
        context->SegGs  = frame->gs;
        context->ContextFlags |= CONTEXT_SEGMENTS;
    }
    if (needed_flags & CONTEXT_FLOATING_POINT)
    {
        if (!xstate_compaction_enabled ||
            (frame->xstate.Mask & XSTATE_MASK_LEGACY_FLOATING_POINT))
        {
            memcpy( &context->u.FltSave, &frame->xsave, FIELD_OFFSET( XSAVE_FORMAT, MxCsr ));
            memcpy( context->u.FltSave.FloatRegisters, frame->xsave.FloatRegisters,
                    sizeof( context->u.FltSave.FloatRegisters ));
        }
        else
        {
            memset( &context->u.FltSave, 0, FIELD_OFFSET( XSAVE_FORMAT, MxCsr ));
            memset( context->u.FltSave.FloatRegisters, 0,
                    sizeof( context->u.FltSave.FloatRegisters ));
            context->u.FltSave.ControlWord = 0x37f;
        }

        if (!xstate_compaction_enabled || (frame->xstate.Mask & XSTATE_MASK_LEGACY_SSE))
        {
            memcpy( context->u.FltSave.XmmRegisters, frame->xsave.XmmRegisters,
                    sizeof( context->u.FltSave.XmmRegisters ));
            context->u.FltSave.MxCsr      = frame->xsave.MxCsr;
            context->u.FltSave.MxCsr_Mask = frame->xsave.MxCsr_Mask;
        }
        else
        {
            memset( context->u.FltSave.XmmRegisters, 0,
                    sizeof( context->u.FltSave.XmmRegisters ));
            context->u.FltSave.MxCsr      = 0x1f80;
            context->u.FltSave.MxCsr_Mask = 0x2ffff;
        }

        context->MxCsr = context->u.FltSave.MxCsr;
        context->ContextFlags |= CONTEXT_FLOATING_POINT;
    }
    if ((needed_flags & CONTEXT_XSTATE) && (cpu_info.ProcessorFeatureBits & CPU_FEATURE_AVX))
    {
        CONTEXT_EX *context_ex = (CONTEXT_EX *)(context + 1);
        XSTATE *xstate = (XSTATE *)((char *)context_ex + context_ex->XState.Offset);
        unsigned int mask;

        if (context_ex->XState.Length < offsetof(XSTATE, YmmContext)
            || context_ex->XState.Length > sizeof(XSTATE))
            return STATUS_INVALID_PARAMETER;

        mask = (xstate_compaction_enabled ? xstate->CompactionMask : xstate->Mask) & XSTATE_MASK_GSSE;
        xstate->Mask = frame->xstate.Mask & mask;
        xstate->CompactionMask = xstate_compaction_enabled ? (0x8000000000000000 | mask) : 0;
        memset( xstate->Reserved, 0, sizeof(xstate->Reserved) );
        if (xstate->Mask)
        {
            if (context_ex->XState.Length < sizeof(XSTATE)) return STATUS_BUFFER_OVERFLOW;
            memcpy( &xstate->YmmContext, &frame->xstate.YmmContext, sizeof(xstate->YmmContext) );
        }
    }
    /* update the cached version of the debug registers */
    if (needed_flags & CONTEXT_DEBUG_REGISTERS)
    {
        amd64_thread_data()->dr0 = context->Dr0;
        amd64_thread_data()->dr1 = context->Dr1;
        amd64_thread_data()->dr2 = context->Dr2;
        amd64_thread_data()->dr3 = context->Dr3;
        amd64_thread_data()->dr6 = context->Dr6;
        amd64_thread_data()->dr7 = context->Dr7;
    }
    return STATUS_SUCCESS;
}


/***********************************************************************
 *              set_thread_wow64_context
 */
NTSTATUS set_thread_wow64_context( HANDLE handle, const void *ctx, ULONG size )
{
    BOOL self = (handle == GetCurrentThread());
    struct syscall_frame *frame = amd64_thread_data()->syscall_frame;
    I386_CONTEXT *wow_frame;
    const I386_CONTEXT *context = ctx;
    DWORD flags = context->ContextFlags & ~CONTEXT_i386;

    if (size != sizeof(I386_CONTEXT)) return STATUS_INFO_LENGTH_MISMATCH;

    /* debug registers require a server call */
    if (self && (flags & CONTEXT_I386_DEBUG_REGISTERS))
        self = (amd64_thread_data()->dr0 == context->Dr0 &&
                amd64_thread_data()->dr1 == context->Dr1 &&
                amd64_thread_data()->dr2 == context->Dr2 &&
                amd64_thread_data()->dr3 == context->Dr3 &&
                amd64_thread_data()->dr6 == context->Dr6 &&
                amd64_thread_data()->dr7 == context->Dr7);

    if (!self)
    {
        NTSTATUS ret = set_thread_context( handle, context, &self, IMAGE_FILE_MACHINE_I386 );
        if (ret || !self) return ret;
        if (flags & CONTEXT_I386_DEBUG_REGISTERS)
        {
            amd64_thread_data()->dr0 = context->Dr0;
            amd64_thread_data()->dr1 = context->Dr1;
            amd64_thread_data()->dr2 = context->Dr2;
            amd64_thread_data()->dr3 = context->Dr3;
            amd64_thread_data()->dr6 = context->Dr6;
            amd64_thread_data()->dr7 = context->Dr7;
        }
        if (!(flags & ~CONTEXT_I386_DEBUG_REGISTERS)) return ret;
    }

    if (!(wow_frame = get_cpu_area( IMAGE_FILE_MACHINE_I386 ))) return STATUS_INVALID_PARAMETER;

    if (flags & CONTEXT_I386_INTEGER)
    {
        wow_frame->Eax = context->Eax;
        wow_frame->Ebx = context->Ebx;
        wow_frame->Ecx = context->Ecx;
        wow_frame->Edx = context->Edx;
        wow_frame->Esi = context->Esi;
        wow_frame->Edi = context->Edi;
    }
    if (flags & CONTEXT_I386_CONTROL)
    {
        WOW64_CPURESERVED *cpu = NtCurrentTeb()->TlsSlots[WOW64_TLS_CPURESERVED];

        wow_frame->Esp    = context->Esp;
        wow_frame->Ebp    = context->Ebp;
        wow_frame->Eip    = context->Eip;
        wow_frame->EFlags = context->EFlags;
        wow_frame->SegCs  = cs32_sel;
        wow_frame->SegSs  = ds64_sel;
        cpu->Flags |= WOW64_CPURESERVED_FLAG_RESET_STATE;
    }
    if (flags & CONTEXT_I386_SEGMENTS)
    {
        wow_frame->SegDs = ds64_sel;
        wow_frame->SegEs = ds64_sel;
        wow_frame->SegFs = amd64_thread_data()->fs;
        wow_frame->SegGs = ds64_sel;
    }
    if (flags & CONTEXT_I386_DEBUG_REGISTERS)
    {
        wow_frame->Dr0 = context->Dr0;
        wow_frame->Dr1 = context->Dr1;
        wow_frame->Dr2 = context->Dr2;
        wow_frame->Dr3 = context->Dr3;
        wow_frame->Dr6 = context->Dr6;
        wow_frame->Dr7 = context->Dr7;
    }
    if (flags & CONTEXT_I386_EXTENDED_REGISTERS)
    {
        memcpy( &frame->xsave, context->ExtendedRegisters, sizeof(frame->xsave) );
        frame->restore_flags |= CONTEXT_FLOATING_POINT;
    }
    else if (flags & CONTEXT_I386_FLOATING_POINT)
    {
        fpu_to_fpux( &frame->xsave, &context->FloatSave );
        frame->restore_flags |= CONTEXT_FLOATING_POINT;
    }
    if (flags & CONTEXT_I386_XSTATE)
    {
        CONTEXT_EX *context_ex = (CONTEXT_EX *)(context + 1);
        XSTATE *xs = (XSTATE *)((char *)context_ex + context_ex->XState.Offset);

        if (xs->Mask & XSTATE_MASK_GSSE)
        {
            frame->xstate.Mask |= XSTATE_MASK_GSSE;
            memcpy( &frame->xstate.YmmContext, &xs->YmmContext, sizeof(xs->YmmContext) );
        }
        else frame->xstate.Mask &= ~XSTATE_MASK_GSSE;
        frame->restore_flags |= CONTEXT_XSTATE;
    }
    return STATUS_SUCCESS;
}


/***********************************************************************
 *              get_thread_wow64_context
 */
NTSTATUS get_thread_wow64_context( HANDLE handle, void *ctx, ULONG size )
{
    DWORD needed_flags;
    struct syscall_frame *frame = amd64_thread_data()->syscall_frame;
    I386_CONTEXT *wow_frame, *context = ctx;
    BOOL self = (handle == GetCurrentThread());

    if (size != sizeof(I386_CONTEXT)) return STATUS_INFO_LENGTH_MISMATCH;

    needed_flags = context->ContextFlags & ~CONTEXT_i386;

    /* debug registers require a server call */
    if (needed_flags & CONTEXT_I386_DEBUG_REGISTERS) self = FALSE;

    if (!self)
    {
        NTSTATUS ret = get_thread_context( handle, context, &self, IMAGE_FILE_MACHINE_I386 );
        if (ret || !self) return ret;
        /* update the cached version of the debug registers */
        if (needed_flags & CONTEXT_I386_DEBUG_REGISTERS)
        {
            amd64_thread_data()->dr0 = context->Dr0;
            amd64_thread_data()->dr1 = context->Dr1;
            amd64_thread_data()->dr2 = context->Dr2;
            amd64_thread_data()->dr3 = context->Dr3;
            amd64_thread_data()->dr6 = context->Dr6;
            amd64_thread_data()->dr7 = context->Dr7;
        }
        if (!(needed_flags & ~CONTEXT_I386_DEBUG_REGISTERS)) return ret;
    }

    if (!(wow_frame = get_cpu_area( IMAGE_FILE_MACHINE_I386 ))) return STATUS_INVALID_PARAMETER;

    if (needed_flags & CONTEXT_I386_INTEGER)
    {
        context->Eax = wow_frame->Eax;
        context->Ebx = wow_frame->Ebx;
        context->Ecx = wow_frame->Ecx;
        context->Edx = wow_frame->Edx;
        context->Esi = wow_frame->Esi;
        context->Edi = wow_frame->Edi;
        context->ContextFlags |= CONTEXT_I386_INTEGER;
    }
    if (needed_flags & CONTEXT_I386_CONTROL)
    {
        context->Esp    = wow_frame->Esp;
        context->Ebp    = wow_frame->Ebp;
        context->Eip    = wow_frame->Eip;
        context->EFlags = wow_frame->EFlags;
        context->SegCs  = wow_frame->SegCs;
        context->SegSs  = wow_frame->SegSs;
        context->ContextFlags |= CONTEXT_I386_CONTROL;
    }
    if (needed_flags & CONTEXT_I386_SEGMENTS)
    {
        context->SegDs = wow_frame->SegDs;
        context->SegEs = wow_frame->SegEs;
        context->SegFs = wow_frame->SegFs;
        context->SegGs = wow_frame->SegGs;
        context->ContextFlags |= CONTEXT_I386_SEGMENTS;
    }
    if (needed_flags & CONTEXT_I386_EXTENDED_REGISTERS)
    {
        memcpy( context->ExtendedRegisters, &frame->xsave, sizeof(context->ExtendedRegisters) );
        context->ContextFlags |= CONTEXT_I386_EXTENDED_REGISTERS;
    }
    if (needed_flags & CONTEXT_I386_FLOATING_POINT)
    {
        fpux_to_fpu( &context->FloatSave, &frame->xsave );
        context->ContextFlags |= CONTEXT_I386_FLOATING_POINT;
    }
    if ((needed_flags & CONTEXT_I386_XSTATE) && (cpu_info.ProcessorFeatureBits & CPU_FEATURE_AVX))
    {
        CONTEXT_EX *context_ex = (CONTEXT_EX *)(context + 1);
        XSTATE *xstate = (XSTATE *)((char *)context_ex + context_ex->XState.Offset);
        unsigned int mask;

        if (context_ex->XState.Length < offsetof(XSTATE, YmmContext) ||
            context_ex->XState.Length > sizeof(XSTATE))
            return STATUS_INVALID_PARAMETER;

        mask = (xstate_compaction_enabled ? xstate->CompactionMask : xstate->Mask) & XSTATE_MASK_GSSE;
        xstate->Mask = frame->xstate.Mask & mask;
        xstate->CompactionMask = xstate_compaction_enabled ? (0x8000000000000000 | mask) : 0;
        memset( xstate->Reserved, 0, sizeof(xstate->Reserved) );
        if (xstate->Mask)
        {
            if (context_ex->XState.Length < sizeof(XSTATE)) return STATUS_BUFFER_OVERFLOW;
            memcpy( &xstate->YmmContext, &frame->xstate.YmmContext, sizeof(xstate->YmmContext) );
        }
    }
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           setup_raise_exception
 */
static void setup_raise_exception( ucontext_t *sigcontext, EXCEPTION_RECORD *rec, struct xcontext *xcontext )
{
    void *stack_ptr = (void *)(RSP_sig(sigcontext) & ~15);
    CONTEXT *context = &xcontext->c;
    struct stack_layout *stack;
    size_t stack_size;
    NTSTATUS status;
    XSTATE *src_xs;

    if (rec->ExceptionCode == EXCEPTION_SINGLE_STEP)
    {
        /* when single stepping can't tell whether this is a hw bp or a
         * single step interrupt. try to avoid as much overhead as possible
         * and only do a server call if there is any hw bp enabled. */

        if (!(context->EFlags & 0x100) || (context->Dr7 & 0xff))
        {
            /* (possible) hardware breakpoint, fetch the debug registers */
            DWORD saved_flags = context->ContextFlags;
            context->ContextFlags = CONTEXT_DEBUG_REGISTERS;
            NtGetContextThread(GetCurrentThread(), context);
            context->ContextFlags |= saved_flags;  /* restore flags */
        }
        context->EFlags &= ~0x100;  /* clear single-step flag */
    }

    status = send_debug_event( rec, context, TRUE );
    if (status == DBG_CONTINUE || status == DBG_EXCEPTION_HANDLED)
    {
        restore_context( xcontext, sigcontext );
        return;
    }

    /* fix up instruction pointer in context for EXCEPTION_BREAKPOINT */
    if (rec->ExceptionCode == EXCEPTION_BREAKPOINT) context->Rip--;

    stack_size = sizeof(*stack) + 0x20;
    if ((src_xs = xstate_from_context( context )))
    {
        stack_size += (ULONG_PTR)stack_ptr - 0x20 - (((ULONG_PTR)stack_ptr - 0x20
                - sizeof(XSTATE)) & ~(ULONG_PTR)63);
    }

    stack = virtual_setup_exception( stack_ptr, stack_size, rec );
    stack->rec          = *rec;
    stack->context      = *context;
    if (src_xs)
    {
        XSTATE *dst_xs = (XSTATE *)stack->xstate;

        assert( !((ULONG_PTR)dst_xs & 63) );
        context_init_xstate( &stack->context, stack->xstate );
        memset( dst_xs, 0, offsetof(XSTATE, YmmContext) );
        dst_xs->CompactionMask = xstate_compaction_enabled ? 0x8000000000000004 : 0;
        if (src_xs->Mask & 4)
        {
            dst_xs->Mask = 4;
            memcpy( &dst_xs->YmmContext, &src_xs->YmmContext, sizeof(dst_xs->YmmContext) );
        }
    }
    else
    {
        context_init_xstate( &stack->context, NULL );
    }

    CS_sig(sigcontext)  = cs64_sel;
    RIP_sig(sigcontext) = (ULONG_PTR)pKiUserExceptionDispatcher;
    RSP_sig(sigcontext) = (ULONG_PTR)stack;
    /* clear single-step, direction, and align check flag */
    EFL_sig(sigcontext) &= ~(0x100|0x400|0x40000);
    leave_handler( sigcontext );
}


/***********************************************************************
 *           setup_exception
 *
 * Setup a proper stack frame for the raise function, and modify the
 * sigcontext so that the return from the signal handler will call
 * the raise function.
 */
static void setup_exception( ucontext_t *sigcontext, EXCEPTION_RECORD *rec )
{
    struct xcontext context;

    rec->ExceptionAddress = (void *)RIP_sig(sigcontext);
    save_context( &context, sigcontext );
    setup_raise_exception( sigcontext, rec, &context );
}


/***********************************************************************
 *           call_user_apc_dispatcher
 */
NTSTATUS call_user_apc_dispatcher( CONTEXT *context, ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3,
                                   PNTAPCFUNC func, NTSTATUS status )
{
    struct syscall_frame *frame = amd64_thread_data()->syscall_frame;
    ULONG64 rsp = context ? context->Rsp : frame->rsp;
    struct apc_stack_layout *stack;

    rsp &= ~15;
    stack = (struct apc_stack_layout *)rsp - 1;
    if (context)
    {
        memmove( &stack->context, context, sizeof(stack->context) );
        NtSetContextThread( GetCurrentThread(), &stack->context );
    }
    else
    {
        stack->context.ContextFlags = CONTEXT_FULL;
        NtGetContextThread( GetCurrentThread(), &stack->context );
        stack->context.Rax = status;
    }
    frame->rbp = stack->context.Rbp;
    frame->rsp = (ULONG64)stack - 8;
    frame->rip = (ULONG64)pKiUserApcDispatcher;
    frame->rcx = (ULONG64)&stack->context;
    frame->rdx = arg1;
    frame->r8  = arg2;
    frame->r9  = arg3;
    stack->func = func;
    frame->restore_flags |= CONTEXT_CONTROL | CONTEXT_INTEGER;
    return status;
}


/***********************************************************************
 *           call_raise_user_exception_dispatcher
 */
void call_raise_user_exception_dispatcher(void)
{
    amd64_thread_data()->syscall_frame->rip = (UINT64)pKiRaiseUserExceptionDispatcher;
}


/***********************************************************************
 *           call_user_exception_dispatcher
 */
NTSTATUS call_user_exception_dispatcher( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    struct syscall_frame *frame = amd64_thread_data()->syscall_frame;
    struct stack_layout *stack;
    ULONG64 rsp;
    NTSTATUS status = NtSetContextThread( GetCurrentThread(), context );

    if (status) return status;
    rsp = (context->Rsp - 0x20) & ~15;
    stack = (struct stack_layout *)rsp - 1;

    if ((context->ContextFlags & CONTEXT_XSTATE) == CONTEXT_XSTATE)
    {
        rsp = (rsp - sizeof(XSTATE)) & ~63;
        stack = (struct stack_layout *)rsp - 1;
        assert( !((ULONG_PTR)stack->xstate & 63) );
        context_init_xstate( &stack->context, stack->xstate );
        memcpy( stack->xstate, &frame->xstate, sizeof(frame->xstate) );
    }

    memmove( &stack->context, context, sizeof(*context) );
    stack->rec = *rec;
    /* fix up instruction pointer in context for EXCEPTION_BREAKPOINT */
    if (stack->rec.ExceptionCode == EXCEPTION_BREAKPOINT) stack->context.Rip--;
    frame->rbp = context->Rbp;
    frame->rsp = (ULONG64)stack;
    frame->rip = (ULONG64)pKiUserExceptionDispatcher;
    frame->restore_flags |= CONTEXT_CONTROL;
    return status;
}


/***********************************************************************
 *           call_user_mode_callback
 */
extern NTSTATUS CDECL call_user_mode_callback( void *func, void *stack, void **ret_ptr,
                                               ULONG *ret_len, TEB *teb ) DECLSPEC_HIDDEN;
__ASM_GLOBAL_FUNC( call_user_mode_callback,
                   "subq $0xe8,%rsp\n\t"
                   __ASM_SEH(".seh_stackalloc 0xf0\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   __ASM_CFI(".cfi_adjust_cfa_offset 0xe8\n\t")
                   "movq %rbp,0xe0(%rsp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %rbp,0xe0\n\t")
                   "leaq 0xe0(%rsp),%rbp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %rbp\n\t")
                   "movq %rbx,-0x08(%rbp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %rbx,-0x08\n\t")
                   "movq %rsi,-0x10(%rbp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %rbx,-0x10\n\t")
                   "movq %rdi,-0x18(%rbp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %rbx,-0x18\n\t")
                   "movq %r12,-0x20(%rbp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %r12,-0x20\n\t")
                   "movq %r13,-0x28(%rbp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %r13,-0x28\n\t")
                   "movq %r14,-0x30(%rbp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %r14,-0x30\n\t")
                   "movq %r15,-0x38(%rbp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %r15,-0x38\n\t")
                   "stmxcsr -0x40(%rbp)\n\t"
                   "fnstcw -0x3c(%rbp)\n\t"
                   "movdqa %xmm6,-0x50(%rbp)\n\t"
                   "movdqa %xmm7,-0x60(%rbp)\n\t"
                   "movdqa %xmm8,-0x70(%rbp)\n\t"
                   "movdqa %xmm9,-0x80(%rbp)\n\t"
                   "movdqa %xmm10,-0x90(%rbp)\n\t"
                   "movdqa %xmm11,-0xa0(%rbp)\n\t"
                   "movdqa %xmm12,-0xb0(%rbp)\n\t"
                   "movdqa %xmm13,-0xc0(%rbp)\n\t"
                   "movdqa %xmm14,-0xd0(%rbp)\n\t"
                   "movdqa %xmm15,-0xe0(%rbp)\n\t"
                   "movq %r8,0x10(%rbp)\n\t"   /* ret_ptr */
                   "movq %r9,0x18(%rbp)\n\t"   /* ret_len */
                   "movq 0x30(%rbp),%r11\n\t"  /* teb */

                   "subq $0x410,%rsp\n\t"      /* sizeof(struct syscall_frame) + ebp + exception */
                   "andq $~63,%rsp\n\t"
                   "movq %rbp,0x400(%rsp)\n\t"
                   "movq 0x328(%r11),%r10\n\t" /* amd64_thread_data()->syscall_frame */
                   "movq (%r11),%rax\n\t"      /* NtCurrentTeb()->Tib.ExceptionList */
                   "movq %rax,0x408(%rsp)\n\t"
                   "movq 0xa8(%r10),%rax\n\t"  /* prev_frame->syscall_table */
                   "movq %rax,0xa8(%rsp)\n\t"  /* frame->syscall_table */
                   "movl 0xb0(%r10),%r14d\n\t" /* prev_frame->syscall_flags */
                   "movl %r14d,0xb0(%rsp)\n\t" /* frame->syscall_flags */
                   "movq %r10,0xa0(%rsp)\n\t"  /* frame->prev_frame */
                   "movq %rsp,0x328(%r11)\n\t" /* amd64_thread_data()->syscall_frame */
#ifdef __linux__
                   "testl $12,%r14d\n\t"       /* SYSCALL_HAVE_PTHREAD_TEB | SYSCALL_HAVE_WRFSGSBASE */
                   "jz 1f\n\t"
                   "movw 0x338(%r11),%fs\n"    /* amd64_thread_data()->fs */
                   "1:\n\t"
#endif
                   "movq %rcx,%r9\n\t"         /* func */
                   "movq %rdx,%rax\n\t"        /* stack */
                   "movq 0x8(%rax),%rcx\n\t"   /* id */
                   "movq 0x10(%rax),%rdx\n\t"  /* args */
                   "movq 0x18(%rax),%r8\n\t"   /* len */
                   "movq %rax,%rsp\n\t"
                   "jmpq *%r9" )


/***********************************************************************
 *           user_mode_callback_return
 */
extern void CDECL DECLSPEC_NORETURN user_mode_callback_return( void *ret_ptr, ULONG ret_len,
                                                               NTSTATUS status, TEB *teb ) DECLSPEC_HIDDEN;
__ASM_GLOBAL_FUNC( user_mode_callback_return,
                   "movq 0x328(%r9),%r10\n\t"  /* amd64_thread_data()->syscall_frame */
                   "movq 0xa0(%r10),%r11\n\t"  /* frame->prev_frame */
                   "movq %r11,0x328(%r9)\n\t"  /* amd64_thread_data()->syscall_frame = prev_frame */
                   "movq 0x400(%r10),%rbp\n\t" /* call_user_mode_callback rbp */
                   __ASM_CFI(".cfi_def_cfa_register %rbp\n\t")
                   __ASM_CFI(".cfi_rel_offset %rbx,-0x08\n\t")
                   __ASM_CFI(".cfi_rel_offset %rbx,-0x10\n\t")
                   __ASM_CFI(".cfi_rel_offset %rbx,-0x18\n\t")
                   __ASM_CFI(".cfi_rel_offset %r12,-0x20\n\t")
                   __ASM_CFI(".cfi_rel_offset %r13,-0x28\n\t")
                   __ASM_CFI(".cfi_rel_offset %r14,-0x30\n\t")
                   __ASM_CFI(".cfi_rel_offset %r15,-0x38\n\t")
                   "movq 0x408(%r10),%rsi\n\t" /* exception list */
                   "movq %rsi,0(%r9)\n\t"      /* teb->Tib.ExceptionList */
                   "movq 0x10(%rbp),%rsi\n\t"  /* ret_ptr */
                   "movq 0x18(%rbp),%rdi\n\t"  /* ret_len */
                   "movq %rcx,(%rsi)\n\t"
                   "movl %edx,(%rdi)\n\t"
                   "movdqa -0xe0(%rbp),%xmm15\n\t"
                   "movdqa -0xd0(%rbp),%xmm14\n\t"
                   "movdqa -0xc0(%rbp),%xmm13\n\t"
                   "movdqa -0xb0(%rbp),%xmm12\n\t"
                   "movdqa -0xa0(%rbp),%xmm11\n\t"
                   "movdqa -0x90(%rbp),%xmm10\n\t"
                   "movdqa -0x80(%rbp),%xmm9\n\t"
                   "movdqa -0x70(%rbp),%xmm8\n\t"
                   "movdqa -0x60(%rbp),%xmm7\n\t"
                   "movdqa -0x50(%rbp),%xmm6\n\t"
                   "ldmxcsr -0x40(%rbp)\n\t"
                   "fnclex\n\t"
                   "fldcw -0x3c(%rbp)\n\t"
                   "movq -0x38(%rbp),%r15\n\t"
                   __ASM_CFI(".cfi_same_value %r15\n\t")
                   "movq -0x30(%rbp),%r14\n\t"
                   __ASM_CFI(".cfi_same_value %r14\n\t")
                   "movq -0x28(%rbp),%r13\n\t"
                   __ASM_CFI(".cfi_same_value %r13\n\t")
                   "movq -0x20(%rbp),%r12\n\t"
                   __ASM_CFI(".cfi_same_value %r12\n\t")
                   "movq -0x18(%rbp),%rdi\n\t"
                   __ASM_CFI(".cfi_same_value %rdi\n\t")
                   "movq -0x10(%rbp),%rsi\n\t"
                   __ASM_CFI(".cfi_same_value %rsi\n\t")
                   "movq -0x08(%rbp),%rbx\n\t"
                   __ASM_CFI(".cfi_same_value %rbx\n\t")
                   "leave\n"
                   __ASM_CFI(".cfi_def_cfa %rsp,8\n\t")
                   __ASM_CFI(".cfi_same_value %rbp\n\t")
                   "movq %r8,%rax\n\t"
                   "retq" )


/***********************************************************************
 *           KeUserModeCallback
 */
NTSTATUS WINAPI KeUserModeCallback( ULONG id, const void *args, ULONG len, void **ret_ptr, ULONG *ret_len )
{
    struct syscall_frame *frame = amd64_thread_data()->syscall_frame;
    void *args_data = (void *)((frame->rsp - len) & ~15);
    ULONG_PTR *stack = args_data;

    /* if we have no syscall frame, call the callback directly */
    if ((char *)&frame < (char *)ntdll_get_thread_data()->kernel_stack ||
        (char *)&frame > (char *)amd64_thread_data()->syscall_frame)
    {
        NTSTATUS (WINAPI *func)(const void *, ULONG) = ((void **)NtCurrentTeb()->Peb->KernelCallbackTable)[id];
        return func( args, len );
    }

    if ((char *)ntdll_get_thread_data()->kernel_stack + min_kernel_stack > (char *)&frame)
        return STATUS_STACK_OVERFLOW;

    memcpy( args_data, args, len );
    *(--stack) = 0;
    *(--stack) = len;
    *(--stack) = (ULONG_PTR)args_data;
    *(--stack) = id;
    *(--stack) = 0xdeadbabe;

    return call_user_mode_callback( pKiUserCallbackDispatcher, stack, ret_ptr, ret_len, NtCurrentTeb() );
}


/***********************************************************************
 *           NtCallbackReturn  (NTDLL.@)
 */
NTSTATUS WINAPI NtCallbackReturn( void *ret_ptr, ULONG ret_len, NTSTATUS status )
{
    if (!amd64_thread_data()->syscall_frame->prev_frame) return STATUS_NO_CALLBACK_ACTIVE;
    user_mode_callback_return( ret_ptr, ret_len, status, NtCurrentTeb() );
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
static inline BOOL handle_interrupt( ucontext_t *sigcontext, EXCEPTION_RECORD *rec, struct xcontext *xcontext )
{
    CONTEXT *context = &xcontext->c;

    switch (ERROR_sig(sigcontext) >> 3)
    {
    case 0x29:
        /* __fastfail: process state is corrupted */
        rec->ExceptionCode = STATUS_STACK_BUFFER_OVERRUN;
        rec->ExceptionFlags = EH_NONCONTINUABLE;
        rec->NumberParameters = 1;
        rec->ExceptionInformation[0] = context->Rcx;
        NtRaiseException( rec, context, FALSE );
        return TRUE;
    case 0x2c:
        rec->ExceptionCode = STATUS_ASSERTION_FAILURE;
        break;
    case 0x2d:
        if (CS_sig(sigcontext) == cs64_sel)
        {
            switch (context->Rax)
            {
            case 1: /* BREAKPOINT_PRINT */
            case 3: /* BREAKPOINT_LOAD_SYMBOLS */
            case 4: /* BREAKPOINT_UNLOAD_SYMBOLS */
            case 5: /* BREAKPOINT_COMMAND_STRING (>= Win2003) */
                RIP_sig(sigcontext) += 3;
                leave_handler( sigcontext );
                return TRUE;
            }
        }
        context->Rip += 3;
        rec->ExceptionCode = EXCEPTION_BREAKPOINT;
        rec->ExceptionAddress = (void *)context->Rip;
        rec->NumberParameters = 1;
        rec->ExceptionInformation[0] = context->Rax;
        break;
    default:
        return FALSE;
    }
    setup_raise_exception( sigcontext, rec, xcontext );
    return TRUE;
}


/***********************************************************************
 *           handle_syscall_fault
 *
 * Handle a page fault happening during a system call.
 */
static BOOL handle_syscall_fault( ucontext_t *sigcontext, EXCEPTION_RECORD *rec, CONTEXT *context )
{
    struct syscall_frame *frame = amd64_thread_data()->syscall_frame;
    DWORD i;

    if (!is_inside_syscall( sigcontext ) && !ntdll_get_thread_data()->jmp_buf) return FALSE;

    TRACE_(seh)( "code=%x flags=%x addr=%p ip=%lx tid=%04x\n",
                 rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
                 context->Rip, GetCurrentThreadId() );
    for (i = 0; i < rec->NumberParameters; i++)
        TRACE_(seh)( " info[%d]=%016lx\n", i, rec->ExceptionInformation[i] );
    TRACE_(seh)( " rax=%016lx rbx=%016lx rcx=%016lx rdx=%016lx\n",
                 context->Rax, context->Rbx, context->Rcx, context->Rdx );
    TRACE_(seh)( " rsi=%016lx rdi=%016lx rbp=%016lx rsp=%016lx\n",
                 context->Rsi, context->Rdi, context->Rbp, context->Rsp );
    TRACE_(seh)( "  r8=%016lx  r9=%016lx r10=%016lx r11=%016lx\n",
                 context->R8, context->R9, context->R10, context->R11 );
    TRACE_(seh)( " r12=%016lx r13=%016lx r14=%016lx r15=%016lx\n",
                 context->R12, context->R13, context->R14, context->R15 );

    if (ntdll_get_thread_data()->jmp_buf)
    {
        TRACE_(seh)( "returning to handler\n" );
        RCX_sig(sigcontext) = (ULONG_PTR)ntdll_get_thread_data()->jmp_buf;
        RDX_sig(sigcontext) = 1;
        RIP_sig(sigcontext) = (ULONG_PTR)__wine_longjmp;
        ntdll_get_thread_data()->jmp_buf = NULL;
    }
    else
    {
        TRACE_(seh)( "returning to user mode ip=%016lx ret=%08x\n", frame->rip, rec->ExceptionCode );
        RCX_sig(sigcontext) = (ULONG_PTR)frame;
        RDX_sig(sigcontext) = rec->ExceptionCode;
        RIP_sig(sigcontext) = (ULONG_PTR)__wine_syscall_dispatcher_return;
    }
    return TRUE;
}


/***********************************************************************
 *           handle_syscall_trap
 *
 * Handle a trap exception during a system call.
 */
static BOOL handle_syscall_trap( ucontext_t *sigcontext )
{
    struct syscall_frame *frame = amd64_thread_data()->syscall_frame;

    /* disallow single-stepping through a syscall */

    if ((void *)RIP_sig( sigcontext ) == __wine_syscall_dispatcher)
    {
        extern void __wine_syscall_dispatcher_prolog_end(void) DECLSPEC_HIDDEN;

        RIP_sig( sigcontext ) = (ULONG64)__wine_syscall_dispatcher_prolog_end;
    }
    else if ((void *)RIP_sig( sigcontext ) == __wine_unix_call_dispatcher)
    {
        extern void __wine_unix_call_dispatcher_prolog_end(void) DECLSPEC_HIDDEN;

        RIP_sig( sigcontext ) = (ULONG64)__wine_unix_call_dispatcher_prolog_end;
        R10_sig( sigcontext ) = RCX_sig( sigcontext );
    }
    else return FALSE;

    TRACE_(seh)( "ignoring trap in syscall rip=%p eflags=%08x\n",
                 (void *)RIP_sig(sigcontext), (ULONG)EFL_sig(sigcontext) );

    frame->rip = *(ULONG64 *)RSP_sig( sigcontext );
    frame->eflags = EFL_sig(sigcontext);
    frame->restore_flags = CONTEXT_CONTROL;

    RCX_sig( sigcontext ) = (ULONG64)frame;
    RSP_sig( sigcontext ) += sizeof(ULONG64);
    EFL_sig( sigcontext ) &= ~0x100;  /* clear single-step flag */
    return TRUE;
}


/**********************************************************************
 *		segv_handler
 *
 * Handler for SIGSEGV and related errors.
 */
static void segv_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    EXCEPTION_RECORD rec = { 0 };
    struct xcontext context;
    ucontext_t *ucontext = sigcontext;

    rec.ExceptionAddress = (void *)RIP_sig(ucontext);
    save_context( &context, sigcontext );

    switch(TRAP_sig(ucontext))
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
        {
            WORD err = ERROR_sig(ucontext);
            if (!err && (rec.ExceptionCode = is_privileged_instr( &context.c ))) break;
            if ((err & 7) == 2 && handle_interrupt( ucontext, &rec, &context )) return;
            rec.ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
            rec.NumberParameters = 2;
            rec.ExceptionInformation[0] = 0;
            rec.ExceptionInformation[1] = 0xffffffffffffffff;
        }
        break;
    case TRAP_x86_PAGEFLT:  /* Page fault */
        rec.NumberParameters = 2;
        rec.ExceptionInformation[0] = (ERROR_sig(ucontext) >> 1) & 0x09;
        rec.ExceptionInformation[1] = (ULONG_PTR)siginfo->si_addr;
        rec.ExceptionCode = virtual_handle_fault( siginfo->si_addr, rec.ExceptionInformation[0],
                                                  (void *)RSP_sig(ucontext) );
        if (!rec.ExceptionCode)
        {
            leave_handler( sigcontext );
            return;
        }
        break;
    case TRAP_x86_ALIGNFLT:  /* Alignment check exception */
        if (EFL_sig(ucontext) & 0x00040000)
        {
            EFL_sig(ucontext) &= ~0x00040000;  /* reset AC flag */
            leave_handler( sigcontext );
            return;
        }
        rec.ExceptionCode = EXCEPTION_DATATYPE_MISALIGNMENT;
        break;
    default:
        ERR_(seh)( "Got unexpected trap %ld\n", (ULONG_PTR)TRAP_sig(ucontext) );
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
    if (handle_syscall_fault( sigcontext, &rec, &context.c )) return;
    setup_raise_exception( sigcontext, &rec, &context );
}


/**********************************************************************
 *		trap_handler
 *
 * Handler for SIGTRAP.
 */
static void trap_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    EXCEPTION_RECORD rec = { 0 };
    struct xcontext context;
    ucontext_t *ucontext = sigcontext;

    if (handle_syscall_trap( sigcontext )) return;

    rec.ExceptionAddress = (void *)RIP_sig(ucontext);
    save_context( &context, sigcontext );

    switch (TRAP_sig(ucontext))
    {
    case TRAP_x86_TRCTRAP:
        rec.ExceptionCode = EXCEPTION_SINGLE_STEP;
        break;
    case TRAP_x86_BPTFLT:
        rec.ExceptionAddress = (char *)rec.ExceptionAddress - 1;  /* back up over the int3 instruction */
        /* fall through */
    default:
        rec.ExceptionCode = EXCEPTION_BREAKPOINT;
        rec.NumberParameters = 1;
        rec.ExceptionInformation[0] = 0;
        break;
    }
    setup_raise_exception( sigcontext, &rec, &context );
}


/**********************************************************************
 *		fpe_handler
 *
 * Handler for SIGFPE.
 */
static void fpe_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    EXCEPTION_RECORD rec = { 0 };
    ucontext_t *ucontext = sigcontext;

    switch (siginfo->si_code)
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
        if (FPU_sig(ucontext) && FPU_sig(ucontext)->StatusWord & 0x40)
            rec.ExceptionCode = EXCEPTION_FLT_STACK_CHECK;
        else
            rec.ExceptionCode = EXCEPTION_FLT_INVALID_OPERATION;
        break;
    }

    if (TRAP_sig(ucontext) == TRAP_x86_CACHEFLT)
    {
        rec.NumberParameters = 2;
        rec.ExceptionInformation[0] = 0;
        rec.ExceptionInformation[1] = FPU_sig(ucontext) ? FPU_sig(ucontext)->MxCsr : 0;
        if (CS_sig(ucontext) != cs64_sel) rec.ExceptionCode = STATUS_FLOAT_MULTIPLE_TRAPS;
    }
    setup_exception( sigcontext, &rec );
}


/**********************************************************************
 *		int_handler
 *
 * Handler for SIGINT.
 */
static void int_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    HANDLE handle;

    if (!p__wine_ctrl_routine) return;
    if (!NtCreateThreadEx( &handle, THREAD_ALL_ACCESS, NULL, NtCurrentProcess(),
                           p__wine_ctrl_routine, 0 /* CTRL_C_EVENT */, 0, 0, 0, 0, NULL ))
        NtClose( handle );
}


/**********************************************************************
 *		abrt_handler
 *
 * Handler for SIGABRT.
 */
static void abrt_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    EXCEPTION_RECORD rec = { EXCEPTION_WINE_ASSERTION, EH_NONCONTINUABLE };

    setup_exception( sigcontext, &rec );
}


/**********************************************************************
 *		quit_handler
 *
 * Handler for SIGQUIT.
 */
static void quit_handler( int signal, siginfo_t *siginfo, void *ucontext )
{
    init_handler( ucontext );
    abort_thread(0);
}


/**********************************************************************
 *		usr1_handler
 *
 * Handler for SIGUSR1, used to signal a thread that it got suspended.
 */
static void usr1_handler( int signal, siginfo_t *siginfo, void *ucontext )
{
    struct xcontext context;

    init_handler( ucontext );
    if (is_inside_syscall( ucontext ))
    {
        DECLSPEC_ALIGN(64) XSTATE xs;
        context.c.ContextFlags = CONTEXT_FULL;
        context_init_xstate( &context.c, &xs );

        NtGetContextThread( GetCurrentThread(), &context.c );
        wait_suspend( &context.c );
        NtSetContextThread( GetCurrentThread(), &context.c );
    }
    else
    {
        save_context( &context, ucontext );
        wait_suspend( &context.c );
        restore_context( &context, ucontext );
    }
}


/***********************************************************************
 *           LDT support
 */

#define LDT_SIZE 8192

#define LDT_FLAGS_DATA      0x13  /* Data segment */
#define LDT_FLAGS_CODE      0x1b  /* Code segment */
#define LDT_FLAGS_32BIT     0x40  /* Segment is 32-bit (code or stack) */
#define LDT_FLAGS_ALLOCATED 0x80  /* Segment is allocated */

static ULONG first_ldt_entry = 32;

struct ldt_copy
{
    void         *base[LDT_SIZE];
    unsigned int  limit[LDT_SIZE];
    unsigned char flags[LDT_SIZE];
} __wine_ldt_copy;

static pthread_mutex_t ldt_mutex = PTHREAD_MUTEX_INITIALIZER;

static inline void *ldt_get_base( LDT_ENTRY ent )
{
    return (void *)(ent.BaseLow |
                    (ULONG_PTR)ent.HighWord.Bits.BaseMid << 16 |
                    (ULONG_PTR)ent.HighWord.Bits.BaseHi << 24);
}

static inline unsigned int ldt_get_limit( LDT_ENTRY ent )
{
    unsigned int limit = ent.LimitLow | (ent.HighWord.Bits.LimitHi << 16);
    if (ent.HighWord.Bits.Granularity) limit = (limit << 12) | 0xfff;
    return limit;
}

static LDT_ENTRY ldt_make_entry( void *base, unsigned int limit, unsigned char flags )
{
    LDT_ENTRY entry;

    entry.BaseLow                   = (WORD)(ULONG_PTR)base;
    entry.HighWord.Bits.BaseMid     = (BYTE)((ULONG_PTR)base >> 16);
    entry.HighWord.Bits.BaseHi      = (BYTE)((ULONG_PTR)base >> 24);
    if ((entry.HighWord.Bits.Granularity = (limit >= 0x100000))) limit >>= 12;
    entry.LimitLow                  = (WORD)limit;
    entry.HighWord.Bits.LimitHi     = limit >> 16;
    entry.HighWord.Bits.Dpl         = 3;
    entry.HighWord.Bits.Pres        = 1;
    entry.HighWord.Bits.Type        = flags;
    entry.HighWord.Bits.Sys         = 0;
    entry.HighWord.Bits.Reserved_0  = 0;
    entry.HighWord.Bits.Default_Big = (flags & LDT_FLAGS_32BIT) != 0;
    return entry;
}

static void ldt_set_entry( WORD sel, LDT_ENTRY entry )
{
    int index = sel >> 3;

#if defined(__APPLE__)
    if (i386_set_ldt(index, (union ldt_entry *)&entry, 1) < 0) perror("i386_set_ldt");
#else
    fprintf( stderr, "No LDT support on this platform\n" );
    exit(1);
#endif

    __wine_ldt_copy.base[index]  = ldt_get_base( entry );
    __wine_ldt_copy.limit[index] = ldt_get_limit( entry );
    __wine_ldt_copy.flags[index] = (entry.HighWord.Bits.Type |
                                    (entry.HighWord.Bits.Default_Big ? LDT_FLAGS_32BIT : 0) |
                                    LDT_FLAGS_ALLOCATED);
}


/**********************************************************************
 *           get_thread_ldt_entry
 */
NTSTATUS get_thread_ldt_entry( HANDLE handle, void *data, ULONG len, ULONG *ret_len )
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
    __asm__( "movw %%cs,%0" : "=m" (cs64_sel) );
    __asm__( "movw %%ss,%0" : "=m" (ds64_sel) );
}


/**********************************************************************
 *		signal_alloc_thread
 */
NTSTATUS signal_alloc_thread( TEB *teb )
{
    struct amd64_thread_data *thread_data = (struct amd64_thread_data *)&teb->GdiTebBatch;

    if (teb->WowTebOffset)
    {
        if (!fs32_sel)
        {
            void *teb32 = (char *)teb + teb->WowTebOffset;
            sigset_t sigset;
            int idx;
            LDT_ENTRY entry = ldt_make_entry( teb32, page_size - 1, LDT_FLAGS_DATA | LDT_FLAGS_32BIT );

            server_enter_uninterrupted_section( &ldt_mutex, &sigset );
            for (idx = first_ldt_entry; idx < LDT_SIZE; idx++)
            {
                if (__wine_ldt_copy.flags[idx]) continue;
                ldt_set_entry( (idx << 3) | 7, entry );
                break;
            }
            server_leave_uninterrupted_section( &ldt_mutex, &sigset );
            if (idx == LDT_SIZE) return STATUS_TOO_MANY_THREADS;
            thread_data->fs = (idx << 3) | 7;
        }
        else thread_data->fs = fs32_sel;
    }
    return STATUS_SUCCESS;
}


/**********************************************************************
 *		signal_free_thread
 */
void signal_free_thread( TEB *teb )
{
    struct amd64_thread_data *thread_data = (struct amd64_thread_data *)&teb->GdiTebBatch;
    sigset_t sigset;

    if (teb->WowTebOffset && !fs32_sel)
    {
        server_enter_uninterrupted_section( &ldt_mutex, &sigset );
        __wine_ldt_copy.flags[thread_data->fs >> 3] = 0;
        server_leave_uninterrupted_section( &ldt_mutex, &sigset );
    }
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

    mach_port_t self = mach_thread_self();
    kern_return_t kr = thread_info(self, THREAD_IDENTIFIER_INFO, (thread_info_t) &tiinfo, &info_count);
    mach_port_deallocate(mach_task_self(), self);

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
 *		signal_init_process
 */
void signal_init_process(void)
{
    struct sigaction sig_act;
    void *ptr, *kernel_stack = (char *)ntdll_get_thread_data()->kernel_stack + kernel_stack_size;

    amd64_thread_data()->syscall_frame = (struct syscall_frame *)kernel_stack - 1;

    /* sneak in a syscall dispatcher pointer at a fixed address (7ffe1000) */
    ptr = (char *)user_shared_data + page_size;
    anon_mmap_fixed( ptr, page_size, PROT_READ | PROT_WRITE, 0 );
    *(void **)ptr = __wine_syscall_dispatcher;

    if (cpu_info.ProcessorFeatureBits & CPU_FEATURE_XSAVE) syscall_flags |= SYSCALL_HAVE_XSAVE;
    if (xstate_compaction_enabled) syscall_flags |= SYSCALL_HAVE_XSAVEC;

#ifdef __linux__
    if (NtCurrentTeb()->WowTebOffset)
    {
        void *teb32 = (char *)NtCurrentTeb() + NtCurrentTeb()->WowTebOffset;
        int sel;

        cs32_sel = 0x23;
        if ((sel = alloc_fs_sel( -1, teb32 )) != -1)
        {
            amd64_thread_data()->fs = fs32_sel = (sel << 3) | 3;
            syscall_flags |= SYSCALL_HAVE_PTHREAD_TEB;
#ifdef AT_HWCAP2
            if (getauxval( AT_HWCAP2 ) & 2) syscall_flags |= SYSCALL_HAVE_WRFSGSBASE;
#endif
        }
        else ERR_(seh)( "failed to allocate %%fs selector\n" );
    }
#elif defined(__APPLE__)
    if (NtCurrentTeb()->WowTebOffset)
    {
        void *teb32 = (char *)NtCurrentTeb() + NtCurrentTeb()->WowTebOffset;
        LDT_ENTRY cs32_entry, fs32_entry;
        int idx;

        cs32_entry = ldt_make_entry( NULL, -1, LDT_FLAGS_CODE | LDT_FLAGS_32BIT );
        fs32_entry = ldt_make_entry( teb32, page_size - 1, LDT_FLAGS_DATA | LDT_FLAGS_32BIT );

        for (idx = first_ldt_entry; idx < LDT_SIZE; idx++)
        {
            if (__wine_ldt_copy.flags[idx]) continue;
            cs32_sel = (idx << 3) | 7;
            ldt_set_entry( cs32_sel, cs32_entry );
            break;
        }

        for (idx = first_ldt_entry; idx < LDT_SIZE; idx++)
        {
            if (__wine_ldt_copy.flags[idx]) continue;
            amd64_thread_data()->fs = (idx << 3) | 7;
            ldt_set_entry( amd64_thread_data()->fs, fs32_entry );
            break;
        }
    }
#endif

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
 *           call_init_thunk
 */
void DECLSPEC_HIDDEN call_init_thunk( LPTHREAD_START_ROUTINE entry, void *arg, BOOL suspend, TEB *teb )
{
    struct amd64_thread_data *thread_data = (struct amd64_thread_data *)&teb->GdiTebBatch;
    struct syscall_frame *frame = thread_data->syscall_frame;
    CONTEXT *ctx, context = { 0 };
    I386_CONTEXT *wow_context;

#if defined __linux__
    arch_prctl( ARCH_SET_GS, teb );
    arch_prctl( ARCH_GET_FS, &amd64_thread_data()->pthread_teb );
    if (fs32_sel) alloc_fs_sel( fs32_sel >> 3, (char *)teb + teb->WowTebOffset );
#elif defined (__FreeBSD__) || defined (__FreeBSD_kernel__)
    amd64_set_gsbase( teb );
#elif defined(__NetBSD__)
    sysarch( X86_64_SET_GSBASE, &teb );
#elif defined (__APPLE__)
    __asm__ volatile (".byte 0x65\n\tmovq %0,%c1" :: "r" (teb->Tib.Self), "n" (FIELD_OFFSET(TEB, Tib.Self)));
    __asm__ volatile (".byte 0x65\n\tmovq %0,%c1" :: "r" (teb->ThreadLocalStoragePointer), "n" (FIELD_OFFSET(TEB, ThreadLocalStoragePointer)));
    amd64_thread_data()->pthread_teb = mac_thread_gsbase();
    /* alloc_tls_slot() needs to poke a value to an address relative to each
       thread's gsbase.  Have each thread record its gsbase pointer into its
       TEB so alloc_tls_slot() can find it. */
    teb->Reserved5[0] = amd64_thread_data()->pthread_teb;
#else
# error Please define setting %gs for your architecture
#endif

    context.ContextFlags = CONTEXT_ALL;
    context.Rcx    = (ULONG_PTR)entry;
    context.Rdx    = (ULONG_PTR)arg;
    context.Rsp    = (ULONG_PTR)teb->Tib.StackBase - 0x28;
    context.Rip    = (ULONG_PTR)pRtlUserThreadStart;
    context.SegCs  = cs64_sel;
    context.SegDs  = ds64_sel;
    context.SegEs  = ds64_sel;
    context.SegFs  = thread_data->fs;
    context.SegGs  = ds64_sel;
    context.SegSs  = ds64_sel;
    context.EFlags = 0x200;
    context.u.FltSave.ControlWord = 0x27f;
    context.u.FltSave.MxCsr = context.MxCsr = 0x1f80;

    if ((wow_context = get_cpu_area( IMAGE_FILE_MACHINE_I386 )))
    {
        wow_context->ContextFlags = CONTEXT_I386_ALL;
        wow_context->Eax = (ULONG_PTR)entry;
        wow_context->Ebx = (arg == peb ? get_wow_teb( teb )->Peb : (ULONG_PTR)arg);
        wow_context->Esp = get_wow_teb( teb )->Tib.StackBase - 16;
        wow_context->Eip = pLdrSystemDllInitBlock->pRtlUserThreadStart;
        wow_context->SegCs = cs32_sel;
        wow_context->SegDs = context.SegDs;
        wow_context->SegEs = context.SegEs;
        wow_context->SegFs = context.SegFs;
        wow_context->SegGs = context.SegGs;
        wow_context->SegSs = context.SegSs;
        wow_context->EFlags = 0x202;
        wow_context->FloatSave.ControlWord = context.u.FltSave.ControlWord;
        *(XSAVE_FORMAT *)wow_context->ExtendedRegisters = context.u.FltSave;
    }

    if (suspend) wait_suspend( &context );

    ctx = (CONTEXT *)((ULONG_PTR)context.Rsp & ~15) - 1;
    *ctx = context;
    ctx->ContextFlags = CONTEXT_FULL;
    memset( frame, 0, sizeof(*frame) );
    NtSetContextThread( GetCurrentThread(), ctx );

    frame->rsp = (ULONG64)ctx - 8;
    frame->rip = (ULONG64)pLdrInitializeThunk;
    frame->rcx = (ULONG64)ctx;
    frame->prev_frame = NULL;
    frame->restore_flags |= CONTEXT_INTEGER;
    frame->syscall_flags = syscall_flags;
    frame->syscall_table = KeServiceDescriptorTable;

    pthread_sigmask( SIG_UNBLOCK, &server_block_set, NULL );
    __wine_syscall_dispatcher_return( frame, 0 );
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
                   "movq %rsp,0x320(%rcx)\n\t"     /* amd64_thread_data()->exit_frame */
                   /* set syscall frame */
                   "movq 0x328(%rcx),%rax\n\t"     /* amd64_thread_data()->syscall_frame */
                   "orq %rax,%rax\n\t"
                   "jnz 1f\n\t"
                   "leaq -0x400(%rsp),%rax\n\t"    /* sizeof(struct syscall_frame) */
                   "andq $~63,%rax\n\t"
                   "movq %rax,0x328(%rcx)\n"       /* amd64_thread_data()->syscall_frame */
                   "1:\tmovq %rax,%rsp\n\t"
                   "call " __ASM_NAME("call_init_thunk"))


/***********************************************************************
 *           signal_exit_thread
 */
__ASM_GLOBAL_FUNC( signal_exit_thread,
                   /* fetch exit frame */
                   "xorl %ecx,%ecx\n\t"
                   "xchgq %rcx,0x320(%rdx)\n\t"      /* amd64_thread_data()->exit_frame */
                   "testq %rcx,%rcx\n\t"
                   "jnz 1f\n\t"
                   "jmp *%rsi\n"
                   /* switch to exit frame stack */
                   "1:\tmovq %rcx,%rsp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 56\n\t")
                   __ASM_CFI(".cfi_rel_offset %rbp,48\n\t")
                   __ASM_CFI(".cfi_rel_offset %rbx,40\n\t")
                   __ASM_CFI(".cfi_rel_offset %r12,32\n\t")
                   __ASM_CFI(".cfi_rel_offset %r13,24\n\t")
                   __ASM_CFI(".cfi_rel_offset %r14,16\n\t")
                   __ASM_CFI(".cfi_rel_offset %r15,8\n\t")
                   "call *%rsi" )

/***********************************************************************
 *           __wine_syscall_dispatcher
 */
__ASM_GLOBAL_FUNC( __wine_syscall_dispatcher,
#ifdef __APPLE__
                   "movq %gs:0x30,%rcx\n\t"
                   "movq 0x328(%rcx),%rcx\n\t"
#else
                   "movq %gs:0x328,%rcx\n\t"       /* amd64_thread_data()->syscall_frame */
#endif
                   "popq 0x70(%rcx)\n\t"           /* frame->rip */
                   __ASM_CFI(".cfi_adjust_cfa_offset -8\n\t")
                   __ASM_CFI_REG_IS_AT2(rip, rcx, 0xf0,0x00)
                   "pushfq\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 8\n\t")
                   "popq 0x80(%rcx)\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset -8\n\t")
                   "movl $0,0x94(%rcx)\n\t"        /* frame->restore_flags */
                   ".globl " __ASM_NAME("__wine_syscall_dispatcher_prolog_end") "\n"
                   __ASM_NAME("__wine_syscall_dispatcher_prolog_end") ":\n\t"
                   "movq %rax,0x00(%rcx)\n\t"
                   "movq %rbx,0x08(%rcx)\n\t"
                   __ASM_CFI_REG_IS_AT1(rbx, rcx, 0x08)
                   "movq %rdx,0x18(%rcx)\n\t"
                   "movq %rsi,0x20(%rcx)\n\t"
                   __ASM_CFI_REG_IS_AT1(rsi, rcx, 0x20)
                   "movq %rdi,0x28(%rcx)\n\t"
                   __ASM_CFI_REG_IS_AT1(rdi, rcx, 0x28)
                   "movq %r12,0x50(%rcx)\n\t"
                   __ASM_CFI_REG_IS_AT2(r12, rcx, 0xd0, 0x00)
                   "movq %r13,0x58(%rcx)\n\t"
                   __ASM_CFI_REG_IS_AT2(r13, rcx, 0xd8, 0x00)
                   "movq %r14,0x60(%rcx)\n\t"
                   __ASM_CFI_REG_IS_AT2(r14, rcx, 0xe0, 0x00)
                   "movq %r15,0x68(%rcx)\n\t"
                   __ASM_CFI_REG_IS_AT2(r15, rcx, 0xe8, 0x00)
                   "movw %cs,0x78(%rcx)\n\t"
                   "movw %ds,0x7a(%rcx)\n\t"
                   "movw %es,0x7c(%rcx)\n\t"
                   "movw %fs,0x7e(%rcx)\n\t"
                   "movq %rsp,0x88(%rcx)\n\t"
                   __ASM_CFI_CFA_IS_AT2(rcx, 0x88, 0x01)
                   __ASM_CFI_REG_IS_AT2(rsp, rcx, 0x88, 0x01)
                   "movw %ss,0x90(%rcx)\n\t"
                   "movw %gs,0x92(%rcx)\n\t"
                   "movq %rbp,0x98(%rcx)\n\t"
                   __ASM_CFI_REG_IS_AT2(rbp, rcx, 0x98, 0x01)
                   /* Legends of Runeterra hooks the first system call return instruction, and
                    * depends on us returning to it. Adjust the return address accordingly. */
                   "subq $0xb,0x70(%rcx)\n\t"
                   "movl 0xb0(%rcx),%r14d\n\t"     /* frame->syscall_flags */
                   "testl $3,%r14d\n\t"            /* SYSCALL_HAVE_XSAVE | SYSCALL_HAVE_XSAVEC */
                   "jz 2f\n\t"
                   "movl $7,%eax\n\t"
                   "xorl %edx,%edx\n\t"
                   "movq %rdx,0x2c0(%rcx)\n\t"
                   "movq %rdx,0x2c8(%rcx)\n\t"
                   "movq %rdx,0x2d0(%rcx)\n\t"
                   "testl $2,%r14d\n\t"            /* SYSCALL_HAVE_XSAVEC */
                   "jz 1f\n\t"
                   "movq %rdx,0x2d8(%rcx)\n\t"
                   "movq %rdx,0x2e0(%rcx)\n\t"
                   "movq %rdx,0x2e8(%rcx)\n\t"
                   "movq %rdx,0x2f0(%rcx)\n\t"
                   "movq %rdx,0x2f8(%rcx)\n\t"
                   /* The xsavec instruction is not supported by
                    * binutils < 2.25. */
                   ".byte 0x48, 0x0f, 0xc7, 0xa1, 0xc0, 0x00, 0x00, 0x00\n\t" /* xsavec64 0xc0(%rcx) */
                   "jmp 3f\n"
                   "1:\txsave64 0xc0(%rcx)\n\t"
                   "jmp 3f\n"
                   "2:\tfxsave64 0xc0(%rcx)\n"
                   /* remember state when $rcx is pointing to "frame" */
                   __ASM_CFI(".cfi_remember_state\n\t")
                   "3:\tleaq 0x98(%rcx),%rbp\n\t"
                   __ASM_CFI_CFA_IS_AT1(rbp, 0x70)
                   __ASM_CFI_REG_IS_AT1(rsp, rbp, 0x70)
                   __ASM_CFI_REG_IS_AT1(rip, rbp, 0x58)
                   __ASM_CFI_REG_IS_AT2(rbx, rbp, 0xf0, 0x7e)
                   __ASM_CFI_REG_IS_AT2(rsi, rbp, 0x88, 0x7f)
                   __ASM_CFI_REG_IS_AT2(rdi, rbp, 0x90, 0x7f)
                   __ASM_CFI_REG_IS_AT2(r12, rbp, 0xb8, 0x7f)
                   __ASM_CFI_REG_IS_AT1(r13, rbp, 0x40)
                   __ASM_CFI_REG_IS_AT1(r14, rbp, 0x48)
                   __ASM_CFI_REG_IS_AT1(r15, rbp, 0x50)
                   __ASM_CFI_REG_IS_AT1(rbp, rbp, 0x00)
#ifdef __linux__
                   "testl $12,%r14d\n\t"           /* SYSCALL_HAVE_PTHREAD_TEB | SYSCALL_HAVE_WRFSGSBASE */
                   "jz 2f\n\t"
                   "movq %gs:0x330,%rsi\n\t"       /* amd64_thread_data()->pthread_teb */
                   "testl $8,%r14d\n\t"            /* SYSCALL_HAVE_WRFSGSBASE */
                   "jz 1f\n\t"
                   "wrfsbase %rsi\n\t"
                   "jmp 2f\n"
                   "1:\tmov $0x1002,%edi\n\t"      /* ARCH_SET_FS */
                   "mov $158,%eax\n\t"             /* SYS_arch_prctl */
                   "syscall\n\t"
                   "leaq -0x98(%rbp),%rcx\n"
                   "2:\n\t"
#endif
                   "leaq 0x28(%rsp),%rsi\n\t"      /* first argument */
                   "movq %rcx,%rsp\n\t"
                   "movq 0x00(%rcx),%rax\n\t"
                   "movq 0x18(%rcx),%rdx\n\t"
                   "movl %eax,%ebx\n\t"
                   "shrl $8,%ebx\n\t"
                   "andl $0x30,%ebx\n\t"           /* syscall table number */
                   "movq 0xa8(%rcx),%rcx\n\t"      /* frame->syscall_table */
                   "leaq (%rcx,%rbx,2),%rbx\n\t"
                   "andl $0xfff,%eax\n\t"          /* syscall number */
                   "cmpq 16(%rbx),%rax\n\t"        /* table->ServiceLimit */
                   "jae 5f\n\t"
                   "movq 24(%rbx),%rcx\n\t"        /* table->ArgumentTable */
                   "movzbl (%rcx,%rax),%ecx\n\t"
                   "subq $0x20,%rcx\n\t"
                   "jbe 1f\n\t"
                   "subq %rcx,%rsp\n\t"
                   "shrq $3,%rcx\n\t"
                   "andq $~15,%rsp\n\t"
                   "movq %rsp,%rdi\n\t"
                   "cld\n\t"
                   "rep; movsq\n"
                   "1:\tmovq %r10,%rcx\n\t"
                   "subq $0x20,%rsp\n\t"
                   "movq (%rbx),%r10\n\t"          /* table->ServiceTable */
                   "callq *(%r10,%rax,8)\n\t"
                   "leaq -0x98(%rbp),%rcx\n\t"
                   /* $rcx is now pointing to "frame" again */
                   __ASM_CFI(".cfi_restore_state\n")
                   ".L__wine_syscall_dispatcher_return:\n\t"
                   "movl 0x94(%rcx),%edx\n\t"  /* frame->restore_flags */
#ifdef __linux__
                   "testl $12,%r14d\n\t"           /* SYSCALL_HAVE_PTHREAD_TEB | SYSCALL_HAVE_WRFSGSBASE */
                   "jz 1f\n\t"
                   "movw 0x7e(%rcx),%fs\n"
                   "1:\n\t"
#endif
                   "testl $0x48,%edx\n\t"          /* CONTEXT_FLOATING_POINT | CONTEXT_XSTATE */
                   "jz 4f\n\t"
                   "testl $3,%r14d\n\t"            /* SYSCALL_HAVE_XSAVE | SYSCALL_HAVE_XSAVEC */
                   "jz 3f\n\t"
                   "movq %rax,%r11\n\t"
                   "movl $7,%eax\n\t"
                   "xorl %edx,%edx\n\t"
                   "xrstor64 0xc0(%rcx)\n\t"
                   "movq %r11,%rax\n\t"
                   "movl 0x94(%rcx),%edx\n\t"
                   "jmp 4f\n"
                   "3:\tfxrstor64 0xc0(%rcx)\n"
                   "4:\tmovq 0x98(%rcx),%rbp\n\t"
                   __ASM_CFI(".cfi_same_value rbp\n\t")
                   "movq 0x68(%rcx),%r15\n\t"
                   __ASM_CFI(".cfi_same_value r15\n\t")
                   "movq 0x60(%rcx),%r14\n\t"
                   __ASM_CFI(".cfi_same_value r14\n\t")
                   "movq 0x58(%rcx),%r13\n\t"
                   __ASM_CFI(".cfi_same_value r13\n\t")
                   "movq 0x50(%rcx),%r12\n\t"
                   __ASM_CFI(".cfi_same_value r12\n\t")
                   "movq 0x28(%rcx),%rdi\n\t"
                   __ASM_CFI(".cfi_same_value rdi\n\t")
                   "movq 0x20(%rcx),%rsi\n\t"
                   __ASM_CFI(".cfi_same_value rsi\n\t")
                   "movq 0x08(%rcx),%rbx\n\t"
                   __ASM_CFI(".cfi_same_value rbx\n\t")
                   "testl $0x3,%edx\n\t"           /* CONTEXT_CONTROL | CONTEXT_INTEGER */
                   "jnz 1f\n\t"
                   __ASM_CFI(".cfi_remember_state\n\t")
                   "movq 0x80(%rcx),%r11\n\t"      /* frame->eflags */
                   "pushq %r11\n\t"
                   "popfq\n\t"
                   "movq 0x88(%rcx),%rsp\n\t"
                   __ASM_CFI(".cfi_def_cfa rsp, 0\n\t")
                   __ASM_CFI(".cfi_same_value rsp\n\t")
                   "movq 0x70(%rcx),%rcx\n\t"      /* frame->rip */
                   __ASM_CFI(".cfi_register rip, rcx\n\t")
                   "pushq %rcx\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 8\n\t")
                   "ret\n\t"
                   /* $rcx is now pointing to "frame" again */
                   __ASM_CFI(".cfi_restore_state\n\t")
                   /* remember state when $rcx is pointing to "frame" */
                   __ASM_CFI(".cfi_remember_state\n\t")
                   "1:\tleaq 0x70(%rcx),%rsp\n\t"
                   __ASM_CFI_CFA_IS_AT1(rsp, 0x18)
                   __ASM_CFI_REG_IS_AT1(rsp, rsp, 0x18)
                   __ASM_CFI_REG_IS_AT1(rip, rsp, 0x00)
                   "testl $0x2,%edx\n\t"           /* CONTEXT_INTEGER */
                   "jnz 1f\n\t"
                   "movq 0x10(%rsp),%r11\n\t"      /* frame->eflags */
                   "movq (%rsp),%rcx\n\t"          /* frame->rip */
                   __ASM_CFI(".cfi_register rip, rcx\n\t")
                   "iretq\n"
                   __ASM_CFI_REG_IS_AT1(rip, rsp, 0x00)
                   "1:\tmovq 0x00(%rcx),%rax\n\t"
                   "movq 0x18(%rcx),%rdx\n\t"
                   "movq 0x30(%rcx),%r8\n\t"
                   "movq 0x38(%rcx),%r9\n\t"
                   "movq 0x40(%rcx),%r10\n\t"
                   "movq 0x48(%rcx),%r11\n\t"
                   "movq 0x10(%rcx),%rcx\n"
                   "iretq\n"
                   __ASM_CFI_CFA_IS_AT1(rbp, 0x70)
                   __ASM_CFI_REG_IS_AT1(rsp, rbp, 0x70)
                   __ASM_CFI_REG_IS_AT1(rip, rbp, 0x58)
                   __ASM_CFI_REG_IS_AT2(rbx, rbp, 0xf0, 0x7e)
                   __ASM_CFI_REG_IS_AT2(rsi, rbp, 0x88, 0x7f)
                   __ASM_CFI_REG_IS_AT2(rdi, rbp, 0x90, 0x7f)
                   __ASM_CFI_REG_IS_AT2(r12, rbp, 0xb8, 0x7f)
                   __ASM_CFI_REG_IS_AT1(r13, rbp, 0x40)
                   __ASM_CFI_REG_IS_AT1(r14, rbp, 0x48)
                   __ASM_CFI_REG_IS_AT1(r15, rbp, 0x50)
                   __ASM_CFI_REG_IS_AT1(rbp, rbp, 0x00)
                   "5:\tmovl $0xc000000d,%edx\n\t" /* STATUS_INVALID_PARAMETER */
                   "movq %rsp,%rcx\n\t"
                   /* $rcx is now pointing to "frame" again */
                   __ASM_CFI(".cfi_restore_state\n\t")
                   ".globl " __ASM_NAME("__wine_syscall_dispatcher_return") "\n"
                   __ASM_NAME("__wine_syscall_dispatcher_return") ":\n\t"
                   "movl 0xb0(%rcx),%r14d\n\t"     /* frame->syscall_flags */
                   "movq %rdx,%rax\n\t"
                   "jmp .L__wine_syscall_dispatcher_return" )


/***********************************************************************
 *           __wine_unix_call_dispatcher
 */
__ASM_GLOBAL_FUNC( __wine_unix_call_dispatcher,
                   "movq %rcx,%r10\n\t"
#ifdef __APPLE__
                   "movq %gs:0x30,%rcx\n\t"
                   "movq 0x328(%rcx),%rcx\n\t"
#else
                   "movq %gs:0x328,%rcx\n\t"       /* amd64_thread_data()->syscall_frame */
#endif
                   "popq 0x70(%rcx)\n\t"           /* frame->rip */
                   __ASM_CFI(".cfi_adjust_cfa_offset -8\n\t")
                   __ASM_CFI_REG_IS_AT2(rip, rcx, 0xf0,0x00)
                   "movl $0,0x94(%rcx)\n\t"        /* frame->restore_flags */
                   ".globl " __ASM_NAME("__wine_unix_call_dispatcher_prolog_end") "\n"
                   __ASM_NAME("__wine_unix_call_dispatcher_prolog_end") ":\n\t"
                   "movq %rbx,0x08(%rcx)\n\t"
                   __ASM_CFI_REG_IS_AT1(rbx, rcx, 0x08)
                   "movq %rsi,0x20(%rcx)\n\t"
                   __ASM_CFI_REG_IS_AT1(rsi, rcx, 0x20)
                   "movq %rdi,0x28(%rcx)\n\t"
                   __ASM_CFI_REG_IS_AT1(rdi, rcx, 0x28)
                   "movq %r12,0x50(%rcx)\n\t"
                   __ASM_CFI_REG_IS_AT2(r12, rcx, 0xd0, 0x00)
                   "movq %r13,0x58(%rcx)\n\t"
                   __ASM_CFI_REG_IS_AT2(r13, rcx, 0xd8, 0x00)
                   "movq %r14,0x60(%rcx)\n\t"
                   __ASM_CFI_REG_IS_AT2(r14, rcx, 0xe0, 0x00)
                   "movq %r15,0x68(%rcx)\n\t"
                   __ASM_CFI_REG_IS_AT2(r15, rcx, 0xe8, 0x00)
                   "movq %rsp,0x88(%rcx)\n\t"
                   __ASM_CFI_CFA_IS_AT2(rcx, 0x88, 0x01)
                   __ASM_CFI_REG_IS_AT2(rsp, rcx, 0x88, 0x01)
                   "movq %rbp,0x98(%rcx)\n\t"
                   __ASM_CFI_REG_IS_AT2(rbp, rcx, 0x98, 0x01)
                   "movdqa %xmm6,0x1c0(%rcx)\n\t"
                   "movdqa %xmm7,0x1d0(%rcx)\n\t"
                   "movdqa %xmm8,0x1e0(%rcx)\n\t"
                   "movdqa %xmm9,0x1f0(%rcx)\n\t"
                   "movdqa %xmm10,0x200(%rcx)\n\t"
                   "movdqa %xmm11,0x210(%rcx)\n\t"
                   "movdqa %xmm12,0x220(%rcx)\n\t"
                   "movdqa %xmm13,0x230(%rcx)\n\t"
                   "movdqa %xmm14,0x240(%rcx)\n\t"
                   "movdqa %xmm15,0x250(%rcx)\n\t"
                   "movl 0xb0(%rcx),%r14d\n\t"     /* frame->syscall_flags */
#ifdef __linux__
                   "testl $12,%r14d\n\t"           /* SYSCALL_HAVE_PTHREAD_TEB | SYSCALL_HAVE_WRFSGSBASE */
                   "jz 2f\n\t"
                   "movw %fs,0x7e(%rcx)\n\t"
                   "movq %gs:0x330,%rsi\n\t"       /* amd64_thread_data()->pthread_teb */
                   "testl $8,%r14d\n\t"            /* SYSCALL_HAVE_WRFSGSBASE */
                   "jz 1f\n\t"
                   "wrfsbase %rsi\n\t"
                   "jmp 2f\n"
                   "1:\tmov $0x1002,%edi\n\t"      /* ARCH_SET_FS */
                   "mov $158,%eax\n\t"             /* SYS_arch_prctl */
                   "mov %rcx,%r9\n\t"
                   "syscall\n\t"
                   "mov %r9,%rcx\n\t"
                   "2:\n\t"
#endif
                   "movq %rcx,%rsp\n"
                   "movq %r8,%rdi\n\t"             /* args */
                   "callq *(%r10,%rdx,8)\n\t"
                   "movq %rsp,%rcx\n"
                   "movdqa 0x1c0(%rcx),%xmm6\n\t"
                   "movdqa 0x1d0(%rcx),%xmm7\n\t"
                   "movdqa 0x1e0(%rcx),%xmm8\n\t"
                   "movdqa 0x1f0(%rcx),%xmm9\n\t"
                   "movdqa 0x200(%rcx),%xmm10\n\t"
                   "movdqa 0x210(%rcx),%xmm11\n\t"
                   "movdqa 0x220(%rcx),%xmm12\n\t"
                   "movdqa 0x230(%rcx),%xmm13\n\t"
                   "movdqa 0x240(%rcx),%xmm14\n\t"
                   "movdqa 0x250(%rcx),%xmm15\n\t"
                   "testl $0xffff,0x94(%rcx)\n\t"  /* frame->restore_flags */
                   "jnz .L__wine_syscall_dispatcher_return\n\t"
#ifdef __linux__
                   "testl $12,%r14d\n\t"           /* SYSCALL_HAVE_PTHREAD_TEB | SYSCALL_HAVE_WRFSGSBASE */
                   "jz 1f\n\t"
                   "movw 0x7e(%rcx),%fs\n"
                   "1:\n\t"
#endif
                   "movq 0x60(%rcx),%r14\n\t"
                   __ASM_CFI(".cfi_same_value r14\n\t")
                   "movq 0x28(%rcx),%rdi\n\t"
                   __ASM_CFI(".cfi_same_value rdi\n\t")
                   "movq 0x20(%rcx),%rsi\n\t"
                   __ASM_CFI(".cfi_same_value rsi\n\t")
                   "movq 0x88(%rcx),%rsp\n\t"
                   __ASM_CFI(".cfi_def_cfa rsp, 0\n\t")
                   __ASM_CFI(".cfi_same_value rsp\n\t")
                   "pushq 0x70(%rcx)\n\t"          /* frame->rip */
                   __ASM_CFI(".cfi_adjust_cfa_offset 8\n\t")
                   "ret" )


/***********************************************************************
 *           __wine_setjmpex
 */
__ASM_GLOBAL_FUNC( __wine_setjmpex,
                   "movq %rdx,(%rcx)\n\t"          /* jmp_buf->Frame */
                   "movq %rbx,0x8(%rcx)\n\t"       /* jmp_buf->Rbx */
                   "leaq 0x8(%rsp),%rax\n\t"
                   "movq %rax,0x10(%rcx)\n\t"      /* jmp_buf->Rsp */
                   "movq %rbp,0x18(%rcx)\n\t"      /* jmp_buf->Rbp */
                   "movq %rsi,0x20(%rcx)\n\t"      /* jmp_buf->Rsi */
                   "movq %rdi,0x28(%rcx)\n\t"      /* jmp_buf->Rdi */
                   "movq %r12,0x30(%rcx)\n\t"      /* jmp_buf->R12 */
                   "movq %r13,0x38(%rcx)\n\t"      /* jmp_buf->R13 */
                   "movq %r14,0x40(%rcx)\n\t"      /* jmp_buf->R14 */
                   "movq %r15,0x48(%rcx)\n\t"      /* jmp_buf->R15 */
                   "movq (%rsp),%rax\n\t"
                   "movq %rax,0x50(%rcx)\n\t"      /* jmp_buf->Rip */
                   "stmxcsr 0x58(%rcx)\n\t"        /* jmp_buf->MxCsr */
                   "fnstcw 0x5c(%rcx)\n\t"         /* jmp_buf->FpCsr */
                   "movdqa %xmm6,0x60(%rcx)\n\t"   /* jmp_buf->Xmm6 */
                   "movdqa %xmm7,0x70(%rcx)\n\t"   /* jmp_buf->Xmm7 */
                   "movdqa %xmm8,0x80(%rcx)\n\t"   /* jmp_buf->Xmm8 */
                   "movdqa %xmm9,0x90(%rcx)\n\t"   /* jmp_buf->Xmm9 */
                   "movdqa %xmm10,0xa0(%rcx)\n\t"  /* jmp_buf->Xmm10 */
                   "movdqa %xmm11,0xb0(%rcx)\n\t"  /* jmp_buf->Xmm11 */
                   "movdqa %xmm12,0xc0(%rcx)\n\t"  /* jmp_buf->Xmm12 */
                   "movdqa %xmm13,0xd0(%rcx)\n\t"  /* jmp_buf->Xmm13 */
                   "movdqa %xmm14,0xe0(%rcx)\n\t"  /* jmp_buf->Xmm14 */
                   "movdqa %xmm15,0xf0(%rcx)\n\t"  /* jmp_buf->Xmm15 */
                   "xorq %rax,%rax\n\t"
                   "retq" )


/***********************************************************************
 *           __wine_longjmp
 */
__ASM_GLOBAL_FUNC( __wine_longjmp,
                   "movq %rdx,%rax\n\t"            /* retval */
                   "movq 0x8(%rcx),%rbx\n\t"       /* jmp_buf->Rbx */
                   "movq 0x18(%rcx),%rbp\n\t"      /* jmp_buf->Rbp */
                   "movq 0x20(%rcx),%rsi\n\t"      /* jmp_buf->Rsi */
                   "movq 0x28(%rcx),%rdi\n\t"      /* jmp_buf->Rdi */
                   "movq 0x30(%rcx),%r12\n\t"      /* jmp_buf->R12 */
                   "movq 0x38(%rcx),%r13\n\t"      /* jmp_buf->R13 */
                   "movq 0x40(%rcx),%r14\n\t"      /* jmp_buf->R14 */
                   "movq 0x48(%rcx),%r15\n\t"      /* jmp_buf->R15 */
                   "ldmxcsr 0x58(%rcx)\n\t"        /* jmp_buf->MxCsr */
                   "fnclex\n\t"
                   "fldcw 0x5c(%rcx)\n\t"          /* jmp_buf->FpCsr */
                   "movdqa 0x60(%rcx),%xmm6\n\t"   /* jmp_buf->Xmm6 */
                   "movdqa 0x70(%rcx),%xmm7\n\t"   /* jmp_buf->Xmm7 */
                   "movdqa 0x80(%rcx),%xmm8\n\t"   /* jmp_buf->Xmm8 */
                   "movdqa 0x90(%rcx),%xmm9\n\t"   /* jmp_buf->Xmm9 */
                   "movdqa 0xa0(%rcx),%xmm10\n\t"  /* jmp_buf->Xmm10 */
                   "movdqa 0xb0(%rcx),%xmm11\n\t"  /* jmp_buf->Xmm11 */
                   "movdqa 0xc0(%rcx),%xmm12\n\t"  /* jmp_buf->Xmm12 */
                   "movdqa 0xd0(%rcx),%xmm13\n\t"  /* jmp_buf->Xmm13 */
                   "movdqa 0xe0(%rcx),%xmm14\n\t"  /* jmp_buf->Xmm14 */
                   "movdqa 0xf0(%rcx),%xmm15\n\t"  /* jmp_buf->Xmm15 */
                   "movq 0x50(%rcx),%rdx\n\t"      /* jmp_buf->Rip */
                   "movq 0x10(%rcx),%rsp\n\t"      /* jmp_buf->Rsp */
                   "jmp *%rdx" )

#endif  /* __x86_64__ */
