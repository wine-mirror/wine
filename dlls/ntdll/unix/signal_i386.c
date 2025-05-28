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

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <assert.h>
#include <unistd.h>
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
#include "unix_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);

#define NTDLL_DWARF_H_NO_UNWINDER
#include "dwarf.h"

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
#define XState_sig(fpu)      (((unsigned int *)fpu->Reserved4)[12] == FP_XSTATE_MAGIC1 ? (XSAVE_AREA_HEADER *)(fpu + 1) : NULL)

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
#define EIP_sig(context)     ((context)->uc_mcontext->__ss.__eip)
#define ESP_sig(context)     ((context)->uc_mcontext->__ss.__esp)
#define TRAP_sig(context)    ((context)->uc_mcontext->__es.__trapno)
#define ERROR_sig(context)   ((context)->uc_mcontext->__es.__err)
#define FPU_sig(context)     NULL
#define FPUX_sig(context)    ((XSAVE_FORMAT *)&(context)->uc_mcontext->__fs.__fpu_fcw)
#define XState_sig(context)  NULL  /* FIXME */

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

/* stack layout when calling KiUserExceptionDispatcher */
struct exc_stack_layout
{
    EXCEPTION_RECORD *rec_ptr;       /* 000 first arg for KiUserExceptionDispatcher */
    CONTEXT          *context_ptr;   /* 004 second arg for KiUserExceptionDispatcher */
    EXCEPTION_RECORD  rec;           /* 008 */
    CONTEXT           context;       /* 058 */
    CONTEXT_EX        context_ex;    /* 324 */
    DWORD             align;         /* 33c */
};
C_ASSERT( offsetof(struct exc_stack_layout, context) == 0x58 );
C_ASSERT( sizeof(struct exc_stack_layout) == 0x340 );

/* stack layout when calling KiUserApcDispatcher */
struct apc_stack_layout
{
    PNTAPCFUNC        func;          /* 000 */
    UINT              arg1;          /* 004 */
    UINT              arg2;          /* 008 */
    UINT              arg3;          /* 00c */
    UINT              alertable;     /* 010 */
    CONTEXT           context;       /* 014 */
    CONTEXT_EX        xctx;          /* 2e0 */
    UINT              unk2[4];       /* 2f8 */
};
C_ASSERT( offsetof(struct apc_stack_layout, context) == 0x14 );
C_ASSERT( sizeof(struct apc_stack_layout) == 0x308 );

/* stack layout when calling KiUserCallbackDispatcher */
struct callback_stack_layout
{
    ULONG             eip;           /* 000 */
    ULONG             id;            /* 004 */
    void             *args;          /* 008 */
    ULONG             len;           /* 00c */
    ULONG             unk[2];        /* 010 */
    ULONG             esp;           /* 018 */
    BYTE              args_data[0];  /* 01c */
};
C_ASSERT( sizeof(struct callback_stack_layout) == 0x1c );

struct syscall_frame
{
    WORD                  syscall_flags;  /* 000 */
    WORD                  restore_flags;  /* 002 */
    UINT                  eflags;         /* 004 */
    UINT                  eip;            /* 008 */
    UINT                  esp;            /* 00c */
    WORD                  cs;             /* 010 */
    WORD                  ss;             /* 012 */
    WORD                  ds;             /* 014 */
    WORD                  es;             /* 016 */
    WORD                  fs;             /* 018 */
    WORD                  gs;             /* 01a */
    UINT                  eax;            /* 01c */
    UINT                  ebx;            /* 020 */
    UINT                  ecx;            /* 024 */
    UINT                  edx;            /* 028 */
    UINT                  edi;            /* 02c */
    UINT                  esi;            /* 030 */
    UINT                  ebp;            /* 034 */
    void                 *syscall_cfa;    /* 038 */
    struct syscall_frame *prev_frame;     /* 03c */
    union                                 /* 040 */
    {
        XSAVE_FORMAT       xsave;
        FLOATING_SAVE_AREA fsave;
    } u;
    DECLSPEC_ALIGN(64) XSAVE_AREA_HEADER xstate; /* 240 */
};

C_ASSERT( sizeof(struct syscall_frame) == 0x280 );

struct x86_thread_data
{
    UINT               fs;            /* 1d4 TEB selector */
    UINT               gs;            /* 1d8 libc selector; update winebuild if you move this! */
    UINT               dr0;           /* 1dc debug registers */
    UINT               dr1;           /* 1e0 */
    UINT               dr2;           /* 1e4 */
    UINT               dr3;           /* 1e8 */
    UINT               dr6;           /* 1ec */
    UINT               dr7;           /* 1f0 */
    UINT               frame_size;    /* 1f4 syscall frame size including xstate */
};

C_ASSERT( sizeof(struct x86_thread_data) <= sizeof(((struct ntdll_thread_data *)0)->cpu_data) );
C_ASSERT( offsetof( TEB, GdiTebBatch ) + offsetof( struct x86_thread_data, gs ) == 0x1d8 );
C_ASSERT( offsetof( TEB, GdiTebBatch ) + offsetof( struct x86_thread_data, frame_size ) == 0x1f4 );

/* flags to control the behavior of the syscall dispatcher */
#define SYSCALL_HAVE_XSAVE    1

static unsigned int syscall_flags;
static unsigned int frame_size;
static unsigned int xstate_size = sizeof(XSAVE_AREA_HEADER);
static UINT64 xstate_extended_features;

static inline struct x86_thread_data *x86_thread_data(void)
{
    return (struct x86_thread_data *)ntdll_get_thread_data()->cpu_data;
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
NTSTATUS unwind_builtin_dll( void *args )
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


void set_process_instrumentation_callback( void *callback )
{
    if (callback) FIXME( "Not supported.\n" );
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
        xctx->XState.Length = xstate_size;
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
        XSAVE_AREA_HEADER *xs;

        context->ContextFlags |= CONTEXT_FLOATING_POINT | CONTEXT_EXTENDED_REGISTERS;
        memcpy( context->ExtendedRegisters, fpux, sizeof(*fpux) );
        if (!fpu) fpux_to_fpu( &context->FloatSave, fpux );
        if (xstate_extended_features && (xs = XState_sig(fpux))) context_init_xstate( context, xs );
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
    if (fpux) memcpy( fpux, context->ExtendedRegisters, sizeof(*fpux) );
    if (!fpu && !fpux) restore_fpu( context );
}


/***********************************************************************
 *           signal_set_full_context
 */
NTSTATUS signal_set_full_context( CONTEXT *context )
{
    NTSTATUS status = NtSetContextThread( GetCurrentThread(), context );

    if (!status && (context->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER)
        get_syscall_frame()->restore_flags |= LOWORD(CONTEXT_INTEGER);
    return status;
}


/***********************************************************************
 *              get_native_context
 */
void *get_native_context( CONTEXT *context )
{
    return is_old_wow64() ? NULL : context;
}


/***********************************************************************
 *              get_wow_context
 */
void *get_wow_context( CONTEXT *context )
{
    return is_old_wow64() ? context : NULL;
}


/***********************************************************************
 *              NtSetContextThread  (NTDLL.@)
 *              ZwSetContextThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetContextThread( HANDLE handle, const CONTEXT *context )
{
    NTSTATUS ret = STATUS_SUCCESS;
    struct syscall_frame *frame = get_syscall_frame();
    DWORD flags = context->ContextFlags & ~CONTEXT_i386;
    BOOL self = (handle == GetCurrentThread());

    if ((flags & CONTEXT_XSTATE) && xstate_extended_features)
    {
        CONTEXT_EX *context_ex = (CONTEXT_EX *)(context + 1);
        XSAVE_AREA_HEADER *xs = (XSAVE_AREA_HEADER *)((char *)context_ex + context_ex->XState.Offset);

        if (context_ex->XState.Length < sizeof(XSAVE_AREA_HEADER) ||
            context_ex->XState.Length > xstate_size)
            return STATUS_INVALID_PARAMETER;
        if ((xs->Mask & xstate_extended_features)
            && (context_ex->XState.Length < xstate_get_size( xs->CompactionMask, xs->Mask )))
            return STATUS_BUFFER_OVERFLOW;
    }
    else flags &= ~CONTEXT_XSTATE;

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
        memcpy( &frame->u.xsave, context->ExtendedRegisters, sizeof(frame->u.xsave) );
        /* reset the current interrupt status */
        frame->u.xsave.StatusWord &= frame->u.xsave.ControlWord | 0xff80;
        frame->xstate.Mask |= XSTATE_MASK_LEGACY;
    }
    else if (flags & CONTEXT_FLOATING_POINT)
    {
        if (user_shared_data->ProcessorFeatures[PF_XMMI_INSTRUCTIONS_AVAILABLE])
        {
            fpu_to_fpux( &frame->u.xsave, &context->FloatSave );
        }
        else
        {
            frame->u.fsave = context->FloatSave;
        }
        frame->xstate.Mask |= XSTATE_MASK_LEGACY_FLOATING_POINT;
    }
    if (flags & CONTEXT_XSTATE)
    {
        CONTEXT_EX *context_ex = (CONTEXT_EX *)(context + 1);
        XSAVE_AREA_HEADER *xs = (XSAVE_AREA_HEADER *)((char *)context_ex + context_ex->XState.Offset);
        UINT64 mask = frame->xstate.Mask;

        if (user_shared_data->XState.CompactionEnabled)
            frame->xstate.CompactionMask |= xstate_extended_features;
        copy_xstate( &frame->xstate, xs, xs->Mask );
        if (xs->CompactionMask) frame->xstate.Mask |= mask & ~xs->CompactionMask;
    }

    frame->restore_flags |= flags & ~CONTEXT_INTEGER;
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
    struct syscall_frame *frame = get_syscall_frame();
    DWORD needed_flags = context->ContextFlags & ~CONTEXT_i386;
    BOOL self = (handle == GetCurrentThread());
    NTSTATUS ret;

    /* debug registers require a server call */
    if (needed_flags & CONTEXT_DEBUG_REGISTERS) self = FALSE;

    if (!self)
    {
        if ((ret = get_thread_context( handle, context, &self, IMAGE_FILE_MACHINE_I386 ))) return ret;
    }

    if (self)
    {
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
            if (!user_shared_data->ProcessorFeatures[PF_XMMI_INSTRUCTIONS_AVAILABLE])
            {
                context->FloatSave = frame->u.fsave;
            }
            else if (!user_shared_data->XState.CompactionEnabled ||
                     (frame->xstate.Mask & XSTATE_MASK_LEGACY_FLOATING_POINT))
            {
                fpux_to_fpu( &context->FloatSave, &frame->u.xsave );
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

            if (!user_shared_data->XState.CompactionEnabled ||
                (frame->xstate.Mask & XSTATE_MASK_LEGACY_FLOATING_POINT))
            {
                memcpy( xs, &frame->u.xsave, FIELD_OFFSET( XSAVE_FORMAT, MxCsr ));
                memcpy( xs->FloatRegisters, frame->u.xsave.FloatRegisters,
                        sizeof( xs->FloatRegisters ));
            }
            else
            {
                memset( xs, 0, FIELD_OFFSET( XSAVE_FORMAT, MxCsr ));
                memset( xs->FloatRegisters, 0, sizeof( xs->FloatRegisters ));
                xs->ControlWord = 0x37f;
            }

            if (!user_shared_data->XState.CompactionEnabled ||
                (frame->xstate.Mask & XSTATE_MASK_LEGACY_SSE))
            {
                memcpy( xs->XmmRegisters, frame->u.xsave.XmmRegisters, sizeof( xs->XmmRegisters ));
                xs->MxCsr      = frame->u.xsave.MxCsr;
                xs->MxCsr_Mask = frame->u.xsave.MxCsr_Mask;
            }
            else
            {
                memset( xs->XmmRegisters, 0, sizeof( xs->XmmRegisters ));
                xs->MxCsr      = 0x1f80;
                xs->MxCsr_Mask = 0x2ffff;
            }

            context->ContextFlags |= CONTEXT_EXTENDED_REGISTERS;
        }
        if ((needed_flags & CONTEXT_XSTATE) && xstate_extended_features)
        {
            CONTEXT_EX *context_ex = (CONTEXT_EX *)(context + 1);
            XSAVE_AREA_HEADER *xstate = (XSAVE_AREA_HEADER *)((char *)context_ex + context_ex->XState.Offset);
            UINT64 mask;

            if (context_ex->XState.Length < sizeof(XSAVE_AREA_HEADER) ||
                context_ex->XState.Length > xstate_size)
                return STATUS_INVALID_PARAMETER;

            if (user_shared_data->XState.CompactionEnabled)
            {
                frame->xstate.CompactionMask |= xstate_extended_features;
                mask = xstate->CompactionMask & xstate_extended_features;
                xstate->Mask = frame->xstate.Mask & mask;
                xstate->CompactionMask = 0x8000000000000000 | mask;
            }
            else
            {
                mask = xstate->Mask & xstate_extended_features;
                xstate->Mask = frame->xstate.Mask & mask;
                xstate->CompactionMask = 0;
            }

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
            x86_thread_data()->dr0 = context->Dr0;
            x86_thread_data()->dr1 = context->Dr1;
            x86_thread_data()->dr2 = context->Dr2;
            x86_thread_data()->dr3 = context->Dr3;
            x86_thread_data()->dr6 = context->Dr6;
            x86_thread_data()->dr7 = context->Dr7;
        }
        set_context_exception_reporting_flags( &context->ContextFlags, CONTEXT_SERVICE_ACTIVE );
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


#pragma pack(push,1)
union atl_thunk
{
    struct
    {
        UINT  movl;  /* movl this,4(%esp) */
        UINT  this;
        BYTE  jmp;   /* jmp func */
        int   func;
    } t1;
    struct
    {
        BYTE  movl;  /* movl this,ecx */
        UINT  this;
        BYTE  jmp;   /* jmp func */
        int   func;
    } t2;
    struct
    {
        BYTE  movl1; /* movl this,edx */
        UINT  this;
        BYTE  movl2; /* movl func,ecx */
        UINT  func;
        WORD  jmp;   /* jmp ecx */
    } t3;
    struct
    {
        BYTE  movl1; /* movl this,ecx */
        UINT  this;
        BYTE  movl2; /* movl func,eax */
        UINT  func;
        WORD  jmp;   /* jmp eax */
    } t4;
    struct
    {
        UINT  inst1; /* pop ecx
                      * pop eax
                      * push ecx
                      * jmp 4(%eax) */
        WORD  inst2;
    } t5;
};
#pragma pack(pop)

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
    XSAVE_AREA_HEADER *src_xs;
    struct exc_stack_layout *stack;
    size_t stack_size;
    NTSTATUS status = send_debug_event( rec, context, TRUE, TRUE );

    if (status == DBG_CONTINUE || status == DBG_EXCEPTION_HANDLED)
    {
        restore_context( xcontext, sigcontext );
        return;
    }

    /* fix up instruction pointer in context for EXCEPTION_BREAKPOINT */
    if (rec->ExceptionCode == EXCEPTION_BREAKPOINT) context->Eip--;

    stack_size = (ULONG_PTR)stack_ptr - (((ULONG_PTR)stack_ptr - sizeof(*stack) - xstate_size) & ~(ULONG_PTR)63);
    stack = virtual_setup_exception( stack_ptr, stack_size, rec );
    stack->rec_ptr      = &stack->rec;
    stack->context_ptr  = &stack->context;
    stack->rec          = *rec;
    stack->context      = *context;

    if ((src_xs = xstate_from_context( context )))
    {
        XSAVE_AREA_HEADER *dst_xs = (XSAVE_AREA_HEADER *)(stack + 1);

        assert( !((ULONG_PTR)dst_xs & 63) );
        context_init_xstate( &stack->context, dst_xs );
        memset( dst_xs, 0, sizeof(*dst_xs) );
        dst_xs->CompactionMask = user_shared_data->XState.CompactionEnabled ? 0x8000000000000000 | xstate_extended_features : 0;
        copy_xstate( dst_xs, src_xs, src_xs->Mask );
    }
    else
    {
        context_init_xstate( &stack->context, NULL );
    }

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


/***********************************************************************
 *           call_user_apc_dispatcher
 */
NTSTATUS call_user_apc_dispatcher( CONTEXT *context, ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3,
                                   PNTAPCFUNC func, NTSTATUS status )
{
    struct syscall_frame *frame = get_syscall_frame();
    ULONG esp = context ? context->Esp : frame->esp;
    struct apc_stack_layout *stack = (struct apc_stack_layout *)esp - 1;

    if (!context)
    {
        stack->context.ContextFlags = CONTEXT_FULL;
        NtGetContextThread( GetCurrentThread(), &stack->context );
        stack->context.Eax = status;
    }
    else memmove( &stack->context, context, sizeof(stack->context) );

    context_init_xstate( &stack->context, NULL );
    stack->func      = func;
    stack->arg1      = arg1;
    stack->arg2      = arg2;
    stack->arg3      = arg3;
    stack->alertable = TRUE;
    frame->ebp = stack->context.Ebp;
    frame->esp = (ULONG)stack;
    frame->eip = (ULONG)pKiUserApcDispatcher;
    return status;
}


/***********************************************************************
 *           call_raise_user_exception_dispatcher
 */
void call_raise_user_exception_dispatcher(void)
{
    get_syscall_frame()->eip = (DWORD)pKiRaiseUserExceptionDispatcher;
}


/***********************************************************************
 *           call_user_exception_dispatcher
 */
NTSTATUS call_user_exception_dispatcher( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    struct syscall_frame *frame = get_syscall_frame();
    ULONG esp = (frame->esp - sizeof(struct exc_stack_layout)) & ~3;
    struct exc_stack_layout *stack;
    XSAVE_AREA_HEADER *src_xs;

    if (rec->ExceptionCode == EXCEPTION_BREAKPOINT) context->Eip--;

    stack = (struct exc_stack_layout *)((esp - sizeof(*stack) - xstate_size) & ~(ULONG_PTR)63);
    stack->rec_ptr      = &stack->rec;
    stack->context_ptr  = &stack->context;
    stack->rec          = *rec;
    stack->context      = *context;

    if ((src_xs = xstate_from_context( context )))
    {
        XSAVE_AREA_HEADER *dst_xs = (XSAVE_AREA_HEADER *)(stack + 1);

        context_init_xstate( &stack->context, dst_xs );
        assert( !((ULONG_PTR)dst_xs & 63) );
        memset( dst_xs, 0, sizeof(*dst_xs) );
        dst_xs->CompactionMask = user_shared_data->XState.CompactionEnabled ? 0x8000000000000000 | xstate_extended_features : 0;
        copy_xstate( dst_xs, src_xs, src_xs->Mask );
    }
    else
    {
        context_init_xstate( &stack->context, NULL );
    }

    frame->esp = (ULONG)stack;
    frame->eip = (ULONG)pKiUserExceptionDispatcher;
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           call_user_mode_callback
 */
extern NTSTATUS call_user_mode_callback( ULONG user_esp, void **ret_ptr, ULONG *ret_len,
                                         void *func, TEB *teb );
__ASM_GLOBAL_FUNC( call_user_mode_callback,
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
                   "movl 8(%ebp),%ebx\n\t"     /* user_esp */
                   "movl 0x18(%ebp),%edx\n\t"  /* teb */
                   "pushl 4(%ebx)\n\t"         /* id */
                   "pushl 0(%edx)\n\t"         /* teb->Tib.ExceptionList */
                   "subl 0x1f4(%edx),%esp\n\t" /* x86_thread_data()->frame_size */
                   "andl $~63,%esp\n\t"
                   "leal 8(%ebp),%eax\n\t"
                   "movl %eax,0x38(%esp)\n\t"  /* frame->syscall_cfa */
                   "movl 0x218(%edx),%ecx\n\t" /* thread_data->syscall_frame */
                   "movl (%ecx),%eax\n\t"      /* frame->syscall_flags */
                   "movl %eax,(%esp)\n\t"
                   "movl %ecx,0x3c(%esp)\n\t"  /* frame->prev_frame */
                   "movl %esp,0x218(%edx)\n\t" /* thread_data->syscall_frame */
                   "testl $1,0x21c(%edx)\n\t"  /* thread_data->syscall_trace */
                   "jz 1f\n\t"
                   "subl $4,%esp\n\t"
                   "push 12(%ebx)\n\t"         /* len */
                   "push 8(%ebx)\n\t"          /* args */
                   "push 4(%ebx)\n\t"          /* id */
                   "call " __ASM_NAME("trace_usercall") "\n"
                   "1:\tmovl 0x14(%ebp),%ecx\n\t" /* func */
                   /* switch to user stack */
                   "movl %ebx,%esp\n\t"
                   "xorl %ebp,%ebp\n\t"
                   "jmpl *%ecx" )


/***********************************************************************
 *           user_mode_callback_return
 */
extern void DECLSPEC_NORETURN user_mode_callback_return( void *ret_ptr, ULONG ret_len,
                                                         NTSTATUS status, TEB *teb );
__ASM_GLOBAL_FUNC( user_mode_callback_return,
                   "movl 16(%esp),%ebx\n"      /* teb */
                   "movl 0x218(%ebx),%eax\n\t" /* thread_data->syscall_frame */
                   "movl 0x3c(%eax),%ecx\n\t"  /* frame->prev_frame */
                   "movl %ecx,0x218(%ebx)\n\t" /* thread_data->syscall_frame */
                   "movl 0x38(%eax),%ebp\n\t"  /* frame->syscall_cfa */
                   "subl $8,%ebp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                   __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                   __ASM_CFI(".cfi_rel_offset %ebx,-4\n\t")
                   __ASM_CFI(".cfi_rel_offset %esi,-8\n\t")
                   __ASM_CFI(".cfi_rel_offset %edi,-12\n\t")
                   "movl 4(%esp),%esi\n\t"     /* ret_ptr */
                   "movl 8(%esp),%edi\n\t"     /* ret_len */
                   "movl 12(%esp),%eax\n\t"    /* status */
                   "leal -20(%ebp),%esp\n\t"
                   "movl 0x0c(%ebp),%ecx\n\t"  /* ret_ptr */
                   "movl %esi,(%ecx)\n\t"
                   "movl 0x10(%ebp),%ecx\n\t"  /* ret_len */
                   "movl %edi,(%ecx)\n\t"
                   "popl (%ebx)\n\t"           /* teb->Tib.ExceptionList */
                   "popl %edx\n\t"             /* id */
                   "testl $1,0x21c(%ebx)\n\t"  /* thread_data->syscall_trace */
                   "jz 1f\n\t"
                   "pushl %eax\n\t"            /* status */
                   "subl $8,%esp\n\t"
                   "pushl %edx\n\t"            /* id */
                   "pushl %eax\n\t"            /* status */
                   "pushl %edi\n\t"            /* ret_len */
                   "pushl %esi\n\t"            /* ret_ptr */
                   "call " __ASM_NAME("trace_userret") "\n\t"
                   "addl $24,%esp\n\t"
                   "popl %eax\n\t"             /* status */
                   "1:\tpopl %edi\n\t"
                   __ASM_CFI(".cfi_same_value %edi\n\t")
                   "popl %esi\n\t"
                   __ASM_CFI(".cfi_same_value %esi\n\t")
                   "popl %ebx\n\t"
                   __ASM_CFI(".cfi_same_value %ebx\n\t")
                   "leave\n"
                   __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                   __ASM_CFI(".cfi_same_value %ebp\n\t")
                   "ret" )


/***********************************************************************
 *           user_mode_abort_thread
 */
extern void DECLSPEC_NORETURN user_mode_abort_thread( NTSTATUS status, struct syscall_frame *frame );
__ASM_GLOBAL_FUNC( user_mode_abort_thread,
                   "movl 8(%esp),%eax\n\t"     /* frame */
                   "movl 0x38(%eax),%ebp\n\t"  /* frame->syscall_cfa */
                   "movl 4(%esp),%eax\n\t "    /* status */
                   "subl $8,%ebp\n\t"
                   /* switch to kernel stack */
                   "leal -16(%ebp),%esp\n\t"
                   __ASM_CFI(".cfi_def_cfa %ebp,8\n\t")
                   __ASM_CFI(".cfi_offset %eip,-4\n\t")
                   __ASM_CFI(".cfi_offset %ebp,-8\n\t")
                   __ASM_CFI(".cfi_offset %ebx,-12\n\t")
                   __ASM_CFI(".cfi_offset %esi,-16\n\t")
                   __ASM_CFI(".cfi_offset %edi,-20\n\t")
                   "movl %eax,(%esp)\n\t"      /* status */
                   "call " __ASM_NAME("abort_thread") )


/***********************************************************************
 *           KeUserModeCallback
 */
NTSTATUS KeUserModeCallback( ULONG id, const void *args, ULONG len, void **ret_ptr, ULONG *ret_len )
{
    struct syscall_frame *frame = get_syscall_frame();
    ULONG esp = (frame->esp - offsetof(struct callback_stack_layout, args_data[len])) & ~3;
    struct callback_stack_layout *stack = (struct callback_stack_layout *)esp;

    if ((char *)ntdll_get_thread_data()->kernel_stack + min_kernel_stack > (char *)&frame)
        return STATUS_STACK_OVERFLOW;

    stack->eip  = frame->eip;
    stack->id   = id;
    stack->args = stack->args_data;
    stack->len  = len;
    stack->esp  = frame->esp;
    memcpy( stack->args_data, args, len );
    return call_user_mode_callback( esp, ret_ptr, ret_len, pKiUserCallbackDispatcher, NtCurrentTeb() );
}


/***********************************************************************
 *           NtCallbackReturn  (NTDLL.@)
 */
NTSTATUS WINAPI NtCallbackReturn( void *ret_ptr, ULONG ret_len, NTSTATUS status )
{
    if (!get_syscall_frame()->prev_frame) return STATUS_NO_CALLBACK_ACTIVE;
    user_mode_callback_return( ret_ptr, ret_len, status, NtCurrentTeb() );
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
    case 0x29:
        /* __fastfail: process state is corrupted */
        rec->ExceptionCode = STATUS_STACK_BUFFER_OVERRUN;
        rec->ExceptionFlags = EXCEPTION_NONCONTINUABLE;
        rec->NumberParameters = 1;
        rec->ExceptionInformation[0] = context->Ecx;
        NtRaiseException( rec, context, FALSE );
        return TRUE;
    case 0x2d:
        if (!is_old_wow64())
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
        rec->NumberParameters = is_old_wow64() ? 1 : 3;
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
    struct syscall_frame *frame = get_syscall_frame();
    UINT i, *stack;

    if (!is_inside_syscall( ESP_sig(sigcontext) )) return FALSE;

    TRACE( "code=%x flags=%x addr=%p ip=%08x\n",
           rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress, context->Eip );
    for (i = 0; i < rec->NumberParameters; i++)
        TRACE( " info[%d]=%08lx\n", i, rec->ExceptionInformation[i] );
    TRACE(" eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x\n",
          context->Eax, context->Ebx, context->Ecx,
          context->Edx, context->Esi, context->Edi );
    TRACE(" ebp=%08x esp=%08x cs=%04x ds=%04x es=%04x fs=%04x gs=%04x flags=%08x\n",
          context->Ebp, context->Esp, context->SegCs, context->SegDs,
          context->SegEs, context->SegFs, context->SegGs, context->EFlags );

    if (ntdll_get_thread_data()->jmp_buf)
    {
        TRACE( "returning to handler\n" );
        /* push stack frame for calling longjmp */
        stack = stack_ptr;
        *(--stack) = 1;
        *(--stack) = (DWORD)ntdll_get_thread_data()->jmp_buf;
        *(--stack) = 0xdeadbabe;  /* return address */
        ESP_sig(sigcontext) = (DWORD)stack;
        EIP_sig(sigcontext) = (DWORD)longjmp;
        ntdll_get_thread_data()->jmp_buf = NULL;
    }
    else
    {
        TRACE( "returning to user mode ip=%08x ret=%08x\n", frame->eip, rec->ExceptionCode );
        EAX_sig(sigcontext) = rec->ExceptionCode;
        EBP_sig(sigcontext) = (DWORD)&frame->ebp;
        EIP_sig(sigcontext) = (DWORD)__wine_syscall_dispatcher_return;
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
    struct syscall_frame *frame = get_syscall_frame();

    /* disallow single-stepping through a syscall */

    if ((void *)EIP_sig( sigcontext ) == __wine_syscall_dispatcher)
    {
        extern void __wine_syscall_dispatcher_prolog_end(void);

        EIP_sig( sigcontext ) = (ULONG)__wine_syscall_dispatcher_prolog_end;
    }
    else if ((void *)EIP_sig( sigcontext ) == __wine_unix_call_dispatcher)
    {
        extern void __wine_unix_call_dispatcher_prolog_end(void);

        EIP_sig( sigcontext ) = (ULONG)__wine_unix_call_dispatcher_prolog_end;
    }
    else if (siginfo->si_code == 4 /* TRAP_HWBKPT */ && is_inside_syscall( ESP_sig(sigcontext) ))
    {
        TRACE_(seh)( "ignoring HWBKPT in syscall eip=%p\n", (void *)EIP_sig(sigcontext) );
        return TRUE;
    }
    else return FALSE;

    TRACE( "ignoring trap in syscall eip=%08x eflags=%08x\n", EIP_sig(sigcontext), EFL_sig(sigcontext) );

    frame->eip = *(ULONG *)ESP_sig( sigcontext );
    frame->eflags = EFL_sig(sigcontext);
    frame->restore_flags = LOWORD(CONTEXT_CONTROL);

    ECX_sig( sigcontext ) = (ULONG)frame;
    ESP_sig( sigcontext ) += sizeof(ULONG);
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
        if (!virtual_handle_fault( &rec, stack )) return;
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

    if (handle_syscall_trap( ucontext, siginfo )) return;

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
        rec.NumberParameters = is_old_wow64() ? 1 : 3;
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
        rec.ExceptionInformation[rec.NumberParameters++] = 0;
        if (is_old_wow64()) rec.ExceptionInformation[rec.NumberParameters++] = ((XSAVE_FORMAT *)xcontext.c.ExtendedRegisters)->MxCsr;
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
    HANDLE handle;

    init_handler( sigcontext );

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
    EXCEPTION_RECORD rec = { EXCEPTION_WINE_ASSERTION, EXCEPTION_NONCONTINUABLE };

    setup_exception( sigcontext, &rec );
}


/**********************************************************************
 *		quit_handler
 *
 * Handler for SIGQUIT.
 */
static void quit_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    ucontext_t *ucontext = sigcontext;

    init_handler( sigcontext );
    if (!is_inside_syscall( ESP_sig(ucontext) )) user_mode_abort_thread( 0, get_syscall_frame() );
    abort_thread( 0 );
}


/**********************************************************************
 *		usr1_handler
 *
 * Handler for SIGUSR1, used to signal a thread that it got suspended.
 */
static void usr1_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    ucontext_t *ucontext = sigcontext;

    init_handler( sigcontext );

    if (is_inside_syscall( ESP_sig(ucontext) ))
    {
        struct syscall_frame *frame = get_syscall_frame();
        ULONG64 saved_compaction = 0;
        struct xcontext *context;

        context = (struct xcontext *)(((ULONG_PTR)ESP_sig(ucontext) - sizeof(*context)) & ~15);
        if ((char *)context < (char *)ntdll_get_thread_data()->kernel_stack)
        {
            ERR_(seh)( "kernel stack overflow.\n" );
            return;
        }
        context->c.ContextFlags = CONTEXT_FULL | CONTEXT_EXCEPTION_REQUEST;
        NtGetContextThread( GetCurrentThread(), &context->c );
        if (xstate_extended_features)
        {
            if (user_shared_data->XState.CompactionEnabled)
                frame->xstate.CompactionMask |= xstate_extended_features;
            context_init_xstate( &context->c, &frame->xstate );
            saved_compaction = frame->xstate.CompactionMask;
        }
        wait_suspend( &context->c );
        if (xstate_extended_features) frame->xstate.CompactionMask = saved_compaction;
        if (context->c.ContextFlags & 0x40)
        {
            /* xstate is updated directly in frame's xstate */
            context->c.ContextFlags &= ~0x40;
            frame->restore_flags |= 0x40;
        }
        NtSetContextThread( GetCurrentThread(), &context->c );
    }
    else
    {
        struct xcontext context;

        save_context( &context, ucontext );
        context.c.ContextFlags |= CONTEXT_EXCEPTION_REPORTING;
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
    unsigned int status = STATUS_SUCCESS;

    if (len != sizeof(*info)) return STATUS_INFO_LENGTH_MISMATCH;
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

    teb->WOW32Reserved = __wine_syscall_dispatcher;
    thread_data->frame_size = frame_size;
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
 *		signal_init_process
 */
void signal_init_process(void)
{
    struct sigaction sig_act;
    struct ntdll_thread_data *thread_data = ntdll_get_thread_data();
    void *kernel_stack = (char *)thread_data->kernel_stack + kernel_stack_size;

    if (user_shared_data->XState.Size) xstate_size = user_shared_data->XState.Size - sizeof(XSAVE_FORMAT);
    frame_size = offsetof( struct syscall_frame, xstate ) + xstate_size;

    thread_data->syscall_frame = (struct syscall_frame *)(((ULONG_PTR)kernel_stack - frame_size) & ~(ULONG_PTR)63);
    x86_thread_data()->frame_size = frame_size;

    xstate_extended_features = user_shared_data->XState.EnabledFeatures & ~(UINT64)3;

    if (user_shared_data->ProcessorFeatures[PF_XSAVE_ENABLED]) syscall_flags |= SYSCALL_HAVE_XSAVE;

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


/***********************************************************************
 *           init_syscall_frame
 */
void init_syscall_frame( LPTHREAD_START_ROUTINE entry, void *arg, BOOL suspend, TEB *teb )
{
    struct x86_thread_data *thread_data = (struct x86_thread_data *)&teb->GdiTebBatch;
    struct syscall_frame *frame = ((struct ntdll_thread_data *)&teb->GdiTebBatch)->syscall_frame;
    CONTEXT *ctx, context = { CONTEXT_ALL };
    DWORD *stack;

    ldt_set_fs( thread_data->fs, teb );
    thread_data->gs = get_gs();
    assert( thread_data->frame_size == frame_size );

    context.SegCs  = get_cs();
    context.SegDs  = get_ds();
    context.SegEs  = get_ds();
    context.SegFs  = thread_data->fs;
    context.SegGs  = thread_data->gs;
    context.SegSs  = get_ds();
    context.EFlags = 0x202;
    context.Eax    = (DWORD)entry;
    context.Ebx    = (DWORD)arg;
    context.Esp    = (DWORD)teb->Tib.StackBase - 16;
    context.Eip    = (DWORD)pRtlUserThreadStart;
    context.FloatSave.ControlWord = 0x27f;
    ((XSAVE_FORMAT *)context.ExtendedRegisters)->ControlWord = 0x27f;
    ((XSAVE_FORMAT *)context.ExtendedRegisters)->MxCsr = 0x1f80;
    if ((ctx = get_cpu_area( IMAGE_FILE_MACHINE_I386 ))) *ctx = context;

    if (suspend)
    {
        context.ContextFlags |= CONTEXT_EXCEPTION_REPORTING | CONTEXT_EXCEPTION_ACTIVE;
        wait_suspend( &context );
    }

    ctx = (CONTEXT *)((ULONG_PTR)context.Esp & ~3) - 1;
    *ctx = context;
    ctx->ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT | CONTEXT_EXTENDED_REGISTERS;
    memset( &frame->xstate, 0, sizeof(frame->xstate) );
    if (user_shared_data->XState.CompactionEnabled)
        frame->xstate.CompactionMask = 0x8000000000000000 | user_shared_data->XState.EnabledFeatures;
    signal_set_full_context( ctx );

    stack = (DWORD *)ctx;
    *(--stack) = 0;
    *(--stack) = 0;
    *(--stack) = 0;
    *(--stack) = (DWORD)ctx;
    *(--stack) = 0xdeadbabe;
    frame->esp = (DWORD)stack;
    frame->eip = (DWORD)pLdrInitializeThunk;
    frame->syscall_flags = syscall_flags;

    pthread_sigmask( SIG_UNBLOCK, &server_block_set, NULL );
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
                   "leal 8(%ebp),%edx\n\t"      /* syscall_cfa */
                   /* set syscall frame */
                   "movl 20(%ebp),%ecx\n\t"     /* teb */
                   "movl 0x218(%ecx),%eax\n\t"  /* thread_data->syscall_frame */
                   "orl %eax,%eax\n\t"
                   "jnz 1f\n\t"
                   "movl %esp,%eax\n\t"
                   "subl 0x1f4(%ecx),%eax\n\t"  /* x86_thread_data()->frame_size */
                   "andl $~63,%eax\n\t"
                   "movl %eax,0x218(%ecx)\n"    /* thread_data->syscall_frame */
                   "1:\tmovl $0,(%eax)\n\t"     /* frame->syscall_flags/restore_flags */
                   "movl %edx,0x38(%eax)\n\t"   /* frame->syscall_cfa */
                   "movl $0,0x3c(%eax)\n\t"     /* frame->prev_frame */
                   /* switch to kernel stack */
                   "movl %eax,%esp\n\t"
                   "pushl %ecx\n\t"             /* teb */
                   "pushl 16(%ebp)\n\t"         /* suspend */
                   "pushl 12(%ebp)\n\t"         /* arg */
                   "pushl 8(%ebp)\n\t"          /* entry */
                   "call " __ASM_NAME("init_syscall_frame") "\n\t"
                   "addl $16,%esp\n\t"
                   "jmp " __ASM_LOCAL_LABEL("__wine_syscall_dispatcher_return") )


/***********************************************************************
 *           __wine_syscall_dispatcher
 */
__ASM_GLOBAL_FUNC( __wine_syscall_dispatcher,
                   "movl %fs:0x218,%ecx\n\t"       /* thread_data->syscall_frame */
                   "movw $0,0x02(%ecx)\n\t"        /* frame->restore_flags */
                   "popl 0x08(%ecx)\n\t"           /* frame->eip */
                   __ASM_CFI(".cfi_adjust_cfa_offset -4\n\t")
                   __ASM_CFI_REG_IS_AT1(eip, ecx, 0x08)
                   "pushfl\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "popl 0x04(%ecx)\n\t"           /* frame->eflags */
                   __ASM_CFI(".cfi_adjust_cfa_offset -4\n\t")
                   __ASM_GLOBL(__ASM_NAME("__wine_syscall_dispatcher_prolog_end")) "\n\t"
                   "movl %esp,0x0c(%ecx)\n\t"      /* frame->esp */
                   __ASM_CFI_CFA_IS_AT1(ecx, 0x0c)
                   "movw %cs,0x10(%ecx)\n\t"
                   "movw %ss,0x12(%ecx)\n\t"
                   "movw %ds,0x14(%ecx)\n\t"
                   "movw %es,0x16(%ecx)\n\t"
                   "movw %fs,0x18(%ecx)\n\t"
                   "movw %gs,0x1a(%ecx)\n\t"
                   "movl %eax,0x1c(%ecx)\n\t"
                   "movl %ebx,0x20(%ecx)\n\t"
                   __ASM_CFI_REG_IS_AT1(ebx, ecx, 0x20)
                   "movl %edi,0x2c(%ecx)\n\t"
                   __ASM_CFI_REG_IS_AT1(edi, ecx, 0x2c)
                   "movl %esi,0x30(%ecx)\n\t"
                   __ASM_CFI_REG_IS_AT1(esi, ecx, 0x30)
                   "movl %ebp,0x34(%ecx)\n\t"
                   __ASM_CFI_REG_IS_AT1(ebp, ecx, 0x34)
                   "leal 0x34(%ecx),%ebp\n\t"
                   __ASM_CFI_CFA_IS_AT1(ebp, 0x58)
                   __ASM_CFI_REG_IS_AT1(eip, ebp, 0x54)
                   __ASM_CFI_REG_IS_AT1(ebx, ebp, 0x6c)
                   __ASM_CFI_REG_IS_AT1(edi, ebp, 0x78)
                   __ASM_CFI_REG_IS_AT1(esi, ebp, 0x7c)
                   __ASM_CFI_REG_IS_AT1(ebp, ebp, 0x00)
                   "leal 4(%esp),%esi\n\t"         /* first argument */
                   "movl %eax,%ebx\n\t"
                   "shrl $8,%ebx\n\t"
                   "andl $0x30,%ebx\n\t"           /* syscall table number */
                   "addl %fs:0x214,%ebx\n\t"       /* thread_data->syscall_table */
                   "testl $1,(%ecx)\n\t"           /* frame->syscall_flags & SYSCALL_HAVE_XSAVE */
                   "jz 2f\n\t"
                   "movl 0x7ffe03d8,%eax\n\t"      /* user_shared_data->XState.EnabledFeatures */
                   "xorl %edx,%edx\n\t"
                   "andl $7,%eax\n\t"
                   "xorl %edi,%edi\n\t"
                   "movl %edi,0x240(%ecx)\n\t"
                   "movl %edi,0x244(%ecx)\n\t"
                   "movl %edi,0x248(%ecx)\n\t"
                   "movl %edi,0x24c(%ecx)\n\t"
                   "movl %edi,0x250(%ecx)\n\t"
                   "movl %edi,0x254(%ecx)\n\t"
                   "testl $2,0x7ffe03ec\n\t"       /* user_shared_data->XState.CompactionEnabled */
                   "jz 1f\n\t"
                   "movl %edi,0x258(%ecx)\n\t"
                   "movl %edi,0x25c(%ecx)\n\t"
                   "movl %edi,0x260(%ecx)\n\t"
                   "movl %edi,0x264(%ecx)\n\t"
                   "movl %edi,0x268(%ecx)\n\t"
                   "movl %edi,0x26c(%ecx)\n\t"
                   "movl %edi,0x270(%ecx)\n\t"
                   "movl %edi,0x274(%ecx)\n\t"
                   "movl %edi,0x278(%ecx)\n\t"
                   "movl %edi,0x27c(%ecx)\n\t"
                   /* The xsavec instruction is not supported by
                    * binutils < 2.25. */
                   ".byte 0x0f, 0xc7, 0x61, 0x40\n\t" /* xsavec 0x40(%ecx) */
                   "jmp 4f\n"
                   "1:\txsave 0x40(%ecx)\n\t"
                   "jmp 4f\n"
                   "2:\tcmpb $0,0x7ffe027a\n\t"    /* user_shared_data->ProcessorFeatures[PF_XMMI_INSTRUCTIONS_AVAILABLE] */
                   "jz 3f\n\t"
                   "fxsave 0x40(%ecx)\n\t"
                   "jmp 4f\n"
                   "3:\tfnsave 0x40(%ecx)\n\t"
                   "fwait\n"
                   /* switch to kernel stack */
                   "4:\tmovl %ecx,%esp\n\t"
                   /* we're now on the kernel stack, stitch unwind info with previous frame */
                   __ASM_CFI_CFA_IS_AT1(ebp, 0x04) /* frame->syscall_cfa */
                   __ASM_CFI(".cfi_offset %eip,-4\n\t")
                   __ASM_CFI(".cfi_offset %ebp,-8\n\t")
                   __ASM_CFI(".cfi_offset %ebx,-12\n\t")
                   __ASM_CFI(".cfi_offset %esi,-16\n\t")
                   __ASM_CFI(".cfi_offset %edi,-20\n\t")
                   "movl 0x1c(%esp),%edx\n\t"      /* frame->eax */
                   "andl $0xfff,%edx\n\t"          /* syscall number */
                   "cmpl 8(%ebx),%edx\n\t"         /* table->ServiceLimit */
                   "jae "__ASM_LOCAL_LABEL("bad_syscall") "\n\t"
                   "pushl 0x1c(%esp)\n\t"          /* frame->eax */
                   "movl 12(%ebx),%eax\n\t"        /* table->ArgumentTable */
                   "movzbl (%eax,%edx,1),%ecx\n\t"
                   "movl (%ebx),%eax\n\t"          /* table->ServiceTable */
                   "subl %ecx,%esp\n\t"
                   "shrl $2,%ecx\n\t"
                   "andl $~15,%esp\n\t"
                   "movl %esp,%edi\n\t"
                   "cld\n\t"
                   "rep; movsl\n\t"
                   "movl (%eax,%edx,4),%edi\n\t"
                   "testl $1,%fs:0x21c\n\t"        /* thread_data->syscall_trace */
                   "jnz " __ASM_LOCAL_LABEL("trace_syscall") "\n\t"
                   "call *%edi\n\t"
                   "leal -0x34(%ebp),%esp\n"

                   __ASM_LOCAL_LABEL("__wine_syscall_dispatcher_return") ":\t"
                   "movl 0(%esp),%ecx\n\t"         /* frame->syscall_flags + (frame->restore_flags << 16) */
                   "testl $0x68 << 16,%ecx\n\t"    /* CONTEXT_FLOATING_POINT | CONTEXT_EXTENDED_REGISTERS | CONTEXT_XSAVE */
                   "jz 3f\n\t"
                   "testl $1,%ecx\n\t"             /* SYSCALL_HAVE_XSAVE */
                   "jz 1f\n\t"
                   "movl %eax,%esi\n\t"
                   "movl 0x7ffe03d8,%eax\n\t"      /* user_shared_data->XState.EnabledFeatures */
                   "movl 0x7ffe03dc,%edx\n\t"
                   "xrstor 0x40(%esp)\n\t"
                   "movl %esi,%eax\n\t"
                   "jmp 3f\n"
                   "1:\tcmpb $0,0x7ffe027a\n\t"    /* user_shared_data->ProcessorFeatures[PF_XMMI_INSTRUCTIONS_AVAILABLE] */
                   "jz 2f\n\t"
                   "fxrstor 0x40(%esp)\n\t"
                   "jmp 3f\n"
                   "2:\tfrstor 0x40(%esp)\n\t"
                   "fwait\n"
                   "3:\tmovl 0x2c(%esp),%edi\n\t"
                   "movl 0x30(%esp),%esi\n\t"
                   "movl 0x34(%esp),%ebp\n\t"
                   /* push ebp-based kernel stack cfi */
                   __ASM_CFI("\t.cfi_remember_state\n")
                   __ASM_CFI_CFA_IS_AT2(esp, 0xb8, 0x00) /* frame->syscall_cfa */
                   "testl $0x7 << 16,%ecx\n\t"     /* CONTEXT_CONTROL | CONTEXT_SEGMENTS | CONTEXT_INTEGER */
                   "jnz 1f\n\t"
                   "movl 0x20(%esp),%ebx\n\t"
                   "movl 0x08(%esp),%ecx\n\t"      /* frame->eip */
                   /* push esp-based kernel stack cfi */
                   __ASM_CFI("\t.cfi_remember_state\n")
                   /* switch to user stack */
                   "movl 0x0c(%esp),%esp\n\t"      /* frame->esp */
                   __ASM_CFI(".cfi_def_cfa %esp,0\n\t")
                   __ASM_CFI(".cfi_register %eip, %ecx\n\t")
                   __ASM_CFI(".cfi_same_value %ebp\n\t")
                   __ASM_CFI(".cfi_same_value %ebx\n\t")
                   __ASM_CFI(".cfi_same_value %esi\n\t")
                   __ASM_CFI(".cfi_same_value %edi\n\t")
                   "pushl %ecx\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "ret\n"
                   /* pop esp-based kernel stack cfi */
                   __ASM_CFI("\t.cfi_restore_state\n")

                   "1:\ttestl $0x2 << 16,%ecx\n\t" /* CONTEXT_INTEGER */
                   "jz 1f\n\t"
                   "movl 0x1c(%esp),%eax\n\t"
                   "movl 0x24(%esp),%ecx\n\t"
                   "movl 0x28(%esp),%edx\n"
                   "1:\tmovl 0x0c(%esp),%ebx\n\t"  /* frame->esp */
                   "movw 0x12(%esp),%ss\n\t"
                   /* switch to user stack */
                   "xchgl %ebx,%esp\n\t"
                   __ASM_CFI(".cfi_def_cfa %esp,0\n\t")
                   __ASM_CFI(".cfi_register %eip, %ecx\n\t")
                   __ASM_CFI(".cfi_same_value %ebp\n\t")
                   __ASM_CFI(".cfi_same_value %ebx\n\t")
                   __ASM_CFI(".cfi_same_value %esi\n\t")
                   __ASM_CFI(".cfi_same_value %edi\n\t")
                   "pushl 0x04(%ebx)\n\t"          /* frame->eflags */
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "pushl 0x10(%ebx)\n\t"          /* frame->cs */
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "pushl 0x08(%ebx)\n\t"          /* frame->eip */
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   __ASM_CFI(".cfi_rel_offset %eip, 0\n\t")
                   "pushl 0x14(%ebx)\n\t"          /* frame->ds */
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "movw 0x16(%ebx),%es\n\t"
                   "movw 0x18(%ebx),%fs\n\t"
                   "movw 0x1a(%ebx),%gs\n\t"
                   "movl 0x20(%ebx),%ebx\n\t"
                   __ASM_CFI(".cfi_same_value %ebx\n\t")
                   "popl %ds\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset -4\n\t")
                   "iret\n"

                   /* pop ebp-based kernel stack cfi */
                   __ASM_CFI("\t.cfi_restore_state\n")
                   __ASM_LOCAL_LABEL("trace_syscall") ":\n\t"
                   "movl %esp,%eax\n\t"
                   "subl $16,%esp\n\t"
                   "movl -0x38(%ebp),%esi\n\t"     /* syscall_id */
                   "movl %esi,(%esp)\n\t"          /* id */
                   "movl %eax,4(%esp)\n\t"         /* args */
                   "movl %esi,%edx\n\t"
                   "andl $0xfff,%edx\n\t"          /* syscall number */
                   "movl 12(%ebx),%eax\n\t"        /* table->ArgumentTable */
                   "movzbl (%eax,%edx,1),%edx\n\t"
                   "movl %edx,8(%esp)\n\t"         /* len */
                   "call " __ASM_NAME("trace_syscall") "\n\t"
                   "addl $16,%esp\n\t"
                   "call *%edi\n"

                   __ASM_LOCAL_LABEL("trace_syscall_ret") ":\n\t"
                   "pushl %eax\n\t"
                   "pushl %eax\n\t"
                   "pushl %eax\n\t"                /* retval */
                   "pushl %esi\n\t"                /* id */
                   "call " __ASM_NAME("trace_sysret") "\n\t"
                   "movl 8(%esp),%eax\n\t"         /* retval */
                   "leal -0x34(%ebp),%esp\n\t"     /* frame */
                   "jmp " __ASM_LOCAL_LABEL("__wine_syscall_dispatcher_return") "\n"

                   __ASM_LOCAL_LABEL("bad_syscall") ":\n\t"
                   "movl $0xc000001c,%eax\n\t"     /* STATUS_INVALID_SYSTEM_SERVICE */
                   "jmp " __ASM_LOCAL_LABEL("__wine_syscall_dispatcher_return") )

__ASM_GLOBAL_FUNC( __wine_syscall_dispatcher_return,
                   "movl -0x38(%ebp),%esi\n\t"     /* syscall id */
                   "leal -0x34(%ebp),%esp\n\t"     /* frame */
                   "testl $1,%fs:0x21c\n\t"        /* thread_data->syscall_trace */
                   "jnz " __ASM_LOCAL_LABEL("trace_syscall_ret") "\n\t"
                   "jmp " __ASM_LOCAL_LABEL("__wine_syscall_dispatcher_return") )


/***********************************************************************
 *           __wine_unix_call_dispatcher
 */
__ASM_GLOBAL_FUNC( __wine_unix_call_dispatcher,
                   "movl %fs:0x218,%ecx\n\t"   /* thread_data->syscall_frame */
                   "movw $0,0x02(%ecx)\n\t"    /* frame->restore_flags */
                   "popl 0x08(%ecx)\n\t"       /* frame->eip */
                   __ASM_CFI(".cfi_adjust_cfa_offset -4\n\t")
                   __ASM_CFI_REG_IS_AT1(eip, ecx, 0x08)
                   __ASM_GLOBL(__ASM_NAME("__wine_unix_call_dispatcher_prolog_end")) "\n\t"
                   "leal 0x10(%esp),%edx\n\t"
                   "movl %edx,0x0c(%ecx)\n\t"  /* frame->esp */
                   __ASM_CFI_CFA_IS_AT1(ecx, 0x0c)
                   "movw %cs,0x10(%ecx)\n\t"
                   "movw %ss,0x12(%ecx)\n\t"
                   "movw %ds,0x14(%ecx)\n\t"
                   "movw %es,0x16(%ecx)\n\t"
                   "movw %fs,0x18(%ecx)\n\t"
                   "movw %gs,0x1a(%ecx)\n\t"
                   "movl %ebx,0x20(%ecx)\n\t"
                   __ASM_CFI_REG_IS_AT1(ebx, ecx, 0x20)
                   "movl %edi,0x2c(%ecx)\n\t"
                   __ASM_CFI_REG_IS_AT1(edi, ecx, 0x2c)
                   "movl %esi,0x30(%ecx)\n\t"
                   __ASM_CFI_REG_IS_AT1(esi, ecx, 0x30)
                   "movl %ebp,0x34(%ecx)\n\t"
                   __ASM_CFI_REG_IS_AT1(ebp, ecx, 0x34)
                   "movl 12(%esp),%edx\n\t"    /* args */
                   "movl %edx,-16(%ecx)\n\t"
                   "movl (%esp),%eax\n\t"      /* handle */
                   "movl 8(%esp),%edx\n\t"     /* code */
                   /* switch to kernel stack */
                   "leal -16(%ecx),%esp\n\t"
                   /* we're now on the kernel stack, stitch unwind info with previous frame */
                   __ASM_CFI_CFA_IS_AT2(esp, 0xc8, 0x00) /* frame->syscall_cfa */
                   __ASM_CFI(".cfi_offset %eip,-4\n\t")
                   __ASM_CFI(".cfi_offset %ebp,-8\n\t")
                   __ASM_CFI(".cfi_offset %ebx,-12\n\t")
                   __ASM_CFI(".cfi_offset %esi,-16\n\t")
                   __ASM_CFI(".cfi_offset %edi,-20\n\t")
                   "call *(%eax,%edx,4)\n\t"
                   "leal 16(%esp),%esp\n\t"
                   "testw $0xffff,2(%esp)\n\t" /* frame->restore_flags */
                   "jnz " __ASM_LOCAL_LABEL("__wine_syscall_dispatcher_return") "\n\t"
                   "movl 0x08(%esp),%ecx\n\t"  /* frame->eip */
                   /* switch to user stack */
                   "movl 0x0c(%esp),%esp\n\t"  /* frame->esp */
                   __ASM_CFI(".cfi_def_cfa %esp,0\n\t")
                   __ASM_CFI(".cfi_register %eip, %ecx\n\t")
                   __ASM_CFI(".cfi_undefined %ebp\n\t")
                   __ASM_CFI(".cfi_undefined %ebx\n\t")
                   __ASM_CFI(".cfi_undefined %esi\n\t")
                   __ASM_CFI(".cfi_undefined %edi\n\t")
                   "pushl %ecx\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "ret" )

#endif  /* __i386__ */
