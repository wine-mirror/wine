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

#if 0
#pragma makedep unix
#endif

#ifdef __aarch64__

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <pthread.h>
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
#include "winnt.h"
#include "winternl.h"
#include "wine/exception.h"
#include "wine/asm.h"
#include "unix_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);

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

#elif defined(__APPLE__)

/* All Registers access - only for local access */
# define REG_sig(reg_name, context) ((context)->uc_mcontext->__ss.__ ## reg_name)
# define REGn_sig(reg_num, context) ((context)->uc_mcontext->__ss.__x[reg_num])

/* Special Registers access  */
# define SP_sig(context)            REG_sig(sp, context)    /* Stack pointer */
# define PC_sig(context)            REG_sig(pc, context)    /* Program counter */
# define PSTATE_sig(context)        REG_sig(cpsr, context)  /* Current State Register */
# define FP_sig(context)            REG_sig(fp, context)    /* Frame pointer */
# define LR_sig(context)            REG_sig(lr, context)    /* Link Register */

static DWORD64 get_fault_esr( ucontext_t *sigcontext )
{
    return sigcontext->uc_mcontext->__es.__esr;
}

#endif /* linux */

static pthread_key_t teb_key;

struct syscall_frame
{
    ULONG64 x[29];          /* 000 */
    ULONG64 fp;             /* 0e8 */
    ULONG64 lr;             /* 0f0 */
    ULONG64 sp;             /* 0f8 */
    ULONG64 pc;             /* 100 */
    ULONG64 cpsr;           /* 108 */
    ULONG64 restore_flags;  /* 110 */
    ULONG   fpcr;           /* 118 */
    ULONG   fpsr;           /* 11c */
    NEON128 v[32];          /* 120 */
};

C_ASSERT( sizeof( struct syscall_frame ) == 0x320 );

struct arm64_thread_data
{
    void                 *exit_frame;    /* 02f0 exit frame pointer */
    struct syscall_frame *syscall_frame; /* 02f8 frame pointer on syscall entry */
};

C_ASSERT( sizeof(struct arm64_thread_data) <= sizeof(((struct ntdll_thread_data *)0)->cpu_data) );
C_ASSERT( offsetof( TEB, GdiTebBatch ) + offsetof( struct arm64_thread_data, exit_frame ) == 0x2f0 );
C_ASSERT( offsetof( TEB, GdiTebBatch ) + offsetof( struct arm64_thread_data, syscall_frame ) == 0x2f8 );

static inline struct arm64_thread_data *arm64_thread_data(void)
{
    return (struct arm64_thread_data *)ntdll_get_thread_data()->cpu_data;
}

static BOOL is_inside_syscall( ucontext_t *sigcontext )
{
    return ((char *)SP_sig(sigcontext) >= (char *)ntdll_get_thread_data()->kernel_stack &&
            (char *)SP_sig(sigcontext) <= (char *)arm64_thread_data()->syscall_frame);
}

extern void raise_func_trampoline( EXCEPTION_RECORD *rec, CONTEXT *context, void *dispatcher );

/***********************************************************************
 *           unwind_builtin_dll
 *
 * Equivalent of RtlVirtualUnwind for builtin modules.
 */
NTSTATUS CDECL unwind_builtin_dll( ULONG type, DISPATCHER_CONTEXT *dispatch, CONTEXT *context )
{
#ifdef HAVE_LIBUNWIND
    ULONG_PTR ip = context->Pc;
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
        int i;
        for (i = 0; i <= 28; i++)
            unw_set_reg( &cursor, UNW_ARM64_X0 + i, context->u.X[i] );
        unw_set_reg( &cursor, UNW_ARM64_FP, context->u.s.Fp );
        unw_set_reg( &cursor, UNW_ARM64_LR, context->u.s.Lr );
        unw_set_reg( &cursor, UNW_ARM64_SP, context->Sp );
        unw_set_reg( &cursor, UNW_REG_IP,   context->Pc );
    }
#else
    memcpy( unw_context.uc_mcontext.regs, context->u.X, sizeof(context->u.X) );
    unw_context.uc_mcontext.sp = context->Sp;
    unw_context.uc_mcontext.pc = context->Pc;

    rc = unw_init_local( &cursor, &unw_context );
#endif
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
        dispatch->LanguageHandler = NULL;
        dispatch->EstablisherFrame = context->Sp;
        context->Pc = context->u.s.Lr;
        context->Sp = context->Sp + sizeof(ULONG64);
        context->ContextFlags |= CONTEXT_UNWOUND_TO_CALL;
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

    dispatch->LanguageHandler  = (void *)info.handler;
    dispatch->HandlerData      = (void *)info.lsda;
    dispatch->EstablisherFrame = context->Sp;
#ifdef __APPLE__
    {
        int i;
        for (i = 0; i <= 28; i++)
            unw_get_reg( &cursor, UNW_ARM64_X0 + i, (unw_word_t *)&context->u.X[i] );
    }
    unw_get_reg( &cursor, UNW_ARM64_FP,    (unw_word_t *)&context->u.s.Fp );
    unw_get_reg( &cursor, UNW_ARM64_X30,   (unw_word_t *)&context->u.s.Lr );
    unw_get_reg( &cursor, UNW_ARM64_SP,    (unw_word_t *)&context->Sp );
#else
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
#endif
    unw_get_reg( &cursor, UNW_REG_IP,      (unw_word_t *)&context->Pc );
    context->ContextFlags |= CONTEXT_UNWOUND_TO_CALL;

    if (info.start_ip == (unw_word_t)raise_func_trampoline) {
        /* raise_func_trampoline has a full CONTEXT stored on the stack;
         * restore the original Lr value from there. The function we unwind
         * to might be a leaf function that hasn't backed up its own original
         * Lr value on the stack.
         * We could also just restore the full context here without doing
         * unw_step at all. */
        const CONTEXT *next_ctx = (const CONTEXT *) dispatch->EstablisherFrame;
        context->u.s.Lr = next_ctx->u.s.Lr;
    }

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
    return STATUS_SUCCESS;
#else
    return STATUS_INVALID_DISPOSITION;
#endif
}


/***********************************************************************
 *           save_context
 *
 * Set the register values from a sigcontext.
 */
static void save_context( CONTEXT *context, const ucontext_t *sigcontext )
{
    DWORD i;

    context->ContextFlags = (CONTEXT_FULL & ~CONTEXT_FLOATING_POINT) |
                             CONTEXT_ARM64;
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
#ifdef linux
    struct fpsimd_context *fp = get_fpsimd_context( sigcontext );

    if (!fp) return;
    context->ContextFlags |= CONTEXT_FLOATING_POINT;
    context->Fpcr = fp->fpcr;
    context->Fpsr = fp->fpsr;
    memcpy( context->V, fp->vregs, sizeof(context->V) );
#elif defined(__APPLE__)
    context->ContextFlags |= CONTEXT_FLOATING_POINT;
    context->Fpcr = sigcontext->uc_mcontext->__ns.__fpcr;
    context->Fpsr = sigcontext->uc_mcontext->__ns.__fpsr;
    memcpy( context->V, sigcontext->uc_mcontext->__ns.__v, sizeof(context->V) );
#endif
}


/***********************************************************************
 *           restore_fpu
 *
 * Restore the FPU context to a sigcontext.
 */
static void restore_fpu( struct syscall_frame *frame, ucontext_t *sigcontext )
{
#ifdef linux
    struct fpsimd_context *fp = get_fpsimd_context( sigcontext );

    if (!fp) return;
    fp->fpcr = frame->fpcr;
    fp->fpsr = frame->fpsr;
    memcpy( fp->vregs, frame->v, sizeof(fp->vregs) );
#elif defined(__APPLE__)
    sigcontext->uc_mcontext->__ns.__fpcr = frame->fpcr;
    sigcontext->uc_mcontext->__ns.__fpsr = frame->fpsr;
    memcpy( sigcontext->uc_mcontext->__ns.__v, frame->v, sizeof(frame->v) );
#endif
}


/***********************************************************************
 *           signal_set_full_context
 */
NTSTATUS signal_set_full_context( CONTEXT *context )
{
    NTSTATUS status = NtSetContextThread( GetCurrentThread(), context );

    if (!status && (context->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) raise( SIGUSR2 );
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
    return NULL;
}


/***********************************************************************
 *              NtSetContextThread  (NTDLL.@)
 *              ZwSetContextThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetContextThread( HANDLE handle, const CONTEXT *context )
{
    struct syscall_frame *frame = arm64_thread_data()->syscall_frame;
    NTSTATUS ret = STATUS_SUCCESS;
    BOOL self = (handle == GetCurrentThread());
    DWORD flags = context->ContextFlags & ~CONTEXT_ARM64;

    if (self && (flags & CONTEXT_DEBUG_REGISTERS)) self = FALSE;

    if (!self)
    {
        ret = set_thread_context( handle, context, &self, IMAGE_FILE_MACHINE_ARM64 );
        if (ret || !self) return ret;
    }

    if (flags & CONTEXT_INTEGER)
    {
        memcpy( frame->x, context->u.X, sizeof(context->u.X[0]) * 18 );
        /* skip x18 */
        memcpy( frame->x + 19, context->u.X + 19, sizeof(context->u.X[0]) * 10 );
    }
    if (flags & CONTEXT_CONTROL)
    {
        frame->fp    = context->u.s.Fp;
        frame->lr    = context->u.s.Lr;
        frame->sp    = context->Sp;
        frame->pc    = context->Pc;
        frame->cpsr  = context->Cpsr;
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
        frame->fpcr = context->Fpcr;
        frame->fpsr = context->Fpsr;
        memcpy( frame->v, context->V, sizeof(frame->v) );
    }
    if (flags & CONTEXT_ARM64_X18)
    {
        frame->x[18] = context->u.X[18];
    }
    if (flags & CONTEXT_DEBUG_REGISTERS) FIXME( "debug registers not supported\n" );
    frame->restore_flags |= flags & ~CONTEXT_INTEGER;
    return STATUS_SUCCESS;
}


/***********************************************************************
 *              NtGetContextThread  (NTDLL.@)
 *              ZwGetContextThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtGetContextThread( HANDLE handle, CONTEXT *context )
{
    struct syscall_frame *frame = arm64_thread_data()->syscall_frame;
    DWORD needed_flags = context->ContextFlags & ~CONTEXT_ARM64;
    BOOL self = (handle == GetCurrentThread());

    if (!self)
    {
        NTSTATUS ret = get_thread_context( handle, context, &self, IMAGE_FILE_MACHINE_ARM64 );
        if (ret || !self) return ret;
    }

    if (needed_flags & CONTEXT_INTEGER)
    {
        memcpy( context->u.X, frame->x, sizeof(context->u.X[0]) * 29 );
        context->ContextFlags |= CONTEXT_INTEGER;
    }
    if (needed_flags & CONTEXT_CONTROL)
    {
        context->u.s.Fp  = frame->fp;
        context->u.s.Lr  = frame->lr;
        context->Sp      = frame->sp;
        context->Pc      = frame->pc;
        context->Cpsr    = frame->cpsr;
        context->ContextFlags |= CONTEXT_CONTROL;
    }
    if (needed_flags & CONTEXT_FLOATING_POINT)
    {
        context->Fpcr = frame->fpcr;
        context->Fpsr = frame->fpsr;
        memcpy( context->V, frame->v, sizeof(context->V) );
        context->ContextFlags |= CONTEXT_FLOATING_POINT;
    }
    if (needed_flags & CONTEXT_DEBUG_REGISTERS) FIXME( "debug registers not supported\n" );
    return STATUS_SUCCESS;
}


/***********************************************************************
 *              set_thread_wow64_context
 */
NTSTATUS set_thread_wow64_context( HANDLE handle, const void *ctx, ULONG size )
{
    BOOL self;
    USHORT machine;

    switch (size)
    {
    case sizeof(I386_CONTEXT): machine = IMAGE_FILE_MACHINE_I386; break;
    case sizeof(ARM_CONTEXT): machine = IMAGE_FILE_MACHINE_ARMNT; break;
    default: return STATUS_INFO_LENGTH_MISMATCH;
    }
    return set_thread_context( handle, ctx, &self, machine );
}


/***********************************************************************
 *              get_thread_wow64_context
 */
NTSTATUS get_thread_wow64_context( HANDLE handle, void *ctx, ULONG size )
{
    BOOL self;
    USHORT machine;

    switch (size)
    {
    case sizeof(I386_CONTEXT): machine = IMAGE_FILE_MACHINE_I386; break;
    case sizeof(ARM_CONTEXT): machine = IMAGE_FILE_MACHINE_ARMNT; break;
    default: return STATUS_INFO_LENGTH_MISMATCH;
    }
    return get_thread_context( handle, ctx, &self, machine );
}


/* Note, unwind_builtin_dll above has hardcoded assumptions on how this
 * function stores things on the stack; if modified, modify that one in
 * sync as well. */
__ASM_GLOBAL_FUNC( raise_func_trampoline,
                   "stp x29, x30, [sp, #-0x30]!\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset 48\n\t")
                   __ASM_CFI(".cfi_offset 29, -48\n\t")
                   __ASM_CFI(".cfi_offset 30, -40\n\t")
                   "stp x0,  x1,  [sp, #0x10]\n\t"
                   "str x2,       [sp, #0x20]\n\t"
                   "mov x29, sp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register 29\n\t")
                   __ASM_CFI(".cfi_remember_state\n\t")

                   /* Memcpy the context onto the stack */
                   "sub sp, sp, #0x390\n\t"
                   "mov x0,  sp\n\t"
                   "mov x2,  #0x390\n\t"
                   "bl " __ASM_NAME("memcpy") "\n\t"
                   __ASM_CFI(".cfi_def_cfa 31, 0\n\t")
                   __ASM_CFI(".cfi_escape 0x0f,0x04,0x8f,0x80,0x02,0x06\n\t") /* CFA, DW_OP_breg31 + 0x100, DW_OP_deref */
                   __ASM_CFI(".cfi_escape 0x10,0x13,0x03,0x8f,0xa0,0x01\n\t") /* x19, DW_OP_breg31 + 0xA0 */
                   __ASM_CFI(".cfi_escape 0x10,0x14,0x03,0x8f,0xa8,0x01\n\t") /* x20 */
                   __ASM_CFI(".cfi_escape 0x10,0x15,0x03,0x8f,0xb0,0x01\n\t") /* x21 */
                   __ASM_CFI(".cfi_escape 0x10,0x16,0x03,0x8f,0xb8,0x01\n\t") /* x22 */
                   __ASM_CFI(".cfi_escape 0x10,0x17,0x03,0x8f,0xc0,0x01\n\t") /* x23 */
                   __ASM_CFI(".cfi_escape 0x10,0x18,0x03,0x8f,0xc8,0x01\n\t") /* x24 */
                   __ASM_CFI(".cfi_escape 0x10,0x19,0x03,0x8f,0xd0,0x01\n\t") /* x25 */
                   __ASM_CFI(".cfi_escape 0x10,0x1a,0x03,0x8f,0xd8,0x01\n\t") /* x26 */
                   __ASM_CFI(".cfi_escape 0x10,0x1b,0x03,0x8f,0xe0,0x01\n\t") /* x27 */
                   __ASM_CFI(".cfi_escape 0x10,0x1c,0x03,0x8f,0xe8,0x01\n\t") /* x28 */
                   __ASM_CFI(".cfi_escape 0x10,0x1d,0x03,0x8f,0xf0,0x01\n\t") /* x29 */
                   __ASM_CFI(".cfi_escape 0x10,0x1e,0x03,0x8f,0x88,0x02\n\t") /* x30 = pc */
                   __ASM_CFI(".cfi_escape 0x10,0x48,0x03,0x8f,0x90,0x03\n\t") /* d8  */
                   __ASM_CFI(".cfi_escape 0x10,0x49,0x03,0x8f,0x98,0x03\n\t") /* d9  */
                   __ASM_CFI(".cfi_escape 0x10,0x4a,0x03,0x8f,0xa0,0x03\n\t") /* d10 */
                   __ASM_CFI(".cfi_escape 0x10,0x4b,0x03,0x8f,0xa8,0x03\n\t") /* d11 */
                   __ASM_CFI(".cfi_escape 0x10,0x4c,0x03,0x8f,0xb0,0x03\n\t") /* d12 */
                   __ASM_CFI(".cfi_escape 0x10,0x4d,0x03,0x8f,0xb8,0x03\n\t") /* d13 */
                   __ASM_CFI(".cfi_escape 0x10,0x4e,0x03,0x8f,0xc0,0x03\n\t") /* d14 */
                   __ASM_CFI(".cfi_escape 0x10,0x4f,0x03,0x8f,0xc8,0x03\n\t") /* d15 */
                   "ldp x0,  x1,  [x29, #0x10]\n\t"
                   "ldr x2,       [x29, #0x20]\n\t"
                   "blr x2\n\t"
                   __ASM_CFI(".cfi_restore_state\n\t")
                   "brk #1")

/***********************************************************************
 *           setup_exception
 *
 * Modify the signal context to call the exception raise function.
 */
static void setup_exception( ucontext_t *sigcontext, EXCEPTION_RECORD *rec )
{
    struct
    {
        CONTEXT           context;
        EXCEPTION_RECORD  rec;
        void             *redzone[3];
    } *stack;

    void *stack_ptr = (void *)(SP_sig(sigcontext) & ~15);
    CONTEXT context;
    NTSTATUS status;

    rec->ExceptionAddress = (void *)PC_sig(sigcontext);
    save_context( &context, sigcontext );
    save_fpu( &context, sigcontext );

    status = send_debug_event( rec, &context, TRUE );
    if (status == DBG_CONTINUE || status == DBG_EXCEPTION_HANDLED)
    {
        restore_context( &context, sigcontext );
        return;
    }

    stack = virtual_setup_exception( stack_ptr, (sizeof(*stack) + 15) & ~15, rec );
    stack->rec = *rec;
    stack->context = context;

    SP_sig(sigcontext) = (ULONG_PTR)stack;
    LR_sig(sigcontext) = PC_sig(sigcontext);
    PC_sig(sigcontext) = (ULONG_PTR)raise_func_trampoline;
    REGn_sig(0, sigcontext) = (ULONG_PTR)&stack->rec;  /* first arg for KiUserExceptionDispatcher */
    REGn_sig(1, sigcontext) = (ULONG_PTR)&stack->context; /* second arg for KiUserExceptionDispatcher */
    REGn_sig(2, sigcontext) = (ULONG_PTR)pKiUserExceptionDispatcher; /* dispatcher arg for raise_func_trampoline */
    REGn_sig(18, sigcontext) = (ULONG_PTR)NtCurrentTeb();
}


/***********************************************************************
 *           call_user_apc_dispatcher
 */
NTSTATUS call_user_apc_dispatcher( CONTEXT *context, ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3,
                                   PNTAPCFUNC func, NTSTATUS status )
{
    struct syscall_frame *frame = arm64_thread_data()->syscall_frame;
    ULONG64 sp = context ? context->Sp : frame->sp;
    struct apc_stack_layout { CONTEXT context; } *stack;

    sp &= ~15;
    stack = (struct apc_stack_layout *)sp - 1;
    if (context)
    {
        memmove( &stack->context, context, sizeof(stack->context) );
        NtSetContextThread( GetCurrentThread(), &stack->context );
    }
    else
    {
        stack->context.ContextFlags = CONTEXT_FULL;
        NtGetContextThread( GetCurrentThread(), &stack->context );
        stack->context.u.s.X0 = status;
    }
    frame->sp   = (ULONG64)stack;
    frame->pc   = (ULONG64)pKiUserApcDispatcher;
    frame->x[0] = (ULONG64)&stack->context;
    frame->x[1] = arg1;
    frame->x[2] = arg2;
    frame->x[3] = arg3;
    frame->x[4] = (ULONG64)func;
    frame->restore_flags |= CONTEXT_CONTROL | CONTEXT_INTEGER;
    return status;
}


/***********************************************************************
 *           call_raise_user_exception_dispatcher
 */
void call_raise_user_exception_dispatcher(void)
{
    arm64_thread_data()->syscall_frame->pc = (UINT64)pKiRaiseUserExceptionDispatcher;
}


/***********************************************************************
 *           call_user_exception_dispatcher
 */
NTSTATUS call_user_exception_dispatcher( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    struct syscall_frame *frame = arm64_thread_data()->syscall_frame;
    NTSTATUS status = NtSetContextThread( GetCurrentThread(), context );

    if (status) return status;
    frame->x[0] = (ULONG64)rec;
    frame->x[1] = (ULONG64)context;
    frame->pc   = (ULONG64)pKiUserExceptionDispatcher;
    frame->restore_flags |= CONTEXT_INTEGER | CONTEXT_CONTROL;
    return status;
}


/***********************************************************************
 *           handle_syscall_fault
 *
 * Handle a page fault happening during a system call.
 */
static BOOL handle_syscall_fault( ucontext_t *context, EXCEPTION_RECORD *rec )
{
    struct syscall_frame *frame = arm64_thread_data()->syscall_frame;
    DWORD i;

    if (!is_inside_syscall( context )) return FALSE;

    TRACE( "code=%x flags=%x addr=%p pc=%p tid=%04x\n",
           rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress,
           (void *)PC_sig(context), GetCurrentThreadId() );
    for (i = 0; i < rec->NumberParameters; i++)
        TRACE( " info[%d]=%016lx\n", i, rec->ExceptionInformation[i] );

    TRACE("  x0=%016lx  x1=%016lx  x2=%016lx  x3=%016lx\n",
          (DWORD64)REGn_sig(0, context), (DWORD64)REGn_sig(1, context),
          (DWORD64)REGn_sig(2, context), (DWORD64)REGn_sig(3, context) );
    TRACE("  x4=%016lx  x5=%016lx  x6=%016lx  x7=%016lx\n",
          (DWORD64)REGn_sig(4, context), (DWORD64)REGn_sig(5, context),
          (DWORD64)REGn_sig(6, context), (DWORD64)REGn_sig(7, context) );
    TRACE("  x8=%016lx  x9=%016lx x10=%016lx x11=%016lx\n",
          (DWORD64)REGn_sig(8, context), (DWORD64)REGn_sig(9, context),
          (DWORD64)REGn_sig(10, context), (DWORD64)REGn_sig(11, context) );
    TRACE(" x12=%016lx x13=%016lx x14=%016lx x15=%016lx\n",
          (DWORD64)REGn_sig(12, context), (DWORD64)REGn_sig(13, context),
          (DWORD64)REGn_sig(14, context), (DWORD64)REGn_sig(15, context) );
    TRACE(" x16=%016lx x17=%016lx x18=%016lx x19=%016lx\n",
          (DWORD64)REGn_sig(16, context), (DWORD64)REGn_sig(17, context),
          (DWORD64)REGn_sig(18, context), (DWORD64)REGn_sig(19, context) );
    TRACE(" x20=%016lx x21=%016lx x22=%016lx x23=%016lx\n",
          (DWORD64)REGn_sig(20, context), (DWORD64)REGn_sig(21, context),
          (DWORD64)REGn_sig(22, context), (DWORD64)REGn_sig(23, context) );
    TRACE(" x24=%016lx x25=%016lx x26=%016lx x27=%016lx\n",
          (DWORD64)REGn_sig(24, context), (DWORD64)REGn_sig(25, context),
          (DWORD64)REGn_sig(26, context), (DWORD64)REGn_sig(27, context) );
    TRACE(" x28=%016lx  fp=%016lx  lr=%016lx  sp=%016lx\n",
          (DWORD64)REGn_sig(28, context), (DWORD64)FP_sig(context),
          (DWORD64)LR_sig(context), (DWORD64)SP_sig(context) );

    if (ntdll_get_thread_data()->jmp_buf)
    {
        TRACE( "returning to handler\n" );
        REGn_sig(0, context) = (ULONG_PTR)ntdll_get_thread_data()->jmp_buf;
        REGn_sig(1, context) = 1;
        PC_sig(context)      = (ULONG_PTR)__wine_longjmp;
        ntdll_get_thread_data()->jmp_buf = NULL;
    }
    else
    {
        TRACE( "returning to user mode ip=%p ret=%08x\n", (void *)frame->pc, rec->ExceptionCode );
        REGn_sig(0, context) = (ULONG_PTR)frame;
        REGn_sig(1, context) = rec->ExceptionCode;
        PC_sig(context)      = (ULONG_PTR)__wine_syscall_dispatcher_return;
    }
    return TRUE;
}


/**********************************************************************
 *		segv_handler
 *
 * Handler for SIGSEGV.
 */
static void segv_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    EXCEPTION_RECORD rec = { 0 };
    ucontext_t *context = sigcontext;

    rec.NumberParameters = 2;
    rec.ExceptionInformation[0] = (get_fault_esr( context ) & 0x40) != 0;
    rec.ExceptionInformation[1] = (ULONG_PTR)siginfo->si_addr;
    rec.ExceptionCode = virtual_handle_fault( siginfo->si_addr, rec.ExceptionInformation[0],
                                              (void *)SP_sig(context) );
    if (!rec.ExceptionCode) return;
    if (handle_syscall_fault( context, &rec )) return;
    setup_exception( context, &rec );
}


/**********************************************************************
 *		ill_handler
 *
 * Handler for SIGILL.
 */
static void ill_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    EXCEPTION_RECORD rec = { EXCEPTION_ILLEGAL_INSTRUCTION };

    setup_exception( sigcontext, &rec );
}


/**********************************************************************
 *		bus_handler
 *
 * Handler for SIGBUS.
 */
static void bus_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    EXCEPTION_RECORD rec = { EXCEPTION_DATATYPE_MISALIGNMENT };

    setup_exception( sigcontext, &rec );
}


/**********************************************************************
 *		trap_handler
 *
 * Handler for SIGTRAP.
 */
static void trap_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    EXCEPTION_RECORD rec = { 0 };

    switch (siginfo->si_code)
    {
    case TRAP_TRACE:
        rec.ExceptionCode = EXCEPTION_SINGLE_STEP;
        break;
    case TRAP_BRKPT:
    default:
        rec.ExceptionCode = EXCEPTION_BREAKPOINT;
        rec.NumberParameters = 1;
        break;
    }
    setup_exception( sigcontext, &rec );
}


/**********************************************************************
 *		fpe_handler
 *
 * Handler for SIGFPE.
 */
static void fpe_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    EXCEPTION_RECORD rec = { 0 };

    switch (siginfo->si_code & 0xffff )
    {
#ifdef FPE_FLTSUB
    case FPE_FLTSUB:
        rec.ExceptionCode = EXCEPTION_ARRAY_BOUNDS_EXCEEDED;
        break;
#endif
#ifdef FPE_INTDIV
    case FPE_INTDIV:
        rec.ExceptionCode = EXCEPTION_INT_DIVIDE_BY_ZERO;
        break;
#endif
#ifdef FPE_INTOVF
    case FPE_INTOVF:
        rec.ExceptionCode = EXCEPTION_INT_OVERFLOW;
        break;
#endif
#ifdef FPE_FLTDIV
    case FPE_FLTDIV:
        rec.ExceptionCode = EXCEPTION_FLT_DIVIDE_BY_ZERO;
        break;
#endif
#ifdef FPE_FLTOVF
    case FPE_FLTOVF:
        rec.ExceptionCode = EXCEPTION_FLT_OVERFLOW;
        break;
#endif
#ifdef FPE_FLTUND
    case FPE_FLTUND:
        rec.ExceptionCode = EXCEPTION_FLT_UNDERFLOW;
        break;
#endif
#ifdef FPE_FLTRES
    case FPE_FLTRES:
        rec.ExceptionCode = EXCEPTION_FLT_INEXACT_RESULT;
        break;
#endif
#ifdef FPE_FLTINV
    case FPE_FLTINV:
#endif
    default:
        rec.ExceptionCode = EXCEPTION_FLT_INVALID_OPERATION;
        break;
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

    if (is_inside_syscall( sigcontext ))
    {
        context.ContextFlags = CONTEXT_FULL;
        NtGetContextThread( GetCurrentThread(), &context );
        wait_suspend( &context );
        NtSetContextThread( GetCurrentThread(), &context );
    }
    else
    {
        save_context( &context, sigcontext );
        wait_suspend( &context );
        restore_context( &context, sigcontext );
    }
}


/**********************************************************************
 *		usr2_handler
 *
 * Handler for SIGUSR2, used to set a thread context.
 */
static void usr2_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    struct syscall_frame *frame = arm64_thread_data()->syscall_frame;
    ucontext_t *context = sigcontext;
    DWORD i;

    if (!is_inside_syscall( sigcontext )) return;

    FP_sig(context)     = frame->fp;
    LR_sig(context)     = frame->lr;
    SP_sig(context)     = frame->sp;
    PC_sig(context)     = frame->pc;
    PSTATE_sig(context) = frame->cpsr;
    for (i = 0; i <= 28; i++) REGn_sig( i, context ) = frame->x[i];
    restore_fpu( frame, context );
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
    pthread_key_create( &teb_key, NULL );
}


/**********************************************************************
 *             signal_alloc_thread
 */
NTSTATUS signal_alloc_thread( TEB *teb )
{
    return STATUS_SUCCESS;
}


/**********************************************************************
 *             signal_free_thread
 */
void signal_free_thread( TEB *teb )
{
}


/**********************************************************************
 *		signal_init_thread
 */
void signal_init_thread( TEB *teb )
{
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
    void *kernel_stack = (char *)ntdll_get_thread_data()->kernel_stack + kernel_stack_size;

    arm64_thread_data()->syscall_frame = (struct syscall_frame *)kernel_stack - 1;

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
    sig_act.sa_sigaction = usr2_handler;
    if (sigaction( SIGUSR2, &sig_act, NULL ) == -1) goto error;
    sig_act.sa_sigaction = trap_handler;
    if (sigaction( SIGTRAP, &sig_act, NULL ) == -1) goto error;
    sig_act.sa_sigaction = segv_handler;
    if (sigaction( SIGSEGV, &sig_act, NULL ) == -1) goto error;
    sig_act.sa_sigaction = ill_handler;
    if (sigaction( SIGILL, &sig_act, NULL ) == -1) goto error;
    sig_act.sa_sigaction = bus_handler;
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
    struct arm64_thread_data *thread_data = (struct arm64_thread_data *)&teb->GdiTebBatch;
    struct syscall_frame *frame = thread_data->syscall_frame;
    CONTEXT *ctx, context = { CONTEXT_ALL };

    context.u.s.X0  = (DWORD64)entry;
    context.u.s.X1  = (DWORD64)arg;
    context.u.s.X18 = (DWORD64)teb;
    context.Sp      = (DWORD64)teb->Tib.StackBase;
    context.Pc      = (DWORD64)pRtlUserThreadStart;

    if (suspend) wait_suspend( &context );

    ctx = (CONTEXT *)((ULONG_PTR)context.Sp & ~15) - 1;
    *ctx = context;
    ctx->ContextFlags = CONTEXT_FULL;
    NtSetContextThread( GetCurrentThread(), ctx );

    frame->sp    = (ULONG64)ctx;
    frame->pc    = (ULONG64)pLdrInitializeThunk;
    frame->x[0]  = (ULONG64)ctx;
    frame->x[18] = (ULONG64)teb;
    frame->restore_flags |= CONTEXT_INTEGER;

    pthread_sigmask( SIG_UNBLOCK, &server_block_set, NULL );
    __wine_syscall_dispatcher_return( frame, 0 );
}


/***********************************************************************
 *           signal_start_thread
 */
__ASM_GLOBAL_FUNC( signal_start_thread,
                   "stp x29, x30, [sp,#-16]!\n\t"
                   /* store exit frame */
                   "mov x29, sp\n\t"
                   "str x29, [x3, #0x2f0]\n\t"  /* arm64_thread_data()->exit_frame */
                   /* set syscall frame */
                   "ldr x8, [x3, #0x2f8]\n\t"   /* arm64_thread_data()->syscall_frame */
                   "cbnz x8, 1f\n\t"
                   "sub x8, sp, #0x320\n\t"     /* sizeof(struct syscall_frame) */
                   "str x8, [x3, #0x2f8]\n\t"   /* arm64_thread_data()->syscall_frame */
                   "1:\tmov sp, x8\n\t"
                   "bl " __ASM_NAME("call_init_thunk") )

/***********************************************************************
 *           signal_exit_thread
 */
__ASM_GLOBAL_FUNC( signal_exit_thread,
                   "stp x29, x30, [sp,#-16]!\n\t"
                   "ldr x3, [x2, #0x2f0]\n\t"  /* arm64_thread_data()->exit_frame */
                   "str xzr, [x2, #0x2f0]\n\t"
                   "cbz x3, 1f\n\t"
                   "mov sp, x3\n"
                   "1:\tldp x29, x30, [sp], #16\n\t"
                   "br x1" )

/**********************************************************************
 *           NtCurrentTeb   (NTDLL.@)
 */
TEB * WINAPI NtCurrentTeb(void)
{
    return pthread_getspecific( teb_key );
}

#endif  /* __aarch64__ */
