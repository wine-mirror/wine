/*
 * ARM signal handling routines
 *
 * Copyright 2002 Marcus Meissner, SuSE Linux AG
 * Copyright 2010-2013, 2015 André Hentschel
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

#ifdef __arm__

#include "config.h"

#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
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
#ifdef HAVE_LINK_H
# include <link.h>
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

#define NTDLL_DWARF_H_NO_UNWINDER
#include "dwarf.h"

/***********************************************************************
 * signal context platform-specific definitions
 */
#ifdef linux

#if defined(__ANDROID__) && !defined(HAVE_SYS_UCONTEXT_H)
typedef struct ucontext
{
    unsigned long uc_flags;
    struct ucontext *uc_link;
    stack_t uc_stack;
    struct sigcontext uc_mcontext;
    sigset_t uc_sigmask;
    unsigned long uc_regspace[128] __attribute__((__aligned__(8)));
} ucontext_t;
#endif

/* All Registers access - only for local access */
# define REG_sig(reg_name, context) ((context)->uc_mcontext.reg_name)
# define REGn_sig(reg_num, context) ((context)->uc_mcontext.arm_r##reg_num)

/* Special Registers access  */
# define SP_sig(context)            REG_sig(arm_sp, context)    /* Stack pointer */
# define LR_sig(context)            REG_sig(arm_lr, context)    /* Link register */
# define PC_sig(context)            REG_sig(arm_pc, context)    /* Program counter */
# define CPSR_sig(context)          REG_sig(arm_cpsr, context)  /* Current State Register */
# define IP_sig(context)            REG_sig(arm_ip, context)    /* Intra-Procedure-call scratch register */
# define FP_sig(context)            REG_sig(arm_fp, context)    /* Frame pointer */

/* Exceptions */
# define ERROR_sig(context)         REG_sig(error_code, context)
# define TRAP_sig(context)          REG_sig(trap_no, context)

struct extended_ctx
{
    unsigned long magic;
    unsigned long size;
};

struct vfp_sigframe
{
    struct extended_ctx ctx;
    unsigned long long fpregs[32];
    unsigned long fpscr;
};

static void *get_extended_sigcontext( const ucontext_t *sigcontext, unsigned int magic )
{
    struct extended_ctx *ctx = (struct extended_ctx *)sigcontext->uc_regspace;
    while ((char *)ctx < (char *)(sigcontext + 1) && ctx->magic && ctx->size)
    {
        if (ctx->magic == magic) return ctx;
        ctx = (struct extended_ctx *)((char *)ctx + ctx->size);
    }
    return NULL;
}

static void save_fpu( CONTEXT *context, const ucontext_t *sigcontext )
{
    struct vfp_sigframe *frame = get_extended_sigcontext( sigcontext, 0x56465001 );

    if (!frame) return;
    memcpy( context->D, frame->fpregs, sizeof(context->D) );
    context->Fpscr = frame->fpscr;
}

static void restore_fpu( const CONTEXT *context, ucontext_t *sigcontext )
{
    struct vfp_sigframe *frame = get_extended_sigcontext( sigcontext, 0x56465001 );

    if (!frame) return;
    memcpy( frame->fpregs, context->D, sizeof(context->D) );
    frame->fpscr = context->Fpscr;
}

#elif defined(__FreeBSD__)

/* All Registers access - only for local access */
# define REGn_sig(reg_num, context) ((context)->uc_mcontext.__gregs[reg_num])

/* Special Registers access  */
# define SP_sig(context)            REGn_sig(_REG_SP, context)    /* Stack pointer */
# define LR_sig(context)            REGn_sig(_REG_LR, context)    /* Link register */
# define PC_sig(context)            REGn_sig(_REG_PC, context)    /* Program counter */
# define CPSR_sig(context)          REGn_sig(_REG_CPSR, context)  /* Current State Register */
# define IP_sig(context)            REGn_sig(_REG_R12, context)   /* Intra-Procedure-call scratch register */
# define FP_sig(context)            REGn_sig(_REG_FP, context)    /* Frame pointer */

static void save_fpu( CONTEXT *context, const ucontext_t *sigcontext ) { }
static void restore_fpu( const CONTEXT *context, ucontext_t *sigcontext ) { }

#endif /* linux */

enum arm_trap_code
{
    TRAP_ARM_UNKNOWN    = -1,  /* Unknown fault (TRAP_sig not defined) */
    TRAP_ARM_PRIVINFLT  =  6,  /* Invalid opcode exception */
    TRAP_ARM_PAGEFLT    = 14,  /* Page fault */
    TRAP_ARM_ALIGNFLT   = 17,  /* Alignment check exception */
};

/* stack layout when calling KiUserExceptionDispatcher */
struct exc_stack_layout
{
    CONTEXT              context;        /* 000 */
    EXCEPTION_RECORD     rec;            /* 1a0 */
    ULONG                redzone[2];     /* 1f0 */
};
C_ASSERT( offsetof(struct exc_stack_layout, rec) == 0x1a0 );
C_ASSERT( sizeof(struct exc_stack_layout) == 0x1f8 );

/* stack layout when calling KiUserApcDispatcher */
struct apc_stack_layout
{
    void                *func;           /* 000 APC to call*/
    ULONG                args[3];        /* 004 function arguments */
    ULONG                alertable;      /* 010 */
    ULONG                align;          /* 014 */
    CONTEXT              context;        /* 018 */
    ULONG                redzone[2];     /* 1b8 */
};
C_ASSERT( offsetof(struct apc_stack_layout, context) == 0x18 );
C_ASSERT( sizeof(struct apc_stack_layout) == 0x1c0 );

/* stack layout when calling KiUserCallbackDispatcher */
struct callback_stack_layout
{
    void                *args;           /* 000 arguments */
    ULONG                len;            /* 004 arguments len */
    ULONG                id;             /* 008 function id */
    ULONG                lr;             /* 00c */
    ULONG                sp;             /* 010 */
    ULONG                pc;             /* 014 */
    BYTE                 args_data[0];   /* 018 copied argument data*/
};
C_ASSERT( sizeof(struct callback_stack_layout) == 0x18 );

struct syscall_frame
{
    UINT                  r0;             /* 000 */
    UINT                  r1;             /* 004 */
    UINT                  r2;             /* 008 */
    UINT                  r3;             /* 00c */
    UINT                  r4;             /* 010 */
    UINT                  r5;             /* 014 */
    UINT                  r6;             /* 018 */
    UINT                  r7;             /* 01c */
    UINT                  r8;             /* 020 */
    UINT                  r9;             /* 024 */
    UINT                  r10;            /* 028 */
    UINT                  r11;            /* 02c */
    UINT                  r12;            /* 030 */
    UINT                  pc;             /* 034 */
    UINT                  sp;             /* 038 */
    UINT                  lr;             /* 03c */
    UINT                  cpsr;           /* 040 */
    UINT                  restore_flags;  /* 044 */
    UINT                  fpscr;          /* 048 */
    struct syscall_frame *prev_frame;     /* 04c */
    void                 *syscall_cfa;    /* 050 */
    UINT                  syscall_id;     /* 054 */
    UINT                  align[2];       /* 058 */
    ULONGLONG             d[32];          /* 060 */
};

C_ASSERT( sizeof( struct syscall_frame ) == 0x160);


void set_process_instrumentation_callback( void *callback )
{
    if (callback) FIXME( "Not supported.\n" );
}


/***********************************************************************
 *           get_trap_code
 *
 * Get the trap code for a signal.
 */
static inline enum arm_trap_code get_trap_code( int signal, const ucontext_t *sigcontext )
{
#ifdef TRAP_sig
    enum arm_trap_code trap = TRAP_sig(sigcontext);
    if (trap)
        return trap;
#endif

    switch (signal)
    {
    case SIGILL:
        return TRAP_ARM_PRIVINFLT;
    case SIGSEGV:
        return TRAP_ARM_PAGEFLT;
    case SIGBUS:
        return TRAP_ARM_ALIGNFLT;
    default:
        return TRAP_ARM_UNKNOWN;
    }
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
 *           get_udf_immediate
 *
 * Get the immediate operand if the PC is at a UDF instruction.
 */
static inline int get_udf_immediate( const ucontext_t *sigcontext )
{
    if (CPSR_sig(sigcontext) & 0x20)
    {
        WORD thumb_insn = *(WORD *)PC_sig(sigcontext);
        if ((thumb_insn >> 8) == 0xde) return thumb_insn & 0xff;
        if ((thumb_insn & 0xfff0) == 0xf7f0)  /* udf.w */
        {
            WORD ext = *(WORD *)(PC_sig(sigcontext) + 2);
            if ((ext & 0xf000) == 0xa000) return ((thumb_insn & 0xf) << 12) | (ext & 0x0fff);
        }
    }
    else
    {
        DWORD arm_insn = *(DWORD *)PC_sig(sigcontext);
        if ((arm_insn & 0xfff000f0) == 0xe7f000f0)
        {
            return ((arm_insn >> 4) & 0xfff0) | (arm_insn & 0xf);
        }
    }
    return -1;
}


/***********************************************************************
 *           save_context
 *
 * Set the register values from a sigcontext.
 */
static void save_context( CONTEXT *context, const ucontext_t *sigcontext )
{
#define C(x) context->R##x = REGn_sig(x,sigcontext)
    /* Save normal registers */
    C(0); C(1); C(2); C(3); C(4); C(5); C(6); C(7); C(8); C(9); C(10);
#undef C

    context->ContextFlags = CONTEXT_FULL;
    context->Sp   = SP_sig(sigcontext);   /* Stack pointer */
    context->Lr   = LR_sig(sigcontext);   /* Link register */
    context->Pc   = PC_sig(sigcontext);   /* Program Counter */
    context->Cpsr = CPSR_sig(sigcontext); /* Current State Register */
    context->R11  = FP_sig(sigcontext);   /* Frame pointer */
    context->R12  = IP_sig(sigcontext);   /* Intra-Procedure-call scratch register */
    if (CPSR_sig(sigcontext) & 0x20) context->Pc |= 1;  /* Thumb mode */
    save_fpu( context, sigcontext );
}


/***********************************************************************
 *           restore_context
 *
 * Build a sigcontext from the register values.
 */
static void restore_context( const CONTEXT *context, ucontext_t *sigcontext )
{
#define C(x)  REGn_sig(x,sigcontext) = context->R##x
    /* Restore normal registers */
    C(0); C(1); C(2); C(3); C(4); C(5); C(6); C(7); C(8); C(9); C(10);
#undef C

    SP_sig(sigcontext)   = context->Sp;   /* Stack pointer */
    LR_sig(sigcontext)   = context->Lr;   /* Link register */
    PC_sig(sigcontext)   = context->Pc;   /* Program Counter */
    CPSR_sig(sigcontext) = context->Cpsr; /* Current State Register */
    FP_sig(sigcontext)   = context->R11;  /* Frame pointer */
    IP_sig(sigcontext)   = context->R12;  /* Intra-Procedure-call scratch register */
    if (PC_sig(sigcontext) & 1) CPSR_sig(sigcontext) |= 0x20;
    else CPSR_sig(sigcontext) &= ~0x20;
    restore_fpu( context, sigcontext );
}


/***********************************************************************
 *           signal_set_full_context
 */
NTSTATUS signal_set_full_context( CONTEXT *context )
{
    NTSTATUS status = NtSetContextThread( GetCurrentThread(), context );

    if (!status && (context->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER)
        get_syscall_frame()->restore_flags |= CONTEXT_INTEGER;
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
    NTSTATUS ret;
    struct syscall_frame *frame = get_syscall_frame();
    DWORD flags = context->ContextFlags & ~CONTEXT_ARM;
    BOOL self = (handle == GetCurrentThread());

    if (!self)
    {
        ret = set_thread_context( handle, context, &self, IMAGE_FILE_MACHINE_ARMNT );
        if (ret || !self) return ret;
    }
    if (flags & CONTEXT_INTEGER)
    {
        frame->r0  = context->R0;
        frame->r1  = context->R1;
        frame->r2  = context->R2;
        frame->r3  = context->R3;
        frame->r4  = context->R4;
        frame->r5  = context->R5;
        frame->r6  = context->R6;
        frame->r7  = context->R7;
        frame->r8  = context->R8;
        frame->r9  = context->R9;
        frame->r10 = context->R10;
        frame->r11 = context->R11;
        frame->r12 = context->R12;
    }
    if (flags & CONTEXT_CONTROL)
    {
        frame->sp = context->Sp;
        frame->lr = context->Lr;
        frame->pc = context->Pc & ~1;
        frame->cpsr = context->Cpsr;
        if (context->Cpsr & 0x20) frame->pc |= 1; /* thumb */
    }
    if (flags & CONTEXT_FLOATING_POINT)
    {
        frame->fpscr = context->Fpscr;
        memcpy( frame->d, context->D, sizeof(context->D) );
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
    struct syscall_frame *frame = get_syscall_frame();
    DWORD needed_flags = context->ContextFlags & ~CONTEXT_ARM;
    BOOL self = (handle == GetCurrentThread());

    if (!self)
    {
        NTSTATUS ret = get_thread_context( handle, context, &self, IMAGE_FILE_MACHINE_ARMNT );
        if (ret || !self) return ret;
    }

    if (needed_flags & CONTEXT_INTEGER)
    {
        context->R0  = frame->r0;
        context->R1  = frame->r1;
        context->R2  = frame->r2;
        context->R3  = frame->r3;
        context->R4  = frame->r4;
        context->R5  = frame->r5;
        context->R6  = frame->r6;
        context->R7  = frame->r7;
        context->R8  = frame->r8;
        context->R9  = frame->r9;
        context->R10 = frame->r10;
        context->R11 = frame->r11;
        context->R12 = frame->r12;
        context->ContextFlags |= CONTEXT_INTEGER;
    }
    if (needed_flags & CONTEXT_CONTROL)
    {
        context->Sp   = frame->sp;
        context->Lr   = frame->lr;
        context->Pc   = frame->pc;
        context->Cpsr = frame->cpsr;
        context->ContextFlags |= CONTEXT_CONTROL;
    }
    if (needed_flags & CONTEXT_FLOATING_POINT)
    {
        context->Fpscr = frame->fpscr;
        memcpy( context->D, frame->d, sizeof(frame->d) );
        context->ContextFlags |= CONTEXT_FLOATING_POINT;
    }
    set_context_exception_reporting_flags( &context->ContextFlags, CONTEXT_SERVICE_ACTIVE );
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
 *           setup_raise_exception
 */
static void setup_raise_exception( ucontext_t *sigcontext, EXCEPTION_RECORD *rec, CONTEXT *context )
{
    struct exc_stack_layout *stack;
    void *stack_ptr = (void *)(SP_sig(sigcontext) & ~7);
    NTSTATUS status;

    status = send_debug_event( rec, context, TRUE, TRUE );
    if (status == DBG_CONTINUE || status == DBG_EXCEPTION_HANDLED)
    {
        restore_context( context, sigcontext );
        return;
    }

    /* fix up instruction pointer in context for EXCEPTION_BREAKPOINT */
    if (rec->ExceptionCode == EXCEPTION_BREAKPOINT) context->Pc -= 2;

    stack = virtual_setup_exception( stack_ptr, sizeof(*stack), rec );
    stack->rec = *rec;
    stack->context = *context;

    /* now modify the sigcontext to return to the raise function */
    SP_sig(sigcontext) = (DWORD)stack;
    PC_sig(sigcontext) = (DWORD)pKiUserExceptionDispatcher;
    if (PC_sig(sigcontext) & 1) CPSR_sig(sigcontext) |= 0x20;
    else CPSR_sig(sigcontext) &= ~0x20;
}


/***********************************************************************
 *           setup_exception
 *
 * Modify the signal context to call the exception raise function.
 */
static void setup_exception( ucontext_t *sigcontext, EXCEPTION_RECORD *rec )
{
    CONTEXT context;

    rec->ExceptionAddress = (void *)PC_sig(sigcontext);
    save_context( &context, sigcontext );
    setup_raise_exception( sigcontext, rec, &context );
}


/***********************************************************************
 *           call_user_apc_dispatcher
 */
NTSTATUS call_user_apc_dispatcher( CONTEXT *context, unsigned int flags, ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3,
                                   PNTAPCFUNC func, NTSTATUS status )
{
    struct syscall_frame *frame = get_syscall_frame();
    ULONG sp = context ? context->Sp : frame->sp;
    struct apc_stack_layout *stack;

    if (flags) FIXME( "flags %#x are not supported.\n", flags );

    sp &= ~7;
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
        stack->context.R0 = status;
    }
    stack->func      = func;
    stack->args[0]   = arg1;
    stack->args[1]   = arg2;
    stack->args[2]   = arg3;
    stack->alertable = TRUE;

    frame->sp = (DWORD)stack;
    frame->pc = (DWORD)pKiUserApcDispatcher;
    frame->restore_flags |= CONTEXT_CONTROL;
    return status;
}


/***********************************************************************
 *           call_raise_user_exception_dispatcher
 */
void call_raise_user_exception_dispatcher(void)
{
    struct syscall_frame *frame = get_syscall_frame();

    frame->sp += 16;
    frame->pc = (DWORD)pKiRaiseUserExceptionDispatcher;
}


/***********************************************************************
 *           call_user_exception_dispatcher
 */
NTSTATUS call_user_exception_dispatcher( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    struct exc_stack_layout *stack;
    struct syscall_frame *frame = get_syscall_frame();
    NTSTATUS status = NtSetContextThread( GetCurrentThread(), context );

    if (status) return status;
    stack = (struct exc_stack_layout *)(context->Sp & ~7) - 1;
    memmove( &stack->context, context, sizeof(*context) );
    memmove( &stack->rec, rec, sizeof(*rec) );
    frame->pc = (DWORD)pKiUserExceptionDispatcher;
    frame->sp = (DWORD)stack;
    frame->restore_flags |= CONTEXT_CONTROL;
    return status;
}


/***********************************************************************
 *           call_user_mode_callback
 */
extern NTSTATUS call_user_mode_callback( ULONG user_sp, void **ret_ptr, ULONG *ret_len,
                                         void *func, TEB *teb );
__ASM_GLOBAL_FUNC( call_user_mode_callback,
                   "push {r4-r12,lr}\n\t"
                   "add r7, sp, #0x28\n\t"    /* syscall_cfa */
                   __ASM_CFI(".cfi_def_cfa 7,0\n\t")
                   __ASM_CFI(".cfi_offset r4,-0x28\n\t")
                   __ASM_CFI(".cfi_offset r5,-0x24\n\t")
                   __ASM_CFI(".cfi_offset r6,-0x20\n\t")
                   __ASM_CFI(".cfi_offset r7,-0x1c\n\t")
                   __ASM_CFI(".cfi_offset r8,-0x18\n\t")
                   __ASM_CFI(".cfi_offset r9,-0x14\n\t")
                   __ASM_CFI(".cfi_offset r10,-0x10\n\t")
                   __ASM_CFI(".cfi_offset r11,-0x0c\n\t")
                   __ASM_CFI(".cfi_offset r12,-0x08\n\t")
                   __ASM_CFI(".cfi_offset lr,-0x04\n\t")
                   "ldr r4, [sp, #0x28]\n\t"  /* teb */
                   "ldr r5, [r4]\n\t"         /* teb->Tib.ExceptionList */
                   "push {r1,r2,r4,r5}\n\t"   /* ret_ptr, ret_len, teb, exception_list */
                   "sub sp, sp, #0x90\n\t"
                   "mov r5, sp\n\t"
                   "vmrs r6, fpscr\n\t"
                   "vstm r5, {d8-d15}\n\t"
                   "str r6, [r5, #0x80]\n\t"
                   "sub sp, sp, #0x160\n\t"   /* sizeof(struct syscall_frame) + registers */
                   "ldr ip, [r4, #0x218]\n\t" /* thread_data->syscall_frame */
                   "str ip, [sp, #0x4c]\n\t"  /* frame->prev_frame */
                   "str r7, [sp, #0x50]\n\t"  /* frame->syscall_cfa */
                   "str sp, [r4, #0x218]\n\t" /* thread_data->syscall_frame */
                   "ldr r7, [r4, #0x21c]\n\t" /* thread_data->syscall_trace */
                   "cbnz r7, 1f\n\t"
                   /* switch to user stack */
                   "mov sp, r0\n\t"
                   "bx r3\n"
                   "1:\tmov r6, r0\n\t"       /* user_sp */
                   "ldr r1, [r6]\n\t"         /* args */
                   "ldr r2, [r6, #4]\n\t"     /* len */
                   "ldr r0, [r6, #8]\n\t"     /* id */
                   "str r0, [r5, #0x84]\n\t"
                   "mov r4, r3\n\t"           /* func */
                   "bl " __ASM_NAME("trace_usercall") "\n\t"
                   "mov sp, r6\n\t"
                   "bx r4" )


/***********************************************************************
 *           user_mode_callback_return
 */
extern void DECLSPEC_NORETURN user_mode_callback_return( void *ret_ptr, ULONG ret_len,
                                                         NTSTATUS status, TEB *teb );
__ASM_GLOBAL_FUNC( user_mode_callback_return,
                   "ldr r4, [r3, #0x218]\n\t" /* thread_data->syscall_frame */
                   "ldr r5, [r4, #0x4c]\n\t"  /* frame->prev_frame */
                   "str r5, [r3, #0x218]\n\t" /* thread_data->syscall_frame */
                   "add r5, r4, #0x160\n\t"
                   "vldm r5, {d8-d15}\n\t"
                   "ldr r6, [r5, #0x80]\n\t"
                   "vmsr fpscr, r6\n\t"
                   "ldr r7, [r3, #0x21c]\n\t" /* thread_data->syscall_trace */
                   "cbz r7, 1f\n\t"
                   "stm r5, {r0-r3}\n\t"
                   "ldr r3, [r5, #0x84]\n\t"  /* id */
                   "bl " __ASM_NAME("trace_userret") "\n\t"
                   "ldm r5, {r0-r3}\n"
                   "1:\tadd r5, r5, #0x90\n\t"
                   "mov sp, r5\n\t"
                   "pop {r4-r7}\n\t"          /* ret_ptr, ret_len, teb, exception_list */
                   "str r7, [r3]\n\t"         /* teb->Tib.ExceptionList */
                   "str r0, [r4]\n\t"         /* ret_ptr */
                   "str r1, [r5]\n\t"         /* ret_len */
                   "mov r0, r2\n\t"           /* status */
                   "pop {r4-r12,pc}" )


/***********************************************************************
 *           user_mode_abort_thread
 */
extern void DECLSPEC_NORETURN user_mode_abort_thread( NTSTATUS status, struct syscall_frame *frame );
__ASM_GLOBAL_FUNC( user_mode_abort_thread,
                   __ASM_EHABI(".cantunwind\n\t")
                   "ldr r7, [r1, #0x28]\n\t"  /* frame->syscall_cfa */
                   "sub r7, r7, #0x28\n\t"
                   /* switch to kernel stack */
                   "mov sp, r7\n\t"
                   __ASM_CFI(".cfi_def_cfa 7,0x28\n\t")
                   __ASM_CFI(".cfi_offset r4,-0x28\n\t")
                   __ASM_CFI(".cfi_offset r5,-0x24\n\t")
                   __ASM_CFI(".cfi_offset r6,-0x20\n\t")
                   __ASM_CFI(".cfi_offset r7,-0x1c\n\t")
                   __ASM_CFI(".cfi_offset r8,-0x18\n\t")
                   __ASM_CFI(".cfi_offset r9,-0x14\n\t")
                   __ASM_CFI(".cfi_offset r10,-0x10\n\t")
                   __ASM_CFI(".cfi_offset r11,-0x0c\n\t")
                   __ASM_CFI(".cfi_offset r12,-0x08\n\t")
                   __ASM_CFI(".cfi_offset lr,-0x04\n\t")
                   "bl " __ASM_NAME("abort_thread") )


/***********************************************************************
 *           KeUserModeCallback
 */
NTSTATUS KeUserModeCallback( ULONG id, const void *args, ULONG len, void **ret_ptr, ULONG *ret_len )
{
    struct syscall_frame *frame = get_syscall_frame();
    ULONG sp = (frame->sp - offsetof( struct callback_stack_layout, args_data[len] ) - 8) & ~7;
    struct callback_stack_layout *stack = (struct callback_stack_layout *)sp;

    if ((char *)ntdll_get_thread_data()->kernel_stack + min_kernel_stack > (char *)&frame)
        return STATUS_STACK_OVERFLOW;

    stack->args = stack->args_data;
    stack->len  = len;
    stack->id   = id;
    stack->lr   = frame->lr;
    stack->sp   = frame->sp;
    stack->pc   = frame->pc;
    memcpy( stack->args_data, args, len );
    return call_user_mode_callback( sp, ret_ptr, ret_len, pKiUserCallbackDispatcher, NtCurrentTeb() );
}


/***********************************************************************
 *           NtCallbackReturn  (NTDLL.@)
 */
NTSTATUS WINAPI NtCallbackReturn( void *ret_ptr, ULONG ret_len, NTSTATUS status )
{
    if (!get_syscall_frame()->prev_frame) return STATUS_NO_CALLBACK_ACTIVE;
    user_mode_callback_return( ret_ptr, ret_len, status, NtCurrentTeb() );
}


/***********************************************************************
 *           handle_syscall_fault
 *
 * Handle a page fault happening during a system call.
 */
static BOOL handle_syscall_fault( ucontext_t *context, EXCEPTION_RECORD *rec )
{
    struct syscall_frame *frame = get_syscall_frame();
    UINT i;

    if (!is_inside_syscall( SP_sig(context) )) return FALSE;

    TRACE( "code=%x flags=%x addr=%p pc=%08x\n",
           rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress, (DWORD)PC_sig(context) );
    for (i = 0; i < rec->NumberParameters; i++)
        TRACE( " info[%d]=%08lx\n", i, rec->ExceptionInformation[i] );

    TRACE( " r0=%08x r1=%08x r2=%08x r3=%08x r4=%08x r5=%08x\n",
           (DWORD)REGn_sig(0, context), (DWORD)REGn_sig(1, context), (DWORD)REGn_sig(2, context),
           (DWORD)REGn_sig(3, context), (DWORD)REGn_sig(4, context), (DWORD)REGn_sig(5, context) );
    TRACE( " r6=%08x r7=%08x r8=%08x r9=%08x r10=%08x r11=%08x\n",
           (DWORD)REGn_sig(6, context), (DWORD)REGn_sig(7, context), (DWORD)REGn_sig(8, context),
           (DWORD)REGn_sig(9, context), (DWORD)REGn_sig(10, context), (DWORD)FP_sig(context) );
    TRACE( " r12=%08x sp=%08x lr=%08x pc=%08x cpsr=%08x\n",
           (DWORD)IP_sig(context), (DWORD)SP_sig(context), (DWORD)LR_sig(context),
           (DWORD)PC_sig(context), (DWORD)CPSR_sig(context) );

    if (ntdll_get_thread_data()->jmp_buf)
    {
        TRACE( "returning to handler\n" );
        REGn_sig(0, context) = (DWORD)ntdll_get_thread_data()->jmp_buf;
        REGn_sig(1, context) = 1;
        PC_sig(context)      = (DWORD)longjmp;
        ntdll_get_thread_data()->jmp_buf = NULL;
    }
    else
    {
        TRACE( "returning to user mode ip=%08x ret=%08x\n", frame->pc, rec->ExceptionCode );
        REGn_sig(0, context) = rec->ExceptionCode;
        REGn_sig(1, context) = (DWORD)NtCurrentTeb();
        REGn_sig(8, context) = (DWORD)frame;
        PC_sig(context)      = (DWORD)__wine_syscall_dispatcher_return;
    }
    if (PC_sig(context) & 1) CPSR_sig(context) |= 0x20;
    else CPSR_sig(context) &= ~0x20;
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
    ucontext_t *context = sigcontext;
    CONTEXT ctx;

    rec.ExceptionAddress = (void *)PC_sig(context);
    save_context( &ctx, sigcontext );

    switch (get_trap_code(signal, context))
    {
    case TRAP_ARM_PRIVINFLT:   /* Invalid opcode exception */
        switch (get_udf_immediate( context ))
        {
        case 0xfb:  /* __fastfail */
        {
            rec.ExceptionCode = STATUS_STACK_BUFFER_OVERRUN;
            rec.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
            rec.NumberParameters = 1;
            rec.ExceptionInformation[0] = ctx.R0;
            NtRaiseException( &rec, &ctx, FALSE );
            return;
        }
        case 0xfe:  /* breakpoint */
            ctx.Pc += 2;  /* skip the breakpoint instruction */
            rec.ExceptionCode = EXCEPTION_BREAKPOINT;
            rec.NumberParameters = 1;
            break;
        default:
            rec.ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
            break;
        }
        break;
    case TRAP_ARM_PAGEFLT:  /* Page fault */
        rec.NumberParameters = 2;
        if (get_error_code(context) & 0x80000000) rec.ExceptionInformation[0] = EXCEPTION_EXECUTE_FAULT;
        else if (get_error_code(context) & 0x800) rec.ExceptionInformation[0] = EXCEPTION_WRITE_FAULT;
        else rec.ExceptionInformation[0] = EXCEPTION_READ_FAULT;
        rec.ExceptionInformation[1] = (ULONG_PTR)siginfo->si_addr;
        if (!virtual_handle_fault( &rec, (void *)SP_sig(context) )) return;
        break;
    case TRAP_ARM_ALIGNFLT:  /* Alignment check exception */
        rec.ExceptionCode = EXCEPTION_DATATYPE_MISALIGNMENT;
        break;
    case TRAP_ARM_UNKNOWN:   /* Unknown fault code */
        rec.ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
        rec.NumberParameters = 2;
        rec.ExceptionInformation[0] = 0;
        rec.ExceptionInformation[1] = 0xffffffff;
        break;
    default:
        ERR("Got unexpected trap %d\n", get_trap_code(signal, context));
        rec.ExceptionCode = EXCEPTION_ILLEGAL_INSTRUCTION;
        break;
    }
    if (handle_syscall_fault( context, &rec )) return;
    setup_raise_exception( context, &rec, &ctx );
}


/**********************************************************************
 *		trap_handler
 *
 * Handler for SIGTRAP.
 */
static void trap_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    EXCEPTION_RECORD rec = { 0 };
    ucontext_t *context = sigcontext;
    CONTEXT ctx;

    rec.ExceptionAddress = (void *)PC_sig(context);
    save_context( &ctx, sigcontext );

    switch (siginfo->si_code)
    {
    case TRAP_TRACE:
        rec.ExceptionCode = EXCEPTION_SINGLE_STEP;
        break;
    case TRAP_BRKPT:
    default:
        ctx.Pc += 2;  /* skip the breakpoint instruction */
        rec.ExceptionCode = EXCEPTION_BREAKPOINT;
        rec.NumberParameters = 1;
        break;
    }
    setup_raise_exception( sigcontext, &rec, &ctx );
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

    if (!is_inside_syscall( SP_sig(ucontext) )) user_mode_abort_thread( 0, get_syscall_frame() );
    abort_thread(0);
}


/**********************************************************************
 *		usr1_handler
 *
 * Handler for SIGUSR1, used to signal a thread that it got suspended.
 */
static void usr1_handler( int signal, siginfo_t *siginfo, void *sigcontext )
{
    ucontext_t *ucontext = sigcontext;
    CONTEXT context;

    if (is_inside_syscall( SP_sig(ucontext) ))
    {
        context.ContextFlags = CONTEXT_FULL | CONTEXT_EXCEPTION_REQUEST;
        NtGetContextThread( GetCurrentThread(), &context );
        wait_suspend( &context );
        NtSetContextThread( GetCurrentThread(), &context );
    }
    else
    {
        save_context( &context, sigcontext );
        context.ContextFlags |= CONTEXT_EXCEPTION_REPORTING;
        wait_suspend( &context );
        restore_context( &context, sigcontext );
    }
}


/**********************************************************************
 *           get_thread_ldt_entry
 */
NTSTATUS get_thread_ldt_entry( HANDLE handle, THREAD_DESCRIPTOR_INFORMATION *info, ULONG len )
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
 *             signal_alloc_thread
 */
NTSTATUS signal_alloc_thread( TEB *teb )
{
    teb->WOW32Reserved = __wine_syscall_dispatcher;
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
void signal_init_process(void)
{
    struct sigaction sig_act;
    struct ntdll_thread_data *thread_data = ntdll_get_thread_data();
    void *kernel_stack = (char *)thread_data->kernel_stack + kernel_stack_size;

    thread_data->syscall_frame = (struct syscall_frame *)kernel_stack - 1;

    signal_alloc_thread( NtCurrentTeb() );

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
void init_syscall_frame( LPTHREAD_START_ROUTINE entry, void *arg, BOOL suspend, TEB *teb )
{
    struct syscall_frame *frame = ((struct ntdll_thread_data *)&teb->GdiTebBatch)->syscall_frame;
    CONTEXT *ctx, context = { CONTEXT_ALL };

    context.R0 = (DWORD)entry;
    context.R1 = (DWORD)arg;
    context.Sp = (DWORD)teb->Tib.StackBase;
    context.Pc = (DWORD)pRtlUserThreadStart;
    if (context.Pc & 1) context.Cpsr |= 0x20; /* thumb mode */
    if ((ctx = get_cpu_area( IMAGE_FILE_MACHINE_ARMNT ))) *ctx = context;

    if (suspend)
    {
        context.ContextFlags |= CONTEXT_EXCEPTION_REPORTING | CONTEXT_EXCEPTION_ACTIVE;
        wait_suspend( &context );
    }

    ctx = (CONTEXT *)((ULONG_PTR)context.Sp & ~15) - 1;
    *ctx = context;
    ctx->ContextFlags = CONTEXT_FULL;
    signal_set_full_context( ctx );

    frame->sp = (DWORD)ctx;
    frame->pc = (DWORD)pLdrInitializeThunk;
    frame->r0 = (DWORD)ctx;

    pthread_sigmask( SIG_UNBLOCK, &server_block_set, NULL );
}


/***********************************************************************
 *           signal_start_thread
 */
__ASM_GLOBAL_FUNC( signal_start_thread,
                   __ASM_EHABI(".cantunwind\n\t")
                   "push {r4-r12,lr}\n\t"
                   "add r7, sp, #0x28\n\t"    /* syscall_cfa */
                   "mcr p15, 0, r3, c13, c0, 2\n\t" /* set teb register */
                   /* set syscall frame */
                   "ldr r6, [r3, #0x218]\n\t" /* thread_data->syscall_frame */
                   "cbnz r6, 1f\n\t"
                   "sub r6, sp, #0x160\n\t"   /* sizeof(struct syscall_frame) */
                   "str r6, [r3, #0x218]\n\t" /* thread_data->syscall_frame */
                   "1:\tmov r4, #0\n\t"
                   "str r4, [r6, #0x44]\n\t"  /* frame->restore_flags */
                   "str r4, [r6, #0x4c]\n\t"  /* frame->prev_frame */
                   "str r7, [r6, #0x50]\n\t"  /* frame->syscall_cfa */
                   /* switch to kernel stack */
                   "mov sp, r6\n\t"
                   "mov r8, r6\n\t"
                   "bl init_syscall_frame\n\t"
                   "b " __ASM_LOCAL_LABEL("__wine_syscall_dispatcher_return") )


/***********************************************************************
 *           __wine_syscall_dispatcher
 */
__ASM_GLOBAL_FUNC( __wine_syscall_dispatcher,
                   __ASM_EHABI(".cantunwind\n\t")
                   "mrc p15, 0, r2, c13, c0, 2\n\t" /* NtCurrentTeb() */
                   "ldr r1, [r2, #0x218]\n\t"       /* thread_data->syscall_frame */
                   "add r0, r1, #0x10\n\t"
                   "stm r0, {r4-r12,lr}\n\t"
                   "str sp, [r1, #0x38]\n\t"
                   "str r3, [r1, #0x3c]\n\t"
                   "mrs r0, CPSR\n\t"
                   "bfi r0, lr, #5, #1\n\t"         /* set thumb bit */
                   "str r0, [r1, #0x40]\n\t"
                   "mov r0, #0\n\t"
                   "str r0, [r1, #0x44]\n\t"        /* frame->restore_flags */
                   "vmrs r0, fpscr\n\t"
                   "str r0, [r1, #0x48]\n\t"
                   "str ip, [r1, #0x54]\n\t"        /* frame->syscall_id */
                   "add r0, r1, #0x60\n\t"
                   "vstm r0, {d0-d15}\n\t"
                   "mov r6, sp\n\t"
                   "mov r8, r1\n\t"
                   /* switch to kernel stack */
                   "mov sp, r1\n\t"
                   __ASM_CFI_CFA_IS_AT2(r8, 0xd0, 0x00) /* frame->syscall_cfa */
                   __ASM_CFI(".cfi_offset r4,-0x28\n\t")
                   __ASM_CFI(".cfi_offset r5,-0x24\n\t")
                   __ASM_CFI(".cfi_offset r6,-0x20\n\t")
                   __ASM_CFI(".cfi_offset r7,-0x1c\n\t")
                   __ASM_CFI(".cfi_offset r8,-0x18\n\t")
                   __ASM_CFI(".cfi_offset r9,-0x14\n\t")
                   __ASM_CFI(".cfi_offset r10,-0x10\n\t")
                   __ASM_CFI(".cfi_offset r11,-0x0c\n\t")
                   __ASM_CFI(".cfi_offset r12,-0x08\n\t")
                   __ASM_CFI(".cfi_offset lr,-0x04\n\t")
                   "ldr r5, [r2, #0x214]\n\t"       /* thread_data->syscall_table */
                   "ubfx r4, ip, #12, #2\n\t"       /* syscall table number */
                   "bfc ip, #12, #20\n\t"           /* syscall number */
                   "add r4, r5, r4, lsl #4\n\t"
                   "ldr r5, [r4, #8]\n\t"           /* table->ServiceLimit */
                   "cmp ip, r5\n\t"
                   "bcs " __ASM_LOCAL_LABEL("bad_syscall") "\n\t"
                   "ldr r5, [r4, #12]\n\t"          /* table->ArgumentTable */
                   "ldrb r5, [r5, ip]\n\t"
                   "cmp r5, #16\n\t"
                   "it le\n\t"
                   "movle r5, #16\n\t"
                   "sub r0, sp, r5\n\t"
                   "and r0, #~7\n\t"
                   "mov sp, r0\n"
                   "2:\tsubs r5, r5, #4\n\t"
                   "ldr r0, [r6, r5]\n\t"
                   "str r0, [sp, r5]\n\t"
                   "bgt 2b\n\t"
                   "ldr r5, [r4]\n\t"               /* table->ServiceTable */
                   "ldr r5, [r5, ip, lsl #2]\n\t"
                   "ldr r7, [r2, #0x21c]\n\t"       /* thread_data->syscall_trace */
                   "cbnz r7, " __ASM_LOCAL_LABEL("trace_syscall") "\n\t"
                   "pop {r0-r3}\n\t"                /* first 4 args are in registers */
                   "blx r5\n"
                   __ASM_LOCAL_LABEL("__wine_syscall_dispatcher_return") ":\n\t"
                   "ldr ip, [r8, #0x44]\n\t"        /* frame->restore_flags */
                   "tst ip, #4\n\t"                 /* CONTEXT_FLOATING_POINT */
                   "beq 3f\n\t"
                   "ldr r4, [r8, #0x48]\n\t"
                   "vmsr fpscr, r4\n\t"
                   "add r4, r8, #0x60\n\t"
                   "vldm r4, {d0-d15}\n"
                   "3:\n\t"
                   "tst ip, #2\n\t"                 /* CONTEXT_INTEGER */
                   "it ne\n\t"
                   "ldmne r8, {r0-r3}\n\t"
                   "ldr lr, [r8, #0x3c]\n\t"
                   /* switch to user stack */
                   "ldr sp, [r8, #0x38]\n\t"
                   "add r8, r8, #0x10\n\t"
                   "ldm r8, {r4-r12,pc}\n"

                   __ASM_LOCAL_LABEL("trace_syscall") ":\n\t"
                   "ldr r0, [r8, #0x54]\n\t"        /* frame->syscall_id */
                   "mov r1, sp\n\t"                 /* args */
                   "ldr r2, [r4, #12]\n\t"          /* table->ArgumentTable */
                   "ubfx ip, r0, #0, #12\n\t"       /* syscall number */
                   "ldrb r2, [r2, ip]\n\t"          /* len */
                   "bl " __ASM_NAME("trace_syscall") "\n\t"
                   "pop {r0-r3}\n\t"
                   "blx r5\n"

                   __ASM_LOCAL_LABEL("trace_syscall_ret") ":\n\t"
                   "mov r6, r0\n\t"                 /* retval */
                   "ldr r0, [r8, #0x54]\n\t"        /* frame->syscall_id */
                   "mov r1, r6\n\t"                 /* retval */
                   "bl " __ASM_NAME("trace_sysret") "\n\t"
                   "mov r0, r6\n\t"                 /* status */
                   "b " __ASM_LOCAL_LABEL("__wine_syscall_dispatcher_return") "\n"

                   __ASM_LOCAL_LABEL("bad_syscall") ":\n\t"
                   "movw r0, #0x001c\n\t"           /* STATUS_INVALID_SYSTEM_SERVICE */
                   "movt r0, #0xc000\n\t"
                   "b " __ASM_LOCAL_LABEL("__wine_syscall_dispatcher_return") )

__ASM_GLOBAL_FUNC( __wine_syscall_dispatcher_return,
                   "mov sp, r8\n\t"
                   "ldr r3, [r1, #0x21c]\n\t"       /* thread_data->syscall_trace */
                   "cbnz r3, 1f\n\t"
                   "b " __ASM_LOCAL_LABEL("__wine_syscall_dispatcher_return") "\n"
                   "1:\tb " __ASM_LOCAL_LABEL("trace_syscall_ret") )


/***********************************************************************
 *           __wine_unix_call_dispatcher
 */
__ASM_GLOBAL_FUNC( __wine_unix_call_dispatcher,
                   __ASM_EHABI(".cantunwind\n\t")
                   "mrc p15, 0, r1, c13, c0, 2\n\t" /* NtCurrentTeb() */
                   "ldr r1, [r1, #0x218]\n\t"       /* thread_data->syscall_frame */
                   "add ip, r1, #0x10\n\t"
                   "stm ip, {r4-r12,lr}\n\t"
                   "str sp, [r1, #0x38]\n\t"
                   "str lr, [r1, #0x3c]\n\t"
                   "mrs r4, CPSR\n\t"
                   "bfi r4, lr, #5, #1\n\t"         /* set thumb bit */
                   "str r4, [r1, #0x40]\n\t"
                   "mov r4, #0\n\t"
                   "str r4, [r1, #0x44]\n\t"        /* frame->restore_flags */
                   "vmrs r4, fpscr\n\t"
                   "str r4, [r1, #0x48]\n\t"
                   "add r4, r1, #0x60\n\t"
                   "vstm r4, {d0-d15}\n\t"
                   "ldr ip, [r0, r2, lsl #2]\n\t"
                   "mov r8, r1\n\t"
                   /* switch to kernel stack */
                   "mov sp, r1\n\t"
                   __ASM_CFI_CFA_IS_AT2(r8, 0xd0, 0x00) /* frame->syscall_cfa */
                   __ASM_CFI(".cfi_offset r4,-0x28\n\t")
                   __ASM_CFI(".cfi_offset r5,-0x24\n\t")
                   __ASM_CFI(".cfi_offset r6,-0x20\n\t")
                   __ASM_CFI(".cfi_offset r7,-0x1c\n\t")
                   __ASM_CFI(".cfi_offset r8,-0x18\n\t")
                   __ASM_CFI(".cfi_offset r9,-0x14\n\t")
                   __ASM_CFI(".cfi_offset r10,-0x10\n\t")
                   __ASM_CFI(".cfi_offset r11,-0x0c\n\t")
                   __ASM_CFI(".cfi_offset r12,-0x08\n\t")
                   __ASM_CFI(".cfi_offset lr,-0x04\n\t")
                   "mov r0, r3\n\t"                 /* args */
                   "blx ip\n"
                   "ldr r1, [r8, #0x44]\n\t"        /* frame->restore_flags */
                   "cbnz r1, 1f\n\t"
                   /* switch to user stack */
                   "ldr sp, [r8, #0x38]\n\t"
                   "add r8, r8, #0x10\n\t"
                   "ldm r8, {r4-r12,pc}\n\t"
                   "1:\tb " __ASM_LOCAL_LABEL("__wine_syscall_dispatcher_return") )

#endif  /* __arm__ */
