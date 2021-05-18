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

#if 0
#pragma makedep unix
#endif

#ifdef __i386__

#include "config.h"
#include "wine/port.h"

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <assert.h>
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "ddk/wdm.h"
#include "wine/asm.h"
#include "wine/exception.h"
#include "unix_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);

#undef ERR  /* Solaris needs to define this */

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

#ifndef FP_XSTATE_MAGIC1
#define FP_XSTATE_MAGIC1 0x46505853
#endif

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
#define FPUX_sig(context)    (FPU_sig(context) && !((context)->uc_mcontext.fpregs->status >> 16) ? (XSAVE_FORMAT *)(FPU_sig(context) + 1) : NULL)
#define XState_sig(fpu)      (((unsigned int *)fpu->Reserved4)[12] == FP_XSTATE_MAGIC1 ? (XSTATE *)(fpu + 1) : NULL)

#ifdef __ANDROID__
/* custom signal restorer since we may have unmapped the one in vdso, and bionic doesn't check for that */
void rt_sigreturn(void);
__ASM_GLOBAL_FUNC( rt_sigreturn,
                   "movl $173,%eax\n\t"  /* NR_rt_sigreturn */
                   "int $0x80" );
#endif

struct modify_ldt_s
{
    unsigned int  entry_number;
    void         *base_addr;
    unsigned int  limit;
    unsigned int  seg_32bit : 1;
    unsigned int  contents : 2;
    unsigned int  read_exec_only : 1;
    unsigned int  limit_in_pages : 1;
    unsigned int  seg_not_present : 1;
    unsigned int  usable : 1;
    unsigned int  garbage : 25;
};

static inline int modify_ldt( int func, struct modify_ldt_s *ptr, unsigned long count )
{
    return syscall( 123 /* SYS_modify_ldt */, func, ptr, count );
}

static inline int set_thread_area( struct modify_ldt_s *ptr )
{
    return syscall( 243 /* SYS_set_thread_area */, ptr );
}

#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)

#include <machine/trap.h>
#include <machine/segments.h>
#include <machine/sysarch.h>

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
#define XState_sig(context)  NULL  /* FIXME */

#elif defined (__OpenBSD__)

#include <machine/segments.h>
#include <machine/sysarch.h>

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
#define XState_sig(context)  NULL  /* FIXME */

#define T_MCHK T_MACHK
#define T_XMMFLT T_XFTRAP

#elif defined(__svr4__) || defined(_SCO_DS) || defined(__sun)

#if defined(_SCO_DS) || defined(__sun)
#include <sys/regset.h>
#endif

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
#define ERROR_sig(context)   ((context)->uc_mcontext.gregs[ERR])
#define TRAP_sig(context)    ((context)->uc_mcontext.gregs[TRAPNO])

#define FPU_sig(context)     NULL  /* FIXME */
#define FPUX_sig(context)    NULL  /* FIXME */
#define XState_sig(context)  NULL  /* FIXME */

#elif defined (__APPLE__)

#include <i386/user_ldt.h>

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
#define FPUX_sig(context)    ((XSAVE_FORMAT *)&(context)->uc_mcontext->__fs.__fpu_fcw)
#define XState_sig(context)  NULL  /* FIXME */
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
#define FPUX_sig(context)    ((XSAVE_FORMAT *)&(context)->uc_mcontext->fs.fpu_fcw)
#define XState_sig(context)  NULL  /* FIXME */
#endif

#elif defined(__NetBSD__)

#include <machine/segments.h>
#include <machine/sysarch.h>

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
#define FPUX_sig(context)    ((XSAVE_FORMAT *)&((context)->uc_mcontext.__fpregs))
#define XState_sig(context)  NULL  /* FIXME */

#define T_MCHK T_MCA
#define T_XMMFLT T_XMM

#elif defined(__GNU__)

#include <mach/i386/mach_i386.h>
#include <mach/mach_traps.h>

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
#define XState_sig(context)  NULL  /* FIXME */

#else
#error You must define the signal context functions for your platform
#endif /* linux */

static ULONG first_ldt_entry = 32;

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
    TRAP_x86_CACHEFLT   = 19   /* SIMD exception (via SIGFPE) if CPU is SSE capable
                                  otherwise Cache flush exception (via SIGSEV) */
#endif
};

struct syscall_xsave
{
    union
    {
        XSAVE_FORMAT       xsave;
        FLOATING_SAVE_AREA fsave;
    } u;
    struct
    {
        ULONG64 mask;
        ULONG64 compaction_mask;
        ULONG64 reserved[6];
        M128A   ymm_high[8];
    } xstate;
};

C_ASSERT( sizeof(struct syscall_xsave) == 0x2c0 );

struct syscall_frame
{
    DWORD              eflags;  /* 00 */
    DWORD              eip;     /* 04 */
    DWORD              esp;     /* 08 */
    WORD               cs;      /* 0c */
    WORD               ss;      /* 0e */
    WORD               ds;      /* 10 */
    WORD               es;      /* 12 */
    WORD               fs;      /* 14 */
    WORD               gs;      /* 16 */
    DWORD              eax;     /* 18 */
    DWORD              ebx;     /* 1c */
    DWORD              ecx;     /* 20 */
    DWORD              edx;     /* 24 */
    DWORD              edi;     /* 28 */
    DWORD              esi;     /* 2c */
    DWORD              ebp;     /* 30 */
};

C_ASSERT( sizeof(struct syscall_frame) == 0x34 );

struct x86_thread_data
{
    DWORD              fs;            /* 1d4 TEB selector */
    DWORD              gs;            /* 1d8 libc selector; update winebuild if you move this! */
    DWORD              dr0;           /* 1dc debug registers */
    DWORD              dr1;           /* 1e0 */
    DWORD              dr2;           /* 1e4 */
    DWORD              dr3;           /* 1e8 */
    DWORD              dr6;           /* 1ec */
    DWORD              dr7;           /* 1f0 */
    void              *exit_frame;    /* 1f4 exit frame pointer */
    struct syscall_frame *syscall_frame; /* 1f8 frame pointer on syscall entry */
};

C_ASSERT( sizeof(struct x86_thread_data) <= sizeof(((struct ntdll_thread_data *)0)->cpu_data) );
C_ASSERT( offsetof( TEB, GdiTebBatch ) + offsetof( struct x86_thread_data, gs ) == 0x1d8 );
C_ASSERT( offsetof( TEB, GdiTebBatch ) + offsetof( struct x86_thread_data, exit_frame ) == 0x1f4 );
C_ASSERT( offsetof( TEB, GdiTebBatch ) + offsetof( struct x86_thread_data, syscall_frame ) == 0x1f8 );

static void *syscall_dispatcher;

static inline struct x86_thread_data *x86_thread_data(void)
{
    return (struct x86_thread_data *)ntdll_get_thread_data()->cpu_data;
}

void *get_syscall_frame(void)
{
    return x86_thread_data()->syscall_frame;
}

void set_syscall_frame(void *frame)
{
    x86_thread_data()->syscall_frame = frame;
}

static struct syscall_xsave *get_syscall_xsave( struct syscall_frame *frame )
{
    return (struct syscall_xsave *)((ULONG_PTR)((struct syscall_xsave *)frame - 1) & ~63);
}

static inline WORD get_cs(void) { WORD res; __asm__( "movw %%cs,%0" : "=r" (res) ); return res; }
static inline WORD get_ds(void) { WORD res; __asm__( "movw %%ds,%0" : "=r" (res) ); return res; }
static inline WORD get_fs(void) { WORD res; __asm__( "movw %%fs,%0" : "=r" (res) ); return res; }
static inline WORD get_gs(void) { WORD res; __asm__( "movw %%gs,%0" : "=r" (res) ); return res; }
static inline void set_fs( WORD val ) { __asm__( "mov %0,%%fs" :: "r" (val)); }
static inline void set_gs( WORD val ) { __asm__( "mov %0,%%gs" :: "r" (val)); }


/***********************************************************************
 *           unwind_builtin_dll
 */
NTSTATUS CDECL unwind_builtin_dll( ULONG type, struct _DISPATCHER_CONTEXT *dispatch, CONTEXT *context )
{
    return STATUS_UNSUCCESSFUL;
}


/***********************************************************************
 *           is_gdt_sel
 */
static inline int is_gdt_sel( WORD sel )
{
    return !(sel & 4);
}


/***********************************************************************
 *           ldt_is_system
 */
static inline int ldt_is_system( WORD sel )
{
    return is_gdt_sel( sel ) || ((sel >> 3) < first_ldt_entry);
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
    return (TEB *)((esp & ~signal_stack_mask) + teb_offset);
}


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
    struct x86_thread_data *thread_data;

    __asm__ __volatile__("mov %ss,%ax; mov %ax,%ds; mov %ax,%es");

    thread_data = (struct x86_thread_data *)get_current_teb()->GdiTebBatch;
    set_fs( thread_data->fs );
    set_gs( thread_data->gs );

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
static inline void *init_handler( const ucontext_t *sigcontext )
{
    TEB *teb = get_current_teb();

    clear_alignment_flag();

#ifndef __sun  /* see above for Solaris handling */
    {
        struct x86_thread_data *thread_data = (struct x86_thread_data *)&teb->GdiTebBatch;
        set_fs( thread_data->fs );
        set_gs( thread_data->gs );
    }
#endif

    if (!ldt_is_system(CS_sig(sigcontext)) || !ldt_is_system(SS_sig(sigcontext)))  /* 16-bit mode */
    {
        /*
         * Win16 or DOS protected mode. Note that during switch
         * from 16-bit mode to linear mode, CS may be set to system
         * segment before FS is restored. Fortunately, in this case
         * SS is still non-system segment. This is why both CS and SS
         * are checked.
         */
        return teb->SystemReserved1[0];
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
    struct
    {
        DWORD ControlWord;
        DWORD StatusWord;
        DWORD TagWord;
        DWORD ErrorOffset;
        DWORD ErrorSelector;
        DWORD DataOffset;
        DWORD DataSelector;
    }
    float_status;

    context->ContextFlags |= CONTEXT_FLOATING_POINT;
    __asm__ __volatile__( "fnsave %0; fwait" : "=m" (context->FloatSave) );

    /* Reset unmasked exceptions status to avoid firing an exception. */
    memcpy(&float_status, &context->FloatSave, sizeof(float_status));
    float_status.StatusWord &= float_status.ControlWord | 0xffffff80;

    __asm__ __volatile__( "fldenv %0" : : "m" (float_status) );
}


/***********************************************************************
 *           restore_fpu
 *
 * Restore the x87 FPU context
 */
static inline void restore_fpu( const CONTEXT *context )
{
    FLOATING_SAVE_AREA float_status = context->FloatSave;
    /* reset the current interrupt status */
    float_status.StatusWord &= float_status.ControlWord | 0xffffff80;
    __asm__ __volatile__( "frstor %0; fwait" : : "m" (float_status) );
}


/***********************************************************************
 *           fpux_to_fpu
 *
 * Build a standard FPU context from an extended one.
 */
static void fpux_to_fpu( FLOATING_SAVE_AREA *fpu, const XSAVE_FORMAT *fpux )
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
 *           fpu_to_fpux
 *
 * Fill extended FPU context from standard one.
 */
static void fpu_to_fpux( XSAVE_FORMAT *fpux, const FLOATING_SAVE_AREA *fpu )
{
    unsigned int i;

    fpux->ControlWord   = fpu->ControlWord;
    fpux->StatusWord    = fpu->StatusWord;
    fpux->ErrorOffset   = fpu->ErrorOffset;
    fpux->ErrorSelector = fpu->ErrorSelector;
    fpux->ErrorOpcode   = fpu->ErrorSelector >> 16;
    fpux->DataOffset    = fpu->DataOffset;
    fpux->DataSelector  = fpu->DataSelector;
    fpux->TagWord       = 0;
    for (i = 0; i < 8; i++)
    {
        if (((fpu->TagWord >> (i * 2)) & 3) != 3)
            fpux->TagWord |= 1 << i;
        memcpy( &fpux->FloatRegisters[i], &fpu->RegisterArea[10 * i], 10 );
    }
}


/***********************************************************************
 *           save_context
 *
 * Build a context structure from the signal info.
 */
static inline void save_context( struct xcontext *xcontext, const ucontext_t *sigcontext )
{
    FLOATING_SAVE_AREA *fpu = FPU_sig(sigcontext);
    XSAVE_FORMAT *fpux = FPUX_sig(sigcontext);
    CONTEXT *context = &xcontext->c;

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
    context->SegFs        = LOWORD(FS_sig(sigcontext));
    context->SegGs        = LOWORD(GS_sig(sigcontext));
    context->SegSs        = LOWORD(SS_sig(sigcontext));
    context->Dr0          = x86_thread_data()->dr0;
    context->Dr1          = x86_thread_data()->dr1;
    context->Dr2          = x86_thread_data()->dr2;
    context->Dr3          = x86_thread_data()->dr3;
    context->Dr6          = x86_thread_data()->dr6;
    context->Dr7          = x86_thread_data()->dr7;

    if (fpu)
    {
        context->ContextFlags |= CONTEXT_FLOATING_POINT;
        context->FloatSave = *fpu;
    }
    if (fpux)
    {
        XSTATE *xs;

        context->ContextFlags |= CONTEXT_FLOATING_POINT | CONTEXT_EXTENDED_REGISTERS;
        memcpy( context->ExtendedRegisters, fpux, sizeof(*fpux) );
        if (!fpu) fpux_to_fpu( &context->FloatSave, fpux );
        if ((cpu_info.ProcessorFeatureBits & CPU_FEATURE_AVX) && (xs = XState_sig(fpux)))
        {
            context_init_xstate( context, xs );
            xcontext->host_compaction_mask = xs->CompactionMask;
        }
    }
    if (!fpu && !fpux) save_fpu( context );
}


/***********************************************************************
 *           restore_context
 *
 * Restore the signal info from the context.
 */
static inline void restore_context( const struct xcontext *xcontext, ucontext_t *sigcontext )
{
    FLOATING_SAVE_AREA *fpu = FPU_sig(sigcontext);
    XSAVE_FORMAT *fpux = FPUX_sig(sigcontext);
    const CONTEXT *context = &xcontext->c;

    x86_thread_data()->dr0 = context->Dr0;
    x86_thread_data()->dr1 = context->Dr1;
    x86_thread_data()->dr2 = context->Dr2;
    x86_thread_data()->dr3 = context->Dr3;
    x86_thread_data()->dr6 = context->Dr6;
    x86_thread_data()->dr7 = context->Dr7;
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
    FS_sig(sigcontext)  = context->SegFs;
    GS_sig(sigcontext)  = context->SegGs;
    SS_sig(sigcontext)  = context->SegSs;

    if (fpu) *fpu = context->FloatSave;
    if (fpux)
    {
        XSTATE *src_xs, *dst_xs;

        memcpy( fpux, context->ExtendedRegisters, sizeof(*fpux) );

        if ((dst_xs = XState_sig(fpux)) && (src_xs = xstate_from_context( context )))
        {
            memcpy( &dst_xs->YmmContext, &src_xs->YmmContext, sizeof(dst_xs->YmmContext) );
            dst_xs->Mask |= src_xs->Mask;
            dst_xs->CompactionMask = xcontext->host_compaction_mask;
        }
    }
    if (!fpu && !fpux) restore_fpu( context );
}


/***********************************************************************
 *           set_full_cpu_context
 *
 * Set the new CPU context.
 */
extern void set_full_cpu_context(void);
__ASM_GLOBAL_FUNC( set_full_cpu_context,
                   "movl %fs:0x1f8,%ecx\n\t"
                   "movl $0,%fs:0x1f8\n\t"    /* x86_thread_data()->syscall_frame = NULL */
                   "movw 0x16(%ecx),%gs\n\t"  /* SegGs */
                   "movw 0x14(%ecx),%fs\n\t"  /* SegFs */
                   "movw 0x12(%ecx),%es\n\t"  /* SegEs */
                   "movl 0x1c(%ecx),%ebx\n\t" /* Ebx */
                   "movl 0x28(%ecx),%edi\n\t" /* Edi */
                   "movl 0x2c(%ecx),%esi\n\t" /* Esi */
                   "movl 0x30(%ecx),%ebp\n\t" /* Ebp */
                   "movw %ss,%ax\n\t"
                   "cmpw 0x0e(%ecx),%ax\n\t"  /* SegSs */
                   "jne 1f\n\t"
                   /* As soon as we have switched stacks the context structure could
                    * be invalid (when signal handlers are executed for example). Copy
                    * values on the target stack before changing ESP. */
                   "movl 0x08(%ecx),%eax\n\t" /* Esp */
                   "leal -4*4(%eax),%eax\n\t"
                   "movl (%ecx),%edx\n\t"     /* EFlags */
                   "movl %edx,3*4(%eax)\n\t"
                   "movl 0x0c(%ecx),%edx\n\t" /* SegCs */
                   "movl %edx,2*4(%eax)\n\t"
                   "movl 0x04(%ecx),%edx\n\t" /* Eip */
                   "movl %edx,1*4(%eax)\n\t"
                   "movl 0x18(%ecx),%edx\n\t" /* Eax */
                   "movl %edx,0*4(%eax)\n\t"
                   "pushl 0x10(%ecx)\n\t"     /* SegDs */
                   "movl 0x24(%ecx),%edx\n\t" /* Edx */
                   "movl 0x20(%ecx),%ecx\n\t" /* Ecx */
                   "popl %ds\n\t"
                   "movl %eax,%esp\n\t"
                   "popl %eax\n\t"
                   "iret\n"
                   /* Restore the context when the stack segment changes. We can't use
                    * the same code as above because we do not know if the stack segment
                    * is 16 or 32 bit, and 'movl' will throw an exception when we try to
                    * access memory above the limit. */
                   "1:\n\t"
                   "movl 0x24(%ecx),%edx\n\t" /* Edx */
                   "movl 0x18(%ecx),%eax\n\t" /* Eax */
                   "movw 0x0e(%ecx),%ss\n\t"  /* SegSs */
                   "movl 0x08(%ecx),%esp\n\t" /* Esp */
                   "pushl 0x00(%ecx)\n\t"     /* EFlags */
                   "pushl 0x0c(%ecx)\n\t"     /* SegCs */
                   "pushl 0x04(%ecx)\n\t"     /* Eip */
                   "pushl 0x10(%ecx)\n\t"     /* SegDs */
                   "movl 0x20(%ecx),%ecx\n\t" /* Ecx */
                   "popl %ds\n\t"
                   "iret" )


/***********************************************************************
 *           signal_restore_full_cpu_context
 *
 * Restore full context from syscall frame
 */
void signal_restore_full_cpu_context(void)
{
    struct syscall_xsave *xsave = get_syscall_xsave( get_syscall_frame() );

    if (cpu_info.ProcessorFeatureBits & CPU_FEATURE_XSAVE)
    {
        __asm__ volatile( "xrstor %0" : : "m"(*xsave), "a" (7), "d" (0) );
    }
    else if (cpu_info.ProcessorFeatureBits & CPU_FEATURE_FXSR)
    {
        __asm__ volatile( "fxrstor %0" : : "m"(xsave->u.xsave) );
    }
    else
    {
        __asm__ volatile( "frstor %0; fwait" : : "m" (xsave->u.fsave) );
    }
    set_full_cpu_context();
}


/***********************************************************************
 *              NtSetContextThread  (NTDLL.@)
 *              ZwSetContextThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetContextThread( HANDLE handle, const CONTEXT *context )
{
    NTSTATUS ret = STATUS_SUCCESS;
    struct syscall_frame *frame = x86_thread_data()->syscall_frame;
    DWORD flags = context->ContextFlags & ~CONTEXT_i386;
    BOOL self = (handle == GetCurrentThread());
    XSTATE *xs;

    /* debug registers require a server call */
    if (self && (flags & CONTEXT_DEBUG_REGISTERS))
        self = (x86_thread_data()->dr0 == context->Dr0 &&
                x86_thread_data()->dr1 == context->Dr1 &&
                x86_thread_data()->dr2 == context->Dr2 &&
                x86_thread_data()->dr3 == context->Dr3 &&
                x86_thread_data()->dr6 == context->Dr6 &&
                x86_thread_data()->dr7 == context->Dr7);

    if (!self)
    {
        ret = set_thread_context( handle, context, &self, IMAGE_FILE_MACHINE_I386 );
        if (ret || !self) return ret;
        if (flags & CONTEXT_DEBUG_REGISTERS)
        {
            x86_thread_data()->dr0 = context->Dr0;
            x86_thread_data()->dr1 = context->Dr1;
            x86_thread_data()->dr2 = context->Dr2;
            x86_thread_data()->dr3 = context->Dr3;
            x86_thread_data()->dr6 = context->Dr6;
            x86_thread_data()->dr7 = context->Dr7;
        }
    }

    if (flags & CONTEXT_INTEGER)
    {
        frame->eax = context->Eax;
        frame->ebx = context->Ebx;
        frame->ecx = context->Ecx;
        frame->edx = context->Edx;
        frame->esi = context->Esi;
        frame->edi = context->Edi;
    }
    if (flags & CONTEXT_CONTROL)
    {
        frame->esp    = context->Esp;
        frame->ebp    = context->Ebp;
        frame->eip    = context->Eip;
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
    if (flags & CONTEXT_EXTENDED_REGISTERS)
    {
        struct syscall_xsave *xsave = get_syscall_xsave( frame );
        memcpy( &xsave->u.xsave, context->ExtendedRegisters, sizeof(xsave->u.xsave) );
        /* reset the current interrupt status */
        xsave->u.xsave.StatusWord &= xsave->u.xsave.ControlWord | 0xff80;
        xsave->xstate.mask |= XSTATE_MASK_LEGACY;
    }
    else if (flags & CONTEXT_FLOATING_POINT)
    {
        struct syscall_xsave *xsave = get_syscall_xsave( frame );
        if (cpu_info.ProcessorFeatureBits & CPU_FEATURE_FXSR)
        {
            fpu_to_fpux( &xsave->u.xsave, &context->FloatSave );
        }
        else
        {
            xsave->u.fsave = context->FloatSave;
        }
        xsave->xstate.mask |= XSTATE_MASK_LEGACY_FLOATING_POINT;
    }
    if ((cpu_info.ProcessorFeatureBits & CPU_FEATURE_AVX) && (xs = xstate_from_context( context )))
    {
        struct syscall_xsave *xsave = get_syscall_xsave( frame );
        CONTEXT_EX *context_ex = (CONTEXT_EX *)(context + 1);

        if (context_ex->XState.Length < offsetof(XSTATE, YmmContext)
            || context_ex->XState.Length > sizeof(XSTATE))
            return STATUS_INVALID_PARAMETER;

        if (xs->Mask & XSTATE_MASK_GSSE)
        {
            if (context_ex->XState.Length < sizeof(XSTATE))
                return STATUS_BUFFER_OVERFLOW;

            xsave->xstate.mask |= XSTATE_MASK_GSSE;
            memcpy( &xsave->xstate.ymm_high, &xs->YmmContext, sizeof(xsave->xstate.ymm_high) );
        }
        else
            xsave->xstate.mask &= ~XSTATE_MASK_GSSE;
    }

    return STATUS_SUCCESS;
}


/***********************************************************************
 *              NtGetContextThread  (NTDLL.@)
 *              ZwGetContextThread  (NTDLL.@)
 *
 * Note: we use a small assembly wrapper to save the necessary registers
 *       in case we are fetching the context of the current thread.
 */
NTSTATUS WINAPI NtGetContextThread( HANDLE handle, CONTEXT *context )
{
    struct syscall_frame *frame = x86_thread_data()->syscall_frame;
    DWORD needed_flags = context->ContextFlags & ~CONTEXT_i386;
    BOOL self = (handle == GetCurrentThread());
    NTSTATUS ret;

    /* debug registers require a server call */
    if (needed_flags & CONTEXT_DEBUG_REGISTERS) self = FALSE;

    if (!self)
    {
        if ((ret = get_thread_context( handle, context, &self, IMAGE_FILE_MACHINE_I386 ))) return ret;
        needed_flags &= ~context->ContextFlags;
    }

    if (self)
    {
        struct syscall_xsave *xsave = get_syscall_xsave( frame );
        XSTATE *xstate;
        if (needed_flags & CONTEXT_INTEGER)
        {
            context->Eax = frame->eax;
            context->Ebx = frame->ebx;
            context->Ecx = frame->ecx;
            context->Edx = frame->edx;
            context->Esi = frame->esi;
            context->Edi = frame->edi;
            context->ContextFlags |= CONTEXT_INTEGER;
        }
        if (needed_flags & CONTEXT_CONTROL)
        {
            context->Esp    = frame->esp;
            context->Ebp    = frame->ebp;
            context->Eip    = frame->eip;
            context->EFlags = frame->eflags;
            context->SegCs  = frame->cs;
            context->SegSs  = frame->ss;
            context->ContextFlags |= CONTEXT_CONTROL;
        }
        if (needed_flags & CONTEXT_SEGMENTS)
        {
            context->SegDs = frame->ds;
            context->SegEs = frame->es;
            context->SegFs = frame->fs;
            context->SegGs = frame->gs;
            context->ContextFlags |= CONTEXT_SEGMENTS;
        }
        if (needed_flags & CONTEXT_FLOATING_POINT)
        {
            if (!(cpu_info.ProcessorFeatureBits & CPU_FEATURE_FXSR))
            {
                context->FloatSave = xsave->u.fsave;
            }
            else if (!xstate_compaction_enabled ||
                     (xsave->xstate.mask & XSTATE_MASK_LEGACY_FLOATING_POINT))
            {
                fpux_to_fpu( &context->FloatSave, &xsave->u.xsave );
            }
            else
            {
                memset( &context->FloatSave, 0, sizeof(context->FloatSave) );
                context->FloatSave.ControlWord = 0x37f;
            }
            context->ContextFlags |= CONTEXT_FLOATING_POINT;
        }
        if (needed_flags & CONTEXT_EXTENDED_REGISTERS)
        {
            XSAVE_FORMAT *xs = (XSAVE_FORMAT *)context->ExtendedRegisters;

            if (!xstate_compaction_enabled ||
                (xsave->xstate.mask & XSTATE_MASK_LEGACY_FLOATING_POINT))
            {
                memcpy( xs, &xsave->u.xsave, FIELD_OFFSET( XSAVE_FORMAT, MxCsr ));
                memcpy( xs->FloatRegisters, xsave->u.xsave.FloatRegisters,
                        sizeof( xs->FloatRegisters ));
            }
            else
            {
                memset( xs, 0, FIELD_OFFSET( XSAVE_FORMAT, MxCsr ));
                memset( xs->FloatRegisters, 0, sizeof( xs->FloatRegisters ));
                xs->ControlWord = 0x37f;
            }

            if (!xstate_compaction_enabled || (xsave->xstate.mask & XSTATE_MASK_LEGACY_SSE))
            {
                memcpy( xs->XmmRegisters, xsave->u.xsave.XmmRegisters, sizeof( xs->XmmRegisters ));
                xs->MxCsr      = xsave->u.xsave.MxCsr;
                xs->MxCsr_Mask = xsave->u.xsave.MxCsr_Mask;
            }
            else
            {
                memset( xs->XmmRegisters, 0, sizeof( xs->XmmRegisters ));
                xs->MxCsr      = 0x1f80;
                xs->MxCsr_Mask = 0x2ffff;
            }

            context->ContextFlags |= CONTEXT_EXTENDED_REGISTERS;
        }
        /* update the cached version of the debug registers */
        if (context->ContextFlags & (CONTEXT_DEBUG_REGISTERS & ~CONTEXT_i386))
        {
            x86_thread_data()->dr0 = context->Dr0;
            x86_thread_data()->dr1 = context->Dr1;
            x86_thread_data()->dr2 = context->Dr2;
            x86_thread_data()->dr3 = context->Dr3;
            x86_thread_data()->dr6 = context->Dr6;
            x86_thread_data()->dr7 = context->Dr7;
        }
        if ((cpu_info.ProcessorFeatureBits & CPU_FEATURE_AVX) && (xstate = xstate_from_context( context )))
        {
            struct syscall_xsave *xsave = get_syscall_xsave( frame );
            CONTEXT_EX *context_ex = (CONTEXT_EX *)(context + 1);
            unsigned int mask;

            if (context_ex->XState.Length < offsetof(XSTATE, YmmContext)
                || context_ex->XState.Length > sizeof(XSTATE))
                return STATUS_INVALID_PARAMETER;

            mask = (xstate_compaction_enabled ? xstate->CompactionMask : xstate->Mask) & XSTATE_MASK_GSSE;
            xstate->Mask = xsave->xstate.mask & mask;
            xstate->CompactionMask = xstate_compaction_enabled ? (0x8000000000000000 | mask) : 0;
            memset( xstate->Reserved, 0, sizeof(xstate->Reserved) );
            if (xstate->Mask)
            {
                if (context_ex->XState.Length < sizeof(XSTATE))
                    return STATUS_BUFFER_OVERFLOW;

                memcpy( &xstate->YmmContext, xsave->xstate.ymm_high, sizeof(xsave->xstate.ymm_high) );
            }
        }
    }

    if (context->ContextFlags & (CONTEXT_INTEGER & ~CONTEXT_i386))
        TRACE( "%p: eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x\n", handle,
               context->Eax, context->Ebx, context->Ecx, context->Edx, context->Esi, context->Edi );
    if (context->ContextFlags & (CONTEXT_CONTROL & ~CONTEXT_i386))
        TRACE( "%p: ebp=%08x esp=%08x eip=%08x cs=%04x ss=%04x flags=%08x\n", handle,
               context->Ebp, context->Esp, context->Eip, context->SegCs, context->SegSs, context->EFlags );
    if (context->ContextFlags & (CONTEXT_SEGMENTS & ~CONTEXT_i386))
        TRACE( "%p: ds=%04x es=%04x fs=%04x gs=%04x\n", handle,
               context->SegDs, context->SegEs, context->SegFs, context->SegGs );
    if (context->ContextFlags & (CONTEXT_DEBUG_REGISTERS & ~CONTEXT_i386))
        TRACE( "%p: dr0=%08x dr1=%08x dr2=%08x dr3=%08x dr6=%08x dr7=%08x\n", handle,
               context->Dr0, context->Dr1, context->Dr2, context->Dr3, context->Dr6, context->Dr7 );

    return STATUS_SUCCESS;
}


/***********************************************************************
 *              set_thread_wow64_context
 */
NTSTATUS set_thread_wow64_context( HANDLE handle, const void *ctx, ULONG size )
{
    return STATUS_INVALID_INFO_CLASS;
}


/***********************************************************************
 *              get_thread_wow64_context
 */
NTSTATUS get_thread_wow64_context( HANDLE handle, void *ctx, ULONG size )
{
    return STATUS_INVALID_INFO_CLASS;
}


/***********************************************************************
 *           is_privileged_instr
 *
 * Check if the fault location is a privileged instruction.
 * Based on the instruction emulation code in dlls/kernel/instr.c.
 */
static inline DWORD is_privileged_instr( CONTEXT *context )
{
    BYTE instr[16];
    unsigned int i, len, prefix_count = 0;

    if (!ldt_is_system( context->SegCs )) return 0;
    len = virtual_uninterrupted_read_memory( (BYTE *)context->Eip, instr, sizeof(instr) );

    for (i = 0; i < len; i++) switch (instr[i])
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
        continue;

    case 0x0f: /* extended instruction */
        if (i == len - 1) return 0;
        switch(instr[i + 1])
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
    return 0;
}


/***********************************************************************
 *           check_invalid_gs
 *
 * Check for fault caused by invalid %gs value (some copy protection schemes mess with it).
 */
static inline BOOL check_invalid_gs( ucontext_t *sigcontext, CONTEXT *context )
{
    unsigned int prefix_count = 0;
    const BYTE *instr = (BYTE *)context->Eip;
    WORD system_gs = x86_thread_data()->gs;

    if (context->SegGs == system_gs) return FALSE;
    if (!ldt_is_system( context->SegCs )) return FALSE;
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
        GS_sig(sigcontext) = system_gs;
        return TRUE;
    default:
        return FALSE;
    }
}


#include "pshpack1.h"
union atl_thunk
{
    struct
    {
        DWORD movl;  /* movl this,4(%esp) */
        DWORD this;
        BYTE  jmp;   /* jmp func */
        int   func;
    } t1;
    struct
    {
        BYTE  movl;  /* movl this,ecx */
        DWORD this;
        BYTE  jmp;   /* jmp func */
        int   func;
    } t2;
    struct
    {
        BYTE  movl1; /* movl this,edx */
        DWORD this;
        BYTE  movl2; /* movl func,ecx */
        DWORD func;
        WORD  jmp;   /* jmp ecx */
    } t3;
    struct
    {
        BYTE  movl1; /* movl this,ecx */
        DWORD this;
        BYTE  movl2; /* movl func,eax */
        DWORD func;
        WORD  jmp;   /* jmp eax */
    } t4;
    struct
    {
        DWORD inst1; /* pop ecx
                      * pop eax
                      * push ecx
                      * jmp 4(%eax) */
        WORD  inst2;
    } t5;
};
#include "poppack.h"

/**********************************************************************
 *		check_atl_thunk
 *
 * Check if code destination is an ATL thunk, and emulate it if so.
 */
static BOOL check_atl_thunk( ucontext_t *sigcontext, EXCEPTION_RECORD *rec, CONTEXT *context )
{
    const union atl_thunk *thunk = (const union atl_thunk *)rec->ExceptionInformation[1];
    union atl_thunk thunk_copy;
    SIZE_T thunk_len;

    thunk_len = virtual_uninterrupted_read_memory( thunk, &thunk_copy, sizeof(*thunk) );
    if (!thunk_len) return FALSE;

    if (thunk_len >= sizeof(thunk_copy.t1) && thunk_copy.t1.movl == 0x042444c7 &&
                                              thunk_copy.t1.jmp == 0xe9)
    {
        if (!virtual_uninterrupted_write_memory( (DWORD *)context->Esp + 1,
                                                 &thunk_copy.t1.this, sizeof(DWORD) ))
        {
            EIP_sig(sigcontext) = (DWORD_PTR)(&thunk->t1.func + 1) + thunk_copy.t1.func;
            TRACE( "emulating ATL thunk type 1 at %p, func=%08x arg=%08x\n",
                   thunk, EIP_sig(sigcontext), thunk_copy.t1.this );
            return TRUE;
        }
    }
    else if (thunk_len >= sizeof(thunk_copy.t2) && thunk_copy.t2.movl == 0xb9 &&
                                                   thunk_copy.t2.jmp == 0xe9)
    {
        ECX_sig(sigcontext) = thunk_copy.t2.this;
        EIP_sig(sigcontext) = (DWORD_PTR)(&thunk->t2.func + 1) + thunk_copy.t2.func;
        TRACE( "emulating ATL thunk type 2 at %p, func=%08x ecx=%08x\n",
               thunk, EIP_sig(sigcontext), ECX_sig(sigcontext) );
        return TRUE;
    }
    else if (thunk_len >= sizeof(thunk_copy.t3) && thunk_copy.t3.movl1 == 0xba &&
                                                   thunk_copy.t3.movl2 == 0xb9 &&
                                                   thunk_copy.t3.jmp == 0xe1ff)
    {
        EDX_sig(sigcontext) = thunk_copy.t3.this;
        ECX_sig(sigcontext) = thunk_copy.t3.func;
        EIP_sig(sigcontext) = thunk_copy.t3.func;
        TRACE( "emulating ATL thunk type 3 at %p, func=%08x ecx=%08x edx=%08x\n",
               thunk, EIP_sig(sigcontext), ECX_sig(sigcontext), EDX_sig(sigcontext) );
        return TRUE;
    }
    else if (thunk_len >= sizeof(thunk_copy.t4) && thunk_copy.t4.movl1 == 0xb9 &&
                                                   thunk_copy.t4.movl2 == 0xb8 &&
                                                   thunk_copy.t4.jmp == 0xe0ff)
    {
        ECX_sig(sigcontext) = thunk_copy.t4.this;
        EAX_sig(sigcontext) = thunk_copy.t4.func;
        EIP_sig(sigcontext) = thunk_copy.t4.func;
        TRACE( "emulating ATL thunk type 4 at %p, func=%08x eax=%08x ecx=%08x\n",
               thunk, EIP_sig(sigcontext), EAX_sig(sigcontext), ECX_sig(sigcontext) );
        return TRUE;
    }
    else if (thunk_len >= sizeof(thunk_copy.t5) && thunk_copy.t5.inst1 == 0xff515859 &&
                                                   thunk_copy.t5.inst2 == 0x0460)
    {
        DWORD func, sp[2];
        if (virtual_uninterrupted_read_memory( (DWORD *)context->Esp, sp, sizeof(sp) ) == sizeof(sp) &&
            virtual_uninterrupted_read_memory( (DWORD *)sp[1] + 1, &func, sizeof(DWORD) ) == sizeof(DWORD) &&
            !virtual_uninterrupted_write_memory( (DWORD *)context->Esp + 1, &sp[0], sizeof(sp[0]) ))
        {
            ECX_sig(sigcontext) = sp[0];
            EAX_sig(sigcontext) = sp[1];
            ESP_sig(sigcontext) += sizeof(DWORD);
            EIP_sig(sigcontext) = func;
            TRACE( "emulating ATL thunk type 5 at %p, func=%08x eax=%08x ecx=%08x esp=%08x\n",
                   thunk, EIP_sig(sigcontext), EAX_sig(sigcontext),
                   ECX_sig(sigcontext), ESP_sig(sigcontext) );
            return TRUE;
        }
    }

    return FALSE;
}


/***********************************************************************
 *           setup_exception_record
 *
 * Setup the exception record and context on the thread stack.
 */
static void *setup_exception_record( ucontext_t *sigcontext, EXCEPTION_RECORD *rec, struct xcontext *xcontext )
{
    void *stack = init_handler( sigcontext );

    rec->ExceptionAddress = (void *)EIP_sig( sigcontext );
    save_context( xcontext, sigcontext );
    return stack;
}

/***********************************************************************
 *           setup_raise_exception
 *
 * Change context to setup a call to a raise exception function.
 */
static void setup_raise_exception( ucontext_t *sigcontext, void *stack_ptr,
                                   EXCEPTION_RECORD *rec, struct xcontext *xcontext )
{
    CONTEXT *context = &xcontext->c;
    size_t stack_size;
    XSTATE *src_xs;

    struct stack_layout
    {
        EXCEPTION_RECORD *rec_ptr;       /* first arg for KiUserExceptionDispatcher */
        CONTEXT          *context_ptr;   /* second arg for KiUserExceptionDispatcher */
        CONTEXT           context;
        CONTEXT_EX        context_ex;
        EXCEPTION_RECORD  rec;
        DWORD             ebp;
        DWORD             eip;
        char              xstate[0];
    } *stack;

C_ASSERT( (offsetof(struct stack_layout, xstate) == sizeof(struct stack_layout)) );

    NTSTATUS status = send_debug_event( rec, context, TRUE );

    if (status == DBG_CONTINUE || status == DBG_EXCEPTION_HANDLED)
    {
        restore_context( xcontext, sigcontext );
        return;
    }

    /* fix up instruction pointer in context for EXCEPTION_BREAKPOINT */
    if (rec->ExceptionCode == EXCEPTION_BREAKPOINT) context->Eip--;

    stack_size = sizeof(*stack);
    if ((src_xs = xstate_from_context( context )))
    {
        stack_size += (ULONG_PTR)stack_ptr - (((ULONG_PTR)stack_ptr
                - sizeof(XSTATE)) & ~(ULONG_PTR)63);
    }

    stack = virtual_setup_exception( stack_ptr, stack_size, rec );
    stack->rec          = *rec;
    stack->context      = *context;

    if (src_xs)
    {
        XSTATE *dst_xs = (XSTATE *)stack->xstate;

        assert(!((ULONG_PTR)dst_xs & 63));
        context_init_xstate( &stack->context, stack->xstate );
        memset( dst_xs, 0, offsetof(XSTATE, YmmContext) );
        dst_xs->CompactionMask = xstate_compaction_enabled ? 0x8000000000000004 : 0;
        if (src_xs->Mask & 4)
        {
            dst_xs->Mask = 4;
            memcpy( &dst_xs->YmmContext, &src_xs->YmmContext, sizeof(dst_xs->YmmContext) );
        }
    }

    stack->rec_ptr      = &stack->rec;
    stack->context_ptr  = &stack->context;
    ESP_sig(sigcontext) = (DWORD)stack;
    EIP_sig(sigcontext) = (DWORD)pKiUserExceptionDispatcher;
    /* clear single-step, direction, and align check flag */
    EFL_sig(sigcontext) &= ~(0x100|0x400|0x40000);
    CS_sig(sigcontext)  = get_cs();
    DS_sig(sigcontext)  = get_ds();
    ES_sig(sigcontext)  = get_ds();
    FS_sig(sigcontext)  = get_fs();
    GS_sig(sigcontext)  = get_gs();
    SS_sig(sigcontext)  = get_ds();
}


/***********************************************************************
 *           setup_exception
 *
 * Do the full setup to raise an exception from an exception record.
 */
static void setup_exception( ucontext_t *sigcontext, EXCEPTION_RECORD *rec )
{
    struct xcontext xcontext;
    void *stack = setup_exception_record( sigcontext, rec, &xcontext );

    setup_raise_exception( sigcontext, stack, rec, &xcontext );
}

/* stack layout when calling an user apc function.
 * FIXME: match Windows ABI. */
struct apc_stack_layout
{
    void         *context_ptr;
    void         *ctx;
    void         *arg1;
    void         *arg2;
    void         *func;
    CONTEXT       context;
};

struct apc_stack_layout * WINAPI setup_user_apc_dispatcher_stack( CONTEXT *context, struct apc_stack_layout *stack,
        void *ctx, void *arg1, void *arg2, void *func )
{
    CONTEXT c;

    if (!context)
    {
        c.ContextFlags = CONTEXT_FULL;
        NtGetContextThread( GetCurrentThread(), &c );
        c.Eax = STATUS_USER_APC;
        context = &c;
    }
    memmove( &stack->context, context, sizeof(stack->context) );
    stack->context_ptr = &stack->context;
    stack->ctx = ctx;
    stack->arg1 = arg1;
    stack->arg2 = arg2;
    stack->func = func;
    return stack;
}

C_ASSERT( sizeof(struct apc_stack_layout) == 0x2e0 );
C_ASSERT( offsetof(struct apc_stack_layout, context) == 20 );

/***********************************************************************
 *           call_user_apc_dispatcher
 */
__ASM_GLOBAL_FUNC( call_user_apc_dispatcher,
                   "movl 4(%esp),%esi\n\t"       /* context_ptr */
                   "movl 24(%esp),%edi\n\t"      /* dispatcher */
                   "leal 4(%esp),%ebp\n\t"
                   "test %esi,%esi\n\t"
                   "jz 1f\n\t"
                   "movl 0xc4(%esi),%eax\n\t"    /* context_ptr->Esp */
                   "jmp 2f\n\t"
                   "1:\tmovl %fs:0x1f8,%eax\n\t" /* x86_thread_data()->syscall_frame */
                   "leal 0x38(%eax),%eax\n\t"    /* &x86_thread_data()->syscall_frame->ret_addr */
                   "2:\tsubl $0x2e0,%eax\n\t"    /* sizeof(struct apc_stack_layout) */
                   "movl %ebp,%esp\n\t"          /* pop return address */
                   "cmpl %esp,%eax\n\t"
                   "cmovbl %eax,%esp\n\t"
                   "pushl 16(%ebp)\n\t"          /* func */
                   "pushl 12(%ebp)\n\t"          /* arg2 */
                   "pushl 8(%ebp)\n\t"           /* arg1 */
                   "pushl 4(%ebp)\n\t"           /* ctx */
                   "pushl %eax\n\t"
                   "pushl %esi\n\t"
                   "call " __ASM_STDCALL("setup_user_apc_dispatcher_stack",24) "\n\t"
                   "movl $0,%fs:0x1f8\n\t"       /* x86_thread_data()->syscall_frame = NULL */
                   "movl %eax,%esp\n\t"
                   "leal 20(%eax),%esi\n\t"
                   "movl 0xb4(%esi),%ebp\n\t"    /* context.Ebp */
                   "pushl 0xb8(%esi)\n\t"        /* context.Eip */
                   "jmp *%edi\n" )


/***********************************************************************
 *           call_raise_user_exception_dispatcher
 */
void WINAPI call_raise_user_exception_dispatcher( NTSTATUS (WINAPI *dispatcher)(void) )
{
    x86_thread_data()->syscall_frame->eip = (DWORD)dispatcher;
}


/***********************************************************************
 *           call_user_exception_dispatcher
 */
__ASM_GLOBAL_FUNC( call_user_exception_dispatcher,
                   "movl 4(%esp),%edx\n\t"        /* rec */
                   "movl 8(%esp),%ecx\n\t"        /* context */
                   "cmpl $0x80000003,(%edx)\n\t"  /* rec->ExceptionCode */
                   "jne 1f\n\t"
                   "decl 0xb8(%ecx)\n"            /* context->Eip */
                   "1:\tmovl %fs:0x1f8,%eax\n\t"  /* x86_thread_data()->syscall_frame */
                   "movl 0x1c(%eax),%ebx\n\t"     /* frame->ebx */
                   "movl 0x28(%eax),%edi\n\t"     /* frame->edi */
                   "movl 0x2c(%eax),%esi\n\t"     /* frame->esi */
                   "movl 0x30(%eax),%ebp\n\t"     /* frame->ebp */
                   "movl %edx,0x30(%eax)\n\t"
                   "movl %ecx,0x34(%eax)\n\t"
                   "movl 12(%esp),%edx\n\t"       /* dispatcher */
                   "movl $0,%fs:0x1f8\n\t"
                   "leal 0x30(%eax),%esp\n\t"
                   "jmp *%edx" )

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


/***********************************************************************
 *           handle_interrupt
 *
 * Handle an interrupt.
 */
static BOOL handle_interrupt( unsigned int interrupt, ucontext_t *sigcontext, void *stack,
                              EXCEPTION_RECORD *rec, struct xcontext *xcontext )
{
    CONTEXT *context = &xcontext->c;

    switch(interrupt)
    {
    case 0x2d:
        if (!is_wow64)
        {
            /* On Wow64, the upper DWORD of Rax contains garbage, and the debug
             * service is usually not recognized when called from usermode. */
            switch (context->Eax)
            {
                case 1: /* BREAKPOINT_PRINT */
                case 3: /* BREAKPOINT_LOAD_SYMBOLS */
                case 4: /* BREAKPOINT_UNLOAD_SYMBOLS */
                case 5: /* BREAKPOINT_COMMAND_STRING (>= Win2003) */
                    EIP_sig(sigcontext) += 3;
                    return TRUE;
            }
        }
        context->Eip += 3;
        rec->ExceptionCode = EXCEPTION_BREAKPOINT;
        rec->ExceptionAddress = (void *)context->Eip;
        rec->NumberParameters = is_wow64 ? 1 : 3;
        rec->ExceptionInformation[0] = context->Eax;
        rec->ExceptionInformation[1] = context->Ecx;
        rec->ExceptionInformation[2] = context->Edx;
        setup_raise_exception( sigcontext, stack, rec, xcontext );
        return TRUE;
    default:
        return FALSE;
    }
}


/***********************************************************************
 *           handle_syscall_fault
 *
 * Handle a page fault happening during a system call.
 */
static BOOL handle_syscall_fault( ucontext_t *sigcontext, void *stack_ptr,
                                  EXCEPTION_RECORD *rec, CONTEXT *context )
{
    struct syscall_frame *frame = x86_thread_data()->syscall_frame;
    __WINE_FRAME *wine_frame = (__WINE_FRAME *)NtCurrentTeb()->Tib.ExceptionList;
    DWORD i;

    if (!frame) return FALSE;

    TRACE( "code=%x flags=%x addr=%p ip=%08x tid=%04x\n",
           rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
           context->Eip, GetCurrentThreadId() );
    for (i = 0; i < rec->NumberParameters; i++)
        TRACE( " info[%d]=%08lx\n", i, rec->ExceptionInformation[i] );
    TRACE(" eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x\n",
          context->Eax, context->Ebx, context->Ecx,
          context->Edx, context->Esi, context->Edi );
    TRACE(" ebp=%08x esp=%08x cs=%04x ds=%04x es=%04x fs=%04x gs=%04x flags=%08x\n",
          context->Ebp, context->Esp, context->SegCs, context->SegDs,
          context->SegEs, context->SegFs, context->SegGs, context->EFlags );

    if ((char *)wine_frame < (char *)frame)
    {
        /* stack frame for calling __wine_longjmp */
        struct
        {
            int             retval;
            __wine_jmp_buf *jmp;
            void           *retaddr;
        } *stack;

        TRACE( "returning to handler\n" );
        stack = virtual_setup_exception( stack_ptr, sizeof(*stack), rec );
        stack->retval  = 1;
        stack->jmp     = &wine_frame->jmp;
        stack->retaddr = (void *)0xdeadbabe;
        ESP_sig(sigcontext) = (DWORD)stack;
        EIP_sig(sigcontext) = (DWORD)__wine_longjmp;
    }
    else
    {
        TRACE( "returning to user mode ip=%08x ret=%08x\n", frame->eip, rec->ExceptionCode );
        EAX_sig(sigcontext) = rec->ExceptionCode;
        EBX_sig(sigcontext) = frame->ebx;
        ESI_sig(sigcontext) = frame->esi;
        EDI_sig(sigcontext) = frame->edi;
        EBP_sig(sigcontext) = frame->ebp;
        ESP_sig(sigcontext) = frame->esp;
        EIP_sig(sigcontext) = frame->eip;
        x86_thread_data()->syscall_frame = NULL;
    }
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
    struct xcontext xcontext;
    ucontext_t *ucontext = sigcontext;
    void *stack = setup_exception_record( sigcontext, &rec, &xcontext );

    switch (TRAP_sig(ucontext))
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
            if (!err && (rec.ExceptionCode = is_privileged_instr( &xcontext.c ))) break;
            if ((err & 7) == 2 && handle_interrupt( err >> 3, ucontext, stack, &rec, &xcontext )) return;
            rec.ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
            rec.NumberParameters = 2;
            rec.ExceptionInformation[0] = 0;
            /* if error contains a LDT selector, use that as fault address */
            if ((err & 7) == 4 && !ldt_is_system( err | 7 )) rec.ExceptionInformation[1] = err & ~7;
            else
            {
                rec.ExceptionInformation[1] = 0xffffffff;
                if (check_invalid_gs( ucontext, &xcontext.c )) return;
            }
        }
        break;
    case TRAP_x86_PAGEFLT:  /* Page fault */
        rec.NumberParameters = 2;
        rec.ExceptionInformation[0] = (ERROR_sig(ucontext) >> 1) & 0x09;
        rec.ExceptionInformation[1] = (ULONG_PTR)siginfo->si_addr;
        rec.ExceptionCode = virtual_handle_fault( siginfo->si_addr, rec.ExceptionInformation[0], stack );
        if (!rec.ExceptionCode) return;
        if (rec.ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
            rec.ExceptionInformation[0] == EXCEPTION_EXECUTE_FAULT)
        {
            ULONG flags;
            NtQueryInformationProcess( GetCurrentProcess(), ProcessExecuteFlags,
                                       &flags, sizeof(flags), NULL );
            if (!(flags & MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION) &&
                check_atl_thunk( ucontext, &rec, &xcontext.c ))
                return;

            /* send EXCEPTION_EXECUTE_FAULT only if data execution prevention is enabled */
            if (!(flags & MEM_EXECUTE_OPTION_DISABLE)) rec.ExceptionInformation[0] = EXCEPTION_READ_FAULT;
        }
        break;
    case TRAP_x86_ALIGNFLT:  /* Alignment check exception */
        /* FIXME: pass through exception handler first? */
        if (xcontext.c.EFlags & 0x00040000)
        {
            EFL_sig(ucontext) &= ~0x00040000;  /* disable AC flag */
            return;
        }
        rec.ExceptionCode = EXCEPTION_DATATYPE_MISALIGNMENT;
        break;
    default:
        WINE_ERR( "Got unexpected trap %d\n", TRAP_sig(ucontext) );
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
    if (handle_syscall_fault( ucontext, stack, &rec, &xcontext.c )) return;
    setup_raise_exception( ucontext, stack, &rec, &xcontext );
}


/**********************************************************************
 *		trap_handler
 *
 * Handler for SIGTRAP.
 */
static void trap_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    EXCEPTION_RECORD rec = { 0 };
    struct xcontext xcontext;
    ucontext_t *ucontext = sigcontext;
    void *stack = setup_exception_record( sigcontext, &rec, &xcontext );

    switch (TRAP_sig(ucontext))
    {
    case TRAP_x86_TRCTRAP:  /* Single-step exception */
        rec.ExceptionCode = EXCEPTION_SINGLE_STEP;
        /* when single stepping can't tell whether this is a hw bp or a
         * single step interrupt. try to avoid as much overhead as possible
         * and only do a server call if there is any hw bp enabled. */
        if (!(xcontext.c.EFlags & 0x100) || (xcontext.c.Dr7 & 0xff))
        {
            /* (possible) hardware breakpoint, fetch the debug registers */
            DWORD saved_flags = xcontext.c.ContextFlags;
            xcontext.c.ContextFlags = CONTEXT_DEBUG_REGISTERS;
            NtGetContextThread( GetCurrentThread(), &xcontext.c );
            xcontext.c.ContextFlags |= saved_flags;  /* restore flags */
        }
        xcontext.c.EFlags &= ~0x100;  /* clear single-step flag */
        break;
    case TRAP_x86_BPTFLT:   /* Breakpoint exception */
        rec.ExceptionAddress = (char *)rec.ExceptionAddress - 1;  /* back up over the int3 instruction */
        /* fall through */
    default:
        rec.ExceptionCode = EXCEPTION_BREAKPOINT;
        rec.NumberParameters = is_wow64 ? 1 : 3;
        rec.ExceptionInformation[0] = 0;
        rec.ExceptionInformation[1] = 0; /* FIXME */
        rec.ExceptionInformation[2] = 0; /* FIXME */
        break;
    }
    setup_raise_exception( sigcontext, stack, &rec, &xcontext );
}


/**********************************************************************
 *		fpe_handler
 *
 * Handler for SIGFPE.
 */
static void fpe_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    EXCEPTION_RECORD rec = { 0 };
    struct xcontext xcontext;
    ucontext_t *ucontext = sigcontext;
    void *stack = setup_exception_record( sigcontext, &rec, &xcontext );

    switch (TRAP_sig(ucontext))
    {
    case TRAP_x86_DIVIDE:   /* Division by zero exception */
        rec.ExceptionCode = EXCEPTION_INT_DIVIDE_BY_ZERO;
        break;
    case TRAP_x86_FPOPFLT:   /* Coprocessor segment overrun */
        rec.ExceptionCode = EXCEPTION_FLT_INVALID_OPERATION;
        break;
    case TRAP_x86_ARITHTRAP:  /* Floating point exception */
        rec.ExceptionCode = get_fpu_code( &xcontext.c );
        rec.ExceptionAddress = (void *)xcontext.c.FloatSave.ErrorOffset;
        break;
    case TRAP_x86_CACHEFLT:  /* SIMD exception */
        /* TODO:
         * Behaviour only tested for divide-by-zero exceptions
         * Check for other SIMD exceptions as well */
        if(siginfo->si_code != FPE_FLTDIV && siginfo->si_code != FPE_FLTINV)
            FIXME("untested SIMD exception: %#x. Might not work correctly\n",
                  siginfo->si_code);

        rec.ExceptionCode = STATUS_FLOAT_MULTIPLE_TRAPS;
        rec.NumberParameters = 1;
        /* no idea what meaning is actually behind this but that's what native does */
        rec.ExceptionInformation[0] = 0;
        break;
    default:
        WINE_ERR( "Got unexpected trap %d\n", TRAP_sig(ucontext) );
        rec.ExceptionCode = EXCEPTION_FLT_INVALID_OPERATION;
        break;
    }
    setup_raise_exception( sigcontext, stack, &rec, &xcontext );
}


/**********************************************************************
 *		int_handler
 *
 * Handler for SIGINT.
 */
static void int_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    EXCEPTION_RECORD rec = { CONTROL_C_EXIT };

    setup_exception( sigcontext, &rec );
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
static void quit_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    init_handler( sigcontext );
    abort_thread(0);
}


/**********************************************************************
 *		usr1_handler
 *
 * Handler for SIGUSR1, used to signal a thread that it got suspended.
 */
static void usr1_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    struct xcontext xcontext;

    init_handler( sigcontext );
    if (x86_thread_data()->syscall_frame)
    {
        DECLSPEC_ALIGN(64) XSTATE xs;
        xcontext.c.ContextFlags = CONTEXT_FULL;
        context_init_xstate( &xcontext.c, &xs );

        NtGetContextThread( GetCurrentThread(), &xcontext.c );
        wait_suspend( &xcontext.c );
        NtSetContextThread( GetCurrentThread(), &xcontext.c );
    }
    else
    {
        save_context( &xcontext, sigcontext );
        wait_suspend( &xcontext.c );
        restore_context( &xcontext, sigcontext );
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

struct ldt_copy
{
    void         *base[LDT_SIZE];
    unsigned int  limit[LDT_SIZE];
    unsigned char flags[LDT_SIZE];
} __wine_ldt_copy;

static WORD gdt_fs_sel;
static pthread_mutex_t ldt_mutex = PTHREAD_MUTEX_INITIALIZER;
static const LDT_ENTRY null_entry;

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

#ifdef linux
    struct modify_ldt_s ldt_info = { index };

    ldt_info.base_addr       = ldt_get_base( entry );
    ldt_info.limit           = entry.LimitLow | (entry.HighWord.Bits.LimitHi << 16);
    ldt_info.seg_32bit       = entry.HighWord.Bits.Default_Big;
    ldt_info.contents        = (entry.HighWord.Bits.Type >> 2) & 3;
    ldt_info.read_exec_only  = !(entry.HighWord.Bits.Type & 2);
    ldt_info.limit_in_pages  = entry.HighWord.Bits.Granularity;
    ldt_info.seg_not_present = !entry.HighWord.Bits.Pres;
    ldt_info.usable          = entry.HighWord.Bits.Sys;
    if (modify_ldt( 0x11, &ldt_info, sizeof(ldt_info) ) < 0) perror( "modify_ldt" );
#elif defined(__NetBSD__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__) || defined(__DragonFly__)
    /* The kernel will only let us set LDTs with user priority level */
    if (entry.HighWord.Bits.Pres && entry.HighWord.Bits.Dpl != 3) entry.HighWord.Bits.Dpl = 3;
    if (i386_set_ldt(index, (union descriptor *)&entry, 1) < 0)
    {
        perror("i386_set_ldt");
        fprintf( stderr, "Did you reconfigure the kernel with \"options USER_LDT\"?\n" );
        exit(1);
    }
#elif defined(__svr4__) || defined(_SCO_DS)
    struct ssd ldt_mod;

    ldt_mod.sel  = sel;
    ldt_mod.bo   = (unsigned long)ldt_get_base( entry );
    ldt_mod.ls   = entry.LimitLow | (entry.HighWord.Bits.LimitHi << 16);
    ldt_mod.acc1 = entry.HighWord.Bytes.Flags1;
    ldt_mod.acc2 = entry.HighWord.Bytes.Flags2 >> 4;
    if (sysi86(SI86DSCR, &ldt_mod) == -1) perror("sysi86");
#elif defined(__APPLE__)
    if (i386_set_ldt(index, (union ldt_entry *)&entry, 1) < 0) perror("i386_set_ldt");
#elif defined(__GNU__)
    if (i386_set_ldt(mach_thread_self(), sel, (descriptor_list_t)&entry, 1) != KERN_SUCCESS)
        perror("i386_set_ldt");
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

static void ldt_set_fs( WORD sel, TEB *teb )
{
    if (sel == gdt_fs_sel)
    {
#ifdef __linux__
        struct modify_ldt_s ldt_info = { sel >> 3 };

        ldt_info.base_addr = teb;
        ldt_info.limit     = page_size - 1;
        ldt_info.seg_32bit = 1;
        if (set_thread_area( &ldt_info ) < 0) perror( "set_thread_area" );
#elif defined(__FreeBSD__) || defined (__FreeBSD_kernel__) || defined(__DragonFly__)
        i386_set_fsbase( teb );
#endif
    }
    set_fs( sel );
}


/**********************************************************************
 *           get_thread_ldt_entry
 */
NTSTATUS get_thread_ldt_entry( HANDLE handle, void *data, ULONG len, ULONG *ret_len )
{
    THREAD_DESCRIPTOR_INFORMATION *info = data;
    NTSTATUS status = STATUS_SUCCESS;

    if (len < sizeof(*info)) return STATUS_INFO_LENGTH_MISMATCH;
    if (info->Selector >> 16) return STATUS_UNSUCCESSFUL;

    if (is_gdt_sel( info->Selector ))
    {
        if (!(info->Selector & ~3))
            info->Entry = null_entry;
        else if ((info->Selector | 3) == get_cs())
            info->Entry = ldt_make_entry( 0, ~0u, LDT_FLAGS_CODE | LDT_FLAGS_32BIT );
        else if ((info->Selector | 3) == get_ds())
            info->Entry = ldt_make_entry( 0, ~0u, LDT_FLAGS_DATA | LDT_FLAGS_32BIT );
        else if ((info->Selector | 3) == get_fs())
            info->Entry = ldt_make_entry( NtCurrentTeb(), 0xfff, LDT_FLAGS_DATA | LDT_FLAGS_32BIT );
        else
            return STATUS_UNSUCCESSFUL;
    }
    else
    {
        SERVER_START_REQ( get_selector_entry )
        {
            req->handle = wine_server_obj_handle( handle );
            req->entry = info->Selector >> 3;
            status = wine_server_call( req );
            if (!status)
            {
                if (reply->flags)
                    info->Entry = ldt_make_entry( (void *)reply->base, reply->limit, reply->flags );
                else
                    status = STATUS_UNSUCCESSFUL;
            }
        }
        SERVER_END_REQ;
    }
    if (status == STATUS_SUCCESS && ret_len)
        /* yes, that's a bit strange, but it's the way it is */
        *ret_len = sizeof(info->Entry);

    return status;
}


/******************************************************************************
 *           NtSetLdtEntries   (NTDLL.@)
 *           ZwSetLdtEntries   (NTDLL.@)
 */
NTSTATUS WINAPI NtSetLdtEntries( ULONG sel1, LDT_ENTRY entry1, ULONG sel2, LDT_ENTRY entry2 )
{
    sigset_t sigset;

    if (sel1 >> 16 || sel2 >> 16) return STATUS_INVALID_LDT_DESCRIPTOR;
    if (sel1 && (sel1 >> 3) < first_ldt_entry) return STATUS_INVALID_LDT_DESCRIPTOR;
    if (sel2 && (sel2 >> 3) < first_ldt_entry) return STATUS_INVALID_LDT_DESCRIPTOR;

    server_enter_uninterrupted_section( &ldt_mutex, &sigset );
    if (sel1) ldt_set_entry( sel1, entry1 );
    if (sel2) ldt_set_entry( sel2, entry2 );
    server_leave_uninterrupted_section( &ldt_mutex, &sigset );
   return STATUS_SUCCESS;
}


/**********************************************************************
 *             signal_init_threading
 */
void signal_init_threading(void)
{
#ifdef __linux__
    /* the preloader may have allocated it already */
    gdt_fs_sel = get_fs();
    if (!gdt_fs_sel || !is_gdt_sel( gdt_fs_sel ))
    {
        struct modify_ldt_s ldt_info = { -1 };

        ldt_info.seg_32bit = 1;
        ldt_info.usable = 1;
        if (set_thread_area( &ldt_info ) >= 0) gdt_fs_sel = (ldt_info.entry_number << 3) | 3;
        else gdt_fs_sel = 0;
    }
#elif defined(__FreeBSD__) || defined (__FreeBSD_kernel__)
    gdt_fs_sel = GSEL( GUFS_SEL, SEL_UPL );
#endif
}


/**********************************************************************
 *		signal_alloc_thread
 */
NTSTATUS signal_alloc_thread( TEB *teb )
{
    struct x86_thread_data *thread_data = (struct x86_thread_data *)&teb->GdiTebBatch;

    if (!gdt_fs_sel)
    {
        static int first_thread = 1;
        sigset_t sigset;
        int idx;
        LDT_ENTRY entry = ldt_make_entry( teb, page_size - 1, LDT_FLAGS_DATA | LDT_FLAGS_32BIT );

        if (first_thread)  /* no locking for first thread */
        {
            /* leave some space if libc is using the LDT for %gs */
            if (!is_gdt_sel( get_gs() )) first_ldt_entry = 512;
            idx = first_ldt_entry;
            ldt_set_entry( (idx << 3) | 7, entry );
            first_thread = 0;
        }
        else
        {
            server_enter_uninterrupted_section( &ldt_mutex, &sigset );
            for (idx = first_ldt_entry; idx < LDT_SIZE; idx++)
            {
                if (__wine_ldt_copy.flags[idx]) continue;
                ldt_set_entry( (idx << 3) | 7, entry );
                break;
            }
            server_leave_uninterrupted_section( &ldt_mutex, &sigset );
            if (idx == LDT_SIZE) return STATUS_TOO_MANY_THREADS;
        }
        thread_data->fs = (idx << 3) | 7;
    }
    else thread_data->fs = gdt_fs_sel;

    teb->WOW32Reserved = syscall_dispatcher;
    return STATUS_SUCCESS;
}


/**********************************************************************
 *		signal_free_thread
 */
void signal_free_thread( TEB *teb )
{
    struct x86_thread_data *thread_data = (struct x86_thread_data *)&teb->GdiTebBatch;
    sigset_t sigset;

    if (gdt_fs_sel) return;

    server_enter_uninterrupted_section( &ldt_mutex, &sigset );
    __wine_ldt_copy.flags[thread_data->fs >> 3] = 0;
    server_leave_uninterrupted_section( &ldt_mutex, &sigset );
}


/**********************************************************************
 *		signal_init_thread
 */
void signal_init_thread( TEB *teb )
{
    const WORD fpu_cw = 0x27f;
    struct x86_thread_data *thread_data = (struct x86_thread_data *)&teb->GdiTebBatch;

    ldt_set_fs( thread_data->fs, teb );
    thread_data->gs = get_gs();

    __asm__ volatile ("fninit; fldcw %0" : : "m" (fpu_cw));
}


/**********************************************************************
 *		signal_init_process
 */
void signal_init_process(void)
{
    struct sigaction sig_act;

    sig_act.sa_mask = server_block_set;
    sig_act.sa_flags = SA_SIGINFO | SA_RESTART | SA_ONSTACK;
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


/**********************************************************************
 *		signal_init_syscalls
 */
void *signal_init_syscalls(void)
{
    extern void __wine_syscall_dispatcher_fxsave(void) DECLSPEC_HIDDEN;
    extern void __wine_syscall_dispatcher_xsave(void) DECLSPEC_HIDDEN;
    extern void __wine_syscall_dispatcher_xsavec(void) DECLSPEC_HIDDEN;

    if (xstate_compaction_enabled)
        syscall_dispatcher = __wine_syscall_dispatcher_xsavec;
    else if (cpu_info.ProcessorFeatureBits & CPU_FEATURE_XSAVE)
        syscall_dispatcher = __wine_syscall_dispatcher_xsave;
    else if (cpu_info.ProcessorFeatureBits & CPU_FEATURE_FXSR)
        syscall_dispatcher = __wine_syscall_dispatcher_fxsave;
    else
        syscall_dispatcher = __wine_syscall_dispatcher;

    return NtCurrentTeb()->WOW32Reserved = syscall_dispatcher;
}


/***********************************************************************
 *           init_thread_context
 */
static void init_thread_context( CONTEXT *context, LPTHREAD_START_ROUTINE entry, void *arg )
{
    context->SegCs  = get_cs();
    context->SegDs  = get_ds();
    context->SegEs  = get_ds();
    context->SegFs  = get_fs();
    context->SegGs  = get_gs();
    context->SegSs  = get_ds();
    context->EFlags = 0x202;
    context->Eax    = (DWORD)entry;
    context->Ebx    = (DWORD)arg;
    context->Esp    = (DWORD)NtCurrentTeb()->Tib.StackBase - 16;
    context->Eip    = (DWORD)pRtlUserThreadStart;
    context->FloatSave.ControlWord = 0x27f;
    ((XSAVE_FORMAT *)context->ExtendedRegisters)->ControlWord = 0x27f;
    ((XSAVE_FORMAT *)context->ExtendedRegisters)->MxCsr = 0x1f80;
    if (NtCurrentTeb64())
    {
        WOW64_CPURESERVED *cpu = ULongToPtr( NtCurrentTeb64()->TlsSlots[WOW64_TLS_CPURESERVED] );
        memcpy( cpu + 1, context, sizeof(*context) );
    }
}


/***********************************************************************
 *           get_initial_context
 */
PCONTEXT DECLSPEC_HIDDEN get_initial_context( LPTHREAD_START_ROUTINE entry, void *arg, BOOL suspend )
{
    CONTEXT *ctx;

    if (suspend)
    {
        CONTEXT context = { CONTEXT_ALL };

        init_thread_context( &context, entry, arg );
        wait_suspend( &context );
        ctx = (CONTEXT *)((ULONG_PTR)context.Esp & ~15) - 1;
        *ctx = context;
    }
    else
    {
        ctx = (CONTEXT *)((char *)NtCurrentTeb()->Tib.StackBase - 16) - 1;
        init_thread_context( ctx, entry, arg );
    }
    pthread_sigmask( SIG_UNBLOCK, &server_block_set, NULL );
    ctx->ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT | CONTEXT_EXTENDED_REGISTERS;
    return ctx;
}


/***********************************************************************
 *           signal_start_thread
 */
__ASM_GLOBAL_FUNC( signal_start_thread,
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
                   /* store exit frame */
                   "movl %ebp,%fs:0x1f4\n\t"    /* x86_thread_data()->exit_frame */
                   /* switch to thread stack */
                   "movl %fs:4,%eax\n\t"        /* NtCurrentTeb()->StackBase */
                   "leal -0x1004(%eax),%esp\n\t"
                   /* attach dlls */
                   "pushl 16(%ebp)\n\t"         /* suspend */
                   "pushl 12(%ebp)\n\t"         /* arg */
                   "pushl 8(%ebp)\n\t"          /* entry */
                   "call " __ASM_NAME("get_initial_context") "\n\t"
                   "movl %eax,(%esp)\n\t"       /* context */
                   "movl 20(%ebp),%edx\n\t"     /* thunk */
                   "xorl %ebp,%ebp\n\t"
                   "pushl $0\n\t"
                   "jmp *%edx" )


/***********************************************************************
 *           signal_exit_thread
 */
__ASM_GLOBAL_FUNC( signal_exit_thread,
                   "movl 8(%esp),%ecx\n\t"
                   /* fetch exit frame */
                   "movl %fs:0x1f4,%edx\n\t"    /* x86_thread_data()->exit_frame */
                   "testl %edx,%edx\n\t"
                   "jnz 1f\n\t"
                   "jmp *%ecx\n\t"
                   /* switch to exit frame stack */
                   "1:\tmovl 4(%esp),%eax\n\t"
                   "movl $0,%fs:0x1f4\n\t"
                   "movl %edx,%ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa %ebp,4\n\t")
                   __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                   __ASM_CFI(".cfi_rel_offset %ebx,-4\n\t")
                   __ASM_CFI(".cfi_rel_offset %esi,-8\n\t")
                   __ASM_CFI(".cfi_rel_offset %edi,-12\n\t")
                   "leal -20(%ebp),%esp\n\t"
                   "pushl %eax\n\t"
                   "call *%ecx" )

/**********************************************************************
 *           NtCurrentTeb   (NTDLL.@)
 */
__ASM_STDCALL_FUNC( NtCurrentTeb, 0, ".byte 0x64\n\tmovl 0x18,%eax\n\tret" )

#endif  /* __i386__ */
