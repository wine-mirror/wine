/*
 * ARM64 signal handling routines
 *
 * Copyright 2010-2013 André Hentschel
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

#ifdef __aarch64__

#include "config.h"
#include "wine/port.h"

#include <assert.h>
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/library.h"
#include "wine/exception.h"
#include "ntdll_misc.h"
#include "wine/debug.h"
#include "winnt.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);
WINE_DECLARE_DEBUG_CHANNEL(relay);

static pthread_key_t teb_key;

/***********************************************************************
 * signal context platform-specific definitions
 */
#ifdef linux

/* All Registers access - only for local access */
# define REG_sig(reg_name, context) ((context)->uc_mcontext.reg_name)
# define REGn_sig(reg_num, context) ((context)->uc_mcontext.regs[reg_num])

/* Special Registers access  */
# define SP_sig(context)            REG_sig(sp, context)    /* Stack pointer */
# define PC_sig(context)            REG_sig(pc, context)    /* Program counter */
# define PSTATE_sig(context)        REG_sig(pstate, context) /* Current State Register */
# define FP_sig(context)            REGn_sig(29, context)    /* Frame pointer */
# define LR_sig(context)            REGn_sig(30, context)    /* Link Register */

static struct _aarch64_ctx *get_extended_sigcontext( ucontext_t *sigcontext, unsigned int magic )
{
    struct _aarch64_ctx *ctx = (struct _aarch64_ctx *)sigcontext->uc_mcontext.__reserved;
    while ((char *)ctx < (char *)(&sigcontext->uc_mcontext + 1) && ctx->magic && ctx->size)
    {
        if (ctx->magic == magic) return ctx;
        ctx = (struct _aarch64_ctx *)((char *)ctx + ctx->size);
    }
    return NULL;
}

static struct fpsimd_context *get_fpsimd_context( ucontext_t *sigcontext )
{
    return (struct fpsimd_context *)get_extended_sigcontext( sigcontext, FPSIMD_MAGIC );
}

static DWORD64 get_fault_esr( ucontext_t *sigcontext )
{
    struct esr_context *esr = (struct esr_context *)get_extended_sigcontext( sigcontext, ESR_MAGIC );
    if (esr) return esr->esr;
    return 0;
}

#endif /* linux */

static const size_t teb_size = 0x2000;  /* we reserve two pages for the TEB */
static const size_t signal_stack_size = max( MINSIGSTKSZ, 8192 );

/* stack layout when calling an exception raise function */
struct stack_layout
{
    CONTEXT           context;
    EXCEPTION_RECORD  rec;
    void             *redzone[2];
};

typedef int (*wine_signal_handler)(unsigned int sig);

static wine_signal_handler handlers[256];

struct arm64_thread_data
{
    void     *exit_frame;    /* exit frame pointer */
    CONTEXT  *context;       /* context to set with SIGUSR2 */
};

C_ASSERT( sizeof(struct arm64_thread_data) <= sizeof(((TEB *)0)->SystemReserved2) );
C_ASSERT( offsetof( TEB, SystemReserved2 ) + offsetof( struct arm64_thread_data, exit_frame ) == 0x300 );

static inline struct arm64_thread_data *arm64_thread_data(void)
{
    return (struct arm64_thread_data *)NtCurrentTeb()->SystemReserved2;
}

/***********************************************************************
 *           dispatch_signal
 */
static inline int dispatch_signal(unsigned int sig)
{
    if (handlers[sig] == NULL) return 0;
    return handlers[sig](sig);
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

/*******************************************************************
 *         is_valid_frame
 */
static inline BOOL is_valid_frame( ULONG_PTR frame )
{
    if (frame & 7) return FALSE;
    return ((void *)frame >= NtCurrentTeb()->Tib.StackLimit &&
            (void **)frame < (void **)NtCurrentTeb()->Tib.StackBase - 1);
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
    DWORD i;

    context->ContextFlags = CONTEXT_FULL;
    context->u.s.Fp = FP_sig(sigcontext);     /* Frame pointer */
    context->u.s.Lr = LR_sig(sigcontext);     /* Link register */
    context->Sp     = SP_sig(sigcontext);     /* Stack pointer */
    context->Pc     = PC_sig(sigcontext);     /* Program Counter */
    context->Cpsr   = PSTATE_sig(sigcontext); /* Current State Register */
    for (i = 0; i <= 28; i++) context->u.X[i] = REGn_sig( i, sigcontext );
}


/***********************************************************************
 *           restore_context
 *
 * Build a sigcontext from the register values.
 */
static void restore_context( const CONTEXT *context, ucontext_t *sigcontext )
{
    DWORD i;

    FP_sig(sigcontext)     = context->u.s.Fp; /* Frame pointer */
    LR_sig(sigcontext)     = context->u.s.Lr; /* Link register */
    SP_sig(sigcontext)     = context->Sp;     /* Stack pointer */
    PC_sig(sigcontext)     = context->Pc;     /* Program Counter */
    PSTATE_sig(sigcontext) = context->Cpsr;   /* Current State Register */
    for (i = 0; i <= 28; i++) REGn_sig( i, sigcontext ) = context->u.X[i];
}


/***********************************************************************
 *           save_fpu
 *
 * Set the FPU context from a sigcontext.
 */
static void save_fpu( CONTEXT *context, ucontext_t *sigcontext )
{
    struct fpsimd_context *fp = get_fpsimd_context( sigcontext );

    if (!fp) return;
    context->ContextFlags |= CONTEXT_FLOATING_POINT;
    context->Fpcr = fp->fpcr;
    context->Fpsr = fp->fpsr;
    memcpy( context->V, fp->vregs, sizeof(context->V) );
}


/***********************************************************************
 *           restore_fpu
 *
 * Restore the FPU context to a sigcontext.
 */
static void restore_fpu( CONTEXT *context, ucontext_t *sigcontext )
{
    struct fpsimd_context *fp = get_fpsimd_context( sigcontext );

    if (!fp) return;
    fp->fpcr = context->Fpcr;
    fp->fpsr = context->Fpsr;
    memcpy( fp->vregs, context->V, sizeof(fp->vregs) );
}

/***********************************************************************
 *		RtlCaptureContext (NTDLL.@)
 */
__ASM_STDCALL_FUNC( RtlCaptureContext, 8,
                    "stp x1, x2, [x0, #0x10]\n\t"    /* context->X1,X2 */
                    "stp x3, x4, [x0, #0x20]\n\t"    /* context->X3,X4 */
                    "stp x5, x6, [x0, #0x30]\n\t"    /* context->X5,X6 */
                    "stp x7, x8, [x0, #0x40]\n\t"    /* context->X7,X8 */
                    "stp x9, x10, [x0, #0x50]\n\t"   /* context->X9,X10 */
                    "stp x11, x12, [x0, #0x60]\n\t"  /* context->X11,X12 */
                    "stp x13, x14, [x0, #0x70]\n\t"  /* context->X13,X14 */
                    "stp x15, x16, [x0, #0x80]\n\t"  /* context->X15,X16 */
                    "stp x17, x18, [x0, #0x90]\n\t"  /* context->X17,X18 */
                    "stp x19, x20, [x0, #0xa0]\n\t"  /* context->X19,X20 */
                    "stp x21, x22, [x0, #0xb0]\n\t"  /* context->X21,X22 */
                    "stp x23, x24, [x0, #0xc0]\n\t"  /* context->X23,X24 */
                    "stp x25, x26, [x0, #0xd0]\n\t"  /* context->X25,X26 */
                    "stp x27, x28, [x0, #0xe0]\n\t"  /* context->X27,X28 */
                    "ldp x1, x2, [x29]\n\t"
                    "stp x1, x2, [x0, #0xf0]\n\t"    /* context->Fp,Lr */
                    "add x1, x29, #0x10\n\t"
                    "stp x1, x30, [x0, #0x100]\n\t"  /* context->Sp,Pc */
                    "mov w1, #0x400000\n\t"          /* CONTEXT_ARM64 */
                    "add w1, w1, #0x3\n\t"           /* CONTEXT_FULL */
                    "str w1, [x0]\n\t"               /* context->ContextFlags */
                    "mrs x1, NZCV\n\t"
                    "str w1, [x0, #0x4]\n\t"         /* context->Cpsr */
                    "ret" )

/***********************************************************************
 *           set_cpu_context
 *
 * Set the new CPU context.
 */
static void set_cpu_context( const CONTEXT *context )
{
    interlocked_xchg_ptr( (void **)&arm64_thread_data()->context, (void *)context );
    raise( SIGUSR2 );
}

/***********************************************************************
 *           get_server_context_flags
 *
 * Convert CPU-specific flags to generic server flags
 */
static unsigned int get_server_context_flags( DWORD flags )
{
    unsigned int ret = 0;

    flags &= ~CONTEXT_ARM64;  /* get rid of CPU id */
    if (flags & CONTEXT_CONTROL) ret |= SERVER_CTX_CONTROL;
    if (flags & CONTEXT_INTEGER) ret |= SERVER_CTX_INTEGER;
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
    flags &= ~CONTEXT_ARM64;  /* get rid of CPU id */
    if (flags & CONTEXT_CONTROL)
    {
        to->u.s.Fp  = from->u.s.Fp;
        to->u.s.Lr  = from->u.s.Lr;
        to->Sp      = from->Sp;
        to->Pc      = from->Pc;
        to->Cpsr    = from->Cpsr;
    }
    if (flags & CONTEXT_INTEGER)
    {
        memcpy( to->u.X, from->u.X, sizeof(to->u.X) );
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
        memcpy( to->V, from->V, sizeof(to->V) );
        to->Fpcr = from->Fpcr;
        to->Fpsr = from->Fpsr;
    }
    if (flags & CONTEXT_DEBUG_REGISTERS)
    {
        memcpy( to->Bcr, from->Bcr, sizeof(to->Bcr) );
        memcpy( to->Bvr, from->Bvr, sizeof(to->Bvr) );
        memcpy( to->Wcr, from->Wcr, sizeof(to->Wcr) );
        memcpy( to->Wvr, from->Wvr, sizeof(to->Wvr) );
    }
}

/***********************************************************************
 *           context_to_server
 *
 * Convert a register context to the server format.
 */
NTSTATUS context_to_server( context_t *to, const CONTEXT *from )
{
    DWORD i, flags = from->ContextFlags & ~CONTEXT_ARM64;  /* get rid of CPU id */

    memset( to, 0, sizeof(*to) );
    to->cpu = CPU_ARM64;

    if (flags & CONTEXT_CONTROL)
    {
        to->flags |= SERVER_CTX_CONTROL;
        to->integer.arm64_regs.x[29] = from->u.s.Fp;
        to->integer.arm64_regs.x[30] = from->u.s.Lr;
        to->ctl.arm64_regs.sp     = from->Sp;
        to->ctl.arm64_regs.pc     = from->Pc;
        to->ctl.arm64_regs.pstate = from->Cpsr;
    }
    if (flags & CONTEXT_INTEGER)
    {
        to->flags |= SERVER_CTX_INTEGER;
        for (i = 0; i <= 28; i++) to->integer.arm64_regs.x[i] = from->u.X[i];
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
        to->flags |= SERVER_CTX_FLOATING_POINT;
        for (i = 0; i < 32; i++)
        {
            to->fp.arm64_regs.q[i].low = from->V[i].s.Low;
            to->fp.arm64_regs.q[i].high = from->V[i].s.High;
        }
        to->fp.arm64_regs.fpcr = from->Fpcr;
        to->fp.arm64_regs.fpsr = from->Fpsr;
    }
    if (flags & CONTEXT_DEBUG_REGISTERS)
    {
        to->flags |= SERVER_CTX_DEBUG_REGISTERS;
        for (i = 0; i < ARM64_MAX_BREAKPOINTS; i++) to->debug.arm64_regs.bcr[i] = from->Bcr[i];
        for (i = 0; i < ARM64_MAX_BREAKPOINTS; i++) to->debug.arm64_regs.bvr[i] = from->Bvr[i];
        for (i = 0; i < ARM64_MAX_WATCHPOINTS; i++) to->debug.arm64_regs.wcr[i] = from->Wcr[i];
        for (i = 0; i < ARM64_MAX_WATCHPOINTS; i++) to->debug.arm64_regs.wvr[i] = from->Wvr[i];
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
    DWORD i;

    if (from->cpu != CPU_ARM64) return STATUS_INVALID_PARAMETER;

    to->ContextFlags = CONTEXT_ARM64;
    if (from->flags & SERVER_CTX_CONTROL)
    {
        to->ContextFlags |= CONTEXT_CONTROL;
        to->u.s.Fp = from->integer.arm64_regs.x[29];
        to->u.s.Lr = from->integer.arm64_regs.x[30];
        to->Sp     = from->ctl.arm64_regs.sp;
        to->Pc     = from->ctl.arm64_regs.pc;
        to->Cpsr   = from->ctl.arm64_regs.pstate;
    }
    if (from->flags & SERVER_CTX_INTEGER)
    {
        to->ContextFlags |= CONTEXT_INTEGER;
        for (i = 0; i <= 28; i++) to->u.X[i] = from->integer.arm64_regs.x[i];
    }
    if (from->flags & SERVER_CTX_FLOATING_POINT)
    {
        to->ContextFlags |= CONTEXT_FLOATING_POINT;
        for (i = 0; i < 32; i++)
        {
            to->V[i].s.Low = from->fp.arm64_regs.q[i].low;
            to->V[i].s.High = from->fp.arm64_regs.q[i].high;
        }
        to->Fpcr = from->fp.arm64_regs.fpcr;
        to->Fpsr = from->fp.arm64_regs.fpsr;
    }
    if (from->flags & SERVER_CTX_DEBUG_REGISTERS)
    {
        to->ContextFlags |= CONTEXT_DEBUG_REGISTERS;
        for (i = 0; i < ARM64_MAX_BREAKPOINTS; i++) to->Bcr[i] = from->debug.arm64_regs.bcr[i];
        for (i = 0; i < ARM64_MAX_BREAKPOINTS; i++) to->Bvr[i] = from->debug.arm64_regs.bvr[i];
        for (i = 0; i < ARM64_MAX_WATCHPOINTS; i++) to->Wcr[i] = from->debug.arm64_regs.wcr[i];
        for (i = 0; i < ARM64_MAX_WATCHPOINTS; i++) to->Wvr[i] = from->debug.arm64_regs.wvr[i];
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
    BOOL self = (handle == GetCurrentThread());

    if (self && (context->ContextFlags & (CONTEXT_DEBUG_REGISTERS & ~CONTEXT_ARM64)))
        self = FALSE;

    if (!self)
    {
        context_t server_context;
        context_to_server( &server_context, context );
        ret = set_thread_context( handle, &server_context, &self );
    }
    if (self && ret == STATUS_SUCCESS) set_cpu_context( context );
    return ret;
}


/***********************************************************************
 *              NtGetContextThread  (NTDLL.@)
 *              ZwGetContextThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtGetContextThread( HANDLE handle, CONTEXT *context )
{
    NTSTATUS ret;
    DWORD needed_flags = context->ContextFlags;
    BOOL self = (handle == GetCurrentThread());

    if (!self)
    {
        context_t server_context;
        unsigned int server_flags = get_server_context_flags( context->ContextFlags );

        if ((ret = get_thread_context( handle, &server_context, server_flags, &self ))) return ret;
        if ((ret = context_from_server( context, &server_context ))) return ret;
        needed_flags &= ~context->ContextFlags;
    }

    if (self && needed_flags)
    {
        CONTEXT ctx;
        RtlCaptureContext( &ctx );
        copy_context( context, &ctx, ctx.ContextFlags & needed_flags );
        context->ContextFlags |= ctx.ContextFlags & needed_flags;
    }
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           libunwind_virtual_unwind
 *
 * Equivalent of RtlVirtualUnwind for builtin modules.
 */
static NTSTATUS libunwind_virtual_unwind( ULONG_PTR ip, ULONG_PTR *frame, CONTEXT *context,
                                          PEXCEPTION_ROUTINE *handler, void **handler_data )
{
#ifdef HAVE_LIBUNWIND
    unw_context_t unw_context;
    unw_cursor_t cursor;
    unw_proc_info_t info;
    int rc;

    memcpy( unw_context.uc_mcontext.regs, context->u.X, sizeof(context->u.X) );
    unw_context.uc_mcontext.sp = context->Sp;
    unw_context.uc_mcontext.pc = context->Pc;

    rc = unw_init_local( &cursor, &unw_context );
    if (rc != UNW_ESUCCESS)
    {
        WARN( "setup failed: %d\n", rc );
        return STATUS_INVALID_DISPOSITION;
    }
    rc = unw_get_proc_info( &cursor, &info );
    if (rc != UNW_ESUCCESS && rc != -UNW_ENOINFO)
    {
        WARN( "failed to get info: %d\n", rc );
        return STATUS_INVALID_DISPOSITION;
    }
    if (rc == -UNW_ENOINFO || ip < info.start_ip || ip > info.end_ip)
    {
        TRACE( "no info found for %lx ip %lx-%lx, assuming leaf function\n",
               ip, info.start_ip, info.end_ip );
        *handler = NULL;
        *frame = context->Sp;
        context->Pc = context->u.s.Lr;
        context->Sp = context->Sp + sizeof(ULONG64);
        return STATUS_SUCCESS;
    }

    TRACE( "ip %#lx function %#lx-%#lx personality %#lx lsda %#lx fde %#lx\n",
           ip, (unsigned long)info.start_ip, (unsigned long)info.end_ip, (unsigned long)info.handler,
           (unsigned long)info.lsda, (unsigned long)info.unwind_info );

    rc = unw_step( &cursor );
    if (rc < 0)
    {
        WARN( "failed to unwind: %d %d\n", rc, UNW_ENOINFO );
        return STATUS_INVALID_DISPOSITION;
    }

    *handler = (void *)info.handler;
    *handler_data = (void *)info.lsda;
    *frame = context->Sp;
    context->Pc = context->u.s.Lr;
    unw_get_reg( &cursor, UNW_AARCH64_X0,  (unw_word_t *)&context->u.s.X0 );
    unw_get_reg( &cursor, UNW_AARCH64_X1,  (unw_word_t *)&context->u.s.X1 );
    unw_get_reg( &cursor, UNW_AARCH64_X2,  (unw_word_t *)&context->u.s.X2 );
    unw_get_reg( &cursor, UNW_AARCH64_X3,  (unw_word_t *)&context->u.s.X3 );
    unw_get_reg( &cursor, UNW_AARCH64_X4,  (unw_word_t *)&context->u.s.X4 );
    unw_get_reg( &cursor, UNW_AARCH64_X5,  (unw_word_t *)&context->u.s.X5 );
    unw_get_reg( &cursor, UNW_AARCH64_X6,  (unw_word_t *)&context->u.s.X6 );
    unw_get_reg( &cursor, UNW_AARCH64_X7,  (unw_word_t *)&context->u.s.X7 );
    unw_get_reg( &cursor, UNW_AARCH64_X8,  (unw_word_t *)&context->u.s.X8 );
    unw_get_reg( &cursor, UNW_AARCH64_X9,  (unw_word_t *)&context->u.s.X9 );
    unw_get_reg( &cursor, UNW_AARCH64_X10, (unw_word_t *)&context->u.s.X10 );
    unw_get_reg( &cursor, UNW_AARCH64_X11, (unw_word_t *)&context->u.s.X11 );
    unw_get_reg( &cursor, UNW_AARCH64_X12, (unw_word_t *)&context->u.s.X12 );
    unw_get_reg( &cursor, UNW_AARCH64_X13, (unw_word_t *)&context->u.s.X13 );
    unw_get_reg( &cursor, UNW_AARCH64_X14, (unw_word_t *)&context->u.s.X14 );
    unw_get_reg( &cursor, UNW_AARCH64_X15, (unw_word_t *)&context->u.s.X15 );
    unw_get_reg( &cursor, UNW_AARCH64_X16, (unw_word_t *)&context->u.s.X16 );
    unw_get_reg( &cursor, UNW_AARCH64_X17, (unw_word_t *)&context->u.s.X17 );
    unw_get_reg( &cursor, UNW_AARCH64_X18, (unw_word_t *)&context->u.s.X18 );
    unw_get_reg( &cursor, UNW_AARCH64_X19, (unw_word_t *)&context->u.s.X19 );
    unw_get_reg( &cursor, UNW_AARCH64_X20, (unw_word_t *)&context->u.s.X20 );
    unw_get_reg( &cursor, UNW_AARCH64_X21, (unw_word_t *)&context->u.s.X21 );
    unw_get_reg( &cursor, UNW_AARCH64_X22, (unw_word_t *)&context->u.s.X22 );
    unw_get_reg( &cursor, UNW_AARCH64_X23, (unw_word_t *)&context->u.s.X23 );
    unw_get_reg( &cursor, UNW_AARCH64_X24, (unw_word_t *)&context->u.s.X24 );
    unw_get_reg( &cursor, UNW_AARCH64_X25, (unw_word_t *)&context->u.s.X25 );
    unw_get_reg( &cursor, UNW_AARCH64_X26, (unw_word_t *)&context->u.s.X26 );
    unw_get_reg( &cursor, UNW_AARCH64_X27, (unw_word_t *)&context->u.s.X27 );
    unw_get_reg( &cursor, UNW_AARCH64_X28, (unw_word_t *)&context->u.s.X28 );
    unw_get_reg( &cursor, UNW_AARCH64_X29, (unw_word_t *)&context->u.s.Fp );
    unw_get_reg( &cursor, UNW_AARCH64_X30, (unw_word_t *)&context->u.s.Lr );
    unw_get_reg( &cursor, UNW_AARCH64_SP,  (unw_word_t *)&context->Sp );

    TRACE( "next function pc=%016lx%s\n", context->Pc, rc ? "" : " (last frame)" );
    TRACE("  x0=%016lx  x1=%016lx  x2=%016lx  x3=%016lx\n",
          context->u.s.X0, context->u.s.X1, context->u.s.X2, context->u.s.X3 );
    TRACE("  x4=%016lx  x5=%016lx  x6=%016lx  x7=%016lx\n",
          context->u.s.X4, context->u.s.X5, context->u.s.X6, context->u.s.X7 );
    TRACE("  x8=%016lx  x9=%016lx x10=%016lx x11=%016lx\n",
          context->u.s.X8, context->u.s.X9, context->u.s.X10, context->u.s.X11 );
    TRACE(" x12=%016lx x13=%016lx x14=%016lx x15=%016lx\n",
          context->u.s.X12, context->u.s.X13, context->u.s.X14, context->u.s.X15 );
    TRACE(" x16=%016lx x17=%016lx x18=%016lx x19=%016lx\n",
          context->u.s.X16, context->u.s.X17, context->u.s.X18, context->u.s.X19 );
    TRACE(" x20=%016lx x21=%016lx x22=%016lx x23=%016lx\n",
          context->u.s.X20, context->u.s.X21, context->u.s.X22, context->u.s.X23 );
    TRACE(" x24=%016lx x25=%016lx x26=%016lx x27=%016lx\n",
          context->u.s.X24, context->u.s.X25, context->u.s.X26, context->u.s.X27 );
    TRACE(" x28=%016lx  fp=%016lx  lr=%016lx  sp=%016lx\n",
          context->u.s.X28, context->u.s.Fp, context->u.s.Lr, context->Sp );
#endif
    return STATUS_SUCCESS;
}


/**********************************************************************
 *           virtual_unwind
 */
static NTSTATUS virtual_unwind( ULONG type, DISPATCHER_CONTEXT *dispatch, CONTEXT *context )
{
    LDR_MODULE *module;
    NTSTATUS status;

    dispatch->ImageBase        = 0;
    dispatch->ScopeIndex       = 0;
    dispatch->EstablisherFrame = 0;
    dispatch->ControlPc        = context->Pc;

    /* first look for PE exception information */

    if ((dispatch->FunctionEntry = lookup_function_info( context->Pc, &dispatch->ImageBase, &module )))
    {
        dispatch->LanguageHandler = RtlVirtualUnwind( type, dispatch->ImageBase, context->Pc,
                                                      dispatch->FunctionEntry, context,
                                                      &dispatch->HandlerData, &dispatch->EstablisherFrame,
                                                      NULL );
        return STATUS_SUCCESS;
    }

    /* then look for host system exception information */

    if (!module || (module->Flags & LDR_WINE_INTERNAL))
    {
        status = libunwind_virtual_unwind( context->Pc, &dispatch->EstablisherFrame, context,
                                           &dispatch->LanguageHandler, &dispatch->HandlerData );
        if (status != STATUS_SUCCESS) return status;

        if (dispatch->EstablisherFrame)
        {
            dispatch->FunctionEntry = NULL;
            if (dispatch->LanguageHandler && !module)
            {
                FIXME( "calling personality routine in system library not supported yet\n" );
                dispatch->LanguageHandler = NULL;
            }
            return STATUS_SUCCESS;
        }
    }
    else WARN( "exception data not found in %s\n", debugstr_w(module->BaseDllName.Buffer) );

    dispatch->EstablisherFrame = context->u.s.Fp;
    dispatch->LanguageHandler = NULL;
    context->Pc = context->u.s.Lr;
    return STATUS_SUCCESS;
}


struct unwind_exception_frame
{
    EXCEPTION_REGISTRATION_RECORD frame;
    DISPATCHER_CONTEXT *dispatch;
};

/**********************************************************************
 *           unwind_exception_handler
 *
 * Handler for exceptions happening while calling an unwind handler.
 */
static DWORD __cdecl unwind_exception_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                                               CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    struct unwind_exception_frame *unwind_frame = (struct unwind_exception_frame *)frame;
    DISPATCHER_CONTEXT *dispatch = (DISPATCHER_CONTEXT *)dispatcher;

    /* copy the original dispatcher into the current one, except for the TargetIp */
    dispatch->ControlPc        = unwind_frame->dispatch->ControlPc;
    dispatch->ImageBase        = unwind_frame->dispatch->ImageBase;
    dispatch->FunctionEntry    = unwind_frame->dispatch->FunctionEntry;
    dispatch->EstablisherFrame = unwind_frame->dispatch->EstablisherFrame;
    dispatch->ContextRecord    = unwind_frame->dispatch->ContextRecord;
    dispatch->LanguageHandler  = unwind_frame->dispatch->LanguageHandler;
    dispatch->HandlerData      = unwind_frame->dispatch->HandlerData;
    dispatch->HistoryTable     = unwind_frame->dispatch->HistoryTable;
    dispatch->ScopeIndex       = unwind_frame->dispatch->ScopeIndex;
    TRACE( "detected collided unwind\n" );
    return ExceptionCollidedUnwind;
}

/**********************************************************************
 *           call_unwind_handler
 *
 * Call a single unwind handler.
 */
static DWORD call_unwind_handler( EXCEPTION_RECORD *rec, DISPATCHER_CONTEXT *dispatch )
{
    struct unwind_exception_frame frame;
    DWORD res;

    frame.frame.Handler = unwind_exception_handler;
    frame.dispatch = dispatch;
    __wine_push_frame( &frame.frame );

    TRACE( "calling handler %p (rec=%p, frame=0x%lx context=%p, dispatch=%p)\n",
         dispatch->LanguageHandler, rec, dispatch->EstablisherFrame, dispatch->ContextRecord, dispatch );
    res = dispatch->LanguageHandler( rec, (void *)dispatch->EstablisherFrame, dispatch->ContextRecord, dispatch );
    TRACE( "handler %p returned %x\n", dispatch->LanguageHandler, res );

    __wine_pop_frame( &frame.frame );

    switch (res)
    {
    case ExceptionContinueSearch:
    case ExceptionCollidedUnwind:
        break;
    default:
        raise_status( STATUS_INVALID_DISPOSITION, rec );
        break;
    }

    return res;
}


/**********************************************************************
 *           call_teb_unwind_handler
 *
 * Call a single unwind handler from the TEB chain.
 */
static DWORD call_teb_unwind_handler( EXCEPTION_RECORD *rec, DISPATCHER_CONTEXT *dispatch,
                                     EXCEPTION_REGISTRATION_RECORD *teb_frame )
{
    DWORD res;

    TRACE( "calling TEB handler %p (rec=%p, frame=%p context=%p, dispatch=%p)\n",
           teb_frame->Handler, rec, teb_frame, dispatch->ContextRecord, dispatch );
    res = teb_frame->Handler( rec, teb_frame, dispatch->ContextRecord, (EXCEPTION_REGISTRATION_RECORD**)dispatch );
    TRACE( "handler at %p returned %u\n", teb_frame->Handler, res );

    switch (res)
    {
    case ExceptionContinueSearch:
    case ExceptionCollidedUnwind:
        break;
    default:
        raise_status( STATUS_INVALID_DISPOSITION, rec );
        break;
    }

    return res;
}


static DWORD __cdecl nested_exception_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                                               CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    if (rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND)) return ExceptionContinueSearch;

    /* FIXME */
    return ExceptionNestedException;
}


/**********************************************************************
 *           call_handler
 *
 * Call a single exception handler.
 * FIXME: Handle nested exceptions.
 */
static DWORD call_handler( EXCEPTION_RECORD *rec, CONTEXT *context, DISPATCHER_CONTEXT *dispatch )
{
    EXCEPTION_REGISTRATION_RECORD frame;
    DWORD res;

    frame.Handler = nested_exception_handler;
    __wine_push_frame( &frame );

    TRACE( "calling handler %p (rec=%p, frame=0x%lx context=%p, dispatch=%p)\n",
           dispatch->LanguageHandler, rec, dispatch->EstablisherFrame, dispatch->ContextRecord, dispatch );
    res = dispatch->LanguageHandler( rec, (void *)dispatch->EstablisherFrame, context, dispatch );
    TRACE( "handler at %p returned %u\n", dispatch->LanguageHandler, res );

    __wine_pop_frame( &frame );
    return res;
}


/**********************************************************************
 *           call_teb_handler
 *
 * Call a single exception handler from the TEB chain.
 * FIXME: Handle nested exceptions.
 */
static DWORD call_teb_handler( EXCEPTION_RECORD *rec, CONTEXT *context, DISPATCHER_CONTEXT *dispatch,
                                  EXCEPTION_REGISTRATION_RECORD *teb_frame )
{
    DWORD res;

    TRACE( "calling TEB handler %p (rec=%p, frame=%p context=%p, dispatch=%p)\n",
           teb_frame->Handler, rec, teb_frame, dispatch->ContextRecord, dispatch );
    res = teb_frame->Handler( rec, teb_frame, context, (EXCEPTION_REGISTRATION_RECORD**)dispatch );
    TRACE( "handler at %p returned %u\n", teb_frame->Handler, res );
    return res;
}


/**********************************************************************
 *           call_function_handlers
 *
 * Call the per-function handlers.
 */
static NTSTATUS call_function_handlers( EXCEPTION_RECORD *rec, CONTEXT *orig_context )
{
    EXCEPTION_REGISTRATION_RECORD *teb_frame = NtCurrentTeb()->Tib.ExceptionList;
    UNWIND_HISTORY_TABLE table;
    DISPATCHER_CONTEXT dispatch;
    CONTEXT context;
    NTSTATUS status;

    context = *orig_context;
    dispatch.TargetPc      = 0;
    dispatch.ContextRecord = &context;
    dispatch.HistoryTable  = &table;
    dispatch.NonVolatileRegisters = (BYTE *)&context.u.s.X19;
    for (;;)
    {
        status = virtual_unwind( UNW_FLAG_EHANDLER, &dispatch, &context );
        if (status != STATUS_SUCCESS) return status;

    unwind_done:
        if (!dispatch.EstablisherFrame) break;

        if (!is_valid_frame( dispatch.EstablisherFrame ))
        {
            ERR( "invalid frame %lx (%p-%p)\n", dispatch.EstablisherFrame,
                 NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
            rec->ExceptionFlags |= EH_STACK_INVALID;
            break;
        }

        if (dispatch.LanguageHandler)
        {
            switch (call_handler( rec, orig_context, &dispatch ))
            {
            case ExceptionContinueExecution:
                if (rec->ExceptionFlags & EH_NONCONTINUABLE) return STATUS_NONCONTINUABLE_EXCEPTION;
                return STATUS_SUCCESS;
            case ExceptionContinueSearch:
                break;
            case ExceptionNestedException:
                FIXME( "nested exception\n" );
                break;
            case ExceptionCollidedUnwind: {
                ULONG64 frame;

                context = *dispatch.ContextRecord;
                dispatch.ContextRecord = &context;
                RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                                  dispatch.ControlPc, dispatch.FunctionEntry,
                                  &context, NULL, &frame, NULL );
                goto unwind_done;
            }
            default:
                return STATUS_INVALID_DISPOSITION;
            }
        }
        /* hack: call wine handlers registered in the tib list */
        else while ((ULONG64)teb_frame < context.Sp)
        {
            TRACE( "found wine frame %p rsp %lx handler %p\n",
                    teb_frame, context.Sp, teb_frame->Handler );
            dispatch.EstablisherFrame = (ULONG64)teb_frame;
            switch (call_teb_handler( rec, orig_context, &dispatch, teb_frame ))
            {
            case ExceptionContinueExecution:
                if (rec->ExceptionFlags & EH_NONCONTINUABLE) return STATUS_NONCONTINUABLE_EXCEPTION;
                return STATUS_SUCCESS;
            case ExceptionContinueSearch:
                break;
            case ExceptionNestedException:
                FIXME( "nested exception\n" );
                break;
            case ExceptionCollidedUnwind: {
                ULONG64 frame;

                context = *dispatch.ContextRecord;
                dispatch.ContextRecord = &context;
                RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                                  dispatch.ControlPc, dispatch.FunctionEntry,
                                  &context, NULL, &frame, NULL );
                teb_frame = teb_frame->Prev;
                goto unwind_done;
            }
            default:
                return STATUS_INVALID_DISPOSITION;
            }
            teb_frame = teb_frame->Prev;
        }

        if (context.Sp == (ULONG64)NtCurrentTeb()->Tib.StackBase) break;
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

        TRACE( "code=%x flags=%x addr=%p pc=%lx tid=%04x\n",
               rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
               context->Pc, GetCurrentThreadId() );
        for (c = 0; c < rec->NumberParameters; c++)
            TRACE( " info[%d]=%016lx\n", c, rec->ExceptionInformation[c] );
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
            TRACE("  x0=%016lx  x1=%016lx  x2=%016lx  x3=%016lx\n",
                  context->u.s.X0, context->u.s.X1, context->u.s.X2, context->u.s.X3 );
            TRACE("  x4=%016lx  x5=%016lx  x6=%016lx  x7=%016lx\n",
                  context->u.s.X4, context->u.s.X5, context->u.s.X6, context->u.s.X7 );
            TRACE("  x8=%016lx  x9=%016lx x10=%016lx x11=%016lx\n",
                  context->u.s.X8, context->u.s.X9, context->u.s.X10, context->u.s.X11 );
            TRACE(" x12=%016lx x13=%016lx x14=%016lx x15=%016lx\n",
                  context->u.s.X12, context->u.s.X13, context->u.s.X14, context->u.s.X15 );
            TRACE(" x16=%016lx x17=%016lx x18=%016lx x19=%016lx\n",
                  context->u.s.X16, context->u.s.X17, context->u.s.X18, context->u.s.X19 );
            TRACE(" x20=%016lx x21=%016lx x22=%016lx x23=%016lx\n",
                  context->u.s.X20, context->u.s.X21, context->u.s.X22, context->u.s.X23 );
            TRACE(" x24=%016lx x25=%016lx x26=%016lx x27=%016lx\n",
                  context->u.s.X24, context->u.s.X25, context->u.s.X26, context->u.s.X27 );
            TRACE(" x28=%016lx  fp=%016lx  lr=%016lx  sp=%016lx\n",
                  context->u.s.X28, context->u.s.Fp, context->u.s.Lr, context->Sp );
        }

        if (call_vectored_handlers( rec, context ) == EXCEPTION_CONTINUE_EXECUTION) goto done;

        if ((status = call_function_handlers( rec, context )) == STATUS_SUCCESS) goto done;
        if (status != STATUS_UNHANDLED_EXCEPTION) return status;
    }

    /* last chance exception */

    status = send_debug_event( rec, FALSE, context );
    if (status != DBG_CONTINUE)
    {
        if (rec->ExceptionFlags & EH_STACK_INVALID)
            ERR("Exception frame is not in stack limits => unable to dispatch exception.\n");
        else if (rec->ExceptionCode == STATUS_NONCONTINUABLE_EXCEPTION)
            ERR("Process attempted to continue execution after noncontinuable exception.\n");
        else
            ERR("Unhandled exception code %x flags %x addr %p\n",
                rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress );
        NtTerminateProcess( NtCurrentProcess(), rec->ExceptionCode );
    }
done:
    return NtSetContextThread( GetCurrentThread(), context );
}

/***********************************************************************
 *           setup_exception
 *
 * Setup the exception record and context on the thread stack.
 */
static struct stack_layout *setup_exception( ucontext_t *sigcontext )
{
    struct stack_layout *stack;
    DWORD exception_code = 0;

    /* push the stack_layout structure */
    stack = (struct stack_layout *)((SP_sig(sigcontext) - sizeof(*stack)) & ~15);

    stack->rec.ExceptionRecord  = NULL;
    stack->rec.ExceptionCode    = exception_code;
    stack->rec.ExceptionFlags   = EXCEPTION_CONTINUABLE;
    stack->rec.ExceptionAddress = (LPVOID)PC_sig(sigcontext);
    stack->rec.NumberParameters = 0;

    save_context( &stack->context, sigcontext );
    save_fpu( &stack->context, sigcontext );
    return stack;
}

/**********************************************************************
 *		raise_generic_exception
 */
static void WINAPI raise_generic_exception( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    NTSTATUS status = raise_exception( rec, context, TRUE );
    raise_status( status, rec );
}

/***********************************************************************
 *           setup_raise_exception
 *
 * Modify the signal context to call the exception raise function.
 */
static void setup_raise_exception( ucontext_t *sigcontext, struct stack_layout *stack )
{
    NTSTATUS status = send_debug_event( &stack->rec, TRUE, &stack->context );

    if (status == DBG_CONTINUE || status == DBG_EXCEPTION_HANDLED)
    {
        restore_context( &stack->context, sigcontext );
        return;
    }
    SP_sig(sigcontext) = (ULONG_PTR)stack;
    PC_sig(sigcontext) = (ULONG_PTR)raise_generic_exception;
    REGn_sig(0, sigcontext) = (ULONG_PTR)&stack->rec;  /* first arg for raise_generic_exception */
    REGn_sig(1, sigcontext) = (ULONG_PTR)&stack->context; /* second arg for raise_generic_exception */
    REGn_sig(18, sigcontext) = (ULONG_PTR)NtCurrentTeb();
}

/**********************************************************************
 *		segv_handler
 *
 * Handler for SIGSEGV and related errors.
 */
static void segv_handler( int signal, siginfo_t *info, void *ucontext )
{
    struct stack_layout *stack;
    ucontext_t *context = ucontext;

    /* check for page fault inside the thread stack */
    if (signal == SIGSEGV)
    {
        switch (virtual_handle_stack_fault( info->si_addr ))
        {
        case 1:  /* handled */
            return;
        case -1:  /* overflow */
            stack = setup_exception( context );
            stack->rec.ExceptionCode = EXCEPTION_STACK_OVERFLOW;
            goto done;
        }
    }

    stack = setup_exception( context );
    if (stack->rec.ExceptionCode == EXCEPTION_STACK_OVERFLOW) goto done;

    switch(signal)
    {
    case SIGILL:   /* Invalid opcode exception */
        stack->rec.ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
        break;
    case SIGSEGV:  /* Segmentation fault */
        stack->rec.NumberParameters = 2;
        stack->rec.ExceptionInformation[0] = (get_fault_esr( context ) & 0x40) != 0;
        stack->rec.ExceptionInformation[1] = (ULONG_PTR)info->si_addr;
        if (!(stack->rec.ExceptionCode = virtual_handle_fault( (void *)stack->rec.ExceptionInformation[1],
                                                         stack->rec.ExceptionInformation[0], FALSE )))
            return;
        break;
    case SIGBUS:  /* Alignment check exception */
        stack->rec.ExceptionCode = EXCEPTION_DATATYPE_MISALIGNMENT;
        break;
    default:
        ERR("Got unexpected signal %i\n", signal);
        stack->rec.ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
        break;
    }
done:
    setup_raise_exception( context, stack );
}

/**********************************************************************
 *		trap_handler
 *
 * Handler for SIGTRAP.
 */
static void trap_handler( int signal, siginfo_t *info, void *ucontext )
{
    ucontext_t *context = ucontext;
    struct stack_layout *stack = setup_exception( context );

    switch (info->si_code)
    {
    case TRAP_TRACE:
        stack->rec.ExceptionCode = EXCEPTION_SINGLE_STEP;
        break;
    case TRAP_BRKPT:
    default:
        stack->rec.ExceptionCode = EXCEPTION_BREAKPOINT;
        stack->context.Pc += 4;
        break;
    }
    setup_raise_exception( context, stack );
}

/**********************************************************************
 *		fpe_handler
 *
 * Handler for SIGFPE.
 */
static void fpe_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    struct stack_layout *stack = setup_exception( sigcontext );

    switch (siginfo->si_code & 0xffff )
    {
#ifdef FPE_FLTSUB
    case FPE_FLTSUB:
        stack->rec.ExceptionCode = EXCEPTION_ARRAY_BOUNDS_EXCEEDED;
        break;
#endif
#ifdef FPE_INTDIV
    case FPE_INTDIV:
        stack->rec.ExceptionCode = EXCEPTION_INT_DIVIDE_BY_ZERO;
        break;
#endif
#ifdef FPE_INTOVF
    case FPE_INTOVF:
        stack->rec.ExceptionCode = EXCEPTION_INT_OVERFLOW;
        break;
#endif
#ifdef FPE_FLTDIV
    case FPE_FLTDIV:
        stack->rec.ExceptionCode = EXCEPTION_FLT_DIVIDE_BY_ZERO;
        break;
#endif
#ifdef FPE_FLTOVF
    case FPE_FLTOVF:
        stack->rec.ExceptionCode = EXCEPTION_FLT_OVERFLOW;
        break;
#endif
#ifdef FPE_FLTUND
    case FPE_FLTUND:
        stack->rec.ExceptionCode = EXCEPTION_FLT_UNDERFLOW;
        break;
#endif
#ifdef FPE_FLTRES
    case FPE_FLTRES:
        stack->rec.ExceptionCode = EXCEPTION_FLT_INEXACT_RESULT;
        break;
#endif
#ifdef FPE_FLTINV
    case FPE_FLTINV:
#endif
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
    if (!dispatch_signal(SIGINT))
    {
        struct stack_layout *stack = setup_exception( sigcontext );

        stack->rec.ExceptionCode = CONTROL_C_EXIT;
        setup_raise_exception( sigcontext, stack );
    }
}


/**********************************************************************
 *		abrt_handler
 *
 * Handler for SIGABRT.
 */
static void abrt_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    struct stack_layout *stack = setup_exception( sigcontext );

    stack->rec.ExceptionCode  = EXCEPTION_WINE_ASSERTION;
    stack->rec.ExceptionFlags = EH_NONCONTINUABLE;
    setup_raise_exception( sigcontext, stack );
}


/**********************************************************************
 *		quit_handler
 *
 * Handler for SIGQUIT.
 */
static void quit_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
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

    save_context( &context, sigcontext );
    wait_suspend( &context );
    restore_context( &context, sigcontext );
}


/**********************************************************************
 *		usr2_handler
 *
 * Handler for SIGUSR2, used to set a thread context.
 */
static void usr2_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    CONTEXT *context = interlocked_xchg_ptr( (void **)&arm64_thread_data()->context, NULL );
    if (!context) return;
    if ((context->ContextFlags & ~CONTEXT_ARM64) & CONTEXT_FLOATING_POINT)
        restore_fpu( context, sigcontext );
    restore_context( context, sigcontext );
}


/***********************************************************************
 *           __wine_set_signal_handler   (NTDLL.@)
 */
int CDECL __wine_set_signal_handler(unsigned int sig, wine_signal_handler wsh)
{
    if (sig >= ARRAY_SIZE(handlers)) return -1;
    if (handlers[sig] != NULL) return -2;
    handlers[sig] = wsh;
    return 0;
}


/**********************************************************************
 *             signal_alloc_thread
 */
NTSTATUS signal_alloc_thread( TEB **teb )
{
    SIZE_T size;
    NTSTATUS status;

    size = teb_size + max( MINSIGSTKSZ, 8192 );
    *teb = NULL;
    if (!(status = virtual_alloc_aligned( (void **)teb, 0, &size, MEM_COMMIT | MEM_TOP_DOWN,
                                          PAGE_READWRITE, 13 )))
    {
        (*teb)->Tib.Self = &(*teb)->Tib;
        (*teb)->Tib.ExceptionList = (void *)~0UL;
    }
    return status;
}


/**********************************************************************
 *             signal_free_thread
 */
void signal_free_thread( TEB *teb )
{
    SIZE_T size = 0;

    NtFreeVirtualMemory( NtCurrentProcess(), (void **)&teb, &size, MEM_RELEASE );
}


/**********************************************************************
 *		signal_init_thread
 */
void signal_init_thread( TEB *teb )
{
    static BOOL init_done;
    stack_t ss;

    if (!init_done)
    {
        pthread_key_create( &teb_key, NULL );
        init_done = TRUE;
    }

    ss.ss_sp    = (char *)teb + teb_size;
    ss.ss_size  = signal_stack_size;
    ss.ss_flags = 0;
    if (sigaltstack( &ss, NULL ) == -1) perror( "sigaltstack" );

    /* Win64/ARM applications expect the TEB pointer to be in the x18 platform register. */
    __asm__ __volatile__( "mov x18, %0" : : "r" (teb) );

    pthread_setspecific( teb_key, teb );
}


/**********************************************************************
 *		signal_init_process
 */
void signal_init_process(void)
{
    struct sigaction sig_act;

    sig_act.sa_mask = server_block_set;
    sig_act.sa_flags = SA_RESTART | SA_SIGINFO | SA_ONSTACK;

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
    sig_act.sa_sigaction = usr2_handler;
    if (sigaction( SIGUSR2, &sig_act, NULL ) == -1) goto error;

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
    return;

 error:
    perror("sigaction");
    exit(1);
}

/***********************************************************************
 * Definitions for Win32 unwind tables
 */

struct unwind_info
{
    DWORD function_length : 18;
    DWORD version : 2;
    DWORD x : 1;
    DWORD e : 1;
    DWORD epilog : 5;
    DWORD codes : 5;
};

struct unwind_info_ext
{
    WORD epilog;
    BYTE codes;
    BYTE reserved;
};

struct unwind_info_epilog
{
    DWORD offset : 18;
    DWORD res : 4;
    DWORD index : 10;
};

static const BYTE unwind_code_len[256] =
{
/* 00 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* 20 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* 40 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* 60 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* 80 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* a0 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* c0 */ 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
/* e0 */ 4,1,2,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};

/***********************************************************************
 *           get_sequence_len
 */
static unsigned int get_sequence_len( BYTE *ptr, BYTE *end )
{
    unsigned int ret = 0;

    while (ptr < end)
    {
        if (*ptr == 0xe4 || *ptr == 0xe5) break;
        ptr += unwind_code_len[*ptr];
        ret++;
    }
    return ret;
}


/***********************************************************************
 *           restore_regs
 */
static void restore_regs( int reg, int count, int pos, CONTEXT *context,
                          KNONVOLATILE_CONTEXT_POINTERS *ptrs )
{
    int i, offset = max( 0, pos );
    for (i = 0; i < count; i++)
    {
        if (ptrs && reg + i >= 19) (&ptrs->X19)[reg + i - 19] = (DWORD64 *)context->Sp + i + offset;
        context->u.X[reg + i] = ((DWORD64 *)context->Sp)[i + offset];
    }
    if (pos < 0) context->Sp += -8 * pos;
}


/***********************************************************************
 *           restore_fpregs
 */
static void restore_fpregs( int reg, int count, int pos, CONTEXT *context,
                            KNONVOLATILE_CONTEXT_POINTERS *ptrs )
{
    int i, offset = max( 0, pos );
    for (i = 0; i < count; i++)
    {
        if (ptrs && reg + i >= 8) (&ptrs->D8)[reg + i - 8] = (DWORD64 *)context->Sp + i + offset;
        context->V[reg + i].D[0] = ((double *)context->Sp)[i + offset];
    }
    if (pos < 0) context->Sp += -8 * pos;
}


/***********************************************************************
 *           process_unwind_codes
 */
static void process_unwind_codes( BYTE *ptr, BYTE *end, CONTEXT *context,
                                  KNONVOLATILE_CONTEXT_POINTERS *ptrs, int skip )
{
    unsigned int val, len, save_next = 2;

    /* skip codes */
    while (ptr < end && skip)
    {
        if (*ptr == 0xe4 || *ptr == 0xe5) break;
        ptr += unwind_code_len[*ptr];
        skip--;
    }

    while (ptr < end)
    {
        if ((len = unwind_code_len[*ptr]) > 1)
        {
            if (ptr + len > end) break;
            val = ptr[0] * 0x100 + ptr[1];
        }
        else val = *ptr;

        if (*ptr < 0x20)  /* alloc_s */
            context->Sp += 16 * (val & 0x1f);
        else if (*ptr < 0x40)  /* save_r19r20_x */
            restore_regs( 19, save_next, -(val & 0x1f), context, ptrs );
        else if (*ptr < 0x80) /* save_fplr */
            restore_regs( 29, 2, val & 0x3f, context, ptrs );
        else if (*ptr < 0xc0)  /* save_fplr_x */
            restore_regs( 29, 2, -(val & 0x3f) - 1, context, ptrs );
        else if (*ptr < 0xc8)  /* alloc_m */
            context->Sp += 16 * (val & 0x7ff);
        else if (*ptr < 0xcc)  /* save_regp */
            restore_regs( 19 + ((val >> 6) & 0xf), save_next, val & 0x3f, context, ptrs );
        else if (*ptr < 0xd0)  /* save_regp_x */
            restore_regs( 19 + ((val >> 6) & 0xf), save_next, -(val & 0x3f) - 1, context, ptrs );
        else if (*ptr < 0xd4)  /* save_reg */
            restore_regs( 19 + ((val >> 6) & 0xf), 1, val & 0x3f, context, ptrs );
        else if (*ptr < 0xd6)  /* save_reg_x */
            restore_regs( 19 + ((val >> 5) & 0xf), 1, -(val & 0x1f) - 1, context, ptrs );
        else if (*ptr < 0xd8)  /* save_lrpair */
            restore_regs( 19 + ((val >> 6) & 0x7), 2, val & 0x3f, context, ptrs );
        else if (*ptr < 0xda)  /* save_fregp */
            restore_fpregs( 8 + ((val >> 6) & 0x7), save_next, val & 0x3f, context, ptrs );
        else if (*ptr < 0xdc)  /* save_fregp_x */
            restore_fpregs( 8 + ((val >> 6) & 0x7), save_next, -(val & 0x3f) - 1, context, ptrs );
        else if (*ptr < 0xde)  /* save_freg */
            restore_fpregs( 8 + ((val >> 6) & 0x7), 1, val & 0x3f, context, ptrs );
        else if (*ptr == 0xde)  /* save_freg_x */
            restore_fpregs( 8 + ((val >> 5) & 0x7), 1, -(val & 0x3f) - 1, context, ptrs );
        else if (*ptr == 0xe0)  /* alloc_l */
            context->Sp += 16 * ((ptr[1] << 16) + (ptr[2] << 8) + ptr[3]);
        else if (*ptr == 0xe1)  /* set_fp */
            context->Sp = context->u.s.Fp;
        else if (*ptr == 0xe2)  /* add_fp */
            context->Sp = context->u.s.Fp - 8 * (val & 0xff);
        else if (*ptr == 0xe3)  /* nop */
            /* nop */ ;
        else if (*ptr == 0xe4)  /* end */
            break;
        else if (*ptr == 0xe5)  /* end_c */
            break;
        else if (*ptr == 0xe6)  /* save_next */
        {
            save_next += 2;
            ptr += len;
            continue;
        }
        else
        {
            WARN( "unsupported code %02x\n", *ptr );
            return;
        }
        save_next = 2;
        ptr += len;
    }
}


/***********************************************************************
 *           unwind_packed_data
 */
static void *unwind_packed_data( ULONG_PTR base, ULONG_PTR pc, RUNTIME_FUNCTION *func,
                                 CONTEXT *context, KNONVOLATILE_CONTEXT_POINTERS *ptrs )
{
    int i;
    unsigned int len, offset, skip = 0;
    unsigned int int_size = func->u.s.RegI * 8, fp_size = func->u.s.RegF * 8, regsave, local_size;

    TRACE( "function %lx-%lx: len=%#x flag=%x regF=%u regI=%u H=%u CR=%u frame=%x\n",
           base + func->BeginAddress, base + func->BeginAddress + func->u.s.FunctionLength * 4,
           func->u.s.FunctionLength, func->u.s.Flag, func->u.s.RegF, func->u.s.RegI,
           func->u.s.H, func->u.s.CR, func->u.s.FrameSize );

    if (func->u.s.CR == 1) int_size += 8;
    if (func->u.s.RegF) fp_size += 8;

    regsave = ((int_size + fp_size + 8 * 8 * func->u.s.H) + 0xf) & ~0xf;
    local_size = func->u.s.FrameSize * 16 - regsave;

    /* check for prolog/epilog */
    if (func->u.s.Flag == 1)
    {
        offset = ((pc - base) - func->BeginAddress) / 4;
        if (offset < 17 || offset >= func->u.s.FunctionLength - 15)
        {
            len = (int_size + 8) / 16 + (fp_size + 8) / 16;
            switch (func->u.s.CR)
            {
            case 3:
                len++; /* stp x29,lr,[sp,0] */
                if (local_size <= 512) break;
                /* fall through */
            case 0:
            case 1:
                if (local_size) len++;  /* sub sp,sp,#local_size */
                if (local_size > 4088) len++;  /* sub sp,sp,#4088 */
                break;
            }
            if (offset < len + 4 * func->u.s.H)  /* prolog */
            {
                skip = len + 4 * func->u.s.H - offset;
            }
            else if (offset >= func->u.s.FunctionLength - (len + 1))  /* epilog */
            {
                skip = offset - (func->u.s.FunctionLength - (len + 1));
            }
        }
    }

    if (!skip)
    {
        if (func->u.s.CR == 3) restore_regs( 29, 2, 0, context, ptrs );
        context->Sp += local_size;
        if (fp_size) restore_fpregs( 8, fp_size / 8, int_size, context, ptrs );
        if (func->u.s.CR == 1) restore_regs( 30, 1, int_size - 8, context, ptrs );
        restore_regs( 19, func->u.s.RegI, -regsave, context, ptrs );
    }
    else
    {
        unsigned int pos = 0;

        switch (func->u.s.CR)
        {
        case 3:
            if (local_size <= 512)
            {
                /* stp x29,lr,[sp,-#local_size]! */
                if (pos++ > skip) restore_regs( 29, 2, -local_size, context, ptrs );
                break;
            }
            /* stp x29,lr,[sp,0] */
            if (pos++ > skip) restore_regs( 29, 2, 0, context, ptrs );
            /* fall through */
        case 0:
        case 1:
            if (!local_size) break;
            /* sub sp,sp,#local_size */
            if (pos++ > skip) context->Sp += (local_size - 1) % 4088 + 1;
            if (local_size > 4088 && pos++ > skip) context->Sp += 4088;
            break;
        }

        if (func->u.s.H && offset < len + 4) pos += 4;

        if (fp_size)
        {
            if (func->u.s.RegF % 2 == 0 && pos++ > skip)
                /* str d%u,[sp,#fp_size] */
                restore_fpregs( 8 + func->u.s.RegF, 1, int_size + fp_size - 8, context, ptrs );
            for (i = func->u.s.RegF / 2 - 1; i >= 0; i--)
            {
                if (pos++ <= skip) continue;
                if (!i && !int_size)
                     /* stp d8,d9,[sp,-#regsave]! */
                    restore_fpregs( 8, 2, -regsave, context, ptrs );
                else
                     /* stp dn,dn+1,[sp,#offset] */
                    restore_fpregs( 8 + 2 * i, 2, int_size + 16 * i, context, ptrs );
            }
        }

        if (pos++ > skip)
        {
            if (func->u.s.RegI % 2)
            {
                /* stp xn,lr,[sp,#offset] */
                if (func->u.s.CR == 1) restore_regs( 30, 1, int_size - 8, context, ptrs );
                /* str xn,[sp,#offset] */
                restore_regs( 18 + func->u.s.RegI, 1,
                              (func->u.s.RegI > 1) ? 8 * func->u.s.RegI - 8 : -regsave,
                              context, ptrs );
            }
            else if (func->u.s.CR == 1)
                /* str lr,[sp,#offset] */
                restore_regs( 30, 1, func->u.s.RegI ? int_size - 8 : -regsave, context, ptrs );
        }

        for (i = func->u.s.RegI / 2 - 1; i >= 0; i--)
        {
            if (pos++ <= skip) continue;
            if (i)
                /* stp xn,xn+1,[sp,#offset] */
                restore_regs( 19 + 2 * i, 2, 16 * i, context, ptrs );
            else
                /* stp x19,x20,[sp,-#regsave]! */
                restore_regs( 19, 2, -regsave, context, ptrs );
        }
    }
    return NULL;
}


/***********************************************************************
 *           unwind_full_data
 */
static void *unwind_full_data( ULONG_PTR base, ULONG_PTR pc, RUNTIME_FUNCTION *func,
                               CONTEXT *context, PVOID *handler_data, KNONVOLATILE_CONTEXT_POINTERS *ptrs )
{
    struct unwind_info *info;
    struct unwind_info_epilog *info_epilog;
    unsigned int i, codes, epilogs, len, offset;
    void *data;
    BYTE *end;

    info = (struct unwind_info *)((char *)base + func->u.UnwindData);
    data = info + 1;
    epilogs = info->epilog;
    codes = info->codes;
    if (!codes && !epilogs)
    {
        struct unwind_info_ext *infoex = data;
        codes = infoex->codes;
        epilogs = infoex->epilog;
        data = infoex + 1;
    }
    info_epilog = data;
    if (!info->e) data = info_epilog + epilogs;

    offset = ((pc - base) - func->BeginAddress) / 4;
    end = (BYTE *)data + codes * 4;

    TRACE( "function %lx-%lx: len=%#x ver=%u X=%u E=%u epilogs=%u codes=%u\n",
           base + func->BeginAddress, base + func->BeginAddress + info->function_length * 4,
           info->function_length, info->version, info->x, info->e, epilogs, codes * 4 );

    /* check for prolog */
    if (offset < codes * 4)
    {
        len = get_sequence_len( data, end );
        if (offset < len)
        {
            process_unwind_codes( data, end, context, ptrs, len - offset );
            return NULL;
        }
    }

    /* check for epilog */
    if (!info->e)
    {
        for (i = 0; i < epilogs; i++)
        {
            if (offset < info_epilog[i].offset) break;
            if (offset - info_epilog[i].offset < codes * 4 - info_epilog[i].index)
            {
                BYTE *ptr = (BYTE *)data + info_epilog[i].index;
                len = get_sequence_len( ptr, end );
                if (offset <= info_epilog[i].offset + len)
                {
                    process_unwind_codes( ptr, end, context, ptrs, offset - info_epilog[i].offset );
                    return NULL;
                }
            }
        }
    }
    else if (info->function_length - offset <= codes * 4 - epilogs)
    {
        BYTE *ptr = (BYTE *)data + epilogs;
        len = get_sequence_len( ptr, end ) + 1;
        if (offset >= info->function_length - len)
        {
            process_unwind_codes( ptr, end, context, ptrs, offset - (info->function_length - len) );
            return NULL;
        }
    }

    process_unwind_codes( data, end, context, ptrs, 0 );

    /* get handler since we are inside the main code */
    if (info->x)
    {
        DWORD *handler_rva = (DWORD *)data + codes;
        *handler_data = handler_rva + 1;
        return (char *)base + *handler_rva;
    }
    return NULL;
}


/**********************************************************************
 *              RtlVirtualUnwind   (NTDLL.@)
 */
PVOID WINAPI RtlVirtualUnwind( ULONG type, ULONG_PTR base, ULONG_PTR pc,
                               RUNTIME_FUNCTION *func, CONTEXT *context,
                               PVOID *handler_data, ULONG_PTR *frame_ret,
                               KNONVOLATILE_CONTEXT_POINTERS *ctx_ptr )
{
    void *handler;

    TRACE( "type %x pc %lx sp %lx func %lx\n", type, pc, context->Sp, base + func->BeginAddress );

    if (func->u.s.Flag)
        handler = unwind_packed_data( base, pc, func, context, ctx_ptr );
    else
        handler = unwind_full_data( base, pc, func, context, handler_data, ctx_ptr );

    TRACE( "ret: lr=%lx sp=%lx handler=%p\n", context->u.s.Lr, context->Sp, handler );
    context->Pc = context->u.s.Lr;
    *frame_ret = context->Sp;
    return handler;
}


/*******************************************************************
 *		RtlUnwindEx (NTDLL.@)
 */
void WINAPI RtlUnwindEx( PVOID end_frame, PVOID target_ip, EXCEPTION_RECORD *rec,
                         PVOID retval, CONTEXT *context, UNWIND_HISTORY_TABLE *table )
{
    EXCEPTION_REGISTRATION_RECORD *teb_frame = NtCurrentTeb()->Tib.ExceptionList;
    EXCEPTION_RECORD record;
    DISPATCHER_CONTEXT dispatch;
    CONTEXT new_context;
    NTSTATUS status;
    DWORD i;

    RtlCaptureContext( context );
    new_context = *context;

    /* build an exception record, if we do not have one */
    if (!rec)
    {
        record.ExceptionCode    = STATUS_UNWIND;
        record.ExceptionFlags   = 0;
        record.ExceptionRecord  = NULL;
        record.ExceptionAddress = (void *)context->Pc;
        record.NumberParameters = 0;
        rec = &record;
    }

    rec->ExceptionFlags |= EH_UNWINDING | (end_frame ? 0 : EH_EXIT_UNWIND);

    TRACE( "code=%x flags=%x end_frame=%p target_ip=%p pc=%016lx\n",
           rec->ExceptionCode, rec->ExceptionFlags, end_frame, target_ip, context->Pc );
    for (i = 0; i < min( EXCEPTION_MAXIMUM_PARAMETERS, rec->NumberParameters ); i++)
        TRACE( " info[%d]=%016lx\n", i, rec->ExceptionInformation[i] );
    TRACE("  x0=%016lx  x1=%016lx  x2=%016lx  x3=%016lx\n",
          context->u.s.X0, context->u.s.X1, context->u.s.X2, context->u.s.X3 );
    TRACE("  x4=%016lx  x5=%016lx  x6=%016lx  x7=%016lx\n",
          context->u.s.X4, context->u.s.X5, context->u.s.X6, context->u.s.X7 );
    TRACE("  x8=%016lx  x9=%016lx x10=%016lx x11=%016lx\n",
          context->u.s.X8, context->u.s.X9, context->u.s.X10, context->u.s.X11 );
    TRACE(" x12=%016lx x13=%016lx x14=%016lx x15=%016lx\n",
          context->u.s.X12, context->u.s.X13, context->u.s.X14, context->u.s.X15 );
    TRACE(" x16=%016lx x17=%016lx x18=%016lx x19=%016lx\n",
          context->u.s.X16, context->u.s.X17, context->u.s.X18, context->u.s.X19 );
    TRACE(" x20=%016lx x21=%016lx x22=%016lx x23=%016lx\n",
          context->u.s.X20, context->u.s.X21, context->u.s.X22, context->u.s.X23 );
    TRACE(" x24=%016lx x25=%016lx x26=%016lx x27=%016lx\n",
          context->u.s.X24, context->u.s.X25, context->u.s.X26, context->u.s.X27 );
    TRACE(" x28=%016lx  fp=%016lx  lr=%016lx  sp=%016lx\n",
          context->u.s.X28, context->u.s.Fp, context->u.s.Lr, context->Sp );

    dispatch.TargetPc         = (ULONG64)target_ip;
    dispatch.ContextRecord    = context;
    dispatch.HistoryTable     = table;
    dispatch.NonVolatileRegisters = (BYTE *)&context->u.s.X19;

    for (;;)
    {
        status = virtual_unwind( UNW_FLAG_UHANDLER, &dispatch, &new_context );
        if (status != STATUS_SUCCESS) raise_status( status, rec );

    unwind_done:
        if (!dispatch.EstablisherFrame) break;

        if (is_inside_signal_stack( (void *)dispatch.EstablisherFrame ))
        {
            TRACE( "frame %lx is inside signal stack (%p-%p)\n", dispatch.EstablisherFrame,
                   get_signal_stack(), (char *)get_signal_stack() + signal_stack_size );
            *context = new_context;
            continue;
        }
        if (!is_valid_frame( dispatch.EstablisherFrame ))
        {
            ERR( "invalid frame %lx (%p-%p)\n", dispatch.EstablisherFrame,
                 NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
            rec->ExceptionFlags |= EH_STACK_INVALID;
            break;
        }

        if (dispatch.LanguageHandler)
        {
            if (end_frame && (dispatch.EstablisherFrame > (ULONG64)end_frame))
            {
                ERR( "invalid end frame %lx/%p\n", dispatch.EstablisherFrame, end_frame );
                raise_status( STATUS_INVALID_UNWIND_TARGET, rec );
            }
            if (dispatch.EstablisherFrame == (ULONG64)end_frame) rec->ExceptionFlags |= EH_TARGET_UNWIND;
            if (call_unwind_handler( rec, &dispatch ) == ExceptionCollidedUnwind)
            {
                ULONG64 frame;

                *context = new_context = *dispatch.ContextRecord;
                dispatch.ContextRecord = context;
                RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                                  dispatch.ControlPc, dispatch.FunctionEntry,
                                  &new_context, NULL, &frame, NULL );
                rec->ExceptionFlags |= EH_COLLIDED_UNWIND;
                goto unwind_done;
            }
            rec->ExceptionFlags &= ~EH_COLLIDED_UNWIND;
        }
        else  /* hack: call builtin handlers registered in the tib list */
        {
            DWORD64 backup_frame = dispatch.EstablisherFrame;
            while ((ULONG64)teb_frame < new_context.Sp && (ULONG64)teb_frame < (ULONG64)end_frame)
            {
                TRACE( "found builtin frame %p handler %p\n", teb_frame, teb_frame->Handler );
                dispatch.EstablisherFrame = (ULONG64)teb_frame;
                if (call_teb_unwind_handler( rec, &dispatch, teb_frame ) == ExceptionCollidedUnwind)
                {
                    ULONG64 frame;

                    teb_frame = __wine_pop_frame( teb_frame );

                    *context = new_context = *dispatch.ContextRecord;
                    dispatch.ContextRecord = context;
                    RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                                      dispatch.ControlPc, dispatch.FunctionEntry,
                                      &new_context, NULL, &frame, NULL );
                    rec->ExceptionFlags |= EH_COLLIDED_UNWIND;
                    goto unwind_done;
                }
                teb_frame = __wine_pop_frame( teb_frame );
            }
            if ((ULONG64)teb_frame == (ULONG64)end_frame && (ULONG64)end_frame < new_context.Sp) break;
            dispatch.EstablisherFrame = backup_frame;
        }

        if (dispatch.EstablisherFrame == (ULONG64)end_frame) break;
        *context = new_context;
    }

    context->u.s.X0 = (ULONG64)retval;
    context->Pc     = (ULONG64)target_ip;
    set_cpu_context( context );
}


/***********************************************************************
 *            RtlUnwind  (NTDLL.@)
 */
void WINAPI RtlUnwind( void *frame, void *target_ip, EXCEPTION_RECORD *rec, void *retval )
{
    CONTEXT context;
    RtlUnwindEx( frame, target_ip, rec, retval, &context, NULL );
}

/*******************************************************************
 *		NtRaiseException (NTDLL.@)
 */
NTSTATUS WINAPI NtRaiseException( EXCEPTION_RECORD *rec, CONTEXT *context, BOOL first_chance )
{
    if (first_chance)
    {
        NTSTATUS status = send_debug_event( rec, TRUE, context );
        if (status == DBG_CONTINUE || status == DBG_EXCEPTION_HANDLED)
            NtSetContextThread( GetCurrentThread(), context );
    }
    return raise_exception( rec, context, first_chance );
}

/***********************************************************************
 *		RtlRaiseException (NTDLL.@)
 */
void WINAPI RtlRaiseException( EXCEPTION_RECORD *rec )
{
    CONTEXT context;

    RtlCaptureContext( &context );
    rec->ExceptionAddress = (LPVOID)context.Pc;
    RtlRaiseStatus( NtRaiseException( rec, &context, TRUE ));
}

/*************************************************************************
 *             RtlCaptureStackBackTrace (NTDLL.@)
 */
USHORT WINAPI RtlCaptureStackBackTrace( ULONG skip, ULONG count, PVOID *buffer, ULONG *hash )
{
    FIXME( "(%d, %d, %p, %p) stub!\n", skip, count, buffer, hash );
    return 0;
}

/***********************************************************************
 *           call_thread_entry_point
 */
static void WINAPI call_thread_entry_point( LPTHREAD_START_ROUTINE entry, void *arg )
{
    __TRY
    {
        TRACE_(relay)( "\1Starting thread proc %p (arg=%p)\n", entry, arg );
        RtlExitUserThread( entry( arg ));
    }
    __EXCEPT(call_unhandled_exception_filter)
    {
        NtTerminateThread( GetCurrentThread(), GetExceptionCode() );
    }
    __ENDTRY
    abort();  /* should not be reached */
}

extern void DECLSPEC_NORETURN start_thread( LPTHREAD_START_ROUTINE entry, void *arg, BOOL suspend,
                                            void *relay, TEB *teb );
__ASM_GLOBAL_FUNC( start_thread,
                   "stp x29, x30, [sp,#-16]!\n\t"
                   "mov x18, x4\n\t"             /* teb */
                   /* store exit frame */
                   "mov x29, sp\n\t"
                   "str x29, [x4, #0x300]\n\t"  /* arm64_thread_data()->exit_frame */
                   /* switch to thread stack */
                   "ldr x5, [x4, #8]\n\t"       /* teb->Tib.StackBase */
                   "sub sp, x5, #0x1000\n\t"
                   /* attach dlls */
                   "bl " __ASM_NAME("attach_thread") "\n\t"
                   "mov sp, x0\n\t"
                   /* clear the stack */
                   "and x0, x0, #~0xfff\n\t"  /* round down to page size */
                   "bl " __ASM_NAME("virtual_clear_thread_stack") "\n\t"
                   /* switch to the initial context */
                   "mov x0, sp\n\t"
                   "ldp q0, q1, [x0, #0x110]\n\t"      /* context->V[0,1] */
                   "ldp q2, q3, [x0, #0x130]\n\t"      /* context->V[2,3] */
                   "ldp q4, q5, [x0, #0x150]\n\t"      /* context->V[4,5] */
                   "ldp q6, q7, [x0, #0x170]\n\t"      /* context->V[6,7] */
                   "ldp q8, q9, [x0, #0x190]\n\t"      /* context->V[8,9] */
                   "ldp q10, q11, [x0, #0x1b0]\n\t"    /* context->V[10,11] */
                   "ldp q12, q13, [x0, #0x1d0]\n\t"    /* context->V[12,13] */
                   "ldp q14, q15, [x0, #0x1f0]\n\t"    /* context->V[14,15] */
                   "ldp q16, q17, [x0, #0x210]\n\t"    /* context->V[16,17] */
                   "ldp q18, q19, [x0, #0x230]\n\t"    /* context->V[18,19] */
                   "ldp q20, q21, [x0, #0x250]\n\t"    /* context->V[20,21] */
                   "ldp q22, q23, [x0, #0x270]\n\t"    /* context->V[22,23] */
                   "ldp q24, q25, [x0, #0x290]\n\t"    /* context->V[24,25] */
                   "ldp q26, q27, [x0, #0x2b0]\n\t"    /* context->V[26,27] */
                   "ldp q28, q29, [x0, #0x2d0]\n\t"    /* context->V[28,29] */
                   "ldp q30, q31, [x0, #0x2f0]\n\t"    /* context->V[30,31] */
                   "ldr w1, [x0, #0x310]\n\t"          /* context->Fpcr */
                   "msr fpcr, x1\n\t"
                   "ldr w1, [x0, #0x314]\n\t"          /* context->Fpsr */
                   "msr fpsr, x1\n\t"
                   "ldp x1, x2, [x0, #0x10]\n\t"       /* context->X1,2 */
                   "ldp x3, x4, [x0, #0x20]\n\t"       /* context->X3,4 */
                   "ldp x5, x6, [x0, #0x30]\n\t"       /* context->X5,6 */
                   "ldp x7, x8, [x0, #0x40]\n\t"       /* context->X7,8 */
                   "ldp x9, x10, [x0, #0x50]\n\t"      /* context->X9,10 */
                   "ldp x11, x12, [x0, #0x60]\n\t"     /* context->X11,12 */
                   "ldp x13, x14, [x0, #0x70]\n\t"     /* context->X13,14 */
                   "ldp x15, x16, [x0, #0x80]\n\t"     /* context->X15,16 */
                   "ldp x17, x18, [x0, #0x90]\n\t"     /* context->X17,18 */
                   "ldp x19, x20, [x0, #0xa0]\n\t"     /* context->X19,20 */
                   "ldp x21, x22, [x0, #0xb0]\n\t"     /* context->X21,22 */
                   "ldp x23, x24, [x0, #0xc0]\n\t"     /* context->X23,24 */
                   "ldp x25, x26, [x0, #0xd0]\n\t"     /* context->X25,26 */
                   "ldp x27, x28, [x0, #0xe0]\n\t"     /* context->X27,28 */
                   "ldp x29, x30, [x0, #0xf0]\n\t"     /* context->Fp,Lr */
                   "ldr x17, [x0, #0x100]\n\t"         /* context->Sp */
                   "mov sp, x17\n\t"
                   "ldr x17, [x0, #0x108]\n\t"         /* context->Pc */
                   "ldr x0, [x0, #0x8]\n\t"            /* context->X0 */
                   "br x17" )

extern void DECLSPEC_NORETURN call_thread_exit_func( int status, void (*func)(int), TEB *teb );
__ASM_GLOBAL_FUNC( call_thread_exit_func,
                   "ldr x3, [x2, #0x300]\n\t"  /* arm64_thread_data()->exit_frame */
                   "str xzr, [x2, #0x300]\n\t"
                   "cbz x3, 1f\n\t"
                   "mov sp, x3\n"
                   "1:\tblr x1" )

/***********************************************************************
 *           init_thread_context
 */
static void init_thread_context( CONTEXT *context, LPTHREAD_START_ROUTINE entry, void *arg, void *relay )
{
    context->u.s.X0  = (DWORD64)entry;
    context->u.s.X1  = (DWORD64)arg;
    context->u.s.X18 = (DWORD64)NtCurrentTeb();
    context->Sp      = (DWORD64)NtCurrentTeb()->Tib.StackBase;
    context->Pc      = (DWORD64)relay;
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
        CONTEXT context = { CONTEXT_ALL };

        init_thread_context( &context, entry, arg, relay );
        wait_suspend( &context );
        ctx = (CONTEXT *)((ULONG_PTR)context.Sp & ~15) - 1;
        *ctx = context;
    }
    else
    {
        ctx = (CONTEXT *)NtCurrentTeb()->Tib.StackBase - 1;
        init_thread_context( ctx, entry, arg, relay );
    }
    ctx->ContextFlags = CONTEXT_FULL;
    LdrInitializeThunk( ctx, (void **)&ctx->u.s.X0, 0, 0 );
    return ctx;
}

/***********************************************************************
 *           signal_start_thread
 *
 * Thread startup sequence:
 * signal_start_thread()
 *   -> start_thread()
 *     -> call_thread_func()
 */
void signal_start_thread( LPTHREAD_START_ROUTINE entry, void *arg, BOOL suspend )
{
    start_thread( entry, arg, suspend, call_thread_entry_point, NtCurrentTeb() );
}

/**********************************************************************
 *		signal_start_process
 *
 * Process startup sequence:
 * signal_start_process()
 *   -> start_thread()
 *     -> kernel32_start_process()
 */
void signal_start_process( LPTHREAD_START_ROUTINE entry, BOOL suspend )
{
    start_thread( entry, NtCurrentTeb()->Peb, suspend, kernel32_start_process, NtCurrentTeb() );
}

/***********************************************************************
 *           signal_exit_thread
 */
void signal_exit_thread( int status )
{
    call_thread_exit_func( status, exit_thread, NtCurrentTeb() );
}

/***********************************************************************
 *           signal_exit_process
 */
void signal_exit_process( int status )
{
    call_thread_exit_func( status, exit, NtCurrentTeb() );
}

/**********************************************************************
 *              DbgBreakPoint   (NTDLL.@)
 */
__ASM_STDCALL_FUNC( DbgBreakPoint, 0, "brk #0; ret")

/**********************************************************************
 *              DbgUserBreakPoint   (NTDLL.@)
 */
__ASM_STDCALL_FUNC( DbgUserBreakPoint, 0, "brk #0; ret")

/**********************************************************************
 *           NtCurrentTeb   (NTDLL.@)
 */
TEB * WINAPI NtCurrentTeb(void)
{
    return pthread_getspecific( teb_key );
}

#endif  /* __aarch64__ */
