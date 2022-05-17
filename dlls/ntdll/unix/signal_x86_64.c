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
#define FPU_sig(context)     ((XMM_SAVE_AREA32 *)&(context)->uc_mcontext->__fs.__fpu_fcw)
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
};

C_ASSERT( sizeof(struct amd64_thread_data) <= sizeof(((struct ntdll_thread_data *)0)->cpu_data) );
C_ASSERT( offsetof( TEB, GdiTebBatch ) + offsetof( struct amd64_thread_data, exit_frame ) == 0x320 );
C_ASSERT( offsetof( TEB, GdiTebBatch ) + offsetof( struct amd64_thread_data, syscall_frame ) == 0x328 );
C_ASSERT( offsetof( TEB, GdiTebBatch ) + offsetof( struct amd64_thread_data, pthread_teb ) == 0x330 );

static inline struct amd64_thread_data *amd64_thread_data(void)
{
    return (struct amd64_thread_data *)ntdll_get_thread_data()->cpu_data;
}

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
 * Definitions for Dwarf unwind tables
 */

enum dwarf_call_frame_info
{
    DW_CFA_advance_loc = 0x40,
    DW_CFA_offset = 0x80,
    DW_CFA_restore = 0xc0,
    DW_CFA_nop = 0x00,
    DW_CFA_set_loc = 0x01,
    DW_CFA_advance_loc1 = 0x02,
    DW_CFA_advance_loc2 = 0x03,
    DW_CFA_advance_loc4 = 0x04,
    DW_CFA_offset_extended = 0x05,
    DW_CFA_restore_extended = 0x06,
    DW_CFA_undefined = 0x07,
    DW_CFA_same_value = 0x08,
    DW_CFA_register = 0x09,
    DW_CFA_remember_state = 0x0a,
    DW_CFA_restore_state = 0x0b,
    DW_CFA_def_cfa = 0x0c,
    DW_CFA_def_cfa_register = 0x0d,
    DW_CFA_def_cfa_offset = 0x0e,
    DW_CFA_def_cfa_expression = 0x0f,
    DW_CFA_expression = 0x10,
    DW_CFA_offset_extended_sf = 0x11,
    DW_CFA_def_cfa_sf = 0x12,
    DW_CFA_def_cfa_offset_sf = 0x13,
    DW_CFA_val_offset = 0x14,
    DW_CFA_val_offset_sf = 0x15,
    DW_CFA_val_expression = 0x16,
};

enum dwarf_operation
{
    DW_OP_addr                 = 0x03,
    DW_OP_deref                = 0x06,
    DW_OP_const1u              = 0x08,
    DW_OP_const1s              = 0x09,
    DW_OP_const2u              = 0x0a,
    DW_OP_const2s              = 0x0b,
    DW_OP_const4u              = 0x0c,
    DW_OP_const4s              = 0x0d,
    DW_OP_const8u              = 0x0e,
    DW_OP_const8s              = 0x0f,
    DW_OP_constu               = 0x10,
    DW_OP_consts               = 0x11,
    DW_OP_dup                  = 0x12,
    DW_OP_drop                 = 0x13,
    DW_OP_over                 = 0x14,
    DW_OP_pick                 = 0x15,
    DW_OP_swap                 = 0x16,
    DW_OP_rot                  = 0x17,
    DW_OP_xderef               = 0x18,
    DW_OP_abs                  = 0x19,
    DW_OP_and                  = 0x1a,
    DW_OP_div                  = 0x1b,
    DW_OP_minus                = 0x1c,
    DW_OP_mod                  = 0x1d,
    DW_OP_mul                  = 0x1e,
    DW_OP_neg                  = 0x1f,
    DW_OP_not                  = 0x20,
    DW_OP_or                   = 0x21,
    DW_OP_plus                 = 0x22,
    DW_OP_plus_uconst          = 0x23,
    DW_OP_shl                  = 0x24,
    DW_OP_shr                  = 0x25,
    DW_OP_shra                 = 0x26,
    DW_OP_xor                  = 0x27,
    DW_OP_bra                  = 0x28,
    DW_OP_eq                   = 0x29,
    DW_OP_ge                   = 0x2a,
    DW_OP_gt                   = 0x2b,
    DW_OP_le                   = 0x2c,
    DW_OP_lt                   = 0x2d,
    DW_OP_ne                   = 0x2e,
    DW_OP_skip                 = 0x2f,
    DW_OP_lit0                 = 0x30,
    DW_OP_lit1                 = 0x31,
    DW_OP_lit2                 = 0x32,
    DW_OP_lit3                 = 0x33,
    DW_OP_lit4                 = 0x34,
    DW_OP_lit5                 = 0x35,
    DW_OP_lit6                 = 0x36,
    DW_OP_lit7                 = 0x37,
    DW_OP_lit8                 = 0x38,
    DW_OP_lit9                 = 0x39,
    DW_OP_lit10                = 0x3a,
    DW_OP_lit11                = 0x3b,
    DW_OP_lit12                = 0x3c,
    DW_OP_lit13                = 0x3d,
    DW_OP_lit14                = 0x3e,
    DW_OP_lit15                = 0x3f,
    DW_OP_lit16                = 0x40,
    DW_OP_lit17                = 0x41,
    DW_OP_lit18                = 0x42,
    DW_OP_lit19                = 0x43,
    DW_OP_lit20                = 0x44,
    DW_OP_lit21                = 0x45,
    DW_OP_lit22                = 0x46,
    DW_OP_lit23                = 0x47,
    DW_OP_lit24                = 0x48,
    DW_OP_lit25                = 0x49,
    DW_OP_lit26                = 0x4a,
    DW_OP_lit27                = 0x4b,
    DW_OP_lit28                = 0x4c,
    DW_OP_lit29                = 0x4d,
    DW_OP_lit30                = 0x4e,
    DW_OP_lit31                = 0x4f,
    DW_OP_reg0                 = 0x50,
    DW_OP_reg1                 = 0x51,
    DW_OP_reg2                 = 0x52,
    DW_OP_reg3                 = 0x53,
    DW_OP_reg4                 = 0x54,
    DW_OP_reg5                 = 0x55,
    DW_OP_reg6                 = 0x56,
    DW_OP_reg7                 = 0x57,
    DW_OP_reg8                 = 0x58,
    DW_OP_reg9                 = 0x59,
    DW_OP_reg10                = 0x5a,
    DW_OP_reg11                = 0x5b,
    DW_OP_reg12                = 0x5c,
    DW_OP_reg13                = 0x5d,
    DW_OP_reg14                = 0x5e,
    DW_OP_reg15                = 0x5f,
    DW_OP_reg16                = 0x60,
    DW_OP_reg17                = 0x61,
    DW_OP_reg18                = 0x62,
    DW_OP_reg19                = 0x63,
    DW_OP_reg20                = 0x64,
    DW_OP_reg21                = 0x65,
    DW_OP_reg22                = 0x66,
    DW_OP_reg23                = 0x67,
    DW_OP_reg24                = 0x68,
    DW_OP_reg25                = 0x69,
    DW_OP_reg26                = 0x6a,
    DW_OP_reg27                = 0x6b,
    DW_OP_reg28                = 0x6c,
    DW_OP_reg29                = 0x6d,
    DW_OP_reg30                = 0x6e,
    DW_OP_reg31                = 0x6f,
    DW_OP_breg0                = 0x70,
    DW_OP_breg1                = 0x71,
    DW_OP_breg2                = 0x72,
    DW_OP_breg3                = 0x73,
    DW_OP_breg4                = 0x74,
    DW_OP_breg5                = 0x75,
    DW_OP_breg6                = 0x76,
    DW_OP_breg7                = 0x77,
    DW_OP_breg8                = 0x78,
    DW_OP_breg9                = 0x79,
    DW_OP_breg10               = 0x7a,
    DW_OP_breg11               = 0x7b,
    DW_OP_breg12               = 0x7c,
    DW_OP_breg13               = 0x7d,
    DW_OP_breg14               = 0x7e,
    DW_OP_breg15               = 0x7f,
    DW_OP_breg16               = 0x80,
    DW_OP_breg17               = 0x81,
    DW_OP_breg18               = 0x82,
    DW_OP_breg19               = 0x83,
    DW_OP_breg20               = 0x84,
    DW_OP_breg21               = 0x85,
    DW_OP_breg22               = 0x86,
    DW_OP_breg23               = 0x87,
    DW_OP_breg24               = 0x88,
    DW_OP_breg25               = 0x89,
    DW_OP_breg26               = 0x8a,
    DW_OP_breg27               = 0x8b,
    DW_OP_breg28               = 0x8c,
    DW_OP_breg29               = 0x8d,
    DW_OP_breg30               = 0x8e,
    DW_OP_breg31               = 0x8f,
    DW_OP_regx                 = 0x90,
    DW_OP_fbreg                = 0x91,
    DW_OP_bregx                = 0x92,
    DW_OP_piece                = 0x93,
    DW_OP_deref_size           = 0x94,
    DW_OP_xderef_size          = 0x95,
    DW_OP_nop                  = 0x96,
    DW_OP_push_object_address  = 0x97,
    DW_OP_call2                = 0x98,
    DW_OP_call4                = 0x99,
    DW_OP_call_ref             = 0x9a,
    DW_OP_form_tls_address     = 0x9b,
    DW_OP_call_frame_cfa       = 0x9c,
    DW_OP_bit_piece            = 0x9d,
    DW_OP_lo_user              = 0xe0,
    DW_OP_hi_user              = 0xff,
    DW_OP_GNU_push_tls_address = 0xe0,
    DW_OP_GNU_uninit           = 0xf0,
    DW_OP_GNU_encoded_addr     = 0xf1,
};

#define DW_EH_PE_native   0x00
#define DW_EH_PE_uleb128  0x01
#define DW_EH_PE_udata2   0x02
#define DW_EH_PE_udata4   0x03
#define DW_EH_PE_udata8   0x04
#define DW_EH_PE_sleb128  0x09
#define DW_EH_PE_sdata2   0x0a
#define DW_EH_PE_sdata4   0x0b
#define DW_EH_PE_sdata8   0x0c
#define DW_EH_PE_signed   0x08
#define DW_EH_PE_abs      0x00
#define DW_EH_PE_pcrel    0x10
#define DW_EH_PE_textrel  0x20
#define DW_EH_PE_datarel  0x30
#define DW_EH_PE_funcrel  0x40
#define DW_EH_PE_aligned  0x50
#define DW_EH_PE_indirect 0x80
#define DW_EH_PE_omit     0xff

struct dwarf_eh_bases
{
    void *tbase;
    void *dbase;
    void *func;
};

struct dwarf_cie
{
    unsigned int  length;
    int           id;
    unsigned char version;
    unsigned char augmentation[1];
};

struct dwarf_fde
{
    unsigned int length;
    unsigned int cie_offset;
};

extern const struct dwarf_fde *_Unwind_Find_FDE (void *, struct dwarf_eh_bases *);

static unsigned char dwarf_get_u1( const unsigned char **p )
{
    return *(*p)++;
}

static unsigned short dwarf_get_u2( const unsigned char **p )
{
    unsigned int ret = (*p)[0] | ((*p)[1] << 8);
    (*p) += 2;
    return ret;
}

static unsigned int dwarf_get_u4( const unsigned char **p )
{
    unsigned int ret = (*p)[0] | ((*p)[1] << 8) | ((*p)[2] << 16) | ((*p)[3] << 24);
    (*p) += 4;
    return ret;
}

static ULONG64 dwarf_get_u8( const unsigned char **p )
{
    ULONG64 low  = dwarf_get_u4( p );
    ULONG64 high = dwarf_get_u4( p );
    return low | (high << 32);
}

static ULONG_PTR dwarf_get_uleb128( const unsigned char **p )
{
    ULONG_PTR ret = 0;
    unsigned int shift = 0;
    unsigned char byte;

    do
    {
        byte = **p;
        ret |= (ULONG_PTR)(byte & 0x7f) << shift;
        shift += 7;
        (*p)++;
    } while (byte & 0x80);
    return ret;
}

static LONG_PTR dwarf_get_sleb128( const unsigned char **p )
{
    ULONG_PTR ret = 0;
    unsigned int shift = 0;
    unsigned char byte;

    do
    {
        byte = **p;
        ret |= (ULONG_PTR)(byte & 0x7f) << shift;
        shift += 7;
        (*p)++;
    } while (byte & 0x80);

    if ((shift < 8 * sizeof(ret)) && (byte & 0x40)) ret |= -((ULONG_PTR)1 << shift);
    return ret;
}

static ULONG_PTR dwarf_get_ptr( const unsigned char **p, unsigned char encoding, const struct dwarf_eh_bases *bases )
{
    ULONG_PTR base;

    if (encoding == DW_EH_PE_omit) return 0;

    switch (encoding & 0x70)
    {
    case DW_EH_PE_abs:
        base = 0;
        break;
    case DW_EH_PE_pcrel:
        base = (ULONG_PTR)*p;
        break;
    case DW_EH_PE_textrel:
        base = (ULONG_PTR)bases->tbase;
        break;
    case DW_EH_PE_datarel:
        base = (ULONG_PTR)bases->dbase;
        break;
    case DW_EH_PE_funcrel:
        base = (ULONG_PTR)bases->func;
        break;
    case DW_EH_PE_aligned:
        base = ((ULONG_PTR)*p + 7) & ~7ul;
        break;
    default:
        FIXME( "unsupported encoding %02x\n", encoding );
        return 0;
    }

    switch (encoding & 0x0f)
    {
    case DW_EH_PE_native:  base += dwarf_get_u8( p ); break;
    case DW_EH_PE_uleb128: base += dwarf_get_uleb128( p ); break;
    case DW_EH_PE_udata2:  base += dwarf_get_u2( p ); break;
    case DW_EH_PE_udata4:  base += dwarf_get_u4( p ); break;
    case DW_EH_PE_udata8:  base += dwarf_get_u8( p ); break;
    case DW_EH_PE_sleb128: base += dwarf_get_sleb128( p ); break;
    case DW_EH_PE_sdata2:  base += (signed short)dwarf_get_u2( p ); break;
    case DW_EH_PE_sdata4:  base += (signed int)dwarf_get_u4( p ); break;
    case DW_EH_PE_sdata8:  base += (LONG64)dwarf_get_u8( p ); break;
    default:
        FIXME( "unsupported encoding %02x\n", encoding );
        return 0;
    }
    if (encoding & DW_EH_PE_indirect) base = *(ULONG_PTR *)base;
    return base;
}

enum reg_rule
{
    RULE_UNSET,          /* not set at all */
    RULE_UNDEFINED,      /* undefined value */
    RULE_SAME,           /* same value as previous frame */
    RULE_CFA_OFFSET,     /* stored at cfa offset */
    RULE_OTHER_REG,      /* stored in other register */
    RULE_EXPRESSION,     /* address specified by expression */
    RULE_VAL_EXPRESSION  /* value specified by expression */
};

#define NB_FRAME_REGS 41
#define MAX_SAVED_STATES 16

struct frame_state
{
    ULONG_PTR     cfa_offset;
    unsigned char cfa_reg;
    enum reg_rule cfa_rule;
    enum reg_rule rules[NB_FRAME_REGS];
    ULONG64       regs[NB_FRAME_REGS];
};

struct frame_info
{
    ULONG_PTR     ip;
    ULONG_PTR     code_align;
    LONG_PTR      data_align;
    unsigned char retaddr_reg;
    unsigned char fde_encoding;
    unsigned char signal_frame;
    unsigned char state_sp;
    struct frame_state state;
    struct frame_state *state_stack;
};

static const char *dwarf_reg_names[NB_FRAME_REGS] =
{
/*  0-7  */ "%rax", "%rdx", "%rcx", "%rbx", "%rsi", "%rdi", "%rbp", "%rsp",
/*  8-16 */ "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15", "%rip",
/* 17-24 */ "%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6", "%xmm7",
/* 25-32 */ "%xmm8", "%xmm9", "%xmm10", "%xmm11", "%xmm12", "%xmm13", "%xmm14", "%xmm15",
/* 33-40 */ "%st0", "%st1", "%st2", "%st3", "%st4", "%st5", "%st6", "%st7"
};

static BOOL valid_reg( ULONG_PTR reg )
{
    if (reg >= NB_FRAME_REGS) FIXME( "unsupported reg %lx\n", reg );
    return (reg < NB_FRAME_REGS);
}

static void execute_cfa_instructions( const unsigned char *ptr, const unsigned char *end,
                                      ULONG_PTR last_ip, struct frame_info *info,
                                      const struct dwarf_eh_bases *bases )
{
    while (ptr < end && info->ip < last_ip + info->signal_frame)
    {
        enum dwarf_call_frame_info op = *ptr++;

        if (op & 0xc0)
        {
            switch (op & 0xc0)
            {
            case DW_CFA_advance_loc:
            {
                ULONG_PTR offset = (op & 0x3f) * info->code_align;
                TRACE( "%lx: DW_CFA_advance_loc %lu\n", info->ip, offset );
                info->ip += offset;
                break;
            }
            case DW_CFA_offset:
            {
                ULONG_PTR reg = op & 0x3f;
                LONG_PTR offset = dwarf_get_uleb128( &ptr ) * info->data_align;
                if (!valid_reg( reg )) break;
                TRACE( "%lx: DW_CFA_offset %s, %ld\n", info->ip, dwarf_reg_names[reg], offset );
                info->state.regs[reg]  = offset;
                info->state.rules[reg] = RULE_CFA_OFFSET;
                break;
            }
            case DW_CFA_restore:
            {
                ULONG_PTR reg = op & 0x3f;
                if (!valid_reg( reg )) break;
                TRACE( "%lx: DW_CFA_restore %s\n", info->ip, dwarf_reg_names[reg] );
                info->state.rules[reg] = RULE_UNSET;
                break;
            }
            }
        }
        else switch (op)
        {
        case DW_CFA_nop:
            break;
        case DW_CFA_set_loc:
        {
            ULONG_PTR loc = dwarf_get_ptr( &ptr, info->fde_encoding, bases );
            TRACE( "%lx: DW_CFA_set_loc %lx\n", info->ip, loc );
            info->ip = loc;
            break;
        }
        case DW_CFA_advance_loc1:
        {
            ULONG_PTR offset = *ptr++ * info->code_align;
            TRACE( "%lx: DW_CFA_advance_loc1 %lu\n", info->ip, offset );
            info->ip += offset;
            break;
        }
        case DW_CFA_advance_loc2:
        {
            ULONG_PTR offset = dwarf_get_u2( &ptr ) * info->code_align;
            TRACE( "%lx: DW_CFA_advance_loc2 %lu\n", info->ip, offset );
            info->ip += offset;
            break;
        }
        case DW_CFA_advance_loc4:
        {
            ULONG_PTR offset = dwarf_get_u4( &ptr ) * info->code_align;
            TRACE( "%lx: DW_CFA_advance_loc4 %lu\n", info->ip, offset );
            info->ip += offset;
            break;
        }
        case DW_CFA_offset_extended:
        case DW_CFA_offset_extended_sf:
        {
            ULONG_PTR reg = dwarf_get_uleb128( &ptr );
            LONG_PTR offset = (op == DW_CFA_offset_extended) ? dwarf_get_uleb128( &ptr ) * info->data_align
                                                             : dwarf_get_sleb128( &ptr ) * info->data_align;
            if (!valid_reg( reg )) break;
            TRACE( "%lx: DW_CFA_offset_extended %s, %ld\n", info->ip, dwarf_reg_names[reg], offset );
            info->state.regs[reg]  = offset;
            info->state.rules[reg] = RULE_CFA_OFFSET;
            break;
        }
        case DW_CFA_restore_extended:
        {
            ULONG_PTR reg = dwarf_get_uleb128( &ptr );
            if (!valid_reg( reg )) break;
            TRACE( "%lx: DW_CFA_restore_extended %s\n", info->ip, dwarf_reg_names[reg] );
            info->state.rules[reg] = RULE_UNSET;
            break;
        }
        case DW_CFA_undefined:
        {
            ULONG_PTR reg = dwarf_get_uleb128( &ptr );
            if (!valid_reg( reg )) break;
            TRACE( "%lx: DW_CFA_undefined %s\n", info->ip, dwarf_reg_names[reg] );
            info->state.rules[reg] = RULE_UNDEFINED;
            break;
        }
        case DW_CFA_same_value:
        {
            ULONG_PTR reg = dwarf_get_uleb128( &ptr );
            if (!valid_reg( reg )) break;
            TRACE( "%lx: DW_CFA_same_value %s\n", info->ip, dwarf_reg_names[reg] );
            info->state.regs[reg]  = reg;
            info->state.rules[reg] = RULE_SAME;
            break;
        }
        case DW_CFA_register:
        {
            ULONG_PTR reg = dwarf_get_uleb128( &ptr );
            ULONG_PTR reg2 = dwarf_get_uleb128( &ptr );
            if (!valid_reg( reg ) || !valid_reg( reg2 )) break;
            TRACE( "%lx: DW_CFA_register %s == %s\n", info->ip, dwarf_reg_names[reg], dwarf_reg_names[reg2] );
            info->state.regs[reg]  = reg2;
            info->state.rules[reg] = RULE_OTHER_REG;
            break;
        }
        case DW_CFA_remember_state:
            TRACE( "%lx: DW_CFA_remember_state\n", info->ip );
            if (info->state_sp >= MAX_SAVED_STATES)
                FIXME( "%lx: DW_CFA_remember_state too many nested saves\n", info->ip );
            else
                info->state_stack[info->state_sp++] = info->state;
            break;
        case DW_CFA_restore_state:
            TRACE( "%lx: DW_CFA_restore_state\n", info->ip );
            if (!info->state_sp)
                FIXME( "%lx: DW_CFA_restore_state without corresponding save\n", info->ip );
            else
                info->state = info->state_stack[--info->state_sp];
            break;
        case DW_CFA_def_cfa:
        case DW_CFA_def_cfa_sf:
        {
            ULONG_PTR reg = dwarf_get_uleb128( &ptr );
            ULONG_PTR offset = (op == DW_CFA_def_cfa) ? dwarf_get_uleb128( &ptr )
                                                      : dwarf_get_sleb128( &ptr ) * info->data_align;
            if (!valid_reg( reg )) break;
            TRACE( "%lx: DW_CFA_def_cfa %s, %lu\n", info->ip, dwarf_reg_names[reg], offset );
            info->state.cfa_reg    = reg;
            info->state.cfa_offset = offset;
            info->state.cfa_rule   = RULE_CFA_OFFSET;
            break;
        }
        case DW_CFA_def_cfa_register:
        {
            ULONG_PTR reg = dwarf_get_uleb128( &ptr );
            if (!valid_reg( reg )) break;
            TRACE( "%lx: DW_CFA_def_cfa_register %s\n", info->ip, dwarf_reg_names[reg] );
            info->state.cfa_reg = reg;
            info->state.cfa_rule = RULE_CFA_OFFSET;
            break;
        }
        case DW_CFA_def_cfa_offset:
        case DW_CFA_def_cfa_offset_sf:
        {
            ULONG_PTR offset = (op == DW_CFA_def_cfa_offset) ? dwarf_get_uleb128( &ptr )
                                                             : dwarf_get_sleb128( &ptr ) * info->data_align;
            TRACE( "%lx: DW_CFA_def_cfa_offset %lu\n", info->ip, offset );
            info->state.cfa_offset = offset;
            info->state.cfa_rule = RULE_CFA_OFFSET;
            break;
        }
        case DW_CFA_def_cfa_expression:
        {
            ULONG_PTR expr = (ULONG_PTR)ptr;
            ULONG_PTR len = dwarf_get_uleb128( &ptr );
            TRACE( "%lx: DW_CFA_def_cfa_expression %lx-%lx\n", info->ip, expr, expr+len );
            info->state.cfa_offset = expr;
            info->state.cfa_rule = RULE_VAL_EXPRESSION;
            ptr += len;
            break;
        }
        case DW_CFA_expression:
        case DW_CFA_val_expression:
        {
            ULONG_PTR reg = dwarf_get_uleb128( &ptr );
            ULONG_PTR expr = (ULONG_PTR)ptr;
            ULONG_PTR len = dwarf_get_uleb128( &ptr );
            if (!valid_reg( reg )) break;
            TRACE( "%lx: DW_CFA_%sexpression %s %lx-%lx\n",
                   info->ip, (op == DW_CFA_expression) ? "" : "val_", dwarf_reg_names[reg], expr, expr+len );
            info->state.regs[reg]  = expr;
            info->state.rules[reg] = (op == DW_CFA_expression) ? RULE_EXPRESSION : RULE_VAL_EXPRESSION;
            ptr += len;
            break;
        }
        default:
            FIXME( "%lx: unknown CFA opcode %02x\n", info->ip, op );
            break;
        }
    }
}

/* retrieve a context register from its dwarf number */
static void *get_context_reg( CONTEXT *context, ULONG_PTR dw_reg )
{
    switch (dw_reg)
    {
    case 0:  return &context->Rax;
    case 1:  return &context->Rdx;
    case 2:  return &context->Rcx;
    case 3:  return &context->Rbx;
    case 4:  return &context->Rsi;
    case 5:  return &context->Rdi;
    case 6:  return &context->Rbp;
    case 7:  return &context->Rsp;
    case 8:  return &context->R8;
    case 9:  return &context->R9;
    case 10: return &context->R10;
    case 11: return &context->R11;
    case 12: return &context->R12;
    case 13: return &context->R13;
    case 14: return &context->R14;
    case 15: return &context->R15;
    case 16: return &context->Rip;
    case 17: return &context->u.s.Xmm0;
    case 18: return &context->u.s.Xmm1;
    case 19: return &context->u.s.Xmm2;
    case 20: return &context->u.s.Xmm3;
    case 21: return &context->u.s.Xmm4;
    case 22: return &context->u.s.Xmm5;
    case 23: return &context->u.s.Xmm6;
    case 24: return &context->u.s.Xmm7;
    case 25: return &context->u.s.Xmm8;
    case 26: return &context->u.s.Xmm9;
    case 27: return &context->u.s.Xmm10;
    case 28: return &context->u.s.Xmm11;
    case 29: return &context->u.s.Xmm12;
    case 30: return &context->u.s.Xmm13;
    case 31: return &context->u.s.Xmm14;
    case 32: return &context->u.s.Xmm15;
    case 33: return &context->u.s.Legacy[0];
    case 34: return &context->u.s.Legacy[1];
    case 35: return &context->u.s.Legacy[2];
    case 36: return &context->u.s.Legacy[3];
    case 37: return &context->u.s.Legacy[4];
    case 38: return &context->u.s.Legacy[5];
    case 39: return &context->u.s.Legacy[6];
    case 40: return &context->u.s.Legacy[7];
    default: return NULL;
    }
}

/* set a context register from its dwarf number */
static void set_context_reg( CONTEXT *context, ULONG_PTR dw_reg, void *val )
{
    switch (dw_reg)
    {
    case 0:  context->Rax = *(ULONG64 *)val; break;
    case 1:  context->Rdx = *(ULONG64 *)val; break;
    case 2:  context->Rcx = *(ULONG64 *)val; break;
    case 3:  context->Rbx = *(ULONG64 *)val; break;
    case 4:  context->Rsi = *(ULONG64 *)val; break;
    case 5:  context->Rdi = *(ULONG64 *)val; break;
    case 6:  context->Rbp = *(ULONG64 *)val; break;
    case 7:  context->Rsp = *(ULONG64 *)val; break;
    case 8:  context->R8  = *(ULONG64 *)val; break;
    case 9:  context->R9  = *(ULONG64 *)val; break;
    case 10: context->R10 = *(ULONG64 *)val; break;
    case 11: context->R11 = *(ULONG64 *)val; break;
    case 12: context->R12 = *(ULONG64 *)val; break;
    case 13: context->R13 = *(ULONG64 *)val; break;
    case 14: context->R14 = *(ULONG64 *)val; break;
    case 15: context->R15 = *(ULONG64 *)val; break;
    case 16: context->Rip = *(ULONG64 *)val; break;
    case 17: memcpy( &context->u.s.Xmm0, val, sizeof(M128A) ); break;
    case 18: memcpy( &context->u.s.Xmm1, val, sizeof(M128A) ); break;
    case 19: memcpy( &context->u.s.Xmm2, val, sizeof(M128A) ); break;
    case 20: memcpy( &context->u.s.Xmm3, val, sizeof(M128A) ); break;
    case 21: memcpy( &context->u.s.Xmm4, val, sizeof(M128A) ); break;
    case 22: memcpy( &context->u.s.Xmm5, val, sizeof(M128A) ); break;
    case 23: memcpy( &context->u.s.Xmm6, val, sizeof(M128A) ); break;
    case 24: memcpy( &context->u.s.Xmm7, val, sizeof(M128A) ); break;
    case 25: memcpy( &context->u.s.Xmm8, val, sizeof(M128A) ); break;
    case 26: memcpy( &context->u.s.Xmm9, val, sizeof(M128A) ); break;
    case 27: memcpy( &context->u.s.Xmm10, val, sizeof(M128A) ); break;
    case 28: memcpy( &context->u.s.Xmm11, val, sizeof(M128A) ); break;
    case 29: memcpy( &context->u.s.Xmm12, val, sizeof(M128A) ); break;
    case 30: memcpy( &context->u.s.Xmm13, val, sizeof(M128A) ); break;
    case 31: memcpy( &context->u.s.Xmm14, val, sizeof(M128A) ); break;
    case 32: memcpy( &context->u.s.Xmm15, val, sizeof(M128A) ); break;
    case 33: memcpy( &context->u.s.Legacy[0], val, sizeof(M128A) ); break;
    case 34: memcpy( &context->u.s.Legacy[1], val, sizeof(M128A) ); break;
    case 35: memcpy( &context->u.s.Legacy[2], val, sizeof(M128A) ); break;
    case 36: memcpy( &context->u.s.Legacy[3], val, sizeof(M128A) ); break;
    case 37: memcpy( &context->u.s.Legacy[4], val, sizeof(M128A) ); break;
    case 38: memcpy( &context->u.s.Legacy[5], val, sizeof(M128A) ); break;
    case 39: memcpy( &context->u.s.Legacy[6], val, sizeof(M128A) ); break;
    case 40: memcpy( &context->u.s.Legacy[7], val, sizeof(M128A) ); break;
    }
}

static ULONG_PTR eval_expression( const unsigned char *p, CONTEXT *context,
                                  const struct dwarf_eh_bases *bases )
{
    ULONG_PTR reg, tmp, stack[64];
    int sp = -1;
    ULONG_PTR len = dwarf_get_uleb128(&p);
    const unsigned char *end = p + len;

    while (p < end)
    {
        unsigned char opcode = dwarf_get_u1(&p);

        if (opcode >= DW_OP_lit0 && opcode <= DW_OP_lit31)
            stack[++sp] = opcode - DW_OP_lit0;
        else if (opcode >= DW_OP_reg0 && opcode <= DW_OP_reg31)
            stack[++sp] = *(ULONG_PTR *)get_context_reg( context, opcode - DW_OP_reg0 );
        else if (opcode >= DW_OP_breg0 && opcode <= DW_OP_breg31)
            stack[++sp] = *(ULONG_PTR *)get_context_reg( context, opcode - DW_OP_breg0 ) + dwarf_get_sleb128(&p);
        else switch (opcode)
        {
        case DW_OP_nop:         break;
        case DW_OP_addr:        stack[++sp] = dwarf_get_u8(&p); break;
        case DW_OP_const1u:     stack[++sp] = dwarf_get_u1(&p); break;
        case DW_OP_const1s:     stack[++sp] = (signed char)dwarf_get_u1(&p); break;
        case DW_OP_const2u:     stack[++sp] = dwarf_get_u2(&p); break;
        case DW_OP_const2s:     stack[++sp] = (short)dwarf_get_u2(&p); break;
        case DW_OP_const4u:     stack[++sp] = dwarf_get_u4(&p); break;
        case DW_OP_const4s:     stack[++sp] = (signed int)dwarf_get_u4(&p); break;
        case DW_OP_const8u:     stack[++sp] = dwarf_get_u8(&p); break;
        case DW_OP_const8s:     stack[++sp] = (LONG_PTR)dwarf_get_u8(&p); break;
        case DW_OP_constu:      stack[++sp] = dwarf_get_uleb128(&p); break;
        case DW_OP_consts:      stack[++sp] = dwarf_get_sleb128(&p); break;
        case DW_OP_deref:       stack[sp] = *(ULONG_PTR *)stack[sp]; break;
        case DW_OP_dup:         stack[sp + 1] = stack[sp]; sp++; break;
        case DW_OP_drop:        sp--; break;
        case DW_OP_over:        stack[sp + 1] = stack[sp - 1]; sp++; break;
        case DW_OP_pick:        stack[sp + 1] = stack[sp - dwarf_get_u1(&p)]; sp++; break;
        case DW_OP_swap:        tmp = stack[sp]; stack[sp] = stack[sp-1]; stack[sp-1] = tmp; break;
        case DW_OP_rot:         tmp = stack[sp]; stack[sp] = stack[sp-1]; stack[sp-1] = stack[sp-2]; stack[sp-2] = tmp; break;
        case DW_OP_abs:         stack[sp] = labs(stack[sp]); break;
        case DW_OP_neg:         stack[sp] = -stack[sp]; break;
        case DW_OP_not:         stack[sp] = ~stack[sp]; break;
        case DW_OP_and:         stack[sp-1] &= stack[sp]; sp--; break;
        case DW_OP_or:          stack[sp-1] |= stack[sp]; sp--; break;
        case DW_OP_minus:       stack[sp-1] -= stack[sp]; sp--; break;
        case DW_OP_mul:         stack[sp-1] *= stack[sp]; sp--; break;
        case DW_OP_plus:        stack[sp-1] += stack[sp]; sp--; break;
        case DW_OP_xor:         stack[sp-1] ^= stack[sp]; sp--; break;
        case DW_OP_shl:         stack[sp-1] <<= stack[sp]; sp--; break;
        case DW_OP_shr:         stack[sp-1] >>= stack[sp]; sp--; break;
        case DW_OP_plus_uconst: stack[sp] += dwarf_get_uleb128(&p); break;
        case DW_OP_shra:        stack[sp-1] = (LONG_PTR)stack[sp-1] / (1 << stack[sp]); sp--; break;
        case DW_OP_div:         stack[sp-1] = (LONG_PTR)stack[sp-1] / (LONG_PTR)stack[sp]; sp--; break;
        case DW_OP_mod:         stack[sp-1] = (LONG_PTR)stack[sp-1] % (LONG_PTR)stack[sp]; sp--; break;
        case DW_OP_ge:          stack[sp-1] = ((LONG_PTR)stack[sp-1] >= (LONG_PTR)stack[sp]); sp--; break;
        case DW_OP_gt:          stack[sp-1] = ((LONG_PTR)stack[sp-1] >  (LONG_PTR)stack[sp]); sp--; break;
        case DW_OP_le:          stack[sp-1] = ((LONG_PTR)stack[sp-1] <= (LONG_PTR)stack[sp]); sp--; break;
        case DW_OP_lt:          stack[sp-1] = ((LONG_PTR)stack[sp-1] <  (LONG_PTR)stack[sp]); sp--; break;
        case DW_OP_eq:          stack[sp-1] = (stack[sp-1] == stack[sp]); sp--; break;
        case DW_OP_ne:          stack[sp-1] = (stack[sp-1] != stack[sp]); sp--; break;
        case DW_OP_skip:        tmp = (short)dwarf_get_u2(&p); p += tmp; break;
        case DW_OP_bra:         tmp = (short)dwarf_get_u2(&p); if (!stack[sp--]) p += tmp; break;
        case DW_OP_GNU_encoded_addr: tmp = *p++; stack[++sp] = dwarf_get_ptr( &p, tmp, bases ); break;
        case DW_OP_regx:        stack[++sp] = *(ULONG_PTR *)get_context_reg( context, dwarf_get_uleb128(&p) ); break;
        case DW_OP_bregx:
            reg = dwarf_get_uleb128(&p);
            tmp = dwarf_get_sleb128(&p);
            stack[++sp] = *(ULONG_PTR *)get_context_reg( context, reg ) + tmp;
            break;
        case DW_OP_deref_size:
            switch (*p++)
            {
            case 1: stack[sp] = *(unsigned char *)stack[sp]; break;
            case 2: stack[sp] = *(unsigned short *)stack[sp]; break;
            case 4: stack[sp] = *(unsigned int *)stack[sp]; break;
            case 8: stack[sp] = *(ULONG_PTR *)stack[sp]; break;
            }
            break;
        default:
            FIXME( "unhandled opcode %02x\n", opcode );
        }
    }
    return stack[sp];
}

/* apply the computed frame info to the actual context */
static void apply_frame_state( CONTEXT *context, struct frame_state *state,
                               const struct dwarf_eh_bases *bases )
{
    unsigned int i;
    ULONG_PTR cfa, value;
    CONTEXT new_context = *context;

    switch (state->cfa_rule)
    {
    case RULE_EXPRESSION:
        cfa = *(ULONG_PTR *)eval_expression( (const unsigned char *)state->cfa_offset, context, bases );
        break;
    case RULE_VAL_EXPRESSION:
        cfa = eval_expression( (const unsigned char *)state->cfa_offset, context, bases );
        break;
    default:
        cfa = *(ULONG_PTR *)get_context_reg( context, state->cfa_reg ) + state->cfa_offset;
        break;
    }
    if (!cfa) return;

    for (i = 0; i < NB_FRAME_REGS; i++)
    {
        switch (state->rules[i])
        {
        case RULE_UNSET:
        case RULE_UNDEFINED:
        case RULE_SAME:
            break;
        case RULE_CFA_OFFSET:
            set_context_reg( &new_context, i, (char *)cfa + state->regs[i] );
            break;
        case RULE_OTHER_REG:
            set_context_reg( &new_context, i, get_context_reg( context, state->regs[i] ));
            break;
        case RULE_EXPRESSION:
            value = eval_expression( (const unsigned char *)state->regs[i], context, bases );
            set_context_reg( &new_context, i, (void *)value );
            break;
        case RULE_VAL_EXPRESSION:
            value = eval_expression( (const unsigned char *)state->regs[i], context, bases );
            set_context_reg( &new_context, i, &value );
            break;
        }
    }
    new_context.Rsp = cfa;
    *context = new_context;
}


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
NTSTATUS CDECL unwind_builtin_dll( ULONG type, DISPATCHER_CONTEXT *dispatch, CONTEXT *context )
{
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
    if (fs32_sel) arch_prctl( ARCH_SET_FS, amd64_thread_data()->pthread_teb );
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
        wow_frame->SegFs = fs32_sel;
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
    }
    else if (flags & CONTEXT_I386_FLOATING_POINT)
    {
        fpu_to_fpux( &frame->xsave, &context->FloatSave );
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


struct user_callback_frame
{
    struct syscall_frame frame;
    void               **ret_ptr;
    ULONG               *ret_len;
    __wine_jmp_buf       jmpbuf;
    NTSTATUS             status;
};

/***********************************************************************
 *           KeUserModeCallback
 */
NTSTATUS WINAPI KeUserModeCallback( ULONG id, const void *args, ULONG len, void **ret_ptr, ULONG *ret_len )
{
    struct user_callback_frame callback_frame = { { 0 }, ret_ptr, ret_len };

    /* if we have no syscall frame, call the callback directly */
    if ((char *)&callback_frame < (char *)ntdll_get_thread_data()->kernel_stack ||
        (char *)&callback_frame > (char *)amd64_thread_data()->syscall_frame)
    {
        NTSTATUS (WINAPI *func)(const void *, ULONG) = ((void **)NtCurrentTeb()->Peb->KernelCallbackTable)[id];
        return func( args, len );
    }

    if ((char *)ntdll_get_thread_data()->kernel_stack + min_kernel_stack > (char *)&callback_frame)
        return STATUS_STACK_OVERFLOW;

    if (!__wine_setjmpex( &callback_frame.jmpbuf, NULL ))
    {
        struct syscall_frame *frame = amd64_thread_data()->syscall_frame;
        void *args_data = (void *)((frame->rsp - len) & ~15);

        memcpy( args_data, args, len );

        callback_frame.frame.rcx           = id;
        callback_frame.frame.rdx           = (ULONG_PTR)args;
        callback_frame.frame.r8            = len;
        callback_frame.frame.cs            = cs64_sel;
        callback_frame.frame.fs            = fs32_sel;
        callback_frame.frame.gs            = ds64_sel;
        callback_frame.frame.ss            = ds64_sel;
        callback_frame.frame.rsp           = (ULONG_PTR)args_data - 0x28;
        callback_frame.frame.rip           = (ULONG_PTR)pKiUserCallbackDispatcher;
        callback_frame.frame.eflags        = 0x200;
        callback_frame.frame.restore_flags = CONTEXT_CONTROL | CONTEXT_INTEGER;
        callback_frame.frame.prev_frame    = frame;
        callback_frame.frame.syscall_flags = frame->syscall_flags;
        callback_frame.frame.syscall_table = frame->syscall_table;
        amd64_thread_data()->syscall_frame = &callback_frame.frame;

        __wine_syscall_dispatcher_return( &callback_frame.frame, 0 );
    }
    return callback_frame.status;
}


/***********************************************************************
 *           NtCallbackReturn  (NTDLL.@)
 */
NTSTATUS WINAPI NtCallbackReturn( void *ret_ptr, ULONG ret_len, NTSTATUS status )
{
    struct user_callback_frame *frame = (struct user_callback_frame *)amd64_thread_data()->syscall_frame;

    if (!frame->frame.prev_frame) return STATUS_NO_CALLBACK_ACTIVE;

    *frame->ret_ptr = ret_ptr;
    *frame->ret_len = ret_len;
    frame->status = status;
    amd64_thread_data()->syscall_frame = frame->frame.prev_frame;
    __wine_longjmp( &frame->jmpbuf, 1 );
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
    extern void __wine_syscall_dispatcher_prolog_end(void);
    struct syscall_frame *frame = amd64_thread_data()->syscall_frame;

    /* disallow single-stepping through a syscall */

    if ((void *)RIP_sig( sigcontext ) != __wine_syscall_dispatcher) return FALSE;

    TRACE_(seh)( "ignoring trap in syscall rip=%p eflags=%08x\n",
                 (void *)RIP_sig(sigcontext), (ULONG)EFL_sig(sigcontext) );

    frame->rip = *(ULONG64 *)RSP_sig( sigcontext );
    frame->eflags = EFL_sig(sigcontext);
    frame->restore_flags = CONTEXT_CONTROL;

    RIP_sig( sigcontext ) = (ULONG64)__wine_syscall_dispatcher_prolog_end;
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

    switch (siginfo->si_code)
    {
    case TRAP_TRACE:  /* Single-step exception */
    case 4 /* TRAP_HWBKPT */: /* Hardware breakpoint exception */
        rec.ExceptionCode = EXCEPTION_SINGLE_STEP;
        break;
    case TRAP_BRKPT:   /* Breakpoint exception */
#ifdef SI_KERNEL
    case SI_KERNEL:
#endif
        /* Check if this is actually icebp instruction */
        if (((unsigned char *)RIP_sig(ucontext))[-1] == 0xF1)
        {
            rec.ExceptionCode = EXCEPTION_SINGLE_STEP;
            break;
        }
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

#if defined __linux__
    arch_prctl( ARCH_SET_GS, teb );
    arch_prctl( ARCH_GET_FS, &amd64_thread_data()->pthread_teb );
    if (fs32_sel) alloc_fs_sel( fs32_sel >> 3, (char *)teb + teb->WowTebOffset );
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
    amd64_thread_data()->pthread_teb = mac_thread_gsbase();

    /* alloc_tls_slot() needs to poke a value to an address relative to each
       thread's gsbase.  Have each thread record its gsbase pointer into its
       TEB so alloc_tls_slot() can find it. */
    teb->Reserved5[0] = amd64_thread_data()->pthread_teb;
#else
# error Please define setting %gs for your architecture
#endif

#ifdef __GNUC__
    __asm__ volatile ("fninit; fldcw %0" : : "m" (fpu_cw));
#else
    FIXME_(seh)("FPU setup not implemented for this platform.\n");
#endif
}


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
            fs32_sel = (sel << 3) | 3;
            syscall_flags |= SYSCALL_HAVE_PTHREAD_TEB;
            if (getauxval( AT_HWCAP2 ) & 2) syscall_flags |= SYSCALL_HAVE_WRFSGSBASE;
        }
        else ERR_(seh)( "failed to allocate %%fs selector\n" );
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

    context.ContextFlags = CONTEXT_ALL;
    context.Rcx    = (ULONG_PTR)entry;
    context.Rdx    = (ULONG_PTR)arg;
    context.Rsp    = (ULONG_PTR)teb->Tib.StackBase - 0x28;
    context.Rip    = (ULONG_PTR)pRtlUserThreadStart;
    context.SegCs  = cs64_sel;
    context.SegDs  = ds64_sel;
    context.SegEs  = ds64_sel;
    context.SegFs  = fs32_sel;
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
                   "movq %gs:0x30,%rcx\n\t"
                   "movq 0x328(%rcx),%rcx\n\t"     /* amd64_thread_data()->syscall_frame */
                   "popq 0x70(%rcx)\n\t"           /* frame->rip */
                   "pushfq\n\t"
                   "popq 0x80(%rcx)\n\t"
                   "movl $0,0x94(%rcx)\n\t"        /* frame->restore_flags */
                   __ASM_NAME("__wine_syscall_dispatcher_prolog_end") ":\n\t"
                   "movq %rax,0x00(%rcx)\n\t"
                   "movq %rbx,0x08(%rcx)\n\t"
                   "movq %rdx,0x18(%rcx)\n\t"
                   "movq %rsi,0x20(%rcx)\n\t"
                   "movq %rdi,0x28(%rcx)\n\t"
                   "movq %r12,0x50(%rcx)\n\t"
                   "movq %r13,0x58(%rcx)\n\t"
                   "movq %r14,0x60(%rcx)\n\t"
                   "movq %r15,0x68(%rcx)\n\t"
                   "movw %cs,0x78(%rcx)\n\t"
                   "movw %ds,0x7a(%rcx)\n\t"
                   "movw %es,0x7c(%rcx)\n\t"
                   "movw %fs,0x7e(%rcx)\n\t"
                   "movq %rsp,0x88(%rcx)\n\t"
                   "movw %ss,0x90(%rcx)\n\t"
                   "movw %gs,0x92(%rcx)\n\t"
                   "movq %rbp,0x98(%rcx)\n\t"
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
                   "xsavec64 0xc0(%rcx)\n\t"
                   "jmp 3f\n"
                   "1:\txsave64 0xc0(%rcx)\n\t"
                   "jmp 3f\n"
                   "2:\tfxsave64 0xc0(%rcx)\n"
                   "3:\tleaq 0x98(%rcx),%rbp\n\t"
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
                   "leaq -0x98(%rbp),%rcx\n"
                   "2:\tmovl 0x94(%rcx),%edx\n\t"  /* frame->restore_flags */
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
                   "movq 0x68(%rcx),%r15\n\t"
                   "movq 0x60(%rcx),%r14\n\t"
                   "movq 0x58(%rcx),%r13\n\t"
                   "movq 0x50(%rcx),%r12\n\t"
                   "movq 0x28(%rcx),%rdi\n\t"
                   "movq 0x20(%rcx),%rsi\n\t"
                   "movq 0x08(%rcx),%rbx\n\t"
                   "testl $0x3,%edx\n\t"           /* CONTEXT_CONTROL | CONTEXT_INTEGER */
                   "jnz 1f\n\t"
                   "movq 0x88(%rcx),%rsp\n\t"
                   "movq 0x70(%rcx),%rcx\n\t"      /* frame->rip */
                   "jmpq *%rcx\n\t"
                   "1:\tleaq 0x70(%rcx),%rsp\n\t"
                   "testl $0x2,%edx\n\t"           /* CONTEXT_INTEGER */
                   "jnz 1f\n\t"
                   "movq (%rsp),%rcx\n\t"          /* frame->rip */
                   "iretq\n"
                   "1:\tmovq 0x00(%rcx),%rax\n\t"
                   "movq 0x18(%rcx),%rdx\n\t"
                   "movq 0x30(%rcx),%r8\n\t"
                   "movq 0x38(%rcx),%r9\n\t"
                   "movq 0x40(%rcx),%r10\n\t"
                   "movq 0x48(%rcx),%r11\n\t"
                   "movq 0x10(%rcx),%rcx\n"
                   "iretq\n"
                   "5:\tmovl $0xc000000d,%edx\n\t" /* STATUS_INVALID_PARAMETER */
                   "movq %rsp,%rcx\n"
                   __ASM_NAME("__wine_syscall_dispatcher_return") ":\n\t"
                   "movl 0xb0(%rcx),%r14d\n\t"     /* frame->syscall_flags */
                   "movq %rdx,%rax\n\t"
                   "jmp 2b" )


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
