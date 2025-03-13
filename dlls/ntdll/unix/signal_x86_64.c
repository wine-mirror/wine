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

extern int alloc_fs_sel( int sel, void *base );
__ASM_GLOBAL_FUNC( alloc_fs_sel,
                   /* switch to 32-bit stack */
                   "pushq %rbx\n\t"
                   "pushq %r12\n\t"
                   "movq %rsp,%r12\n\t"
                   "movl 0x4(%rsi),%esp\n\t"  /* Tib.StackBase */
                   "subl $0x20,%esp\n\t"
                   /* setup modify_ldt struct on 32-bit stack */
                   "movl %edi,(%rsp)\n\t"     /* entry_number */
                   "movl %esi,4(%rsp)\n\t"    /* base */
                   "movl $~0,8(%rsp)\n\t"     /* limit */
                   "movl $0x41,12(%rsp)\n\t"  /* seg_32bit | usable */
                   /* invoke 32-bit syscall */
                   "movl %esp,%ebx\n\t"
                   "movl $0xf3,%eax\n\t"      /* SYS_set_thread_area */
                   "int $0x80\n\t"
                   /* restore stack */
                   "movl (%rsp),%eax\n\t"     /* entry_number */
                   "movq %r12,%rsp\n\t"
                   "popq %r12\n\t"
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
#define XState_sig(fpu)      (((unsigned int *)fpu->Reserved4)[12] == FP_XSTATE_MAGIC1 ? (XSAVE_AREA_HEADER *)(fpu + 1) : NULL)

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

struct machine_frame
{
    ULONG64 rip;
    ULONG64 cs;
    ULONG64 eflags;
    ULONG64 rsp;
    ULONG64 ss;
};

/* stack layout when calling KiUserExceptionDispatcher */
struct exc_stack_layout
{
    CONTEXT              context;        /* 000 */
    CONTEXT_EX           context_ex;     /* 4d0 */
    EXCEPTION_RECORD     rec;            /* 4f0 */
    ULONG64              align;          /* 588 */
    struct machine_frame machine_frame;  /* 590 */
    ULONG64              align2;         /* 5b8 */
};
C_ASSERT( offsetof(struct exc_stack_layout, rec) == 0x4f0 );
C_ASSERT( offsetof(struct exc_stack_layout, machine_frame) == 0x590 );
C_ASSERT( sizeof(struct exc_stack_layout) == 0x5c0 );

/* stack layout when calling KiUserApcDispatcher */
struct apc_stack_layout
{
    CONTEXT              context;        /* 000 */
    struct machine_frame machine_frame;  /* 4d0 */
    ULONG64              align;          /* 4f8 */
};
C_ASSERT( offsetof(struct apc_stack_layout, machine_frame) == 0x4d0 );
C_ASSERT( sizeof(struct apc_stack_layout) == 0x500 );

/* stack layout when calling KiUserCallbackDispatcher */
struct callback_stack_layout
{
    ULONG64              padding[4];     /* 000 parameter space for called function */
    void                *args;           /* 020 arguments */
    ULONG                len;            /* 028 arguments len */
    ULONG                id;             /* 02c function id */
    struct machine_frame machine_frame;  /* 030 machine frame */
    BYTE                 args_data[0];   /* 058 copied argument data */
};
C_ASSERT( offsetof(struct callback_stack_layout, machine_frame) == 0x30 );
C_ASSERT( sizeof(struct callback_stack_layout) == 0x58 );

/* flags to control the behavior of the syscall dispatcher */
#define SYSCALL_HAVE_XSAVE       1
#define SYSCALL_HAVE_XSAVEC      2
#define SYSCALL_HAVE_PTHREAD_TEB 4
#define SYSCALL_HAVE_WRFSGSBASE  8

static unsigned int syscall_flags;

#define RESTORE_FLAGS_INSTRUMENTATION CONTEXT_i386

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
    ULONG64               cs;            /* 0078 */
    ULONG64               eflags;        /* 0080 */
    ULONG64               rsp;           /* 0088 */
    ULONG64               ss;            /* 0090 */
    ULONG64               rbp;           /* 0098 */
    struct syscall_frame *prev_frame;    /* 00a0 */
    void                 *syscall_cfa;   /* 00a8 */
    DWORD                 syscall_flags; /* 00b0 */
    DWORD                 restore_flags; /* 00b4 */
    DWORD                 align[2];      /* 00b8 */
    XMM_SAVE_AREA32       xsave;         /* 00c0 */
    DECLSPEC_ALIGN(64) XSAVE_AREA_HEADER xstate;    /* 02c0 */
};

C_ASSERT( offsetof( struct syscall_frame, xsave ) == 0xc0 );
C_ASSERT( offsetof( struct syscall_frame, xstate ) == 0x2c0 );
C_ASSERT( sizeof( struct syscall_frame ) == 0x300);

struct amd64_thread_data
{
    DWORD_PTR             dr0;           /* 02f0 debug registers */
    DWORD_PTR             dr1;           /* 02f8 */
    DWORD_PTR             dr2;           /* 0300 */
    DWORD_PTR             dr3;           /* 0308 */
    DWORD_PTR             dr6;           /* 0310 */
    DWORD_PTR             dr7;           /* 0318 */
    void                 *pthread_teb;   /* 0320 thread data for pthread */
    struct syscall_frame *syscall_frame; /* 0328 syscall frame pointer */
    SYSTEM_SERVICE_TABLE *syscall_table; /* 0330 syscall table */
    DWORD                 fs;            /* 0338 WOW TEB selector */
    DWORD                 xstate_features_size;  /* 033c */
    UINT64                xstate_features_mask;  /* 0340 */
    void                **instrumentation_callback; /* 0348 */
};

C_ASSERT( sizeof(struct amd64_thread_data) <= sizeof(((struct ntdll_thread_data *)0)->cpu_data) );
C_ASSERT( offsetof( TEB, GdiTebBatch ) + offsetof( struct amd64_thread_data, pthread_teb ) == 0x320 );
C_ASSERT( offsetof( TEB, GdiTebBatch ) + offsetof( struct amd64_thread_data, syscall_frame ) == 0x328 );
C_ASSERT( offsetof( TEB, GdiTebBatch ) + offsetof( struct amd64_thread_data, syscall_table ) == 0x330 );
C_ASSERT( offsetof( TEB, GdiTebBatch ) + offsetof( struct amd64_thread_data, fs ) == 0x338 );
C_ASSERT( offsetof( TEB, GdiTebBatch ) + offsetof( struct amd64_thread_data, xstate_features_size ) == 0x33c );
C_ASSERT( offsetof( TEB, GdiTebBatch ) + offsetof( struct amd64_thread_data, xstate_features_mask ) == 0x340 );

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


extern void __wine_syscall_dispatcher_instrumentation(void);
static void *instrumentation_callback;
static pthread_mutex_t instrumentation_callback_mutex = PTHREAD_MUTEX_INITIALIZER;

void set_process_instrumentation_callback( void *callback )
{
    void *ptr = (char *)user_shared_data + page_size;
    sigset_t sigset;
    void *old;

    server_enter_uninterrupted_section( &instrumentation_callback_mutex, &sigset );
    old = InterlockedExchangePointer( &instrumentation_callback, callback );
    if (!old && callback)      InterlockedExchangePointer( ptr, __wine_syscall_dispatcher_instrumentation );
    else if (old && !callback) InterlockedExchangePointer( ptr, __wine_syscall_dispatcher );
    server_leave_uninterrupted_section( &instrumentation_callback_mutex, &sigset );
}

struct xcontext
{
    CONTEXT c;
    CONTEXT_EX c_ex;
};

static inline XSAVE_AREA_HEADER *xstate_from_context( const CONTEXT *context )
{
    CONTEXT_EX *xctx = (CONTEXT_EX *)(context + 1);

    if ((context->ContextFlags & CONTEXT_XSTATE) != CONTEXT_XSTATE) return NULL;
    return (XSAVE_AREA_HEADER *)((char *)xctx + xctx->XState.Offset);
}

static inline void context_init_xstate( CONTEXT *context, void *xstate_buffer )
{
    CONTEXT_EX *xctx;

    xctx = (CONTEXT_EX *)(context + 1);
    xctx->Legacy.Length = sizeof(CONTEXT);
    xctx->Legacy.Offset = -(LONG)sizeof(CONTEXT);

    if (xstate_buffer)
    {
        xctx->XState.Length = sizeof(XSAVE_AREA_HEADER) + xstate_features_size;
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
    EFL_sig(sigcontext) = context->EFlags;
}


/***********************************************************************
 *           init_handler
 */
static inline ucontext_t *init_handler( void *sigcontext )
{
#ifdef __linux__
    if (fs32_sel)
    {
        struct ntdll_thread_data *thread_data = (struct ntdll_thread_data *)&get_current_teb()->GdiTebBatch;
        arch_prctl( ARCH_SET_FS, ((struct amd64_thread_data *)thread_data->cpu_data)->pthread_teb );
    }
#endif
    return sigcontext;
}


/***********************************************************************
 *           leave_handler
 */
static inline void leave_handler( ucontext_t *sigcontext )
{
#ifdef __linux__
    if (fs32_sel && !is_inside_signal_stack( (void *)RSP_sig(sigcontext )) && !is_inside_syscall(sigcontext))
        __asm__ volatile( "movw %0,%%fs" :: "r" (fs32_sel) );
#endif
#ifdef DS_sig
    DS_sig(sigcontext) = ds64_sel;
#else
    __asm__ volatile( "movw %0,%%ds" :: "r" (ds64_sel) );
#endif
#ifdef ES_sig
    ES_sig(sigcontext) = ds64_sel;
#else
    __asm__ volatile( "movw %0,%%es" :: "r" (ds64_sel) );
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
    context->EFlags = EFL_sig(sigcontext);
    context->SegDs  = ds64_sel;
    context->SegEs  = ds64_sel;
    context->SegGs  = ds64_sel;
    context->SegSs  = ds64_sel;
    context->Dr0    = amd64_thread_data()->dr0;
    context->Dr1    = amd64_thread_data()->dr1;
    context->Dr2    = amd64_thread_data()->dr2;
    context->Dr3    = amd64_thread_data()->dr3;
    context->Dr6    = amd64_thread_data()->dr6;
    context->Dr7    = amd64_thread_data()->dr7;
    if (FPU_sig(sigcontext))
    {
        XSAVE_AREA_HEADER *xs;

        context->ContextFlags |= CONTEXT_FLOATING_POINT;
        context->FltSave = *FPU_sig(sigcontext);
        context->MxCsr = context->FltSave.MxCsr;
        if (xstate_extended_features() && (xs = XState_sig(FPU_sig(sigcontext))))
        {
            /* xcontext and sigcontext are both on the signal stack, so we can
             * just reference sigcontext without overflowing 32 bit XState.Offset */
            context_init_xstate( context, xs );
            assert( xcontext->c_ex.XState.Offset == (BYTE *)xs - (BYTE *)&xcontext->c_ex );
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

    amd64_thread_data()->dr0 = context->Dr0;
    amd64_thread_data()->dr1 = context->Dr1;
    amd64_thread_data()->dr2 = context->Dr2;
    amd64_thread_data()->dr3 = context->Dr3;
    amd64_thread_data()->dr6 = context->Dr6;
    amd64_thread_data()->dr7 = context->Dr7;
    set_sigcontext( context, sigcontext );
    if (FPU_sig(sigcontext)) *FPU_sig(sigcontext) = context->FltSave;
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

    if ((flags & CONTEXT_XSTATE) && xstate_extended_features())
    {
        CONTEXT_EX *context_ex = (CONTEXT_EX *)(context + 1);
        XSAVE_AREA_HEADER *xs = (XSAVE_AREA_HEADER *)((char *)context_ex + context_ex->XState.Offset);

        if (context_ex->XState.Length < sizeof(XSAVE_AREA_HEADER) ||
            context_ex->XState.Length > sizeof(XSAVE_AREA_HEADER) + xstate_features_size)
            return STATUS_INVALID_PARAMETER;
        if ((xs->Mask & xstate_extended_features())
            && (context_ex->XState.Length < xstate_get_size( xs->CompactionMask, xs->Mask )))
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
#ifdef __APPLE__
        if ((flags & CONTEXT_DEBUG_REGISTERS) && (ret == STATUS_UNSUCCESSFUL))
            WARN_(seh)( "Setting debug registers is not supported under Rosetta\n" );
#endif
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
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
        frame->xsave = context->FltSave;
        frame->xstate.Mask |= XSTATE_MASK_LEGACY;
    }
    if (flags & CONTEXT_XSTATE)
    {
        CONTEXT_EX *context_ex = (CONTEXT_EX *)(context + 1);
        XSAVE_AREA_HEADER *xs = (XSAVE_AREA_HEADER *)((char *)context_ex + context_ex->XState.Offset);
        UINT64 mask = frame->xstate.Mask;

        if (xstate_compaction_enabled) frame->xstate.CompactionMask |= xstate_extended_features();
        copy_xstate( &frame->xstate, xs, xs->Mask );
        if (xs->CompactionMask) frame->xstate.Mask |= mask & ~xs->CompactionMask;
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
        context->SegCs  = cs64_sel;
        context->SegSs  = ds64_sel;
        context->ContextFlags |= CONTEXT_CONTROL;
    }
    if (needed_flags & CONTEXT_SEGMENTS)
    {
        context->SegDs  = ds64_sel;
        context->SegEs  = ds64_sel;
        context->SegFs  = amd64_thread_data()->fs;
        context->SegGs  = ds64_sel;
        context->ContextFlags |= CONTEXT_SEGMENTS;
    }
    if (needed_flags & CONTEXT_FLOATING_POINT)
    {
        if (!xstate_compaction_enabled ||
            (frame->xstate.Mask & XSTATE_MASK_LEGACY_FLOATING_POINT))
        {
            memcpy( &context->FltSave, &frame->xsave, FIELD_OFFSET( XSAVE_FORMAT, MxCsr ));
            memcpy( context->FltSave.FloatRegisters, frame->xsave.FloatRegisters,
                    sizeof( context->FltSave.FloatRegisters ));
        }
        else
        {
            memset( &context->FltSave, 0, FIELD_OFFSET( XSAVE_FORMAT, MxCsr ));
            memset( context->FltSave.FloatRegisters, 0,
                    sizeof( context->FltSave.FloatRegisters ));
            context->FltSave.ControlWord = 0x37f;
        }

        if (!xstate_compaction_enabled || (frame->xstate.Mask & XSTATE_MASK_LEGACY_SSE))
        {
            memcpy( context->FltSave.XmmRegisters, frame->xsave.XmmRegisters,
                    sizeof( context->FltSave.XmmRegisters ));
            context->FltSave.MxCsr      = frame->xsave.MxCsr;
            context->FltSave.MxCsr_Mask = frame->xsave.MxCsr_Mask;
        }
        else
        {
            memset( context->FltSave.XmmRegisters, 0,
                    sizeof( context->FltSave.XmmRegisters ));
            context->FltSave.MxCsr      = 0x1f80;
            context->FltSave.MxCsr_Mask = 0x2ffff;
        }

        context->MxCsr = context->FltSave.MxCsr;
        context->ContextFlags |= CONTEXT_FLOATING_POINT;
    }
    if ((needed_flags & CONTEXT_XSTATE) && xstate_extended_features())
    {
        CONTEXT_EX *context_ex = (CONTEXT_EX *)(context + 1);
        XSAVE_AREA_HEADER *xstate = (XSAVE_AREA_HEADER *)((char *)context_ex + context_ex->XState.Offset);
        UINT64 mask;

        if (context_ex->XState.Length < sizeof(XSAVE_AREA_HEADER) ||
            context_ex->XState.Length > sizeof(XSAVE_AREA_HEADER) + xstate_features_size)
            return STATUS_INVALID_PARAMETER;

        if (xstate_compaction_enabled) frame->xstate.CompactionMask |= xstate_extended_features();
        mask = (xstate_compaction_enabled ? xstate->CompactionMask : xstate->Mask) & xstate_extended_features();
        xstate->Mask = frame->xstate.Mask & mask;
        xstate->CompactionMask = xstate_compaction_enabled ? (0x8000000000000000 | mask) : 0;
        memset( xstate->Reserved2, 0, sizeof(xstate->Reserved2) );
        if (xstate->Mask)
        {
            if (context_ex->XState.Length < xstate_get_size( xstate->CompactionMask, xstate->Mask ))
                return STATUS_BUFFER_OVERFLOW;
            copy_xstate( xstate, &frame->xstate, xstate->Mask );
            /* copy_xstate may use avx in memcpy, restore xstate not to break the tests. */
            frame->restore_flags |= CONTEXT_XSTATE;
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
    set_context_exception_reporting_flags( &context->ContextFlags, CONTEXT_SERVICE_ACTIVE );
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
        XSAVE_AREA_HEADER *xs = (XSAVE_AREA_HEADER *)((char *)context_ex + context_ex->XState.Offset);
        UINT64 mask = frame->xstate.Mask;

        if (xstate_compaction_enabled) frame->xstate.CompactionMask |= xstate_extended_features();
        copy_xstate( &frame->xstate, xs, xs->Mask );
        if (xs->CompactionMask) frame->xstate.Mask |= mask & ~xs->CompactionMask;
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
        context->SegCs  = cs32_sel;
        context->SegSs  = ds64_sel;
        context->ContextFlags |= CONTEXT_I386_CONTROL;
    }
    if (needed_flags & CONTEXT_I386_SEGMENTS)
    {
        context->SegDs = ds64_sel;
        context->SegEs = ds64_sel;
        context->SegFs = amd64_thread_data()->fs;
        context->SegGs = ds64_sel;
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
    if ((needed_flags & CONTEXT_I386_XSTATE) && xstate_extended_features())
    {
        CONTEXT_EX *context_ex = (CONTEXT_EX *)(context + 1);
        XSAVE_AREA_HEADER *xstate = (XSAVE_AREA_HEADER *)((char *)context_ex + context_ex->XState.Offset);
        UINT64 mask;

        if (context_ex->XState.Length < sizeof(XSAVE_AREA_HEADER) ||
            context_ex->XState.Length > sizeof(XSAVE_AREA_HEADER) + xstate_features_size)
            return STATUS_INVALID_PARAMETER;

        if (xstate_compaction_enabled) frame->xstate.CompactionMask |= xstate_extended_features();
        mask = (xstate_compaction_enabled ? xstate->CompactionMask : xstate->Mask) & xstate_extended_features();
        xstate->Mask = frame->xstate.Mask & mask;
        xstate->CompactionMask = xstate_compaction_enabled ? (0x8000000000000000 | mask) : 0;
        memset( xstate->Reserved2, 0, sizeof(xstate->Reserved2) );
        if (xstate->Mask)
        {
            if (context_ex->XState.Length < xstate_get_size( xstate->CompactionMask, xstate->Mask ))
                return STATUS_BUFFER_OVERFLOW;
            copy_xstate( xstate, &frame->xstate, xstate->Mask );
            /* copy_xstate may use avx in memcpy, restore xstate not to break the tests. */
            frame->restore_flags |= CONTEXT_XSTATE;
        }
    }
    set_context_exception_reporting_flags( &context->ContextFlags, CONTEXT_SERVICE_ACTIVE );
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           setup_raise_exception
 */
static void setup_raise_exception( ucontext_t *sigcontext, EXCEPTION_RECORD *rec, struct xcontext *xcontext )
{
    void *stack_ptr = (void *)(RSP_sig(sigcontext) & ~15);
    CONTEXT *context = &xcontext->c;
    struct exc_stack_layout *stack;
    size_t stack_size;
    NTSTATUS status;
    XSAVE_AREA_HEADER *src_xs;
    unsigned int xstate_size;
    void *callback;

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

    status = send_debug_event( rec, context, TRUE, TRUE );
    if (status == DBG_CONTINUE || status == DBG_EXCEPTION_HANDLED)
    {
        restore_context( xcontext, sigcontext );
        return;
    }

    /* fix up instruction pointer in context for EXCEPTION_BREAKPOINT */
    if (rec->ExceptionCode == EXCEPTION_BREAKPOINT) context->Rip--;

    xstate_size = sizeof(XSAVE_AREA_HEADER) + xstate_features_size;
    stack_size = (ULONG_PTR)stack_ptr - (((ULONG_PTR)stack_ptr - sizeof(*stack) - xstate_size) & ~(ULONG_PTR)63);
    stack = virtual_setup_exception( stack_ptr, stack_size, rec );
    stack->rec               = *rec;
    stack->context           = *context;
    stack->machine_frame.rip = context->Rip;
    stack->machine_frame.rsp = context->Rsp;

    if ((src_xs = xstate_from_context( context )))
    {
        XSAVE_AREA_HEADER *dst_xs = (XSAVE_AREA_HEADER *)(stack + 1);
        assert( !((ULONG_PTR)dst_xs & 63) );
        context_init_xstate( &stack->context, dst_xs );
        memset( dst_xs, 0, sizeof(*dst_xs) );
        dst_xs->CompactionMask = xstate_compaction_enabled ? 0x8000000000000000 | xstate_extended_features() : 0;
        copy_xstate( dst_xs, src_xs, src_xs->Mask );
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
    if ((callback = instrumentation_callback))
    {
        R10_sig(sigcontext) = RIP_sig(sigcontext);
        RIP_sig(sigcontext) = (ULONG64)callback;
    }
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
    stack->context.P1Home    = arg1;
    stack->context.P2Home    = arg2;
    stack->context.P3Home    = arg3;
    stack->context.P4Home    = (ULONG64)func;
    stack->machine_frame.rip = stack->context.Rip;
    stack->machine_frame.rsp = stack->context.Rsp;
    frame->rbp = stack->context.Rbp;
    frame->rsp = (ULONG64)stack;
    frame->rip = (ULONG64)pKiUserApcDispatcher;
    frame->restore_flags |= CONTEXT_CONTROL;
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
    struct exc_stack_layout *stack;
    NTSTATUS status = NtSetContextThread( GetCurrentThread(), context );
    unsigned int xstate_size;

    if (status) return status;
    xstate_size = sizeof(XSAVE_AREA_HEADER) + xstate_features_size;
    stack = (struct exc_stack_layout *)((context->Rsp - sizeof(*stack) - xstate_size) & ~(ULONG_PTR)63);
    memmove( &stack->context, context, sizeof(*context) );

    if ((context->ContextFlags & CONTEXT_XSTATE) == CONTEXT_XSTATE)
    {
        XSAVE_AREA_HEADER *dst_xs = (XSAVE_AREA_HEADER *)(stack + 1);
        assert( !((ULONG_PTR)dst_xs & 63) );
        context_init_xstate( &stack->context, dst_xs );
        memcpy( dst_xs, &frame->xstate, sizeof(XSAVE_AREA_HEADER) + xstate_features_size );
    }
    else context_init_xstate( &stack->context, NULL );

    stack->rec = *rec;
    /* fix up instruction pointer in context for EXCEPTION_BREAKPOINT */
    if (stack->rec.ExceptionCode == EXCEPTION_BREAKPOINT) stack->context.Rip--;
    stack->machine_frame.rip = stack->context.Rip;
    stack->machine_frame.rsp = stack->context.Rsp;
    frame->rbp = stack->context.Rbp;
    frame->rsp = (ULONG64)stack;
    frame->rip = (ULONG64)pKiUserExceptionDispatcher;
    frame->restore_flags |= CONTEXT_CONTROL;
    return status;
}


/***********************************************************************
 *           call_user_mode_callback
 */
extern NTSTATUS call_user_mode_callback( ULONG64 user_rsp, void **ret_ptr, ULONG *ret_len, void *func, TEB *teb );
__ASM_GLOBAL_FUNC( call_user_mode_callback,
                   "subq $0x48,%rsp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 0x48\n\t")
                   "movq %rbp,0x40(%rsp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %rbp,0x40\n\t")
                   "leaq 0x40(%rsp),%rbp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %rbp\n\t")
                   "movq %rbx,-0x08(%rbp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %rbx,-0x08\n\t")
                   "movq %r12,-0x10(%rbp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %r12,-0x10\n\t")
                   "movq %r13,-0x18(%rbp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %r13,-0x18\n\t")
                   "movq %r14,-0x20(%rbp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %r14,-0x20\n\t")
                   "movq %r15,-0x28(%rbp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %r15,-0x28\n\t")
                   "stmxcsr -0x30(%rbp)\n\t"
                   "fnstcw -0x2c(%rbp)\n\t"
                   "movq %rsi,-0x38(%rbp)\n\t" /* ret_ptr */
                   "movq %rdx,-0x40(%rbp)\n\t" /* ret_len */
                   "subq $0x308,%rsp\n\t"      /* sizeof(struct syscall_frame) + exception */
                   "movl 0x33c(%r8),%esi\n\t"  /* amd64_thread_data()->xstate_features_size */
                   "subq %rsi,%rsp\n\t"
                   "andq $~63,%rsp\n\t"
                   "leaq 0x10(%rbp),%rax\n\t"
                   "movq %rax,0xa8(%rsp)\n\t"  /* frame->syscall_cfa */
                   "movq 0x328(%r8),%r10\n\t"  /* amd64_thread_data()->syscall_frame */
                   "movq (%r8),%rax\n\t"       /* NtCurrentTeb()->Tib.ExceptionList */
                   "movq %rax,0x300(%rsp,%rsi)\n\t"
                   "movl 0xb0(%r10),%r14d\n\t" /* prev_frame->syscall_flags */
                   "movl %r14d,0xb0(%rsp)\n\t" /* frame->syscall_flags */
                   "movq %r10,0xa0(%rsp)\n\t"  /* frame->prev_frame */
                   "movq %rsp,0x328(%r8)\n\t"  /* amd64_thread_data()->syscall_frame */
                   /* switch to user stack */
                   "movq %rdi,%rsp\n\t"        /* user_rsp */
#ifdef __linux__
                   "testl $12,%r14d\n\t"       /* SYSCALL_HAVE_PTHREAD_TEB | SYSCALL_HAVE_WRFSGSBASE */
                   "jz 1f\n\t"
                   "movw 0x338(%r8),%fs\n"     /* amd64_thread_data()->fs */
                   "1:\n\t"
#endif
                   "movq 0x348(%r8),%r10\n\t"    /* amd64_thread_data()->instrumentation_callback */
                   "movq (%r10),%r10\n\t"
                   "test %r10,%r10\n\t"
                   "jz 1f\n\t"
                   "xchgq %rcx,%r10\n\t"
                   "1\t:jmpq *%rcx" )          /* func */


/***********************************************************************
 *           user_mode_callback_return
 */
extern void DECLSPEC_NORETURN user_mode_callback_return( void *ret_ptr, ULONG ret_len,
                                                         NTSTATUS status, TEB *teb );
__ASM_GLOBAL_FUNC( user_mode_callback_return,
                   "movq 0x328(%rcx),%r10\n\t" /* amd64_thread_data()->syscall_frame */
                   "movq 0xa0(%r10),%r11\n\t"  /* frame->prev_frame */
                   "movq %r11,0x328(%rcx)\n\t" /* amd64_thread_data()->syscall_frame = prev_frame */
                   "movq 0xa8(%r10),%rbp\n\t"  /* frame->syscall_cfa */
                   "subq $0x10,%rbp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %rbp\n\t")
                   __ASM_CFI(".cfi_rel_offset %rbx,-0x08\n\t")
                   __ASM_CFI(".cfi_rel_offset %r12,-0x10\n\t")
                   __ASM_CFI(".cfi_rel_offset %r13,-0x18\n\t")
                   __ASM_CFI(".cfi_rel_offset %r14,-0x20\n\t")
                   __ASM_CFI(".cfi_rel_offset %r15,-0x28\n\t")
                   "movl 0x33c(%rcx),%eax\n\t" /* amd64_thread_data()->xstate_features_size */
                   "movq 0x300(%r10,%rax),%rax\n\t" /* exception list */
                   "movq %rax,0(%rcx)\n\t"     /* teb->Tib.ExceptionList */
                   "movq -0x38(%rbp),%r10\n\t"  /* ret_ptr */
                   "movq -0x40(%rbp),%r11\n\t"  /* ret_len */
                   "movq %rdi,(%r10)\n\t"
                   "movl %esi,(%r11)\n\t"
                   "ldmxcsr -0x30(%rbp)\n\t"
                   "fnclex\n\t"
                   "fldcw -0x2c(%rbp)\n\t"
                   "movq -0x28(%rbp),%r15\n\t"
                   __ASM_CFI(".cfi_same_value %r15\n\t")
                   "movq -0x20(%rbp),%r14\n\t"
                   __ASM_CFI(".cfi_same_value %r14\n\t")
                   "movq -0x18(%rbp),%r13\n\t"
                   __ASM_CFI(".cfi_same_value %r13\n\t")
                   "movq -0x10(%rbp),%r12\n\t"
                   __ASM_CFI(".cfi_same_value %r12\n\t")
                   "movq -0x08(%rbp),%rbx\n\t"
                   __ASM_CFI(".cfi_same_value %rbx\n\t")
                   "leave\n"
                   __ASM_CFI(".cfi_def_cfa %rsp,8\n\t")
                   __ASM_CFI(".cfi_same_value %rbp\n\t")
                   "movq %rdx,%rax\n\t"
                   "retq" )


/***********************************************************************
 *           user_mode_abort_thread
 */
extern void DECLSPEC_NORETURN user_mode_abort_thread( NTSTATUS status, struct syscall_frame *frame );
__ASM_GLOBAL_FUNC( user_mode_abort_thread,
                   "movq 0xa8(%rsi),%rbp\n\t"  /* frame->syscall_cfa */
                   "subq $0x10,%rbp\n\t"
                   /* switch to kernel stack */
                   "movq %rbp,%rsp\n\t"
                   __ASM_CFI(".cfi_def_cfa %rbp,0x10\n\t")
                   __ASM_CFI(".cfi_offset %rip,-0x08\n\t")
                   __ASM_CFI(".cfi_offset %rbp,-0x10\n\t")
                   __ASM_CFI(".cfi_offset %rbx,-0x18\n\t")
                   __ASM_CFI(".cfi_offset %r12,-0x20\n\t")
                   __ASM_CFI(".cfi_offset %r13,-0x28\n\t")
                   __ASM_CFI(".cfi_offset %r14,-0x30\n\t")
                   __ASM_CFI(".cfi_offset %r15,-0x38\n\t")
                   "call " __ASM_NAME("abort_thread") )


/***********************************************************************
 *           KeUserModeCallback
 */
NTSTATUS KeUserModeCallback( ULONG id, const void *args, ULONG len, void **ret_ptr, ULONG *ret_len )
{
    struct syscall_frame *frame = amd64_thread_data()->syscall_frame;
    ULONG64 rsp = (frame->rsp - offsetof( struct callback_stack_layout, args_data[len] )) & ~15;
    struct callback_stack_layout *stack = (struct callback_stack_layout *)rsp;

    if ((char *)ntdll_get_thread_data()->kernel_stack + min_kernel_stack > (char *)&frame)
        return STATUS_STACK_OVERFLOW;

    stack->args              = stack->args_data;
    stack->len               = len;
    stack->id                = id;
    stack->machine_frame.rip = frame->rip;
    stack->machine_frame.rsp = frame->rsp;
    memcpy( stack->args_data, args, len );
    return call_user_mode_callback( rsp, ret_ptr, ret_len, pKiUserCallbackDispatcher, NtCurrentTeb() );
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
        rec->ExceptionFlags = EXCEPTION_NONCONTINUABLE;
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

    if (!is_inside_syscall( sigcontext )) return FALSE;

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
        RDI_sig(sigcontext) = (ULONG_PTR)ntdll_get_thread_data()->jmp_buf;
        RSI_sig(sigcontext) = 1;
        RIP_sig(sigcontext) = (ULONG_PTR)longjmp;
        ntdll_get_thread_data()->jmp_buf = NULL;
    }
    else
    {
        TRACE_(seh)( "returning to user mode ip=%016lx ret=%08x\n", frame->rip, rec->ExceptionCode );
        RDI_sig(sigcontext) = (ULONG_PTR)frame;
        RSI_sig(sigcontext) = rec->ExceptionCode;
        RIP_sig(sigcontext) = (ULONG_PTR)__wine_syscall_dispatcher_return;
    }
    return TRUE;
}


/***********************************************************************
 *           handle_syscall_trap
 *
 * Handle a trap exception during a system call.
 */
static BOOL handle_syscall_trap( ucontext_t *sigcontext, siginfo_t *siginfo )
{
    struct syscall_frame *frame = amd64_thread_data()->syscall_frame;

    /* disallow single-stepping through a syscall */

    if ((void *)RIP_sig( sigcontext ) == __wine_syscall_dispatcher
        || (void *)RIP_sig( sigcontext ) == __wine_syscall_dispatcher_instrumentation)
    {
        extern const void *__wine_syscall_dispatcher_prolog_end_ptr;

        RIP_sig( sigcontext ) = (ULONG64)__wine_syscall_dispatcher_prolog_end_ptr;
    }
    else if ((void *)RIP_sig( sigcontext ) == __wine_unix_call_dispatcher)
    {
        extern const void *__wine_unix_call_dispatcher_prolog_end_ptr;

        RIP_sig( sigcontext ) = (ULONG64)__wine_unix_call_dispatcher_prolog_end_ptr;
        R10_sig( sigcontext ) = RCX_sig( sigcontext );
    }
    else if (siginfo->si_code == 4 /* TRAP_HWBKPT */ && is_inside_syscall( sigcontext ))
    {
        TRACE_(seh)( "ignoring HWBKPT in syscall rip=%p\n", (void *)RIP_sig(sigcontext) );
        return TRUE;
    }
    else return FALSE;

    TRACE_(seh)( "ignoring trap in syscall rip=%p eflags=%08x\n",
                 (void *)RIP_sig(sigcontext), (ULONG)EFL_sig(sigcontext) );

    frame->rip = *(ULONG64 *)RSP_sig( sigcontext );
    frame->eflags = EFL_sig(sigcontext);
    frame->restore_flags = CONTEXT_CONTROL;
    if (instrumentation_callback) frame->restore_flags |= RESTORE_FLAGS_INSTRUMENTATION;

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
    ucontext_t *ucontext = init_handler( sigcontext );

    rec.ExceptionAddress = (void *)RIP_sig(ucontext);
    save_context( &context, ucontext );

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
        if (!virtual_handle_fault( &rec, (void *)RSP_sig(ucontext) ))
        {
            leave_handler( ucontext );
            return;
        }
        if (rec.ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
            rec.ExceptionInformation[0] == EXCEPTION_EXECUTE_FAULT)
        {
            ULONG flags;
            NtQueryInformationProcess( GetCurrentProcess(), ProcessExecuteFlags,
                                       &flags, sizeof(flags), NULL );
            /* send EXCEPTION_EXECUTE_FAULT only if data execution prevention is enabled */
            if (!(flags & MEM_EXECUTE_OPTION_DISABLE)) rec.ExceptionInformation[0] = EXCEPTION_READ_FAULT;
        }
        break;
    case TRAP_x86_ALIGNFLT:  /* Alignment check exception */
        if (EFL_sig(ucontext) & 0x00040000)
        {
            EFL_sig(ucontext) &= ~0x00040000;  /* reset AC flag */
            leave_handler( ucontext );
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
    if (handle_syscall_fault( ucontext, &rec, &context.c )) return;
    setup_raise_exception( ucontext, &rec, &context );
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
    ucontext_t *ucontext = init_handler( sigcontext );

    if (handle_syscall_trap( ucontext, siginfo )) return;

    rec.ExceptionAddress = (void *)RIP_sig(ucontext);
    save_context( &context, ucontext );

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
    setup_raise_exception( ucontext, &rec, &context );
}


/**********************************************************************
 *		fpe_handler
 *
 * Handler for SIGFPE.
 */
static void fpe_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    EXCEPTION_RECORD rec = { 0 };
    ucontext_t *ucontext = init_handler( sigcontext );

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
    setup_exception( ucontext, &rec );
}


/**********************************************************************
 *		int_handler
 *
 * Handler for SIGINT.
 */
static void int_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    ucontext_t *ucontext = init_handler( sigcontext );
    HANDLE handle;

    if (p__wine_ctrl_routine)
    {
        if (!NtCreateThreadEx( &handle, THREAD_ALL_ACCESS, NULL, NtCurrentProcess(),
                               p__wine_ctrl_routine, 0 /* CTRL_C_EVENT */, 0, 0, 0, 0, NULL ))
            NtClose( handle );
    }
    leave_handler( ucontext );
}


/**********************************************************************
 *		abrt_handler
 *
 * Handler for SIGABRT.
 */
static void abrt_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    ucontext_t *ucontext = init_handler( sigcontext );
    EXCEPTION_RECORD rec = { EXCEPTION_WINE_ASSERTION, EXCEPTION_NONCONTINUABLE };

    setup_exception( ucontext, &rec );
}


/**********************************************************************
 *		quit_handler
 *
 * Handler for SIGQUIT.
 */
static void quit_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    ucontext_t *ucontext = init_handler( sigcontext );

    if (!is_inside_syscall( ucontext )) user_mode_abort_thread( 0, amd64_thread_data()->syscall_frame );
    abort_thread( 0 );
}


/**********************************************************************
 *		usr1_handler
 *
 * Handler for SIGUSR1, used to signal a thread that it got suspended.
 */
static void usr1_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    ucontext_t *ucontext = init_handler( sigcontext );

    if (is_inside_syscall( ucontext ))
    {
        struct syscall_frame *frame = amd64_thread_data()->syscall_frame;
        ULONG64 saved_compaction = 0;
        I386_CONTEXT *wow_context;
        struct xcontext *context;

        context = (struct xcontext *)(((ULONG_PTR)RSP_sig(ucontext) - 128 /* red zone */ - sizeof(*context)) & ~15);
        if ((char *)context < (char *)ntdll_get_thread_data()->kernel_stack)
        {
            ERR_(seh)( "kernel stack overflow.\n" );
            return;
        }
        context->c.ContextFlags = CONTEXT_FULL | CONTEXT_SEGMENTS | CONTEXT_EXCEPTION_REQUEST;
        NtGetContextThread( GetCurrentThread(), &context->c );
        if (xstate_extended_features())
        {
            if (xstate_compaction_enabled) frame->xstate.CompactionMask |= xstate_extended_features();
            context_init_xstate( &context->c, &frame->xstate );
            saved_compaction = frame->xstate.CompactionMask;
        }
        wait_suspend( &context->c );
        if (xstate_extended_features()) frame->xstate.CompactionMask = saved_compaction;
        if (context->c.ContextFlags & 0x40)
        {
            /* xstate is updated directly in frame's xstate */
            context->c.ContextFlags &= ~0x40;
            frame->restore_flags |= 0x40;
        }
        if ((wow_context = get_cpu_area( IMAGE_FILE_MACHINE_I386 ))
             && (wow_context->ContextFlags & CONTEXT_I386_CONTROL) == CONTEXT_I386_CONTROL)
        {
            WOW64_CPURESERVED *cpu = NtCurrentTeb()->TlsSlots[WOW64_TLS_CPURESERVED];

            cpu->Flags |= WOW64_CPURESERVED_FLAG_RESET_STATE;
        }
        NtSetContextThread( GetCurrentThread(), &context->c );
    }
    else
    {
        struct xcontext context;

        save_context( &context, ucontext );
        context.c.ContextFlags |= CONTEXT_EXCEPTION_REPORTING;
        if (is_wow64() && context.c.SegCs == cs64_sel) context.c.ContextFlags |= CONTEXT_EXCEPTION_ACTIVE;
        wait_suspend( &context.c );
        restore_context( &context, ucontext );
    }
}


#ifdef __APPLE__
/**********************************************************************
 *		sigsys_handler
 *
 * Handler for SIGSYS, signals that a non-existent system call was invoked.
 * Only called on macOS 14 Sonoma and later.
 */
static void sigsys_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    extern const void *__wine_syscall_dispatcher_prolog_end_ptr;
    ucontext_t *ucontext = init_handler( sigcontext );
    struct syscall_frame *frame = amd64_thread_data()->syscall_frame;

    TRACE_(seh)("SIGSYS, rax %#llx, rip %#llx.\n", RAX_sig(ucontext), RIP_sig(ucontext));

    frame->rip = RIP_sig(ucontext) + 0xb;
    frame->rcx = RIP_sig(ucontext);
    frame->eflags = EFL_sig(ucontext);
    frame->restore_flags = 0;
    if (instrumentation_callback) frame->restore_flags |= RESTORE_FLAGS_INSTRUMENTATION;
    RCX_sig(ucontext) = (ULONG_PTR)frame;
    R11_sig(ucontext) = frame->eflags;
    EFL_sig(ucontext) &= ~0x100;  /* clear single-step flag */
    RIP_sig(ucontext) = (ULONG64)__wine_syscall_dispatcher_prolog_end_ptr;
}
#endif


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
    WOW_TEB *wow_teb = get_wow_teb( teb );

    if (wow_teb)
    {
        if (!fs32_sel)
        {
            sigset_t sigset;
            int idx;
            LDT_ENTRY entry = ldt_make_entry( wow_teb, page_size - 1, LDT_FLAGS_DATA | LDT_FLAGS_32BIT );

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
    thread_data->xstate_features_size = xstate_features_size;
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
    WOW_TEB *wow_teb = get_wow_teb( NtCurrentTeb() );
    void *ptr, *kernel_stack = (char *)ntdll_get_thread_data()->kernel_stack + kernel_stack_size;

    amd64_thread_data()->syscall_frame = (struct syscall_frame *)((ULONG_PTR)((char *)kernel_stack
                                         - sizeof(struct syscall_frame) - xstate_features_size) & ~(ULONG_PTR)63);
    amd64_thread_data()->xstate_features_size = xstate_features_size;

    /* sneak in a syscall dispatcher pointer at a fixed address (7ffe1000) */
    ptr = (char *)user_shared_data + page_size;
    anon_mmap_fixed( ptr, page_size, PROT_READ | PROT_WRITE, 0 );
    *(void **)ptr = __wine_syscall_dispatcher;

    if (cpu_info.ProcessorFeatureBits & CPU_FEATURE_XSAVE) syscall_flags |= SYSCALL_HAVE_XSAVE;
    if (xstate_compaction_enabled) syscall_flags |= SYSCALL_HAVE_XSAVEC;

#ifdef __linux__
    if (wow_teb)
    {
        int sel;

        cs32_sel = 0x23;
        if ((sel = alloc_fs_sel( -1, wow_teb )) != -1)
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
    if (wow_teb)
    {
        LDT_ENTRY cs32_entry, fs32_entry;
        int idx;

        cs32_entry = ldt_make_entry( NULL, -1, LDT_FLAGS_CODE | LDT_FLAGS_32BIT );
        fs32_entry = ldt_make_entry( wow_teb, page_size - 1, LDT_FLAGS_DATA | LDT_FLAGS_32BIT );

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
#ifdef __APPLE__
    sig_act.sa_sigaction = sigsys_handler;
    if (sigaction( SIGSYS, &sig_act, NULL ) == -1) goto error;
#endif
    return;

 error:
    perror("sigaction");
    exit(1);
}


/***********************************************************************
 *           call_init_thunk
 */
void call_init_thunk( LPTHREAD_START_ROUTINE entry, void *arg, BOOL suspend, TEB *teb,
                      struct syscall_frame *frame, void *syscall_cfa )
{
    struct amd64_thread_data *thread_data = (struct amd64_thread_data *)&teb->GdiTebBatch;
    CONTEXT *ctx, context = { 0 };
    I386_CONTEXT *wow_context;
    void *callback;

    thread_data->syscall_table = KeServiceDescriptorTable;
    thread_data->xstate_features_mask = xstate_supported_features_mask;
    assert( thread_data->xstate_features_size == xstate_features_size );
    thread_data->instrumentation_callback = &instrumentation_callback;

#if defined __linux__
    arch_prctl( ARCH_SET_GS, teb );
    arch_prctl( ARCH_GET_FS, &thread_data->pthread_teb );
    if (fs32_sel) alloc_fs_sel( fs32_sel >> 3, get_wow_teb( teb ));
#elif defined (__FreeBSD__) || defined (__FreeBSD_kernel__)
    amd64_set_gsbase( teb );
#elif defined(__NetBSD__)
    sysarch( X86_64_SET_GSBASE, &teb );
#elif defined (__APPLE__)
    __asm__ volatile ("movq %0,%%gs:%c1" :: "r" (teb->Tib.Self), "n" (FIELD_OFFSET(TEB, Tib.Self)));
    __asm__ volatile ("movq %0,%%gs:%c1" :: "r" (teb->ThreadLocalStoragePointer), "n" (FIELD_OFFSET(TEB, ThreadLocalStoragePointer)));
    thread_data->pthread_teb = mac_thread_gsbase();
    /* alloc_tls_slot() needs to poke a value to an address relative to each
       thread's gsbase.  Have each thread record its gsbase pointer into its
       TEB so alloc_tls_slot() can find it. */
    teb->Instrumentation[0] = thread_data->pthread_teb;
#else
# error Please define setting %gs for your architecture
#endif

    context.ContextFlags = CONTEXT_ALL | CONTEXT_EXCEPTION_REPORTING | CONTEXT_EXCEPTION_ACTIVE;
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
    context.FltSave.ControlWord = 0x27f;
    context.FltSave.MxCsr = context.MxCsr = 0x1f80;

    if ((wow_context = get_cpu_area( IMAGE_FILE_MACHINE_I386 )))
    {
        wow_context->ContextFlags = CONTEXT_I386_ALL;
        wow_context->Eax = (ULONG_PTR)entry;
        wow_context->Ebx = (arg == peb ? (ULONG_PTR)wow_peb : (ULONG_PTR)arg);
        wow_context->Esp = get_wow_teb( teb )->Tib.StackBase - 16;
        wow_context->Eip = pLdrSystemDllInitBlock->pRtlUserThreadStart;
        wow_context->SegCs = cs32_sel;
        wow_context->SegDs = ds64_sel;
        wow_context->SegEs = ds64_sel;
        wow_context->SegFs = thread_data->fs;
        wow_context->SegGs = ds64_sel;
        wow_context->SegSs = ds64_sel;
        wow_context->EFlags = 0x202;
        wow_context->FloatSave.ControlWord = context.FltSave.ControlWord;
        *(XSAVE_FORMAT *)wow_context->ExtendedRegisters = context.FltSave;
    }

    if (suspend) wait_suspend( &context );

    ctx = (CONTEXT *)((ULONG_PTR)context.Rsp & ~15) - 1;
    *ctx = context;
    ctx->ContextFlags = CONTEXT_FULL;
    memset( frame, 0, sizeof(*frame) );
    if (xstate_compaction_enabled)
        frame->xstate.CompactionMask = 0x8000000000000000 | xstate_supported_features_mask;
    NtSetContextThread( GetCurrentThread(), ctx );

    frame->cs  = cs64_sel;
    frame->ss  = ds64_sel;
    frame->rsp = (ULONG64)ctx - 8;
    frame->rip = (ULONG64)pLdrInitializeThunk;
    frame->rcx = (ULONG64)ctx;
    frame->prev_frame = NULL;
    frame->restore_flags |= CONTEXT_INTEGER;
    frame->syscall_flags = syscall_flags;
    frame->syscall_cfa   = syscall_cfa;
    if ((callback = instrumentation_callback))
    {
        frame->r10 = frame->rip;
        frame->rip = (ULONG64)callback;
    }

    pthread_sigmask( SIG_UNBLOCK, &server_block_set, NULL );
    __wine_syscall_dispatcher_return( frame, 0 );
}


/***********************************************************************
 *           signal_start_thread
 */
__ASM_GLOBAL_FUNC( signal_start_thread,
                   "subq $0x38,%rsp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 0x38\n\t")
                   "movq %rbp,0x30(%rsp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %rbp,0x30\n\t")
                   "leaq 0x30(%rsp),%rbp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %rbp\n\t")
                   "movq %rbx,-0x08(%rbp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %rbx,-0x08\n\t")
                   "movq %r12,-0x10(%rbp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %r12,-0x10\n\t")
                   "movq %r13,-0x18(%rbp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %r13,-0x18\n\t")
                   "movq %r14,-0x20(%rbp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %r14,-0x20\n\t")
                   "movq %r15,-0x28(%rbp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %r15,-0x28\n\t")
                   "leaq 0x10(%rbp),%r9\n\t"       /* syscall_cfa */
                   /* set syscall frame */
                   "movq 0x328(%rcx),%r8\n\t"      /* amd64_thread_data()->syscall_frame */
                   "orq %r8,%r8\n\t"
                   "jnz 1f\n\t"
                   "leaq -0x300(%rsp),%r8\n\t"     /* sizeof(struct syscall_frame) */
                   "movl 0x33c(%rcx),%eax\n\t"     /* amd64_thread_data()->xstate_features_size */
                   "subq %rax,%r8\n\t"
                   "andq $~63,%r8\n\t"
                   "movq %r8,0x328(%rcx)\n"        /* amd64_thread_data()->syscall_frame */
                   /* switch to kernel stack */
                   "1:\tmovq %r8,%rsp\n\t"
                   "call " __ASM_NAME("call_init_thunk"))


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
                   "movl $0,0xb4(%rcx)\n\t"        /* frame->restore_flags */
                   __ASM_LOCAL_LABEL("__wine_syscall_dispatcher_prolog_end") ":\n\t"
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
                   "movq %rsp,0x88(%rcx)\n\t"
                   __ASM_CFI_CFA_IS_AT2(rcx, 0x88, 0x01)
                   "movw %ss,0x90(%rcx)\n\t"
                   "movq %rbp,0x98(%rcx)\n\t"
                   __ASM_CFI_REG_IS_AT2(rbp, rcx, 0x98, 0x01)
                   /* Legends of Runeterra hooks the first system call return instruction, and
                    * depends on us returning to it. Adjust the return address accordingly. */
                   "subq $0xb,0x70(%rcx)\n\t"
                   "movl 0xb0(%rcx),%r14d\n\t"     /* frame->syscall_flags */
                   "testl $3,%r14d\n\t"            /* SYSCALL_HAVE_XSAVE | SYSCALL_HAVE_XSAVEC */
                   "jz 2f\n\t"
#ifdef __APPLE__
                   "movq %gs:0x30,%rdx\n\t"
                   "movl 0x340(%rdx),%eax\n\t"
#else
                   "movl %gs:0x340,%eax\n\t"       /* amd64_thread_data()->xstate_features_mask */
#endif
                   "xorl %edx,%edx\n\t"
                   "andl $7,%eax\n\t"
                   "xorq %rbp,%rbp\n\t"
                   "movq %rbp,0x2c0(%rcx)\n\t"
                   "movq %rbp,0x2c8(%rcx)\n\t"
                   "movq %rbp,0x2d0(%rcx)\n\t"
                   "testl $2,%r14d\n\t"            /* SYSCALL_HAVE_XSAVEC */
                   "jz 1f\n\t"
                   "movq %rbp,0x2d8(%rcx)\n\t"
                   "movq %rbp,0x2e0(%rcx)\n\t"
                   "movq %rbp,0x2e8(%rcx)\n\t"
                   "movq %rbp,0x2f0(%rcx)\n\t"
                   "movq %rbp,0x2f8(%rcx)\n\t"
                   /* The xsavec instruction is not supported by
                    * binutils < 2.25. */
                   ".byte 0x48, 0x0f, 0xc7, 0xa1, 0xc0, 0x00, 0x00, 0x00\n\t" /* xsavec64 0xc0(%rcx) */
                   "jmp 3f\n"
                   "1:\txsave64 0xc0(%rcx)\n\t"
                   "jmp 3f\n"
                   "2:\tfxsave64 0xc0(%rcx)\n"
                   "3:\tleaq 0x98(%rcx),%rbp\n\t"
                   __ASM_CFI_CFA_IS_AT1(rbp, 0x70)
                   __ASM_CFI_REG_IS_AT1(rip, rbp, 0x58)
                   __ASM_CFI_REG_IS_AT2(rbx, rbp, 0xf0, 0x7e)
                   __ASM_CFI_REG_IS_AT2(rsi, rbp, 0x88, 0x7f)
                   __ASM_CFI_REG_IS_AT2(rdi, rbp, 0x90, 0x7f)
                   __ASM_CFI_REG_IS_AT2(r12, rbp, 0xb8, 0x7f)
                   __ASM_CFI_REG_IS_AT1(r13, rbp, 0x40)
                   __ASM_CFI_REG_IS_AT1(r14, rbp, 0x48)
                   __ASM_CFI_REG_IS_AT1(r15, rbp, 0x50)
                   __ASM_CFI_REG_IS_AT1(rbp, rbp, 0x00)
                   "movq 0x28(%rsp),%r12\n\t"      /* 5th argument */
                   "movq 0x30(%rsp),%r13\n\t"      /* 6th argument */
                   "leaq 0x38(%rsp),%r15\n\t"      /* 7th argument */
                   /* switch to kernel stack */
                   "movq %rcx,%rsp\n\t"
                   /* we're now on the kernel stack, stitch unwind info with previous frame */
                   __ASM_CFI_CFA_IS_AT1(rbp, 0x10) /* frame->syscall_cfa */
                   __ASM_CFI(".cfi_offset %rip,-0x08\n\t")
                   __ASM_CFI(".cfi_offset %rbp,-0x10\n\t")
                   __ASM_CFI(".cfi_offset %rbx,-0x18\n\t")
                   __ASM_CFI(".cfi_offset %r12,-0x20\n\t")
                   __ASM_CFI(".cfi_offset %r13,-0x28\n\t")
                   __ASM_CFI(".cfi_offset %r14,-0x30\n\t")
                   __ASM_CFI(".cfi_offset %r15,-0x38\n\t")
                   __ASM_CFI(".cfi_undefined %rdi\n\t")
                   __ASM_CFI(".cfi_undefined %rsi\n\t")
#ifdef __linux__
                   "testl $12,%r14d\n\t"           /* SYSCALL_HAVE_PTHREAD_TEB | SYSCALL_HAVE_WRFSGSBASE */
                   "jz 2f\n\t"
                   "movq %gs:0x320,%rsi\n\t"       /* amd64_thread_data()->pthread_teb */
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
                   "movq 0x00(%rcx),%rax\n\t"
                   "movq 0x18(%rcx),%r11\n\t"      /* 2nd argument */
                   "movl %eax,%ebx\n\t"
                   "shrl $8,%ebx\n\t"
                   "andl $0x30,%ebx\n\t"           /* syscall table number */
#ifdef __APPLE__
                   "movq %gs:0x30,%rcx\n\t"
                   "movq 0x330(%rcx),%rcx\n\t"
#else
                   "movq %gs:0x330,%rcx\n\t"       /* amd64_thread_data()->syscall_table */
#endif
                   "leaq (%rcx,%rbx,2),%rbx\n\t"
                   "andl $0xfff,%eax\n\t"          /* syscall number */
                   "cmpq 16(%rbx),%rax\n\t"        /* table->ServiceLimit */
                   "jae 5f\n\t"
                   "movq 24(%rbx),%rcx\n\t"        /* table->ArgumentTable */
                   "movzbl (%rcx,%rax),%ecx\n\t"
                   "subq $0x30,%rcx\n\t"
                   "jbe 1f\n\t"
                   "subq %rcx,%rsp\n\t"
                   "shrq $3,%rcx\n\t"
                   "andq $~15,%rsp\n\t"
                   "movq %rsp,%rdi\n\t"
                   "movq %r15,%rsi\n\t"
                   "cld\n\t"
                   "rep; movsq\n"
                   "1:\tmovq %r10,%rdi\n\t"        /* 1st argument */
                   "movq %r11,%rsi\n\t"            /* 2nd argument */
                   "movq %r8,%rdx\n\t"             /* 3rd argument */
                   "movq %r9,%rcx\n\t"             /* 4th argument */
                   "movq %r12,%r8\n\t"             /* 5th argument */
                   "movq %r13,%r9\n\t"             /* 6th argument */
                   "movq (%rbx),%r10\n\t"          /* table->ServiceTable */
                   "callq *(%r10,%rax,8)\n\t"
                   "leaq -0x98(%rbp),%rcx\n\t"
                   __ASM_LOCAL_LABEL("__wine_syscall_dispatcher_return") ":\n\t"
                   "movl 0xb4(%rcx),%edx\n\t"      /* frame->restore_flags */
                   "testl $0x48,%edx\n\t"          /* CONTEXT_FLOATING_POINT | CONTEXT_XSTATE */
                   "jnz 2f\n\t"
                   "movaps 0x1c0(%rcx),%xmm6\n\t"
                   "movaps 0x1d0(%rcx),%xmm7\n\t"
                   "movaps 0x1e0(%rcx),%xmm8\n\t"
                   "movaps 0x1f0(%rcx),%xmm9\n\t"
                   "movaps 0x200(%rcx),%xmm10\n\t"
                   "movaps 0x210(%rcx),%xmm11\n\t"
                   "movaps 0x220(%rcx),%xmm12\n\t"
                   "movaps 0x230(%rcx),%xmm13\n\t"
                   "movaps 0x240(%rcx),%xmm14\n\t"
                   "movaps 0x250(%rcx),%xmm15\n\t"
                   "jmp 4f\n"
                   "2:\ttestl $3,%r14d\n\t"        /* SYSCALL_HAVE_XSAVE | SYSCALL_HAVE_XSAVEC */
                   "jz 3f\n\t"
                   "movq %rax,%r11\n\t"
#ifdef __APPLE__
                   "movq %gs:0x30,%rdx\n\t"
                   "movl 0x340(%rdx),%eax\n\t"
                   "movl 0x344(%rdx),%edx\n\t"
#else
                   "movl %gs:0x340,%eax\n\t"       /* amd64_thread_data()->xstate_features_mask */
                   "movl %gs:0x344,%edx\n\t"       /* amd64_thread_data()->xstate_features_mask high dword */
#endif
                   "xrstor64 0xc0(%rcx)\n\t"
                   "movq %r11,%rax\n\t"
                   "movl 0xb4(%rcx),%edx\n\t"      /* frame->restore_flags */
                   "jmp 4f\n"
                   "3:\tfxrstor64 0xc0(%rcx)\n"
                   "4:\tmovq 0x98(%rcx),%rbp\n\t"
                   /* push rbp-based kernel stack cfi */
                   __ASM_CFI(".cfi_remember_state\n\t")
                   __ASM_CFI_CFA_IS_AT2(rcx, 0xa8, 0x01) /* frame->syscall_cfa */
                   "movq 0x68(%rcx),%r15\n\t"
                   "movq 0x58(%rcx),%r13\n\t"
                   "movq 0x50(%rcx),%r12\n\t"
                   "movq 0x80(%rcx),%r11\n\t"      /* frame->eflags */
                   "movq 0x28(%rcx),%rdi\n\t"
                   "movq 0x20(%rcx),%rsi\n\t"
                   "movq 0x08(%rcx),%rbx\n\t"
                   "leaq 0x70(%rcx),%rsp\n\t"      /* %rsp > frame means no longer inside syscall */
#ifdef __linux__
                   "testl $12,%r14d\n\t"           /* SYSCALL_HAVE_PTHREAD_TEB | SYSCALL_HAVE_WRFSGSBASE */
                   "jz 1f\n\t"
                   "movw %gs:0x338,%fs\n"          /* amd64_thread_data()->fs */
                   "1:\n\t"
#endif
                   "testl $0x10000,%edx\n\t"       /* RESTORE_FLAGS_INSTRUMENTATION */
                   "movq 0x60(%rcx),%r14\n\t"
                   "jnz 2f\n\t"
                   "3:\ttestl $0x3,%edx\n\t"       /* CONTEXT_CONTROL | CONTEXT_INTEGER */
                   "jnz 1f\n\t"

                   /* switch to user stack */
                   "movq 0x88(%rcx),%rsp\n\t"
                   /* push rcx-based kernel stack cfi */
                   __ASM_CFI(".cfi_remember_state\n\t")
                   __ASM_CFI(".cfi_def_cfa %rsp, 0\n\t")
                   __ASM_CFI_REG_IS_AT2(rip, rcx, 0xf0, 0x00)
                   __ASM_CFI(".cfi_same_value %rbp\n\t")
                   __ASM_CFI(".cfi_same_value %rbx\n\t")
                   __ASM_CFI(".cfi_same_value %r12\n\t")
                   __ASM_CFI(".cfi_same_value %r13\n\t")
                   __ASM_CFI(".cfi_same_value %r14\n\t")
                   __ASM_CFI(".cfi_same_value %r15\n\t")
                   __ASM_CFI(".cfi_same_value %rdi\n\t")
                   __ASM_CFI(".cfi_same_value %rsi\n\t")
                   "movq 0x70(%rcx),%rcx\n\t"      /* frame->rip */
                   __ASM_CFI(".cfi_register rip, rcx\n\t")
                   "pushq %r11\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 8\n\t")
                   "popfq\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset -8\n\t")
                   "pushq %rcx\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 8\n\t")
                   "ret\n\t"
                   /* pop rcx-based kernel stack cfi */
                   __ASM_CFI(".cfi_restore_state\n")

                   "1:\ttestl $0x2,%edx\n\t"       /* CONTEXT_INTEGER */
                   "jnz 1f\n\t"
                   /* CONTEXT_CONTROL */
                   "movq (%rsp),%rcx\n\t"          /* frame->rip */
                   "iretq\n"
                   /* CONTEXT_INTEGER */
                   "1:\tmovq 0x00(%rcx),%rax\n\t"
                   "movq 0x18(%rcx),%rdx\n\t"
                   "movq 0x30(%rcx),%r8\n\t"
                   "movq 0x38(%rcx),%r9\n\t"
                   "movq 0x40(%rcx),%r10\n\t"
                   "movq 0x48(%rcx),%r11\n\t"
                   "movq 0x10(%rcx),%rcx\n\t"
                   "iretq\n"
                   /* RESTORE_FLAGS_INSTRUMENTATION */
#ifdef __APPLE__
                   "2:\tmovq %gs:0x30,%r10\n\t"
                   "movq 0x348(%r10),%r10\n\t"
#else
                   "2:\tmovq %gs:0x348,%r10\n\t"  /* amd64_thread_data()->instrumentation_callback */
#endif
                   "movq (%r10),%r10\n\t"
                   "test %r10,%r10\n\t"
                   "jz 3b\n\t"
                   "testl $0x2,%edx\n\t"          /* CONTEXT_INTEGER */
                   "jnz 1b\n\t"
                   "xchgq %r10,(%rsp)\n\t"
                   "iretq\n\t"

                   /* pop rbp-based kernel stack cfi */
                   __ASM_CFI("\t.cfi_restore_state\n")
                   "5:\tmovl $0xc000000d,%eax\n\t" /* STATUS_INVALID_PARAMETER */
                   "movq %rsp,%rcx\n\t"
                   "jmp " __ASM_LOCAL_LABEL("__wine_syscall_dispatcher_return") )

__ASM_GLOBAL_FUNC( __wine_syscall_dispatcher_return,
                   "movq %rdi,%rcx\n\t"
                   "movl 0xb0(%rcx),%r14d\n\t"     /* frame->syscall_flags */
                   "movq %rsi,%rax\n\t"
                   "jmp " __ASM_LOCAL_LABEL("__wine_syscall_dispatcher_return") )


__ASM_GLOBAL_FUNC( __wine_syscall_dispatcher_instrumentation,
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
                   "movl $0x10000,0xb4(%rcx)\n\t"    /* frame->restore_flags <- RESTORE_FLAGS_INSTRUMENTATION */
                   "jmp " __ASM_LOCAL_LABEL("__wine_syscall_dispatcher_prolog_end") )


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
                   "movl $0,0xb4(%rcx)\n\t"        /* frame->restore_flags */
                   __ASM_LOCAL_LABEL("__wine_unix_call_dispatcher_prolog_end") ":\n\t"
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
                   /* switch to kernel stack */
                   "movq %rcx,%rsp\n\t"
                   /* we're now on the kernel stack, stitch unwind info with previous frame */
                   __ASM_CFI(".cfi_remember_state\n\t")
                   __ASM_CFI_CFA_IS_AT2(rsp, 0xa8, 0x01) /* frame->syscall_cfa */
                   __ASM_CFI(".cfi_offset %rip,-0x08\n\t")
                   __ASM_CFI(".cfi_offset %rbp,-0x10\n\t")
                   __ASM_CFI(".cfi_offset %rbx,-0x18\n\t")
                   __ASM_CFI(".cfi_offset %r12,-0x20\n\t")
                   __ASM_CFI(".cfi_offset %r13,-0x28\n\t")
                   __ASM_CFI(".cfi_offset %r14,-0x30\n\t")
                   __ASM_CFI(".cfi_offset %r15,-0x38\n\t")
                   __ASM_CFI(".cfi_undefined %rdi\n\t")
                   __ASM_CFI(".cfi_undefined %rsi\n\t")
#ifdef __linux__
                   "testl $12,%r14d\n\t"           /* SYSCALL_HAVE_PTHREAD_TEB | SYSCALL_HAVE_WRFSGSBASE */
                   "jz 2f\n\t"
                   "movq %gs:0x320,%rsi\n\t"       /* amd64_thread_data()->pthread_teb */
                   "testl $8,%r14d\n\t"            /* SYSCALL_HAVE_WRFSGSBASE */
                   "jz 1f\n\t"
                   "wrfsbase %rsi\n\t"
                   "jmp 2f\n"
                   "1:\tmov $0x1002,%edi\n\t"      /* ARCH_SET_FS */
                   "mov $158,%eax\n\t"             /* SYS_arch_prctl */
                   "syscall\n\t"
                   "2:\n\t"
#endif
                   "movq %r8,%rdi\n\t"             /* args */
                   "callq *(%r10,%rdx,8)\n\t"
                   "movq %rsp,%rcx\n\t"
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
                   "testl $0xffff,0xb4(%rcx)\n\t"  /* frame->restore_flags */
                   "jnz " __ASM_LOCAL_LABEL("__wine_syscall_dispatcher_return") "\n\t"
                   /* switch to user stack */
                   "movq 0x88(%rcx),%rsp\n\t"
                   __ASM_CFI(".cfi_restore_state\n\t")
#ifdef __linux__
                   "testl $12,%r14d\n\t"           /* SYSCALL_HAVE_PTHREAD_TEB | SYSCALL_HAVE_WRFSGSBASE */
                   "jz 1f\n\t"
                   "movw %gs:0x338,%fs\n"          /* amd64_thread_data()->fs */
                   "1:\n\t"
#endif
                   "movq 0x60(%rcx),%r14\n\t"
                   "movq 0x28(%rcx),%rdi\n\t"
                   "movq 0x20(%rcx),%rsi\n\t"
                   "pushq 0x70(%rcx)\n\t"          /* frame->rip */
                   "ret" )

asm( ".data\n\t"
     __ASM_GLOBL(__ASM_NAME("__wine_syscall_dispatcher_prolog_end_ptr")) "\n"
     __ASM_NAME("__wine_syscall_dispatcher_prolog_end_ptr") ":\n\t"
     ".quad " __ASM_LOCAL_LABEL("__wine_syscall_dispatcher_prolog_end") "\n\t"
     __ASM_GLOBL(__ASM_NAME("__wine_unix_call_dispatcher_prolog_end_ptr")) "\n"
     __ASM_NAME("__wine_unix_call_dispatcher_prolog_end_ptr") ":\n\t"
     ".quad " __ASM_LOCAL_LABEL("__wine_unix_call_dispatcher_prolog_end") "\n\t"
     ".text\n\t" );

#endif  /* __x86_64__ */
