#if 0
#pragma makedep unix
#endif

#if defined(__riscv) && (__riscv_xlen == 64)

#include "config.h"

#include <assert.h>
#include <stddef.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winnt.h"
#include "winternl.h"
#include "wine/asm.h"
#include "unix_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);

#define REGn_sig(reg, context) ((context)->uc_mcontext.__gregs[(reg)])
#define PC_sig(context) REGn_sig( REG_PC, context )
#define RA_sig(context) REGn_sig( REG_RA, context )
#define SP_sig(context) REGn_sig( REG_SP, context )

struct syscall_frame
{
    ULONG64 x[32];
    ULONG64 pc;
    ULONG64 sp;
    ULONG64 ra;

    ULONG64 f[32];
    ULONG64 fcsr;

    ULONG restore_flags;

    struct syscall_frame *prev_frame;
    void *syscall_cfa;
    ULONG syscall_id;
};

C_ASSERT( sizeof( struct syscall_frame ) % 16 == 0 );
C_ASSERT( offsetof( struct syscall_frame, x ) == 0 );
C_ASSERT( offsetof( struct syscall_frame, pc ) == 256 );
C_ASSERT( offsetof( struct syscall_frame, sp ) == 264 );
C_ASSERT( offsetof( struct syscall_frame, ra ) == 272 );
C_ASSERT( offsetof( struct syscall_frame, f ) == 280 );
C_ASSERT( offsetof( struct syscall_frame, fcsr ) == 536 );
C_ASSERT( offsetof( struct syscall_frame, restore_flags ) == 544 );
C_ASSERT( offsetof( struct syscall_frame, prev_frame ) == 552 );
C_ASSERT( offsetof( struct syscall_frame, syscall_cfa ) == 560 );
C_ASSERT( offsetof( struct syscall_frame, syscall_id ) == 568 );

static struct syscall_frame *init_syscall_frame( PRTL_THREAD_START_ROUTINE entry,
                                                 void *arg, TEB *teb )
{
    struct thread_data *data = get_thread_data();
    struct syscall_frame *frame = get_syscall_frame( data );
    CONTEXT context = { CONTEXT_ALL };
    CONTEXT *ctx;
    ULONG_PTR stack_top;

    memset( frame, 0, sizeof(*frame) );

    stack_top = (ULONG_PTR)teb->Tib.StackBase & ~(ULONG_PTR)15;

    context.Gpr.X[10] = (ULONG64)entry;
    context.Gpr.X[11] = (ULONG64)arg;
    context.Gpr.X[4]  = (ULONG64)teb;
    context.Gpr.X[2]  = stack_top;
    context.Pc        = (ULONG64)pRtlUserThreadStart;

    ctx = (CONTEXT *)(stack_top - sizeof(*ctx));
    *ctx = context;
    ctx->ContextFlags = CONTEXT_FULL;

    /*
     * First enter LdrInitializeThunk with the initial CONTEXT.
     * It will initialize the loader and continue at RtlUserThreadStart.
     */
    frame->pc = (ULONG64)pLdrInitializeThunk;
    frame->sp = (ULONG64)ctx;
    frame->x[10] = (ULONG64)ctx;
    frame->x[4] = (ULONG64)teb;
    frame->x[2] = frame->sp;
    frame->x[1] = frame->ra;

    frame->restore_flags = CONTEXT_CONTROL | CONTEXT_INTEGER;

    return frame;
}


void set_process_instrumentation_callback( void *callback )
{
}


NTSTATUS signal_set_full_context( CONTEXT *context )
{
    struct thread_data *data = get_thread_data();
    struct syscall_frame *frame = get_syscall_frame( data );
    NTSTATUS status;

    status = NtSetContextThread( GetCurrentThread(), context );

    if (!status && frame && (context->ContextFlags & CONTEXT_INTEGER))
        frame->restore_flags |= CONTEXT_INTEGER;

    return status;
}


NTSTATUS WINAPI NtGetContextThread( HANDLE handle, CONTEXT *context )
{
    struct thread_data *data = get_thread_data();
    struct syscall_frame *frame = get_syscall_frame( data );
    DWORD flags = context->ContextFlags & ~CONTEXT_RISCV64;

    if (handle != GetCurrentThread())
        return STATUS_NOT_IMPLEMENTED;

    if (!frame)
        return STATUS_ACCESS_DENIED;

    if (flags & CONTEXT_INTEGER)
        memcpy( context->Gpr.X, frame->x, sizeof(frame->x) );

    if (flags & CONTEXT_CONTROL)
    {
        context->Pc = frame->pc;
        context->Gpr.X[2] = frame->sp;
        context->Gpr.X[1] = frame->ra;
    }

    if (flags & CONTEXT_FLOATING_POINT)
    {
        unsigned int i;
        for (i = 0; i < 32; i++) context->F[i].Q = frame->f[i];
        context->Fcsr = frame->fcsr;
    }

    return STATUS_SUCCESS;
}


NTSTATUS WINAPI NtSetContextThread( HANDLE handle, const CONTEXT *context )
{
    struct thread_data *data = get_thread_data();
    struct syscall_frame *frame = get_syscall_frame( data );
    DWORD flags = context->ContextFlags & ~CONTEXT_RISCV64;

    if (handle != GetCurrentThread())
        return STATUS_NOT_IMPLEMENTED;

    if (!frame)
        return STATUS_ACCESS_DENIED;

    if (flags & CONTEXT_INTEGER)
        memcpy( frame->x, context->Gpr.X, sizeof(frame->x) );

    if (flags & CONTEXT_CONTROL)
    {
        frame->pc = context->Pc;
        frame->sp = context->Gpr.X[2];
        frame->ra = context->Gpr.X[1];
    }

    if (flags & CONTEXT_FLOATING_POINT)
    {
        unsigned int i;
        for (i = 0; i < 32; i++) frame->f[i] = context->F[i].Q;
        frame->fcsr = context->Fcsr;
    }

    frame->restore_flags |= flags & (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_FLOATING_POINT);
 
    return STATUS_SUCCESS;
}


NTSTATUS set_thread_wow64_context( HANDLE handle, const void *ctx, ULONG size )
{
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS get_thread_wow64_context( HANDLE handle, void *ctx, ULONG size )
{
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS call_user_apc_dispatcher( CONTEXT *context_ptr, unsigned int flags,
                                   ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3,
                                   void (*func)(ULONG_PTR, ULONG_PTR, ULONG_PTR),
                                   NTSTATUS status )
{
    return STATUS_NOT_IMPLEMENTED;
}


void call_raise_user_exception_dispatcher( struct thread_data *data )
{
}


NTSTATUS call_user_exception_dispatcher( struct thread_data *data,
                                         EXCEPTION_RECORD *rec,
                                         CONTEXT *context )
{
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS get_thread_ldt_entry( HANDLE handle,
                               THREAD_DESCRIPTOR_INFORMATION *info,
                               ULONG len )
{
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS signal_alloc_thread( TEB *teb )
{
    return STATUS_SUCCESS;
}


void signal_free_thread( TEB *teb )
{
}


void signal_init_process( TEB *teb )
{
    alloc_syscall_frame( sizeof(struct syscall_frame) );
}


void DECLSPEC_NORETURN signal_start_thread( PRTL_THREAD_START_ROUTINE entry,
                                            void *arg, TEB *teb )
{
    struct syscall_frame *frame;

    frame = init_syscall_frame( entry, arg, teb );

    /*
     * Enter the RV64 dispatcher-return path with the syscall
     * frame pointer in a0.
     *
     * The indirect register form avoids an R_RISCV_JAL relocation
     * against an externally-bound symbol when building ntdll.so.
     */
    __asm__ volatile (
        "mv a0, %0\n"
        "jr %[dispatcher]\n"
        :
        : "r"(frame),
          [dispatcher] "r"(__wine_syscall_dispatcher_return)
        : "a0", "memory"
    );

    __builtin_unreachable();
}

void __wine_syscall_dispatcher_return( void )
{
    /*
     * a0 = struct syscall_frame *
     *
     * Preserve the frame pointer in t0 while restoring registers.
     * Since t0 is x5, restore it last.
     * t6 holds the destination PC and is not restored in this POC.
     */
    __asm__ volatile (
        "mv t0, a0\n"
        "ld t6, 256(t0)\n"

        "ld x1,    8(t0)\n"
        "ld x2,   16(t0)\n"
        "ld x3,   24(t0)\n"
        "ld x4,   32(t0)\n"
        "ld x6,   48(t0)\n"
        "ld x7,   56(t0)\n"
        "ld x8,   64(t0)\n"
        "ld x9,   72(t0)\n"
        "ld x10,  80(t0)\n"
        "ld x11,  88(t0)\n"
        "ld x12,  96(t0)\n"
        "ld x13, 104(t0)\n"
        "ld x14, 112(t0)\n"
        "ld x15, 120(t0)\n"
        "ld x16, 128(t0)\n"
        "ld x17, 136(t0)\n"
        "ld x18, 144(t0)\n"
        "ld x19, 152(t0)\n"
        "ld x20, 160(t0)\n"
        "ld x21, 168(t0)\n"
        "ld x22, 176(t0)\n"
        "ld x23, 184(t0)\n"
        "ld x24, 192(t0)\n"
        "ld x25, 200(t0)\n"
        "ld x26, 208(t0)\n"
        "ld x27, 216(t0)\n"
        "ld x28, 224(t0)\n"
        "ld x29, 232(t0)\n"
        "ld x30, 240(t0)\n"

        "ld sp, 264(t0)\n"
        "ld x5,  40(t0)\n"

        "jr t6\n"
    );

    __builtin_unreachable();
}

/* Initial RV64 NT-service bridge.  Calls with more than seven argument
 * registers are deliberately rejected until stack-argument marshalling
 * is implemented.  The helper runs on the Unix stack. */
static NTSTATUS __attribute__((used)) riscv64_dispatch_syscall( ULONG id, const ULONG64 *args )
{
    struct thread_data *data = get_thread_data();
    SYSTEM_SERVICE_TABLE *tables;
    SYSTEM_SERVICE_TABLE *table;
    ULONG index = id & 0xfff;
    ULONG table_id = (id >> 12) & 3;
    ULONG arg_size;
    typedef NTSTATUS (*service_t)(ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR,
                                  ULONG_PTR, ULONG_PTR, ULONG_PTR, ULONG_PTR);
    service_t service;

    if (!data || !data->teb) return STATUS_INVALID_SYSTEM_SERVICE;
    tables = get_teb_data(data)->syscall_table;
    if (!tables) return STATUS_INVALID_SYSTEM_SERVICE;
    table = &tables[table_id];
    if (!table->ServiceTable || !table->ArgumentTable || index >= table->ServiceLimit)
        return STATUS_INVALID_SYSTEM_SERVICE;
    arg_size = table->ArgumentTable[index];
    if (arg_size > 7 * sizeof(ULONG_PTR)) return STATUS_NOT_IMPLEMENTED;
    service = (service_t)table->ServiceTable[index];
    if (!service) return STATUS_INVALID_SYSTEM_SERVICE;
    return service( args[0], args[1], args[2], args[3],
                    args[4], args[5], args[6], 0 );
}

/* The Windows stub tail-jumps here with a7 = NT service id, ra = caller.
 * Save arguments on the original stack before using the C ABI to locate
 * the per-thread syscall frame.  The syscall frame is at the high end of
 * Wine's Unix kernel stack; C calls grow downwards from it. */
__ASM_GLOBAL_FUNC( __wine_syscall_dispatcher,
                   "addi sp, sp, -96\n\t"
                   "sd ra, 0(sp)\n\t"
                   "sd s0, 8(sp)\n\t"
                   "sd s1, 16(sp)\n\t"
                   "sd a0, 24(sp)\n\t"
                   "sd a1, 32(sp)\n\t"
                   "sd a2, 40(sp)\n\t"
                   "sd a3, 48(sp)\n\t"
                   "sd a4, 56(sp)\n\t"
                   "sd a5, 64(sp)\n\t"
                   "sd a6, 72(sp)\n\t"
                   "sd a7, 80(sp)\n\t"
                   "mv a0, sp\n\t"
                   "call " __ASM_NAME("riscv64_prepare_syscall") "\n\t"
                   "beqz a0, 2f\n\t"
                   "mv t0, a0\n\t"        /* frame */
                   "ld t1, 80(sp)\n\t"    /* service id */
                   "sw t1, 568(t0)\n\t"
                   "ld t1, 0(sp)\n\t"     /* return address */
                   "sd t1, 272(t0)\n\t"
                   "sd t1, 256(t0)\n\t"
                   "addi t1, sp, 96\n\t"
                   "sd t1, 264(t0)\n\t"
                   "ld t1, 8(sp)\n\t"
                   "sd t1, 64(t0)\n\t"
                   "ld t1, 16(sp)\n\t"
                   "sd t1, 72(t0)\n\t"
                   "sd s2, 144(t0)\n\t"
                   "sd s3, 152(t0)\n\t"
                   "sd s4, 160(t0)\n\t"
                   "sd s5, 168(t0)\n\t"
                   "sd s6, 176(t0)\n\t"
                   "sd s7, 184(t0)\n\t"
                   "sd s8, 192(t0)\n\t"
                   "sd s9, 200(t0)\n\t"
                   "sd s10, 208(t0)\n\t"
                   "sd s11, 216(t0)\n\t"
                   "ld t1, 24(sp)\n\t"
                   "sd t1, 80(t0)\n\t"
                   "ld t1, 32(sp)\n\t"
                   "sd t1, 88(t0)\n\t"
                   "ld t1, 40(sp)\n\t"
                   "sd t1, 96(t0)\n\t"
                   "ld t1, 48(sp)\n\t"
                   "sd t1, 104(t0)\n\t"
                   "ld t1, 56(sp)\n\t"
                   "sd t1, 112(t0)\n\t"
                   "ld t1, 64(sp)\n\t"
                   "sd t1, 120(t0)\n\t"
                   "ld t1, 72(sp)\n\t"
                   "sd t1, 128(t0)\n\t"
                   "ld t1, 80(sp)\n\t"
                   "sd t1, 136(t0)\n\t"
                   "mv sp, t0\n\t"
                   "lw a0, 568(sp)\n\t"
                   "addi a1, sp, 80\n\t"
                   "call " __ASM_NAME("riscv64_dispatch_syscall") "\n\t"
                   "mv t0, sp\n\t"
                   "ld s0, 64(t0)\n\t"
                   "ld s1, 72(t0)\n\t"
                   "ld s2, 144(t0)\n\t"
                   "ld s3, 152(t0)\n\t"
                   "ld s4, 160(t0)\n\t"
                   "ld s5, 168(t0)\n\t"
                   "ld s6, 176(t0)\n\t"
                   "ld s7, 184(t0)\n\t"
                   "ld s8, 192(t0)\n\t"
                   "ld s9, 200(t0)\n\t"
                   "ld s10, 208(t0)\n\t"
                   "ld s11, 216(t0)\n\t"
                   "ld ra, 272(t0)\n\t"
                   "ld sp, 264(t0)\n\t"
                   "ret\n"
                   "2:\tld ra, 0(sp)\n\t"
                   "ld s0, 8(sp)\n\t"
                   "ld s1, 16(sp)\n\t"
                   "addi sp, sp, 96\n\t"
                   "li a0, 0xc000001c\n\t" /* STATUS_INVALID_SYSTEM_SERVICE */
                   "ret" )

/* Called before switching stacks.  Its return value is an existing
 * per-thread frame, or NULL if thread initialization is incomplete. */
static struct syscall_frame *__attribute__((used)) riscv64_prepare_syscall( void *unused )
{
    struct thread_data *data = get_thread_data();
    return data && data->teb ? get_syscall_frame(data) : NULL;
}
 

void __wine_unix_call_dispatcher(void)
{
    abort();
}

/***********************************************************************
 *           unwind_builtin_dll
 *
 * RISC-V ELF unwinding is not implemented yet.
 */
NTSTATUS unwind_builtin_dll( void *args )
{
    return STATUS_UNSUCCESSFUL;
}

void *get_native_context( CONTEXT *context )
{
    return NULL;
}


void *get_wow_context( CONTEXT *context )
{
    return NULL;
}

NTSTATUS DECLSPEC_EXPORT KeUserModeCallback( ULONG id, const void *args, ULONG len,
                             void **ret_ptr, ULONG *ret_len )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI NtCallbackReturn( void *ret_ptr, ULONG ret_len, NTSTATUS status )
{
    return STATUS_NOT_IMPLEMENTED;
}

#endif /* defined(__riscv) && (__riscv_xlen == 64) */

