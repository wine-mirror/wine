/*
 * PowerPC64 signal handling routines
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

#ifdef __powerpc64__

#include "config.h"

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
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
#include "winnt.h"
#include "winternl.h"
#include "wine/asm.h"
#include "unix_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);


/***********************************************************************
 * signal context platform-specific definitions
 *
 * On 64-bit Linux/PowerPC a machine context is a sigcontext: uc_mcontext
 * carries the 48-entry gp_regs array (the same layout ptrace uses), 33
 * doubles of fp_regs (f0-f31 plus FPSCR), and a *pointer* to the VMX state
 * which is NULL on a machine without AltiVec.  The PT_* indices come from
 * <asm/ptrace.h>; they are spelled out here so that this file does not have
 * to pull a kernel header into the Wine namespace.
 */
#define PPC64_PT_NIP    32
#define PPC64_PT_MSR    33
#define PPC64_PT_CTR    35
#define PPC64_PT_LNK    36
#define PPC64_PT_XER    37
#define PPC64_PT_CCR    38
#define PPC64_PT_TRAP   40
#define PPC64_PT_DAR    41
#define PPC64_PT_DSISR  42

#define GPR_sig(n,ctx)  ((ctx)->uc_mcontext.gp_regs[n])
#define SP_sig(ctx)     GPR_sig(1,ctx)
#define NIP_sig(ctx)    GPR_sig(PPC64_PT_NIP,ctx)
#define TRAP_sig(ctx)   GPR_sig(PPC64_PT_TRAP,ctx)
#define DSISR_sig(ctx)  GPR_sig(PPC64_PT_DSISR,ctx)

/* DSISR bit 6 (0x02000000 in the big-endian bit numbering the ISA uses) is set
 * when the access that faulted was a store.  TRAP == 0x400 is an instruction
 * storage interrupt, i.e. a fetch fault; 0x300 is a data storage interrupt. */
#define PPC64_DSISR_STORE   0x02000000
#define PPC64_TRAP_ISI      0x400

/* the ELFv2 red zone: 288 bytes below r1 are scratch for the current function */
#define PPC64_RED_ZONE      288
/* an ELFv2 stack frame owes its callee a 32-byte linkage area plus a 64-byte
 * parameter save area for the eight GPR argument registers */
#define PPC64_LINKAGE       96

/* first stack argument of a syscall: 32 (linkage) + 8*8 (r3-r10 save slots) */
#define PPC64_STACK_ARGS    96


/***********************************************************************
 *           the TEB, on an architecture with no register to spare
 *
 * r13 is glibc's thread pointer and r2 is the TOC, so there is no reserved
 * TEB register the way x86_64 has gs and arm64 has x18.  The PE-side syscall
 * thunks hand the TEB to __wine_syscall_dispatcher in r0, but
 * __wine_unix_call_dispatcher is reached by an ordinary indirect call from
 * arbitrary PE code and gets nothing, so the unix library keeps its own
 * initial-exec thread-local copy that assembly can reach with the standard
 * @got@tprel sequence.  Initial-exec (rather than global-dynamic) matters:
 * a __tls_get_addr call would need a stack frame and a TOC we do not have yet.
 */
__attribute__((visibility("hidden"))) __thread TEB *ppc64_unix_teb
    __attribute__((tls_model("initial-exec")));


/* stack layout when calling KiUserExceptionDispatcher.
 *
 * The first 96 bytes are the ELFv2 linkage and parameter save area that the
 * dispatcher, an ordinary C function, is entitled to write into its caller's
 * frame.  Everything the dispatcher is passed lives above that. */
struct exc_stack_layout
{
    ULONG64              linkage[12];    /* 000 back chain, CR, LR, TOC, 8 param slots */
    ULONG64              align[2];       /* 060 */
    CONTEXT              context;        /* 070 */
    EXCEPTION_RECORD     rec;            /* 510 */
};
C_ASSERT( offsetof(struct exc_stack_layout, context) == 0x70 );

/* stack layout when calling KiUserApcDispatcher */
struct apc_stack_layout
{
    ULONG64              linkage[12];    /* 000 */
    ULONG64              align[2];       /* 060 */
    CONTEXT              context;        /* 070 */
};
C_ASSERT( offsetof(struct apc_stack_layout, context) == 0x70 );

/* stack layout when calling KiUserCallbackDispatcher */
struct callback_stack_layout
{
    ULONG64              linkage[12];    /* 000 */
    void                *args;           /* 060 arguments */
    ULONG                len;            /* 068 arguments len */
    ULONG                id;             /* 06c function id */
    ULONG64              lr;             /* 070 */
    ULONG64              sp;             /* 078 */
    ULONG64              pc;             /* 080 */
    ULONG64              align;          /* 088 */
    BYTE                 args_data[0];   /* 090 copied argument data */
};
C_ASSERT( offsetof(struct callback_stack_layout, args_data) == 0x90 );


/***********************************************************************
 *           struct syscall_frame
 *
 * Kept in sync with the FRAME_* offsets used by the assembly below; every
 * offset has a C_ASSERT.  gpr[1] is the user stack pointer and gpr[2] the
 * user TOC; there are no separate fields for them.
 */
struct syscall_frame
{
    ULONG64               gpr[32];        /* 000 */
    ULONG64               lr;             /* 100 */
    ULONG64               ctr;            /* 108 */
    ULONG64               cr;             /* 110 */
    ULONG64               xer;            /* 118 */
    ULONG64               pc;             /* 120 */
    ULONG64               fpscr;          /* 128 */
    ULONG                 restore_flags;  /* 130 */
    ULONG                 syscall_id;     /* 134 */
    struct syscall_frame *prev_frame;     /* 138 */
    void                 *syscall_cfa;    /* 140 */
    TEB                  *teb;            /* 148 */
    double                fpr[32];        /* 150 */
    VECTOR128             v[32];          /* 250 */
};

C_ASSERT( offsetof(struct syscall_frame, lr)            == 0x100 );
C_ASSERT( offsetof(struct syscall_frame, ctr)           == 0x108 );
C_ASSERT( offsetof(struct syscall_frame, cr)            == 0x110 );
C_ASSERT( offsetof(struct syscall_frame, xer)           == 0x118 );
C_ASSERT( offsetof(struct syscall_frame, pc)            == 0x120 );
C_ASSERT( offsetof(struct syscall_frame, fpscr)         == 0x128 );
C_ASSERT( offsetof(struct syscall_frame, restore_flags) == 0x130 );
C_ASSERT( offsetof(struct syscall_frame, syscall_id)    == 0x134 );
C_ASSERT( offsetof(struct syscall_frame, prev_frame)    == 0x138 );
C_ASSERT( offsetof(struct syscall_frame, syscall_cfa)   == 0x140 );
C_ASSERT( offsetof(struct syscall_frame, teb)           == 0x148 );
C_ASSERT( offsetof(struct syscall_frame, fpr)           == 0x150 );
C_ASSERT( offsetof(struct syscall_frame, v)             == 0x250 );
C_ASSERT( sizeof(struct syscall_frame) == 0x450 );

/* the TEB offsets the assembly hardcodes */
C_ASSERT( offsetof( TEB, GdiTebBatch ) + offsetof( struct teb_data, syscall_table ) == 0x370 );
C_ASSERT( offsetof( TEB, GdiTebBatch ) + offsetof( struct teb_data, syscall_frame ) == 0x378 );
C_ASSERT( offsetof( TEB, GdiTebBatch ) + offsetof( struct teb_data, syscall_trace ) == 0x380 );

/* SYSTEM_SERVICE_TABLE offsets the assembly hardcodes */
C_ASSERT( sizeof(SYSTEM_SERVICE_TABLE) == 32 );
C_ASSERT( offsetof(SYSTEM_SERVICE_TABLE, ServiceTable) == 0 );
C_ASSERT( offsetof(SYSTEM_SERVICE_TABLE, ServiceLimit) == 16 );
C_ASSERT( offsetof(SYSTEM_SERVICE_TABLE, ArgumentTable) == 24 );

/* the restore_flags bits the assembly tests, with the CONTEXT_PPC64 tag removed */
C_ASSERT( (CONTEXT_CONTROL & 0xff) == 0x01 );
C_ASSERT( (CONTEXT_INTEGER & 0xff) == 0x02 );
C_ASSERT( (CONTEXT_FLOATING_POINT & 0xff) == 0x04 );
C_ASSERT( (CONTEXT_VECTOR & 0xff) == 0x10 );

extern void __wine_syscall_dispatcher_return(void);
extern void init_syscall_frame( LPTHREAD_START_ROUTINE entry, void *arg, TEB *teb );


/***********************************************************************
 *           set_process_instrumentation_callback
 */
void set_process_instrumentation_callback( void *callback )
{
    if (callback) FIXME( "Not supported.\n" );
}


/***********************************************************************
 *           save_fpu
 */
static void save_fpu( CONTEXT *context, const ucontext_t *sigcontext )
{
    DWORD i;

    for (i = 0; i < 32; i++) (&context->Fpr0)[i] = sigcontext->uc_mcontext.fp_regs[i];
    context->Fpscr = sigcontext->uc_mcontext.fp_regs[32];
}


/***********************************************************************
 *           restore_fpu
 */
static void restore_fpu( const CONTEXT *context, ucontext_t *sigcontext )
{
    DWORD i;

    for (i = 0; i < 32; i++) sigcontext->uc_mcontext.fp_regs[i] = (&context->Fpr0)[i];
    sigcontext->uc_mcontext.fp_regs[32] = context->Fpscr;
}


/***********************************************************************
 *           save_vmx
 *
 * v_regs is NULL on a processor without AltiVec, and the kernel documents
 * that it must not be dereferenced in that case.
 */
static void save_vmx( CONTEXT *context, const ucontext_t *sigcontext )
{
    const vrregset_t *v = sigcontext->uc_mcontext.v_regs;

    if (!v) return;
    memcpy( context->Vr, v->vrregs, sizeof(context->Vr) );
    context->Vscr = v->vscr.vscr_word;
    context->Vrsave = v->vrsave;
}


/***********************************************************************
 *           restore_vmx
 */
static void restore_vmx( const CONTEXT *context, ucontext_t *sigcontext )
{
    vrregset_t *v = sigcontext->uc_mcontext.v_regs;

    if (!v) return;
    memcpy( v->vrregs, context->Vr, sizeof(context->Vr) );
    v->vscr.vscr_word = context->Vscr;
    v->vrsave = context->Vrsave;
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
    for (i = 0; i < 32; i++) (&context->Gpr0)[i] = GPR_sig( i, sigcontext );
    context->Cr    = GPR_sig( PPC64_PT_CCR, sigcontext );
    context->Xer   = GPR_sig( PPC64_PT_XER, sigcontext );
    context->Msr   = GPR_sig( PPC64_PT_MSR, sigcontext );
    context->Iar   = GPR_sig( PPC64_PT_NIP, sigcontext );
    context->Lr    = GPR_sig( PPC64_PT_LNK, sigcontext );
    context->Ctr   = GPR_sig( PPC64_PT_CTR, sigcontext );
    context->Dar   = GPR_sig( PPC64_PT_DAR, sigcontext );
    context->Dsisr = GPR_sig( PPC64_PT_DSISR, sigcontext );
    context->Trap  = GPR_sig( PPC64_PT_TRAP, sigcontext );
    save_fpu( context, sigcontext );
    save_vmx( context, sigcontext );
}


/***********************************************************************
 *           restore_context
 *
 * Build a sigcontext from the register values.  MSR, DAR, DSISR and TRAP are
 * capture-only: the kernel owns them and writing them back is meaningless.
 */
static void restore_context( const CONTEXT *context, ucontext_t *sigcontext )
{
    DWORD i;

    for (i = 0; i < 32; i++) GPR_sig( i, sigcontext ) = (&context->Gpr0)[i];
    GPR_sig( PPC64_PT_CCR, sigcontext ) = context->Cr;
    GPR_sig( PPC64_PT_XER, sigcontext ) = context->Xer;
    GPR_sig( PPC64_PT_NIP, sigcontext ) = context->Iar;
    GPR_sig( PPC64_PT_LNK, sigcontext ) = context->Lr;
    GPR_sig( PPC64_PT_CTR, sigcontext ) = context->Ctr;
    restore_fpu( context, sigcontext );
    restore_vmx( context, sigcontext );
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
    return NULL;  /* no WoW64 machines are supported on PowerPC64 */
}


/***********************************************************************
 *              NtSetContextThread  (NTDLL.@)
 *              ZwSetContextThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetContextThread( HANDLE handle, const CONTEXT *context )
{
    struct thread_data *data = get_thread_data();
    struct syscall_frame *frame = get_syscall_frame( data );
    DWORD flags = context->ContextFlags & ~CONTEXT_PPC64;
    BOOL self = (handle == GetCurrentThread());
    NTSTATUS ret;
    DWORD i;
    /* Function scope on purpose: the cross-thread arm below can repoint
     * "context" at it, and the local-frame path further down may still read it. */
    CONTEXT merged;

    if (!self)
    {
        /* One read-modify-write *is* needed, for the floating-point half.  The
         * server keeps the FPRs and the VMX registers in a single wire block
         * (fp.powerpc64_regs holds fpr[]/fpscr *and* vr[]/vscr/vrsave) behind a
         * single SERVER_CTX_FLOATING_POINT flag, while CONTEXT has two
         * independent flags for those halves.  context_to_server() memsets the
         * wire context, so a request naming only one half shipped the other half
         * as zeros and the server installed the whole block: a VECTOR-only set
         * wiped the target's f0-f31 and Fpscr (measured -- probes/vector-only-set.c
         * reported "sent 1000.25, read back 0"), and an FP-only set wiped
         * Vr/Vscr/Vrsave the same way.  Read the half the caller did not send out
         * of the target and send both halves, so that a partial set is still a
         * set of exactly what was asked for.
         *
         * No such read-modify-write is needed for a CONTROL-only request: Gpr1
         * has its own slot in the server's control block
         * (ctl.powerpc64_regs.gpr1), so the stack pointer travels without
         * dragging the integer block -- which copy_context() copies wholesale,
         * and which used to arrive with 31 zeroed GPRs and a zeroed Cr/Xer when a
         * CONTROL-only set faked the integer flag.
         * probes/check-thread-context.sh covers both cases. */
        /* "flags" already has the CONTEXT_PPC64 base bit stripped, so the
         * constants must have it stripped too -- these are equality tests, not
         * the loose bit tests the rest of this function gets away with. */
        const DWORD fp_both = (CONTEXT_FLOATING_POINT | CONTEXT_VECTOR) & ~CONTEXT_PPC64;
        DWORD fp_half = flags & fp_both;

        if (fp_half && fp_half != fp_both)
        {
            CONTEXT cur;
            BOOL cur_self = FALSE;

            memset( &cur, 0, sizeof(cur) );
            cur.ContextFlags = CONTEXT_FLOATING_POINT | CONTEXT_VECTOR;
            ret = get_thread_context( handle, &cur, &cur_self, IMAGE_FILE_MACHINE_POWERPC64 );
            if (ret) return ret;
            /* A real handle to this very thread: the server never filled cur, and
             * the frame path below already applies only the requested half, so
             * there is nothing to merge. */
            if (!cur_self)
            {
                merged = *context;
                if (fp_half == (CONTEXT_VECTOR & ~CONTEXT_PPC64))   /* refill the FPR half */
                {
                    /* memcpy, not assignment: these are 64-bit register images,
                     * and a double copy of a signalling NaN is allowed to
                     * quieten the bit pattern.  Same reason Fpscr is memcpy'd on
                     * the wire in dlls/ntdll/unix/thread.c. */
                    memcpy( &merged.Fpr0, &cur.Fpr0, 32 * sizeof(merged.Fpr0) );
                    memcpy( &merged.Fpscr, &cur.Fpscr, sizeof(merged.Fpscr) );
                }
                else
                {
                    memcpy( merged.Vr, cur.Vr, sizeof(merged.Vr) );
                    merged.Vscr   = cur.Vscr;
                    merged.Vrsave = cur.Vrsave;
                }
                merged.ContextFlags |= CONTEXT_FLOATING_POINT | CONTEXT_VECTOR;
                context = &merged;
                flags   = merged.ContextFlags & ~CONTEXT_PPC64;
            }
        }
        ret = set_thread_context( handle, context, &self, IMAGE_FILE_MACHINE_POWERPC64 );
        if (ret || !self) return ret;
    }
    else if (!frame) return STATUS_ACCESS_DENIED;

    if (flags & CONTEXT_INTEGER)
    {
        for (i = 0; i < 32; i++)
        {
            if (i == 1 || i == 13) continue;  /* sp is CONTROL; r13 is the thread pointer */
            frame->gpr[i] = (&context->Gpr0)[i];
        }
        frame->cr  = context->Cr;
        frame->xer = context->Xer;
    }
    if (flags & CONTEXT_CONTROL)
    {
        frame->gpr[1] = context->Gpr1;
        frame->pc     = context->Iar;
        frame->lr     = context->Lr;
        frame->ctr    = context->Ctr;
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
        for (i = 0; i < 32; i++) frame->fpr[i] = (&context->Fpr0)[i];
        frame->fpscr = *(const ULONG64 *)&context->Fpscr;
    }
    if (flags & CONTEXT_VECTOR) memcpy( frame->v, context->Vr, sizeof(frame->v) );

    frame->restore_flags |= flags & ~CONTEXT_INTEGER;
    return STATUS_SUCCESS;
}


/***********************************************************************
 *              NtGetContextThread  (NTDLL.@)
 *              ZwGetContextThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtGetContextThread( HANDLE handle, CONTEXT *context )
{
    struct thread_data *data = get_thread_data();
    struct syscall_frame *frame = get_syscall_frame( data );
    DWORD needed_flags = context->ContextFlags & ~CONTEXT_PPC64;
    BOOL self = (handle == GetCurrentThread());
    DWORD flags, i;

    if (!self)
    {
        /* A real handle to this thread comes back self=TRUE and is handled below;
         * a genuine cross-thread request is served entirely by the conversion
         * arms in thread.c.  The buffer-left-untouched case that used to make
         * this return STATUS_SUCCESS over uninitialised memory is now caught
         * inside get_thread_context() itself, which is the only layer that can
         * see the server returned no context blocks. */
        NTSTATUS ret = get_thread_context( handle, context, &self, IMAGE_FILE_MACHINE_POWERPC64 );
        if (ret || !self) return ret;
    }
    else if (!frame) return STATUS_ACCESS_DENIED;

    if (needed_flags & CONTEXT_INTEGER)
    {
        for (i = 0; i < 32; i++) (&context->Gpr0)[i] = frame->gpr[i];
        context->Cr  = frame->cr;
        context->Xer = frame->xer;
        context->ContextFlags |= CONTEXT_INTEGER;
    }
    if (needed_flags & CONTEXT_CONTROL)
    {
        context->Gpr1 = frame->gpr[1];
        context->Iar  = frame->pc;
        context->Lr   = frame->lr;
        context->Ctr  = frame->ctr;
        context->Msr  = 0;
        context->ContextFlags |= CONTEXT_CONTROL;
    }
    if (needed_flags & CONTEXT_FLOATING_POINT)
    {
        for (i = 0; i < 32; i++) (&context->Fpr0)[i] = frame->fpr[i];
        *(ULONG64 *)&context->Fpscr = frame->fpscr;
        context->ContextFlags |= CONTEXT_FLOATING_POINT;
    }
    if (needed_flags & CONTEXT_VECTOR)
    {
        memcpy( context->Vr, frame->v, sizeof(frame->v) );
        context->ContextFlags |= CONTEXT_VECTOR;
    }
    flags = context->ContextFlags;
    set_context_exception_reporting_flags( &flags, CONTEXT_SERVICE_ACTIVE );
    context->ContextFlags = flags;
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           signal_set_full_context
 */
NTSTATUS signal_set_full_context( CONTEXT *context )
{
    NTSTATUS status = NtSetContextThread( GetCurrentThread(), context );

    if (!status && (context->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER)
        get_syscall_frame( get_thread_data() )->restore_flags |= CONTEXT_INTEGER;
    return status;
}


/***********************************************************************
 *              get_thread_wow64_context
 */
NTSTATUS get_thread_wow64_context( HANDLE handle, void *ctx, ULONG size )
{
    return STATUS_INVALID_INFO_CLASS;
}


/***********************************************************************
 *              set_thread_wow64_context
 */
NTSTATUS set_thread_wow64_context( HANDLE handle, const void *ctx, ULONG size )
{
    return STATUS_INVALID_INFO_CLASS;
}


/**********************************************************************
 *           get_thread_ldt_entry
 */
NTSTATUS get_thread_ldt_entry( HANDLE handle, THREAD_DESCRIPTOR_INFORMATION *info, ULONG len )
{
    return STATUS_NOT_IMPLEMENTED;
}


/***********************************************************************
 *           unwind_builtin_dll
 *
 * Never reached on this architecture.  The only caller of the
 * unix_unwind_builtin_dll unixlib entry is dlls/ntdll/signal_x86_64.c; the
 * PE-side ppc64 RtlVirtualUnwind walks the ELFv2 back chain instead and never
 * asks the unix side for DWARF.  dlls/ntdll/unix/dwarf.h has no ppc64 register
 * tables and this port does not need them.  i386 stubs this out the same way.
 */
NTSTATUS unwind_builtin_dll( void *args )
{
    return STATUS_UNSUCCESSFUL;
}


/***********************************************************************
 *           set_frame_entry
 *
 * Point the syscall frame at a PE-side entry point.  ELFv2 requires r12 to
 * hold the callee's address at a global entry point, because that is what the
 * prologue the compiler emits uses to compute the TOC; every KiUser* function
 * in dlls/ntdll/signal_ppc64.c documents this contract.
 */
static void set_frame_entry( struct syscall_frame *frame, void *entry )
{
    frame->pc = (ULONG64)entry;
    frame->gpr[12] = (ULONG64)entry;
    frame->restore_flags |= CONTEXT_CONTROL | CONTEXT_INTEGER;
}


/***********************************************************************
 *           call_user_apc_dispatcher
 */
NTSTATUS call_user_apc_dispatcher( CONTEXT *context, unsigned int flags, ULONG_PTR arg1, ULONG_PTR arg2,
                                   ULONG_PTR arg3, PNTAPCFUNC func, NTSTATUS status )
{
    struct thread_data *data = get_thread_data();
    struct syscall_frame *frame = get_syscall_frame( data );
    ULONG64 sp = context ? context->Gpr1 : frame->gpr[1];
    ULONG64 caller_sp = sp;
    struct apc_stack_layout *stack;

    if (flags) FIXME( "flags %#x are not supported.\n", flags );

    sp = (sp - PPC64_RED_ZONE) & ~15;
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
        stack->context.Gpr3 = status;
    }
    /* Back chain points at the interrupted frame, not at `sp`: `sp` is the
     * cursor after the red-zone skip, so it addresses the middle of the 288-byte
     * gap rather than a frame.  A back-chain walk -- this port's only unwinder --
     * would read uninitialized red zone there.  setup_raise_exception stores
     * SP_sig for the same reason. */
    stack->linkage[0] = caller_sp;  /* back chain */

    frame->gpr[1] = (ULONG64)stack;
    frame->gpr[3] = (ULONG64)&stack->context;
    frame->gpr[4] = arg1;
    frame->gpr[5] = arg2;
    frame->gpr[6] = arg3;
    frame->gpr[7] = (ULONG64)func;
    set_frame_entry( frame, pKiUserApcDispatcher );
    return status;
}


/***********************************************************************
 *           call_raise_user_exception_dispatcher
 */
void call_raise_user_exception_dispatcher( struct thread_data *data )
{
    set_frame_entry( get_syscall_frame( data ), pKiRaiseUserExceptionDispatcher );
}


/***********************************************************************
 *           call_user_exception_dispatcher
 */
NTSTATUS call_user_exception_dispatcher( struct thread_data *data, EXCEPTION_RECORD *rec, CONTEXT *context )
{
    struct exc_stack_layout *stack;
    struct syscall_frame *frame = get_syscall_frame( data );
    NTSTATUS status = NtSetContextThread( GetCurrentThread(), context );
    ULONG64 sp = (context->Gpr1 - PPC64_RED_ZONE) & ~15;

    if (status) return status;
    stack = (struct exc_stack_layout *)sp - 1;
    memmove( &stack->context, context, sizeof(*context) );
    memmove( &stack->rec, rec, sizeof(*rec) );
    stack->linkage[0] = context->Gpr1;  /* back chain: the interrupted frame */

    frame->gpr[1] = (ULONG64)stack;
    frame->gpr[3] = (ULONG64)&stack->rec;
    frame->gpr[4] = (ULONG64)&stack->context;
    set_frame_entry( frame, pKiUserExceptionDispatcher );
    return status;
}


/***********************************************************************
 *           setup_raise_exception
 */
static void setup_raise_exception( struct thread_data *data, ucontext_t *sigcontext,
                                   EXCEPTION_RECORD *rec, CONTEXT *context )
{
    struct exc_stack_layout *stack;
    void *stack_ptr = (void *)((SP_sig(sigcontext) - PPC64_RED_ZONE) & ~15);
    NTSTATUS status;

    status = send_debug_event( data, rec, context, TRUE, TRUE );
    if (status == DBG_CONTINUE || status == DBG_EXCEPTION_HANDLED)
    {
        restore_context( context, sigcontext );
        return;
    }

    /* trap leaves NIP on the trap instruction; the handlers below advance it so
     * that a continuing debugger steps over it.  Report the instruction itself. */
    if (rec->ExceptionCode == EXCEPTION_BREAKPOINT) context->Iar -= 4;

    stack = virtual_setup_exception( data, stack_ptr, sizeof(*stack), rec );
    stack->rec = *rec;
    stack->context = *context;
    stack->linkage[0] = SP_sig(sigcontext);  /* back chain */

    /* now modify the sigcontext to enter KiUserExceptionDispatcher */
    SP_sig(sigcontext)  = (ULONG_PTR)stack;
    GPR_sig(3, sigcontext)  = (ULONG_PTR)&stack->rec;
    GPR_sig(4, sigcontext)  = (ULONG_PTR)&stack->context;
    GPR_sig(12, sigcontext) = (ULONG_PTR)pKiUserExceptionDispatcher;
    NIP_sig(sigcontext) = (ULONG_PTR)pKiUserExceptionDispatcher;
}


/***********************************************************************
 *           unwind information for the assembly below
 *
 * __ASM_GLOBAL_FUNC wraps every routine in .cfi_startproc/.cfi_endproc.  With
 * no directives in between the FDE is *empty*, so the CIE's initial rules --
 * CFA = r1+0, and no rule at all for the return address column (65, i.e. lr) --
 * describe the entire function.  That is not just uninformative, it hangs:
 * pthread_exit() runs a forced unwind over the exiting thread's own stack, and
 * on a frame whose CFA does not advance and whose return address does not
 * change, libgcc keeps unwinding the same frame forever.  Every Win32 thread
 * exit goes out through NtTerminateThread on the kernel stack, i.e. through the
 * syscall dispatcher, so every thread exit burned a core -- invisibly, because
 * the server reports the thread dead before pthread_exit() is reached and the
 * join therefore succeeds.  probes/check-thread-exit-spin.sh measures this.
 *
 * The dispatchers cannot be described in terms of r1: they run on the kernel
 * stack, and below them are PE frames with no unwind information whatsoever.
 * They are stitched onto the last unix-side frame instead, as x86_64 does:
 * the CFA is frame->syscall_cfa, which signal_start_thread and
 * call_user_mode_callback each set to their own CFA before entering user mode.
 * Both of them save lr at CFA+16, r2 at CFA+24 and r14-r31 at CFA-0x90..CFA-8,
 * so a single set of rules describes either entry path.
 */

/* CFA = *(r31 + 0x140) == frame->syscall_cfa.  DW_CFA_def_cfa_expression (0x0f)
 * with a 4-byte expression: DW_OP_breg31 (0x8f), sleb128 0x140 (0xc0 0x02),
 * DW_OP_deref (0x06).  The offset is the one C_ASSERTed above. */
#define __ASM_CFI_CFA_IS_SYSCALL_CFA \
    __ASM_CFI(".cfi_escape 0x0f, 0x04, 0x8f, 0xc0, 0x02, 0x06\n\t")

/* the nonvolatile GPRs, at the slots both user-mode entry paths use */
#define __ASM_CFI_ENTRY_FRAME_GPRS \
    __ASM_CFI(".cfi_offset 14, -0x90\n\t") \
    __ASM_CFI(".cfi_offset 15, -0x88\n\t") \
    __ASM_CFI(".cfi_offset 16, -0x80\n\t") \
    __ASM_CFI(".cfi_offset 17, -0x78\n\t") \
    __ASM_CFI(".cfi_offset 18, -0x70\n\t") \
    __ASM_CFI(".cfi_offset 19, -0x68\n\t") \
    __ASM_CFI(".cfi_offset 20, -0x60\n\t") \
    __ASM_CFI(".cfi_offset 21, -0x58\n\t") \
    __ASM_CFI(".cfi_offset 22, -0x50\n\t") \
    __ASM_CFI(".cfi_offset 23, -0x48\n\t") \
    __ASM_CFI(".cfi_offset 24, -0x40\n\t") \
    __ASM_CFI(".cfi_offset 25, -0x38\n\t") \
    __ASM_CFI(".cfi_offset 26, -0x30\n\t") \
    __ASM_CFI(".cfi_offset 27, -0x28\n\t") \
    __ASM_CFI(".cfi_offset 28, -0x20\n\t") \
    __ASM_CFI(".cfi_offset 29, -0x18\n\t") \
    __ASM_CFI(".cfi_offset 30, -0x10\n\t") \
    __ASM_CFI(".cfi_offset 31, -0x08\n\t")

/* lr and r2 live in the caller's frame on ppc64, hence the positive offsets */
#define __ASM_CFI_ENTRY_FRAME_LINKAGE \
    __ASM_CFI(".cfi_offset 65, 16\n\t") \
    __ASM_CFI(".cfi_offset 2, 24\n\t")

/* the whole stitch: unwind straight out of the kernel stack into the caller of
 * whichever routine entered user mode, skipping the PE frames in between */
#define __ASM_CFI_STITCH_TO_ENTRY_FRAME \
    __ASM_CFI_CFA_IS_SYSCALL_CFA \
    __ASM_CFI_ENTRY_FRAME_LINKAGE \
    __ASM_CFI_ENTRY_FRAME_GPRS

/* "the return address is not recoverable from here" -- terminates an unwind
 * cleanly (libgcc reports END_OF_STACK) instead of letting it spin.  Used where
 * the next frame up is PE code, which has no unwind information at all. */
#define __ASM_CFI_RA_UNRECOVERABLE \
    __ASM_CFI(".cfi_undefined 65\n\t")


/***********************************************************************
 *           call_user_mode_callback
 */
extern NTSTATUS call_user_mode_callback( ULONG_PTR user_sp, void **ret_ptr, ULONG *ret_len,
                                         void *func, TEB *teb );
__ASM_GLOBAL_FUNC( call_user_mode_callback,
                   /* r3 = user_sp, r4 = ret_ptr, r5 = ret_len, r6 = func, r7 = teb */
                   "mflr 0\n\t"
                   "std 0, 16(1)\n\t"
                   "std 2, 24(1)\n\t"
                   "stdu 1, -0x100(1)\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset 0x100\n\t")
                   __ASM_CFI_ENTRY_FRAME_LINKAGE
                   /* r14-r31 go at CFA-0x90..CFA-8, the same slots
                    * signal_start_thread uses, so that the dispatchers can be
                    * stitched to either frame with one set of rules */
                   "std 14, 0x70(1)\n\t"
                   "std 15, 0x78(1)\n\t"
                   "std 16, 0x80(1)\n\t"
                   "std 17, 0x88(1)\n\t"
                   "std 18, 0x90(1)\n\t"
                   "std 19, 0x98(1)\n\t"
                   "std 20, 0xa0(1)\n\t"
                   "std 21, 0xa8(1)\n\t"
                   "std 22, 0xb0(1)\n\t"
                   "std 23, 0xb8(1)\n\t"
                   "std 24, 0xc0(1)\n\t"
                   "std 25, 0xc8(1)\n\t"
                   "std 26, 0xd0(1)\n\t"
                   "std 27, 0xd8(1)\n\t"
                   "std 28, 0xe0(1)\n\t"
                   "std 29, 0xe8(1)\n\t"
                   "std 30, 0xf0(1)\n\t"
                   "std 31, 0xf8(1)\n\t"
                   __ASM_CFI_ENTRY_FRAME_GPRS
                   "addi 8, 1, 0x100\n\t"           /* syscall_cfa */
                   "std 4, 0x20(1)\n\t"             /* ret_ptr */
                   "std 5, 0x28(1)\n\t"             /* ret_len */
                   "std 7, 0x30(1)\n\t"             /* teb */
                   "ld 9, 0(7)\n\t"                 /* teb->Tib.ExceptionList */
                   "std 9, 0x38(1)\n\t"
                   /* build a new syscall frame below the current kernel stack */
                   "addi 9, 1, -0x450\n\t"
                   "rldicr 9, 9, 0, 59\n\t"         /* 16-byte aligned */
                   "ld 10, 0x378(7)\n\t"            /* thread_data->syscall_frame */
                   "std 10, 0x138(9)\n\t"           /* frame->prev_frame */
                   "std 8, 0x140(9)\n\t"            /* frame->syscall_cfa */
                   "std 7, 0x148(9)\n\t"            /* frame->teb */
                   "li 0, 0\n\t"
                   "stw 0, 0x130(9)\n\t"            /* frame->restore_flags */
                   "std 9, 0x378(7)\n\t"            /* thread_data->syscall_frame */
                   /* save the non-volatile FPRs and VRs in the new frame */
                   "stfd 14, 0x1c0(9)\n\t"
                   "stfd 15, 0x1c8(9)\n\t"
                   "stfd 16, 0x1d0(9)\n\t"
                   "stfd 17, 0x1d8(9)\n\t"
                   "stfd 18, 0x1e0(9)\n\t"
                   "stfd 19, 0x1e8(9)\n\t"
                   "stfd 20, 0x1f0(9)\n\t"
                   "stfd 21, 0x1f8(9)\n\t"
                   "stfd 22, 0x200(9)\n\t"
                   "stfd 23, 0x208(9)\n\t"
                   "stfd 24, 0x210(9)\n\t"
                   "stfd 25, 0x218(9)\n\t"
                   "stfd 26, 0x220(9)\n\t"
                   "stfd 27, 0x228(9)\n\t"
                   "stfd 28, 0x230(9)\n\t"
                   "stfd 29, 0x238(9)\n\t"
                   "stfd 30, 0x240(9)\n\t"
                   "stfd 31, 0x248(9)\n\t"
                   "li 11, 0x390\n\t"               /* 0x250 + 20*16 */
                   "stvx 20, 9, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 21, 9, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 22, 9, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 23, 9, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 24, 9, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 25, 9, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 26, 9, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 27, 9, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 28, 9, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 29, 9, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 30, 9, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 31, 9, 11\n\t"
                   /* switch to the user stack and enter the dispatcher */
                   "mr 12, 6\n\t"
                   "mtctr 12\n\t"
                   "ld 0, 0x60(3)\n\t"              /* stack->args */
                   "lwz 10, 0x68(3)\n\t"            /* stack->len */
                   "lwz 11, 0x6c(3)\n\t"            /* stack->id */
                   "mr 1, 3\n\t"
                   /* r1 is the user stack from here on, so keep describing our
                    * own frame through the new syscall frame in r9: CFA =
                    * *(r9 + 0x140), which is the CFA the rules above use. */
                   __ASM_CFI(".cfi_escape 0x0f, 0x04, 0x79, 0xc0, 0x02, 0x06\n\t")
                   "mr 3, 11\n\t"
                   "mr 4, 0\n\t"
                   "mr 5, 10\n\t"
                   "bctr" )


/***********************************************************************
 *           user_mode_callback_return
 */
extern void DECLSPEC_NORETURN user_mode_callback_return( void *ret_ptr, ULONG ret_len,
                                                         NTSTATUS status, TEB *teb );
__ASM_GLOBAL_FUNC( user_mode_callback_return,
                   /* r3 = ret_ptr, r4 = ret_len, r5 = status, r6 = teb */
                   /* Reached from PE callback code, which carries no unwind
                    * information, and this body then reloads r1 from the frame
                    * and mtlr's -- so the empty FDE __ASM_GLOBAL_FUNC would
                    * otherwise leave describes a frame identical to this one and
                    * a forced unwind crossing it never advances.  Terminate the
                    * unwind here instead; same reasoning as
                    * __wine_unix_call_dispatcher below.
                    * Gated by probes/check-empty-fde.sh. */
                   __ASM_CFI_RA_UNRECOVERABLE
                   "ld 7, 0x378(6)\n\t"             /* thread_data->syscall_frame */
                   "ld 8, 0x138(7)\n\t"             /* frame->prev_frame */
                   "std 8, 0x378(6)\n\t"
                   /* restore the FPRs and VRs saved in the callback frame */
                   "lfd 14, 0x1c0(7)\n\t"
                   "lfd 15, 0x1c8(7)\n\t"
                   "lfd 16, 0x1d0(7)\n\t"
                   "lfd 17, 0x1d8(7)\n\t"
                   "lfd 18, 0x1e0(7)\n\t"
                   "lfd 19, 0x1e8(7)\n\t"
                   "lfd 20, 0x1f0(7)\n\t"
                   "lfd 21, 0x1f8(7)\n\t"
                   "lfd 22, 0x200(7)\n\t"
                   "lfd 23, 0x208(7)\n\t"
                   "lfd 24, 0x210(7)\n\t"
                   "lfd 25, 0x218(7)\n\t"
                   "lfd 26, 0x220(7)\n\t"
                   "lfd 27, 0x228(7)\n\t"
                   "lfd 28, 0x230(7)\n\t"
                   "lfd 29, 0x238(7)\n\t"
                   "lfd 30, 0x240(7)\n\t"
                   "lfd 31, 0x248(7)\n\t"
                   "li 12, 0x390\n\t"
                   "lvx 20, 7, 12\n\t"
                   "addi 12, 12, 16\n\t"
                   "lvx 21, 7, 12\n\t"
                   "addi 12, 12, 16\n\t"
                   "lvx 22, 7, 12\n\t"
                   "addi 12, 12, 16\n\t"
                   "lvx 23, 7, 12\n\t"
                   "addi 12, 12, 16\n\t"
                   "lvx 24, 7, 12\n\t"
                   "addi 12, 12, 16\n\t"
                   "lvx 25, 7, 12\n\t"
                   "addi 12, 12, 16\n\t"
                   "lvx 26, 7, 12\n\t"
                   "addi 12, 12, 16\n\t"
                   "lvx 27, 7, 12\n\t"
                   "addi 12, 12, 16\n\t"
                   "lvx 28, 7, 12\n\t"
                   "addi 12, 12, 16\n\t"
                   "lvx 29, 7, 12\n\t"
                   "addi 12, 12, 16\n\t"
                   "lvx 30, 7, 12\n\t"
                   "addi 12, 12, 16\n\t"
                   "lvx 31, 7, 12\n\t"
                   /* back to call_user_mode_callback's frame */
                   "ld 1, 0x140(7)\n\t"             /* frame->syscall_cfa */
                   "addi 1, 1, -0x100\n\t"
                   "ld 9, 0x20(1)\n\t"              /* ret_ptr */
                   "ld 10, 0x28(1)\n\t"             /* ret_len */
                   "ld 11, 0x38(1)\n\t"             /* saved teb->Tib.ExceptionList */
                   "std 11, 0(6)\n\t"
                   "cmpdi 9, 0\n\t"
                   "beq 1f\n\t"
                   "std 3, 0(9)\n"
                   "1:\tcmpdi 10, 0\n\t"
                   "beq 2f\n\t"
                   "stw 4, 0(10)\n"
                   "2:\tmr 3, 5\n\t"
                   /* the slots call_user_mode_callback saved these in */
                   "ld 14, 0x70(1)\n\t"
                   "ld 15, 0x78(1)\n\t"
                   "ld 16, 0x80(1)\n\t"
                   "ld 17, 0x88(1)\n\t"
                   "ld 18, 0x90(1)\n\t"
                   "ld 19, 0x98(1)\n\t"
                   "ld 20, 0xa0(1)\n\t"
                   "ld 21, 0xa8(1)\n\t"
                   "ld 22, 0xb0(1)\n\t"
                   "ld 23, 0xb8(1)\n\t"
                   "ld 24, 0xc0(1)\n\t"
                   "ld 25, 0xc8(1)\n\t"
                   "ld 26, 0xd0(1)\n\t"
                   "ld 27, 0xd8(1)\n\t"
                   "ld 28, 0xe0(1)\n\t"
                   "ld 29, 0xe8(1)\n\t"
                   "ld 30, 0xf0(1)\n\t"
                   "ld 31, 0xf8(1)\n\t"
                   "addi 1, 1, 0x100\n\t"
                   "ld 0, 16(1)\n\t"
                   "ld 2, 24(1)\n\t"
                   "mtlr 0\n\t"
                   "blr" )


/***********************************************************************
 *           user_mode_abort_thread
 */
extern void DECLSPEC_NORETURN user_mode_abort_thread( int status, struct syscall_frame *frame );
__ASM_GLOBAL_FUNC( user_mode_abort_thread,
                   "ld 1, 0x140(4)\n\t"             /* frame->syscall_cfa */
                   "stdu 1, -0x60(1)\n\t"
                   /* abort_thread() ends in pthread_exit(), whose forced unwind
                    * climbs straight back through here: r1 is the entry frame's
                    * CFA minus this 0x60 linkage area, so say so.  r20-r31 are
                    * *not* described: this frame is written over their slots,
                    * which is harmless only because we never return. */
                   __ASM_CFI(".cfi_def_cfa_offset 0x60\n\t")
                   __ASM_CFI_ENTRY_FRAME_LINKAGE
                   __ASM_CFI(".cfi_offset 14, -0x90\n\t")
                   __ASM_CFI(".cfi_offset 15, -0x88\n\t")
                   __ASM_CFI(".cfi_offset 16, -0x80\n\t")
                   __ASM_CFI(".cfi_offset 17, -0x78\n\t")
                   __ASM_CFI(".cfi_offset 18, -0x70\n\t")
                   __ASM_CFI(".cfi_offset 19, -0x68\n\t")
                   "bl " __ASM_NAME("abort_thread") "\n\t"
                   "trap" )


/***********************************************************************
 *           KeUserModeCallback
 */
NTSTATUS KeUserModeCallback( ULONG id, const void *args, ULONG len, void **ret_ptr, ULONG *ret_len )
{
    struct thread_data *data = get_thread_data();
    struct syscall_frame *frame = get_syscall_frame( data );
    ULONG_PTR sp = (frame->gpr[1] - PPC64_RED_ZONE -
                    offsetof( struct callback_stack_layout, args_data[len] )) & ~15;
    struct callback_stack_layout *stack = (struct callback_stack_layout *)sp;

    if ((char *)get_kernel_stack( data ) + min_kernel_stack > (char *)&frame) return STATUS_STACK_OVERFLOW;

    memset( stack->linkage, 0, sizeof(stack->linkage) );
    stack->linkage[0] = frame->gpr[1];  /* back chain */
    stack->args = stack->args_data;
    stack->len  = len;
    stack->id   = id;
    stack->lr   = frame->lr;
    stack->sp   = frame->gpr[1];
    stack->pc   = frame->pc;
    memcpy( stack->args_data, args, len );
    return call_user_mode_callback( sp, ret_ptr, ret_len, pKiUserCallbackDispatcher, data->teb );
}


/***********************************************************************
 *           NtCallbackReturn  (NTDLL.@)
 */
NTSTATUS WINAPI NtCallbackReturn( void *ret_ptr, ULONG ret_len, NTSTATUS status )
{
    struct thread_data *data = get_thread_data();
    struct syscall_frame *frame = get_syscall_frame( data );

    if (!frame->prev_frame) return STATUS_NO_CALLBACK_ACTIVE;
    user_mode_callback_return( ret_ptr, ret_len, status, data->teb );
}


/***********************************************************************
 *           handle_syscall_fault
 *
 * Handle a page fault happening during a system call.
 */
static BOOL handle_syscall_fault( struct thread_data *data, ucontext_t *context, EXCEPTION_RECORD *rec )
{
    struct syscall_frame *frame;
    UINT i;

    if (!is_inside_syscall( data, SP_sig(context) )) return FALSE;

    TRACE( "code=%x flags=%x addr=%p pc=%I64x\n",
           rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress, (ULONG64)NIP_sig(context) );
    for (i = 0; i < rec->NumberParameters; i++)
        TRACE( " info[%d]=%016I64x\n", i, (ULONG64)rec->ExceptionInformation[i] );
    TRACE( " r1=%016I64x r2=%016I64x r3=%016I64x lr=%016I64x\n",
           (ULONG64)GPR_sig(1, context), (ULONG64)GPR_sig(2, context),
           (ULONG64)GPR_sig(3, context), (ULONG64)GPR_sig(PPC64_PT_LNK, context) );

    if (data->jmp_buf)
    {
        TRACE( "returning to handler\n" );
        GPR_sig(3, context)  = (ULONG_PTR)data->jmp_buf;
        GPR_sig(4, context)  = 1;
        GPR_sig(12, context) = (ULONG_PTR)longjmp;
        NIP_sig(context)     = (ULONG_PTR)longjmp;
        data->jmp_buf = NULL;
    }
    else if ((frame = get_syscall_frame( data )))
    {
        TRACE( "returning to user mode pc=%I64x ret=%x\n", frame->pc, rec->ExceptionCode );
        GPR_sig(3, context)  = rec->ExceptionCode;
        GPR_sig(31, context) = (ULONG_PTR)frame;
        GPR_sig(12, context) = (ULONG_PTR)__wine_syscall_dispatcher_return;
        NIP_sig(context)     = (ULONG_PTR)__wine_syscall_dispatcher_return;
    }
    else return FALSE;

    return TRUE;
}


/**********************************************************************
 *              fault forensics
 *
 * Debug aid, enabled by setting WINE_FAULTDUMP=1 in the environment.  It
 * reports a fault in terms of the *ELF* mappings the code is actually
 * executing out of, read fresh from /proc/self/maps, rather than the PE view
 * that map_so_dll() synthesises -- an address printed by +seh or by
 * build_module cannot be turned into a .so file offset by naive arithmetic,
 * because those two views of a builtin module do not share a base.
 *
 * Everything here runs on the signal stack with the faulting thread's state
 * still intact, so it only uses write(2), open(2) and read(2), and it
 * validates every pointer it dereferences against the mapping table first.
 */

#define FD_MAPS_BUF   (512 * 1024)
#define FD_MAX_RANGES 2048

struct fd_range
{
    ULONG_PTR start, end;
    char      perms[4];
    const char *name;
};

static char            fd_maps[FD_MAPS_BUF];
static struct fd_range fd_ranges[FD_MAX_RANGES];
static unsigned int    fd_nranges;

static void fd_out( const char *str, size_t len )
{
    ssize_t ret;
    while (len)
    {
        ret = write( 2, str, len );
        if (ret <= 0) return;
        str += ret;
        len -= ret;
    }
}

static void fd_str( const char *str )
{
    size_t len = 0;
    while (str[len]) len++;
    fd_out( str, len );
}

static void fd_hex( ULONG_PTR val )
{
    char buf[19] = "0x0000000000000000";
    int i;
    for (i = 17; i >= 2; i--)
    {
        buf[i] = "0123456789abcdef"[val & 0xf];
        val >>= 4;
    }
    fd_out( buf, 18 );
}

static void fd_dec( ULONG_PTR val )
{
    char buf[24];
    int i = sizeof(buf);
    if (!val) buf[--i] = '0';
    while (val) { buf[--i] = '0' + (val % 10); val /= 10; }
    fd_out( buf + i, sizeof(buf) - i );
}

/* read /proc/self/maps into fd_maps and index it into fd_ranges */
static void fd_read_maps(void)
{
    int fd, i;
    ssize_t ret;
    size_t total = 0;
    char *p, *end;

    fd_nranges = 0;
    if ((fd = open( "/proc/self/maps", O_RDONLY )) == -1) return;
    while (total < sizeof(fd_maps) - 1)
    {
        ret = read( fd, fd_maps + total, sizeof(fd_maps) - 1 - total );
        if (ret <= 0) break;
        total += ret;
    }
    close( fd );
    fd_maps[total] = 0;

    p = fd_maps;
    end = fd_maps + total;
    while (p < end && fd_nranges < FD_MAX_RANGES)
    {
        struct fd_range *r = &fd_ranges[fd_nranges];
        char *line = p, *eol = p;

        while (eol < end && *eol != '\n') eol++;
        if (eol < end) *eol = 0;
        p = eol + 1;

        r->start = 0;
        while (*line && *line != '-')
        {
            int d = (*line >= 'a') ? *line - 'a' + 10 : *line - '0';
            if (d < 0 || d > 15) break;
            r->start = r->start * 16 + d;
            line++;
        }
        if (*line != '-') continue;
        line++;
        r->end = 0;
        while (*line && *line != ' ')
        {
            int d = (*line >= 'a') ? *line - 'a' + 10 : *line - '0';
            if (d < 0 || d > 15) break;
            r->end = r->end * 16 + d;
            line++;
        }
        if (*line != ' ') continue;
        line++;
        for (i = 0; i < 4 && line[i] && line[i] != ' '; i++) r->perms[i] = line[i];
        while (i < 4) r->perms[i++] = '-';
        r->perms[3] = 0;
        /* skip perms, offset, dev, inode; what remains is the pathname */
        for (i = 0; i < 4; i++)
        {
            while (*line && *line != ' ') line++;
            while (*line == ' ') line++;
        }
        r->name = line;
        fd_nranges++;
    }
}

static const struct fd_range *fd_find( ULONG_PTR addr )
{
    unsigned int i;
    for (i = 0; i < fd_nranges; i++)
        if (addr >= fd_ranges[i].start && addr < fd_ranges[i].end) return &fd_ranges[i];
    return NULL;
}

/* first mapping of the same file: the ELF load base that addr2line wants */
static ULONG_PTR fd_module_base( const struct fd_range *r )
{
    unsigned int i;
    const char *name = r->name;
    if (!name || !name[0]) return 0;
    for (i = 0; i < fd_nranges; i++)
    {
        const char *a = fd_ranges[i].name, *b = name;
        if (!a) continue;
        while (*a && *a == *b) { a++; b++; }
        if (!*a && !*b) return fd_ranges[i].start;
    }
    return 0;
}

static BOOL fd_readable( ULONG_PTR addr, size_t len )
{
    const struct fd_range *r = fd_find( addr );
    if (!r || r->perms[0] != 'r') return FALSE;
    return addr + len <= r->end;
}

/* "<addr>  <perms> <file>+<elf offset>" -- the offset is what to feed addr2line */
static void fd_describe( const char *label, ULONG_PTR addr )
{
    const struct fd_range *r;

    fd_str( "  " );
    fd_str( label );
    fd_str( " = " );
    fd_hex( addr );
    if (!(r = fd_find( addr )))
    {
        fd_str( "  <unmapped>\n" );
        return;
    }
    fd_str( "  " );
    fd_out( r->perms, 3 );
    fd_str( "  " );
    if (r->name && r->name[0])
    {
        ULONG_PTR base = fd_module_base( r );
        fd_str( r->name );
        if (base)
        {
            fd_str( "+" );
            fd_hex( addr - base );
        }
    }
    else fd_str( "<anon>" );
    fd_str( "  [map " );
    fd_hex( r->start );
    fd_str( "-" );
    fd_hex( r->end );
    fd_str( "]\n" );
}

static void fd_dump_words( const char *label, ULONG_PTR addr, int count )
{
    int i;
    if (!fd_readable( addr, count * 8 ))
    {
        fd_str( "  " );
        fd_str( label );
        fd_str( ": <unreadable>\n" );
        return;
    }
    for (i = 0; i < count; i++)
    {
        ULONG_PTR val = ((ULONG_PTR *)addr)[i];
        fd_str( "  " );
        fd_str( label );
        fd_str( "[" );
        fd_dec( i );
        fd_str( "]" );
        fd_describe( " ->", val );
    }
}

/* walk the ELFv2 back chain; the LR a frame saved lives at offset 16 of its
 * caller's frame, so each step reports who called the frame we are standing in */
static void fd_backtrace( ULONG_PTR sp, ULONG_PTR lr )
{
    int depth;

    fd_str( "  --- back chain ---\n" );
    fd_describe( "lr  ", lr );
    for (depth = 0; depth < 24; depth++)
    {
        ULONG_PTR next;
        char lbl[8] = "#00 ";

        if (!fd_readable( sp, 24 )) break;
        next = *(ULONG_PTR *)sp;
        if (!next || next <= sp) break;
        if (!fd_readable( next, 24 )) break;
        lbl[1] = '0' + depth / 10;
        lbl[2] = '0' + depth % 10;
        fd_describe( lbl, ((ULONG_PTR *)next)[2] );
        sp = next;
    }
}

static void faultdump( int signal, siginfo_t *siginfo, ucontext_t *context )
{
    static const char *names[32] = {
        "r0 ","r1 ","r2 ","r3 ","r4 ","r5 ","r6 ","r7 ","r8 ","r9 ","r10","r11",
        "r12","r13","r14","r15","r16","r17","r18","r19","r20","r21","r22","r23",
        "r24","r25","r26","r27","r28","r29","r30","r31" };
    ULONG_PTR nip = NIP_sig(context), lr = GPR_sig(PPC64_PT_LNK,context);
    ULONG_PTR ctr = GPR_sig(PPC64_PT_CTR,context);
    int i;

    fd_read_maps();

    fd_str( "\n=== FAULTDUMP pid=" );
    fd_dec( getpid() );
    fd_str( " tid=" );
    fd_dec( syscall( SYS_gettid ) );
    fd_str( " sig=" );
    fd_dec( signal );
    fd_str( " code=" );
    if (siginfo->si_code < 0) { fd_str( "-" ); fd_dec( -(LONG_PTR)siginfo->si_code ); }
    else fd_dec( siginfo->si_code );
    fd_str( " trap=" );
    fd_hex( TRAP_sig(context) );
    fd_str( " dsisr=" );
    fd_hex( DSISR_sig(context) );
    fd_str( " ===\n" );

    fd_describe( "si_addr", (ULONG_PTR)siginfo->si_addr );
    fd_describe( "nip    ", nip );
    fd_describe( "ctr    ", ctr );
    fd_describe( "lr     ", lr );

    fd_str( "  --- gprs ---\n" );
    for (i = 0; i < 32; i++) fd_describe( names[i], GPR_sig(i,context) );

    /* an indirect call through a bad pointer leaves the pointer in r12/ctr and
     * the slot it was loaded from usually still addressable via r11 or r2 */
    fd_str( "  --- r2 (TOC) window ---\n" );
    fd_dump_words( "toc", GPR_sig(2,context) - 32, 8 );

    fd_backtrace( SP_sig(context), lr );
    fd_str( "=== END FAULTDUMP ===\n" );
}

static BOOL faultdump_enabled(void)
{
    static int cached = -1;
    if (cached == -1)
    {
        const char *v = getenv( "WINE_FAULTDUMP" );
        cached = (v && v[0] && v[0] != '0');
    }
    return cached;
}


/**********************************************************************
 *		segv_handler
 *
 * Handler for SIGSEGV, SIGBUS and SIGILL.
 */
static void segv_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    ucontext_t *context = sigcontext;
    struct thread_data *data = get_thread_data();
    CONTEXT ctx;
    EXCEPTION_RECORD rec = { .ExceptionCode = EXCEPTION_ACCESS_VIOLATION,
                             .ExceptionAddress = (void *)NIP_sig(context) };

    switch (signal)
    {
    case SIGILL:
        rec.ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
        break;
    case SIGBUS:
        rec.ExceptionCode = EXCEPTION_DATATYPE_MISALIGNMENT;
        break;
    default:  /* SIGSEGV */
        rec.NumberParameters = 2;
        if (TRAP_sig(context) == PPC64_TRAP_ISI)
            rec.ExceptionInformation[0] = EXCEPTION_EXECUTE_FAULT;
        else if (DSISR_sig(context) & PPC64_DSISR_STORE)
            rec.ExceptionInformation[0] = EXCEPTION_WRITE_FAULT;
        else
            rec.ExceptionInformation[0] = EXCEPTION_READ_FAULT;
        rec.ExceptionInformation[1] = (ULONG_PTR)siginfo->si_addr;
        if (!virtual_handle_fault( data, &rec, (void *)SP_sig(context) )) return;
        break;
    }

    if (handle_syscall_fault( data, context, &rec )) return;
    if (faultdump_enabled()) faultdump( signal, siginfo, context );
    save_context( &ctx, context );
    setup_raise_exception( data, context, &rec, &ctx );
}


/**********************************************************************
 *		trap_handler
 *
 * Handler for SIGTRAP.
 */
static void trap_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    ucontext_t *context = sigcontext;
    struct thread_data *data = get_thread_data();
    CONTEXT ctx;
    EXCEPTION_RECORD rec = { .ExceptionAddress = (void *)NIP_sig(context) };

    save_context( &ctx, context );

    switch (siginfo->si_code)
    {
    case TRAP_TRACE:
        rec.ExceptionCode = EXCEPTION_SINGLE_STEP;
        break;
    case TRAP_BRKPT:
    default:
        /* a "trap" instruction leaves NIP pointing at itself; step over it so
         * that a debugger continuing the exception makes progress */
        ctx.Iar += 4;
        rec.ExceptionCode = EXCEPTION_BREAKPOINT;
        rec.NumberParameters = 1;
        break;
    }
    setup_raise_exception( data, context, &rec, &ctx );
}


/**********************************************************************
 *		fpe_handler
 *
 * Handler for SIGFPE.
 */
static void fpe_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    ucontext_t *context = sigcontext;
    struct thread_data *data = get_thread_data();
    CONTEXT ctx;
    EXCEPTION_RECORD rec = { .ExceptionAddress = (void *)NIP_sig(context) };

    save_context( &ctx, context );

    switch (siginfo->si_code & 0xffff)
    {
#ifdef FPE_FLTSUB
    case FPE_FLTSUB: rec.ExceptionCode = EXCEPTION_ARRAY_BOUNDS_EXCEEDED; break;
#endif
#ifdef FPE_INTDIV
    case FPE_INTDIV: rec.ExceptionCode = EXCEPTION_INT_DIVIDE_BY_ZERO; break;
#endif
#ifdef FPE_INTOVF
    case FPE_INTOVF: rec.ExceptionCode = EXCEPTION_INT_OVERFLOW; break;
#endif
#ifdef FPE_FLTDIV
    case FPE_FLTDIV: rec.ExceptionCode = EXCEPTION_FLT_DIVIDE_BY_ZERO; break;
#endif
#ifdef FPE_FLTOVF
    case FPE_FLTOVF: rec.ExceptionCode = EXCEPTION_FLT_OVERFLOW; break;
#endif
#ifdef FPE_FLTUND
    case FPE_FLTUND: rec.ExceptionCode = EXCEPTION_FLT_UNDERFLOW; break;
#endif
#ifdef FPE_FLTRES
    case FPE_FLTRES: rec.ExceptionCode = EXCEPTION_FLT_INEXACT_RESULT; break;
#endif
#ifdef FPE_FLTINV
    case FPE_FLTINV:
#endif
    default:
        rec.ExceptionCode = EXCEPTION_FLT_INVALID_OPERATION;
        break;
    }
    setup_raise_exception( data, context, &rec, &ctx );
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
    ucontext_t *context = sigcontext;
    struct thread_data *data = get_thread_data();
    CONTEXT ctx;
    EXCEPTION_RECORD rec = { .ExceptionCode = EXCEPTION_WINE_ASSERTION,
                             .ExceptionFlags = EXCEPTION_NONCONTINUABLE,
                             .ExceptionAddress = (void *)NIP_sig(context) };

    if (faultdump_enabled()) faultdump( signal, siginfo, context );
    save_context( &ctx, context );
    setup_raise_exception( data, context, &rec, &ctx );
}


/**********************************************************************
 *		quit_handler
 *
 * Handler for SIGQUIT.
 */
static void quit_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    ucontext_t *context = sigcontext;
    struct thread_data *data = get_thread_data();

    if (is_inside_syscall( data, SP_sig(context) )) abort_thread(0);
    user_mode_abort_thread( 0, get_syscall_frame( data ));
}


/**********************************************************************
 *		usr1_handler
 *
 * Handler for SIGUSR1, used to signal a thread that it got suspended.
 */
static void usr1_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    ucontext_t *sigctx = sigcontext;
    struct thread_data *data = get_thread_data();
    CONTEXT context;

    if (!data->teb)
    {
        server_select( NULL, 0, SELECT_INTERRUPTIBLE, 0, NULL, NULL );
    }
    else if (is_inside_syscall( data, SP_sig(sigctx) ))
    {
        context.ContextFlags = CONTEXT_FULL | CONTEXT_EXCEPTION_REQUEST;
        NtGetContextThread( GetCurrentThread(), &context );
        wait_suspend( &context );
        NtSetContextThread( GetCurrentThread(), &context );
    }
    else
    {
        save_context( &context, sigctx );
        context.ContextFlags |= CONTEXT_EXCEPTION_REPORTING;
        wait_suspend( &context );
        restore_context( &context, sigctx );
    }
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
 *		signal_init_process
 */
void signal_init_process( TEB *teb )
{
    struct sigaction sig_act;

    ppc64_unix_teb = teb;
    alloc_syscall_frame( sizeof(struct syscall_frame) );
    signal_alloc_thread( teb );

    /* Debug aid: with kernel.yama.ptrace_scope=1 (the default) a sampler that
     * is not an ancestor of this process cannot read /proc/<tid>/syscall or
     * attach gdb.  WINE_ALLOW_PTRACE=1 opts this process in, so that
     * probes/wine-snap.py can see where its threads are stuck. */
    if (getenv( "WINE_ALLOW_PTRACE" ))
        prctl( PR_SET_PTRACER, PR_SET_PTRACER_ANY, 0, 0, 0 );

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
void init_syscall_frame( LPTHREAD_START_ROUTINE entry, void *arg, TEB *teb )
{
    struct thread_data *data = get_thread_data();
    struct syscall_frame *frame = get_syscall_frame( data );
    CONTEXT *ctx, context = { 0 };
    ULONG_PTR stack;

    /* the unix side's own copy, for __wine_unix_call_dispatcher */
    ppc64_unix_teb = teb;
    /* and the PE side's, which is what NtCurrentTeb() and every syscall thunk
     * read; nothing else in the tree calls this */
    if (p__wine_init_teb) ((void (CDECL *)(TEB *))p__wine_init_teb)( teb );

    /* leave a full linkage area between the top of the stack and the first
     * frame, since the callee writes into its caller's frame on this ABI */
    stack = ((ULONG_PTR)teb->Tib.StackBase - PPC64_RED_ZONE) & ~15;
    *(ULONG64 *)stack = 0;  /* back chain terminator */

    context.ContextFlags = CONTEXT_ALL;
    context.Gpr1  = stack;
    context.Gpr3  = (ULONG64)entry;
    context.Gpr4  = (ULONG64)arg;
    context.Gpr12 = (ULONG64)pRtlUserThreadStart;
    context.Iar   = (ULONG64)pRtlUserThreadStart;

    if (data->suspend)
    {
        context.ContextFlags |= CONTEXT_EXCEPTION_REPORTING | CONTEXT_EXCEPTION_ACTIVE;
        wait_suspend( &context );
    }

    ctx = (CONTEXT *)((context.Gpr1 - sizeof(CONTEXT)) & ~15);
    *ctx = context;
    ctx->ContextFlags = CONTEXT_FULL;
    signal_set_full_context( ctx );

    frame->gpr[1]  = ((ULONG_PTR)ctx - PPC64_LINKAGE) & ~15;
    frame->gpr[3]  = (ULONG64)ctx;
    frame->gpr[12] = (ULONG64)pLdrInitializeThunk;
    frame->pc      = (ULONG64)pLdrInitializeThunk;
    frame->lr      = 0;
    *(ULONG64 *)frame->gpr[1] = context.Gpr1;  /* back chain */

    pthread_sigmask( SIG_UNBLOCK, &server_block_set, NULL );
}


/***********************************************************************
 *           signal_start_thread
 *
 * r3 = entry, r4 = arg, r5 = teb.  Called from within ntdll.so, so r2 is
 * already this module's TOC on entry.
 */
__ASM_GLOBAL_FUNC( signal_start_thread,
                   "mflr 0\n\t"
                   "std 0, 16(1)\n\t"
                   "std 2, 24(1)\n\t"
                   "stdu 1, -0xd0(1)\n\t"
                   __ASM_CFI(".cfi_def_cfa_offset 0xd0\n\t")
                   __ASM_CFI_ENTRY_FRAME_LINKAGE
                   "std 14, 0x40(1)\n\t"
                   "std 15, 0x48(1)\n\t"
                   "std 16, 0x50(1)\n\t"
                   "std 17, 0x58(1)\n\t"
                   "std 18, 0x60(1)\n\t"
                   "std 19, 0x68(1)\n\t"
                   "std 20, 0x70(1)\n\t"
                   "std 21, 0x78(1)\n\t"
                   "std 22, 0x80(1)\n\t"
                   "std 23, 0x88(1)\n\t"
                   "std 24, 0x90(1)\n\t"
                   "std 25, 0x98(1)\n\t"
                   "std 26, 0xa0(1)\n\t"
                   "std 27, 0xa8(1)\n\t"
                   "std 28, 0xb0(1)\n\t"
                   "std 29, 0xb8(1)\n\t"
                   "std 30, 0xc0(1)\n\t"
                   "std 31, 0xc8(1)\n\t"
                   __ASM_CFI_ENTRY_FRAME_GPRS
                   "addi 8, 1, 0xd0\n\t"            /* syscall_cfa */
                   /* set up the syscall frame */
                   "ld 9, 0x378(5)\n\t"             /* thread_data->syscall_frame */
                   "cmpdi 9, 0\n\t"
                   "bne 1f\n\t"
                   "addi 9, 1, -0x450\n\t"          /* sizeof(struct syscall_frame) */
                   "rldicr 9, 9, 0, 59\n\t"
                   "std 9, 0x378(5)\n"
                   "1:\tli 0, 0\n\t"
                   "stw 0, 0x130(9)\n\t"            /* frame->restore_flags */
                   "std 0, 0x138(9)\n\t"            /* frame->prev_frame */
                   "std 8, 0x140(9)\n\t"            /* frame->syscall_cfa */
                   "std 5, 0x148(9)\n\t"            /* frame->teb */
                   "mr 31, 9\n\t"
                   "mr 30, 5\n\t"
                   /* switch to the kernel stack */
                   "addi 1, 9, -0x60\n\t"
                   /* r1 no longer describes this frame; frame->syscall_cfa
                    * (just stored from r8) still does, and r31 holds the frame
                    * from here through the dispatcher tail we branch into. */
                   __ASM_CFI_CFA_IS_SYSCALL_CFA
                   "std 0, 0(1)\n\t"                /* back chain terminator */
                   "std 2, 24(1)\n\t"
                   "bl " __ASM_NAME("init_syscall_frame") "\n\t"
                   "ld 2, 24(1)\n\t"
                   "b " __ASM_LOCAL_LABEL("__wine_syscall_dispatcher_return") )


/***********************************************************************
 *           __wine_syscall_dispatcher
 *
 * Entered by a tail branch from a syscall thunk in the PE ntdll or in win32u
 * (see include/wine/asm.h), never by a call, with:
 *
 *      r11 = syscall id
 *      r12 = this function's address (so that we can derive our own TOC)
 *      LR  = the PE caller's return address
 *      r1  = the PE caller's frame; r3-r10 and the parameter save area at
 *            32(r1) hold the arguments
 *
 * The TEB comes from this module's own initial-exec thread-local, not from the
 * thunk: the thunk is emitted into every module with a syscall table and could
 * not reach a thread-local defined in ntdll.
 *
 * Nothing here may write 24(r1) before a new frame is pushed: that word is
 * the caller's own TOC save slot and the caller reloads r2 from it after the
 * call.  The 288-byte red zone below r1 is scratch - legal because a function
 * that calls us is not a leaf and keeps nothing live there - and is used to
 * free up the registers we need before the TEB is reachable.
 *
 * There is no .localentry directive, i.e. st_other is 0: this function never
 * requires the caller to set up r2, it derives r2 from r12 itself.  That is
 * also what keeps the linker from interposing a "std r2,24(r1)" stub.
 */
__ASM_GLOBAL_FUNC( __wine_syscall_dispatcher,
                   /* Until the kernel stack is entered below we are still on
                    * the PE caller's frame, and PE code carries no unwind
                    * information: an unwinder must stop here rather than walk
                    * into it (or, with the CIE's empty rules, spin on us). */
                   __ASM_CFI_RA_UNRECOVERABLE
                   "std 2, -24(1)\n\t"              /* red zone: the caller's TOC */
                   "addis 2, 12, .TOC.-" __ASM_NAME("__wine_syscall_dispatcher") "@ha\n\t"
                   "addi 2, 2, .TOC.-" __ASM_NAME("__wine_syscall_dispatcher") "@l\n\t"
                   "std 30, -8(1)\n\t"
                   "std 31, -16(1)\n\t"
                   "addis 30, 2, ppc64_unix_teb@got@tprel@ha\n\t"
                   "ld 30, ppc64_unix_teb@got@tprel@l(30)\n\t"
                   "add 30, 30, ppc64_unix_teb@tls\n\t"
                   "ld 30, 0(30)\n\t"               /* r30 = TEB */
                   "ld 31, 0x378(30)\n\t"           /* r31 = frame */
                   "std 1, 0x008(31)\n\t"           /* user sp */
                   "ld 0, -24(1)\n\t"
                   "std 0, 0x010(31)\n\t"           /* user TOC */
                   "std 13, 0x068(31)\n\t"
                   "std 14, 0x070(31)\n\t"
                   "std 15, 0x078(31)\n\t"
                   "std 16, 0x080(31)\n\t"
                   "std 17, 0x088(31)\n\t"
                   "std 18, 0x090(31)\n\t"
                   "std 19, 0x098(31)\n\t"
                   "std 20, 0x0a0(31)\n\t"
                   "std 21, 0x0a8(31)\n\t"
                   "std 22, 0x0b0(31)\n\t"
                   "std 23, 0x0b8(31)\n\t"
                   "std 24, 0x0c0(31)\n\t"
                   "std 25, 0x0c8(31)\n\t"
                   "std 26, 0x0d0(31)\n\t"
                   "std 27, 0x0d8(31)\n\t"
                   "std 28, 0x0e0(31)\n\t"
                   "std 29, 0x0e8(31)\n\t"
                   "ld 0, -8(1)\n\t"
                   "std 0, 0x0f0(31)\n\t"
                   "ld 0, -16(1)\n\t"
                   "std 0, 0x0f8(31)\n\t"
                   "std 30, 0x148(31)\n\t"          /* frame->teb */
                   "stw 11, 0x134(31)\n\t"          /* frame->syscall_id */
                   "mflr 0\n\t"
                   "std 0, 0x100(31)\n\t"           /* frame->lr */
                   "std 0, 0x120(31)\n\t"           /* frame->pc == lr on entry */
                   "mfcr 0\n\t"
                   "std 0, 0x110(31)\n\t"
                   "mfxer 0\n\t"
                   "std 0, 0x118(31)\n\t"
                   "mfctr 0\n\t"
                   "std 0, 0x108(31)\n\t"
                   "li 0, 0\n\t"
                   "stw 0, 0x130(31)\n\t"           /* frame->restore_flags */
                   /* non-volatile FPRs */
                   "stfd 14, 0x1c0(31)\n\t"
                   "stfd 15, 0x1c8(31)\n\t"
                   "stfd 16, 0x1d0(31)\n\t"
                   "stfd 17, 0x1d8(31)\n\t"
                   "stfd 18, 0x1e0(31)\n\t"
                   "stfd 19, 0x1e8(31)\n\t"
                   "stfd 20, 0x1f0(31)\n\t"
                   "stfd 21, 0x1f8(31)\n\t"
                   "stfd 22, 0x200(31)\n\t"
                   "stfd 23, 0x208(31)\n\t"
                   "stfd 24, 0x210(31)\n\t"
                   "stfd 25, 0x218(31)\n\t"
                   "stfd 26, 0x220(31)\n\t"
                   "stfd 27, 0x228(31)\n\t"
                   "stfd 28, 0x230(31)\n\t"
                   "stfd 29, 0x238(31)\n\t"
                   "stfd 30, 0x240(31)\n\t"
                   "stfd 31, 0x248(31)\n\t"
                   /* FPSCR.  This must go through a *volatile* FPR: the restore
                    * path below only reloads f14-f31 when restore_flags carries
                    * CONTEXT_FLOATING_POINT, which an ordinary syscall return
                    * does not, so anything this leaves in a non-volatile FPR is
                    * handed back to the caller.  f0 is volatile in ELFv2 and the
                    * dispatcher is entered by an ordinary call, so the caller has
                    * already written it off; f31 is not, and using it here silently
                    * replaced the caller's f31 with the FPSCR on every syscall. */
                   "mffs 31\n\t"                    /* GATE-SABOTAGE */
                   "stfd 31, 0x128(31)\n\t"            /* GATE-SABOTAGE */
                   /* non-volatile VRs; stvx needs an index register, and r11 is
                    * free now that the syscall id is in the frame */
                   "li 11, 0x390\n\t"               /* 0x250 + 20*16 */
                   "stvx 20, 31, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 21, 31, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 22, 31, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 23, 31, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 24, 31, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 25, 31, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 26, 31, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 27, 31, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 28, 31, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 29, 31, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 30, 31, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 31, 31, 11\n\t"
                   /* switch to the kernel stack: the frame sits at the top of it */
                   "ld 0, 0x008(31)\n\t"
                   "addi 1, 31, -0x60\n\t"
                   /* Stitch the unwind onto the last unix-side frame.  Everything
                    * called from here (the syscall handlers, and so exit_thread ->
                    * pthread_exit -> forced unwind) unwinds through this rule; r1
                    * is useless for it, since the frame moves again for stack
                    * arguments and the frames below are PE. */
                   __ASM_CFI_STITCH_TO_ENTRY_FRAME
                   "std 0, 0(1)\n\t"                /* back chain into the user stack */
                   /* look the syscall up */
                   "lwz 11, 0x134(31)\n\t"          /* syscall id */
                   "ld 14, 0x370(30)\n\t"           /* thread_data->syscall_table */
                   "rldicl 12, 11, 52, 62\n\t"      /* (id >> 12) & 3 */
                   "clrldi 11, 11, 52\n\t"          /* id & 0xfff */
                   "sldi 12, 12, 5\n\t"
                   "add 14, 14, 12\n\t"             /* table */
                   "lwz 12, 16(14)\n\t"             /* table->ServiceLimit */
                   "cmpld 11, 12\n\t"
                   "bge " __ASM_LOCAL_LABEL("bad_syscall") "\n\t"
                   "ld 12, 24(14)\n\t"              /* table->ArgumentTable */
                   "lbzx 22, 12, 11\n\t"            /* argument size in bytes */
                   "ld 12, 0(14)\n\t"               /* table->ServiceTable */
                   "sldi 16, 11, 3\n\t"
                   "ldx 20, 12, 16\n\t"             /* handler */
                   /* make room for the stack arguments, if any */
                   "addi 15, 22, -64\n\t"
                   "cmpdi 15, 0\n\t"
                   "bgt 2f\n\t"
                   "li 15, 0\n"
                   "2:\taddi 15, 15, 15\n\t"
                   "clrrdi 15, 15, 4\n\t"
                   "subf 1, 15, 1\n\t"
                   "ld 18, 0x008(31)\n\t"           /* user sp */
                   "std 18, 0(1)\n\t"               /* back chain */
                   "cmpdi 15, 0\n\t"
                   "beq 4f\n\t"
                   "addi 18, 18, 88\n\t"            /* user param area, arg 9, minus 8 */
                   "addi 19, 1, 88\n\t"
                   "srdi 21, 15, 3\n\t"
                   "mtctr 21\n"
                   "3:\tldu 0, 8(18)\n\t"
                   "stdu 0, 8(19)\n\t"
                   "bdnz 3b\n"
                   "4:\tmr 12, 20\n\t"
                   "mtctr 12\n\t"
                   "std 2, 24(1)\n\t"
                   "lwz 11, 0x380(30)\n\t"          /* thread_data->syscall_trace */
                   "cmpwi 11, 0\n\t"
                   "bne " __ASM_LOCAL_LABEL("trace_syscall") "\n\t"
                   "bctrl\n\t"
                   "ld 2, 24(1)\n"

                   __ASM_LOCAL_LABEL("__wine_syscall_dispatcher_return") ":\n\t"
                   /* r3 = return value, r30 = TEB, r31 = frame */
                   "lwz 14, 0x130(31)\n\t"          /* frame->restore_flags */
                   "andi. 0, 14, 4\n\t"             /* CONTEXT_FLOATING_POINT */
                   "beq 5f\n\t"
                   "lfd 0, 0x150(31)\n\t"
                   "lfd 1, 0x158(31)\n\t"
                   "lfd 2, 0x160(31)\n\t"
                   "lfd 3, 0x168(31)\n\t"
                   "lfd 4, 0x170(31)\n\t"
                   "lfd 5, 0x178(31)\n\t"
                   "lfd 6, 0x180(31)\n\t"
                   "lfd 7, 0x188(31)\n\t"
                   "lfd 8, 0x190(31)\n\t"
                   "lfd 9, 0x198(31)\n\t"
                   "lfd 10, 0x1a0(31)\n\t"
                   "lfd 11, 0x1a8(31)\n\t"
                   "lfd 12, 0x1b0(31)\n\t"
                   "lfd 13, 0x1b8(31)\n\t"
                   "lfd 14, 0x1c0(31)\n\t"
                   "lfd 15, 0x1c8(31)\n\t"
                   "lfd 16, 0x1d0(31)\n\t"
                   "lfd 17, 0x1d8(31)\n\t"
                   "lfd 18, 0x1e0(31)\n\t"
                   "lfd 19, 0x1e8(31)\n\t"
                   "lfd 20, 0x1f0(31)\n\t"
                   "lfd 21, 0x1f8(31)\n\t"
                   "lfd 22, 0x200(31)\n\t"
                   "lfd 23, 0x208(31)\n\t"
                   "lfd 24, 0x210(31)\n\t"
                   "lfd 25, 0x218(31)\n\t"
                   "lfd 26, 0x220(31)\n\t"
                   "lfd 27, 0x228(31)\n\t"
                   "lfd 28, 0x230(31)\n\t"
                   "lfd 29, 0x238(31)\n\t"
                   "lfd 30, 0x240(31)\n\t"
                   "lfd 31, 0x128(31)\n\t"
                   "mtfsf 0xff, 31\n\t"
                   "lfd 31, 0x248(31)\n"
                   "5:\tandi. 0, 14, 16\n\t"        /* CONTEXT_VECTOR */
                   "beq 6f\n\t"
                   "li 15, 0x250\n\t"
                   "lvx 0, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 1, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 2, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 3, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 4, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 5, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 6, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 7, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 8, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 9, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 10, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 11, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 12, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 13, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 14, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 15, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 16, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 17, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 18, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 19, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 20, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 21, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 22, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 23, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 24, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 25, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 26, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 27, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 28, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 29, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 30, 31, 15\n\t"
                   "addi 15, 15, 16\n\t"
                   "lvx 31, 31, 15\n"
                   "6:\tandi. 0, 14, 2\n\t"         /* CONTEXT_INTEGER */
                   "beq 7f\n\t"
                   "ld 3, 0x018(31)\n\t"
                   "ld 4, 0x020(31)\n\t"
                   "ld 5, 0x028(31)\n\t"
                   "ld 6, 0x030(31)\n\t"
                   "ld 7, 0x038(31)\n\t"
                   "ld 8, 0x040(31)\n\t"
                   "ld 9, 0x048(31)\n\t"
                   "ld 10, 0x050(31)\n\t"
                   "ld 11, 0x058(31)\n\t"
                   "ld 12, 0x060(31)\n\t"
                   "ld 0, 0x000(31)\n"
                   /* r13 is glibc's thread pointer and is never restored */
                   "7:\tld 16, 0x100(31)\n\t"       /* frame->lr */
                   "mtlr 16\n\t"
                   "ld 16, 0x110(31)\n\t"           /* frame->cr */
                   "mtcr 16\n\t"
                   "ld 16, 0x118(31)\n\t"           /* frame->xer */
                   "mtxer 16\n\t"
                   "ld 16, 0x108(31)\n\t"           /* frame->ctr -- the user's */
                   "mtctr 16\n\t"
                   /* The resume address goes in TAR, not CTR.  CTR carries the
                    * user's own value, and on ppc64 GCC keeps counted-loop state
                    * there (bdnz), so branching through CTR silently rewrites the
                    * loop counter of any code resumed by a handler returning
                    * EXCEPTION_CONTINUE_EXECUTION -- guard-page and lazy-commit
                    * handlers being exactly the pattern that does this.
                    * TAR is ISA 2.07, so this is POWER8-safe.  TAR itself is not
                    * preserved across the resume; CONTEXT has no field for it. */
                   "ld 16, 0x120(31)\n\t"           /* frame->pc */
                   "mtspr 815, 16\n\t"              /* 815 = TAR */
                   "ld 2, 0x010(31)\n\t"
                   "ld 14, 0x070(31)\n\t"
                   "ld 15, 0x078(31)\n\t"
                   "ld 16, 0x080(31)\n\t"
                   "ld 17, 0x088(31)\n\t"
                   "ld 18, 0x090(31)\n\t"
                   "ld 19, 0x098(31)\n\t"
                   "ld 20, 0x0a0(31)\n\t"
                   "ld 21, 0x0a8(31)\n\t"
                   "ld 22, 0x0b0(31)\n\t"
                   "ld 23, 0x0b8(31)\n\t"
                   "ld 24, 0x0c0(31)\n\t"
                   "ld 25, 0x0c8(31)\n\t"
                   "ld 26, 0x0d0(31)\n\t"
                   "ld 27, 0x0d8(31)\n\t"
                   "ld 28, 0x0e0(31)\n\t"
                   "ld 29, 0x0e8(31)\n\t"
                   "ld 30, 0x0f0(31)\n\t"
                   "ld 1, 0x008(31)\n\t"            /* back to the user stack */
                   "ld 31, 0x0f8(31)\n\t"
                   "bctar 20, 0, 0\n"

                   __ASM_LOCAL_LABEL("trace_syscall") ":\n\t"
                   /* Spill the register arguments into frame->gpr[3..10], which
                    * gives trace_syscall a contiguous list and leaves them
                    * somewhere it cannot touch.  Not the parameter save area at
                    * 32(1): ELFv2 lets a callee use its caller's parameter save
                    * area as scratch, so trace_syscall would be entitled to
                    * clobber the very arguments we are about to reload. */
                   "std 3, 0x018(31)\n\t"
                   "std 4, 0x020(31)\n\t"
                   "std 5, 0x028(31)\n\t"
                   "std 6, 0x030(31)\n\t"
                   "std 7, 0x038(31)\n\t"
                   "std 8, 0x040(31)\n\t"
                   "std 9, 0x048(31)\n\t"
                   "std 10, 0x050(31)\n\t"
                   "lwz 3, 0x134(31)\n\t"           /* frame->syscall_id */
                   "addi 4, 31, 0x018\n\t"          /* args */
                   /* clamp to the eight register arguments: the ninth onwards
                    * live in the parameter save area and are not contiguous
                    * with these, so +syscall silently omits them */
                   "cmpldi 22, 64\n\t"
                   "bgt 8f\n\t"
                   "mr 5, 22\n\t"
                   "b 9f\n"
                   "8:\tli 5, 64\n"
                   "9:\tbl " __ASM_NAME("trace_syscall") "\n\t"
                   "ld 2, 24(1)\n\t"
                   "ld 3, 0x018(31)\n\t"
                   "ld 4, 0x020(31)\n\t"
                   "ld 5, 0x028(31)\n\t"
                   "ld 6, 0x030(31)\n\t"
                   "ld 7, 0x038(31)\n\t"
                   "ld 8, 0x040(31)\n\t"
                   "ld 9, 0x048(31)\n\t"
                   "ld 10, 0x050(31)\n\t"
                   "mr 12, 20\n\t"
                   "mtctr 12\n\t"
                   "bctrl\n\t"
                   "ld 2, 24(1)\n"

                   __ASM_LOCAL_LABEL("trace_syscall_ret") ":\n\t"
                   "mr 21, 3\n\t"                   /* retval */
                   "lwz 3, 0x134(31)\n\t"           /* frame->syscall_id */
                   "mr 4, 21\n\t"
                   "std 2, 24(1)\n\t"
                   "bl " __ASM_NAME("trace_sysret") "\n\t"
                   "ld 2, 24(1)\n\t"
                   "mr 3, 21\n\t"
                   "b " __ASM_LOCAL_LABEL("__wine_syscall_dispatcher_return") "\n"

                   __ASM_LOCAL_LABEL("bad_syscall") ":\n\t"
                   "li 3, 0x1c\n\t"
                   "oris 3, 3, 0xc000\n\t"          /* STATUS_INVALID_SYSTEM_SERVICE */
                   "b " __ASM_LOCAL_LABEL("__wine_syscall_dispatcher_return") )


/***********************************************************************
 *           __wine_syscall_dispatcher_return
 *
 * Re-entered from a signal handler with r3 = status, r31 = frame and
 * r12 = this function's address.  Everything else, including the kernel stack
 * pointer, is rebuilt from the frame.
 */
__ASM_GLOBAL_FUNC( __wine_syscall_dispatcher_return,
                   "addis 2, 12, .TOC.-" __ASM_NAME("__wine_syscall_dispatcher_return") "@ha\n\t"
                   "addi 2, 2, .TOC.-" __ASM_NAME("__wine_syscall_dispatcher_return") "@l\n\t"
                   "addi 1, 31, -0x60\n\t"
                   __ASM_CFI_STITCH_TO_ENTRY_FRAME
                   "li 0, 0\n\t"
                   "std 0, 0(1)\n\t"
                   "ld 30, 0x148(31)\n\t"           /* frame->teb */
                   "lwz 11, 0x380(30)\n\t"          /* thread_data->syscall_trace */
                   "cmpwi 11, 0\n\t"
                   "bne " __ASM_LOCAL_LABEL("trace_syscall_ret") "\n\t"
                   "b " __ASM_LOCAL_LABEL("__wine_syscall_dispatcher_return") )


/***********************************************************************
 *           __wine_unix_call_dispatcher
 *
 * An ordinary WINAPI function pointer as far as PE code is concerned, so it
 * is reached by "mtctr rX; bctrl" with r12 = its own address and nothing else
 * arranged for us; in particular there is no TEB in a register, which is why
 * the unix library keeps its own initial-exec thread-local copy.
 */
__ASM_GLOBAL_FUNC( __wine_unix_call_dispatcher,
                   /* r3 = handle, r4 = code, r5 = args */
                   __ASM_CFI_RA_UNRECOVERABLE       /* PE caller, see the dispatcher above */
                   "std 2, -8(1)\n\t"               /* red zone: stash the caller's TOC */
                   "addis 2, 12, .TOC.-" __ASM_NAME("__wine_unix_call_dispatcher") "@ha\n\t"
                   "addi 2, 2, .TOC.-" __ASM_NAME("__wine_unix_call_dispatcher") "@l\n\t"
                   "std 30, -16(1)\n\t"
                   "std 31, -24(1)\n\t"
                   "addis 30, 2, ppc64_unix_teb@got@tprel@ha\n\t"
                   "ld 30, ppc64_unix_teb@got@tprel@l(30)\n\t"
                   "add 30, 30, ppc64_unix_teb@tls\n\t"
                   "ld 30, 0(30)\n\t"               /* r30 = TEB */
                   "ld 31, 0x378(30)\n\t"           /* r31 = frame */
                   "std 1, 0x008(31)\n\t"
                   "ld 0, -8(1)\n\t"
                   "std 0, 0x010(31)\n\t"           /* user TOC */
                   "std 13, 0x068(31)\n\t"
                   "std 14, 0x070(31)\n\t"
                   "std 15, 0x078(31)\n\t"
                   "std 16, 0x080(31)\n\t"
                   "std 17, 0x088(31)\n\t"
                   "std 18, 0x090(31)\n\t"
                   "std 19, 0x098(31)\n\t"
                   "std 20, 0x0a0(31)\n\t"
                   "std 21, 0x0a8(31)\n\t"
                   "std 22, 0x0b0(31)\n\t"
                   "std 23, 0x0b8(31)\n\t"
                   "std 24, 0x0c0(31)\n\t"
                   "std 25, 0x0c8(31)\n\t"
                   "std 26, 0x0d0(31)\n\t"
                   "std 27, 0x0d8(31)\n\t"
                   "std 28, 0x0e0(31)\n\t"
                   "std 29, 0x0e8(31)\n\t"
                   "ld 0, -16(1)\n\t"
                   "std 0, 0x0f0(31)\n\t"
                   "ld 0, -24(1)\n\t"
                   "std 0, 0x0f8(31)\n\t"
                   "std 30, 0x148(31)\n\t"
                   "mflr 0\n\t"
                   "std 0, 0x100(31)\n\t"
                   "std 0, 0x120(31)\n\t"
                   "mfcr 0\n\t"
                   "std 0, 0x110(31)\n\t"
                   "mfxer 0\n\t"
                   "std 0, 0x118(31)\n\t"
                   "mfctr 0\n\t"
                   "std 0, 0x108(31)\n\t"
                   "li 0, 0\n\t"
                   "stw 0, 0x130(31)\n\t"
                   "stw 0, 0x134(31)\n\t"           /* no syscall id for a unix call */
                   "stfd 14, 0x1c0(31)\n\t"
                   "stfd 15, 0x1c8(31)\n\t"
                   "stfd 16, 0x1d0(31)\n\t"
                   "stfd 17, 0x1d8(31)\n\t"
                   "stfd 18, 0x1e0(31)\n\t"
                   "stfd 19, 0x1e8(31)\n\t"
                   "stfd 20, 0x1f0(31)\n\t"
                   "stfd 21, 0x1f8(31)\n\t"
                   "stfd 22, 0x200(31)\n\t"
                   "stfd 23, 0x208(31)\n\t"
                   "stfd 24, 0x210(31)\n\t"
                   "stfd 25, 0x218(31)\n\t"
                   "stfd 26, 0x220(31)\n\t"
                   "stfd 27, 0x228(31)\n\t"
                   "stfd 28, 0x230(31)\n\t"
                   "stfd 29, 0x238(31)\n\t"
                   "stfd 30, 0x240(31)\n\t"
                   "stfd 31, 0x248(31)\n\t"
                   /* FPSCR through a volatile FPR -- see __wine_syscall_dispatcher */
                   "mffs 31\n\t"                    /* GATE-SABOTAGE */
                   "stfd 31, 0x128(31)\n\t"            /* GATE-SABOTAGE */
                   "li 11, 0x390\n\t"
                   "stvx 20, 31, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 21, 31, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 22, 31, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 23, 31, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 24, 31, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 25, 31, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 26, 31, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 27, 31, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 28, 31, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 29, 31, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 30, 31, 11\n\t"
                   "addi 11, 11, 16\n\t"
                   "stvx 31, 31, 11\n\t"
                   /* switch to the kernel stack and call funcs[code]( args ) */
                   "sldi 0, 4, 3\n\t"
                   "ldx 12, 3, 0\n\t"
                   "mr 3, 5\n\t"
                   "ld 0, 0x008(31)\n\t"
                   "addi 1, 31, -0x60\n\t"
                   __ASM_CFI_STITCH_TO_ENTRY_FRAME
                   "std 0, 0(1)\n\t"
                   "std 2, 24(1)\n\t"
                   "mtctr 12\n\t"
                   "bctrl\n\t"
                   "ld 2, 24(1)\n\t"
                   "b " __ASM_LOCAL_LABEL("__wine_syscall_dispatcher_return") )

#endif  /* __powerpc64__ */
