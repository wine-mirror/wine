/*
 * RISC-V64 native signal handling and syscall entry points.
 *
 * This file provides build-enabling stubs for the native RISC-V64 port.
 * Context handling, exception dispatch, unwinding, and thread startup
 * require proper implementations.
 */

#if defined(__riscv) && (__riscv_xlen == 64)

#include <stdarg.h>
#include "ntstatus.h"
#include "windef.h"
#include "winternl.h"
#include "wine/asm.h"
#include "ntdll_misc.h"
#include "ntsyscalls.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(riscv64);

#define SYSCALL_ENTRY(id,name,args) __ASM_SYSCALL_FUNC( id, name )
ALL_SYSCALLS
#undef SYSCALL_ENTRY

/*
 * Temporary build-enabling implementations.
 * Proper RISC-V64 SEH dispatch and breakpoint handling are still required.
 */
NTSTATUS call_seh_handlers( EXCEPTION_RECORD *rec, CONTEXT *context )
{
    (void)rec;
    (void)context;
    return STATUS_UNHANDLED_EXCEPTION;
}

void WINAPI process_breakpoint(void)
{
    __asm__ volatile("ebreak");
}


/* Stack probing. */
__ASM_GLOBAL_FUNC( __chkstk, "ret" )

/* Capture the integer/control state at the caller boundary. */
__ASM_GLOBAL_FUNC( RtlCaptureContext,
                   "sd t0, 48(a0)\n\t"
                   "li t0, 0x800003\n\t"
                   "sw t0, 0(a0)\n\t"
                   "sd zero, 8(a0)\n\t"
                   "sd x1, 16(a0)\n\t"
                   "sd x2, 24(a0)\n\t"
                   "sd x3, 32(a0)\n\t"
                   "sd x4, 40(a0)\n\t"
                   "sd x6, 56(a0)\n\t"
                   "sd x7, 64(a0)\n\t"
                   "sd x8, 72(a0)\n\t"
                   "sd x9, 80(a0)\n\t"
                   "sd x11, 96(a0)\n\t"
                   "sd x12, 104(a0)\n\t"
                   "sd x13, 112(a0)\n\t"
                   "sd x14, 120(a0)\n\t"
                   "sd x15, 128(a0)\n\t"
                   "sd x16, 136(a0)\n\t"
                   "sd x17, 144(a0)\n\t"
                   "sd x18, 152(a0)\n\t"
                   "sd x19, 160(a0)\n\t"
                   "sd x20, 168(a0)\n\t"
                   "sd x21, 176(a0)\n\t"
                   "sd x22, 184(a0)\n\t"
                   "sd x23, 192(a0)\n\t"
                   "sd x24, 200(a0)\n\t"
                   "sd x25, 208(a0)\n\t"
                   "sd x26, 216(a0)\n\t"
                   "sd x27, 224(a0)\n\t"
                   "sd x28, 232(a0)\n\t"
                   "sd x29, 240(a0)\n\t"
                   "sd x30, 248(a0)\n\t"
                   "sd x31, 256(a0)\n\t"
                   "sd zero, 88(a0)\n\t" /* a0 is the context pointer */
                   "sd ra, 264(a0)\n\t"
                   "ret" )

/* NtContinue performs the non-returning context transfer. */
void WINAPI RtlRestoreContext( CONTEXT *context, EXCEPTION_RECORD *record )
{
    NTSTATUS status = NtContinue( context, FALSE );
    ERR( "RISCV64 RtlRestoreContext: NtContinue returned %#lx\n", (unsigned long)status );
    __builtin_trap();
}

/* Exception dispatch. */
__ASM_GLOBAL_FUNC( RtlRaiseException, "ret" )
__ASM_GLOBAL_FUNC( KiUserExceptionDispatcher, "ret" )
__ASM_GLOBAL_FUNC( KiUserApcDispatcher, "ret" )
__ASM_GLOBAL_FUNC( KiUserCallbackDispatcher, "ret" )

/* Unwinding. */
__ASM_GLOBAL_FUNC( RtlUnwindEx, "ret" )
__ASM_GLOBAL_FUNC( RtlVirtualUnwind, "ret" )
__ASM_GLOBAL_FUNC( RtlVirtualUnwind2, "li a0, -1073741822\n\tret" )
__ASM_GLOBAL_FUNC( RtlWalkFrameChain, "li a0, 0\n\tret" )
__ASM_GLOBAL_FUNC( __C_specific_handler, "li a0, 1\n\tret" )

/* Thread startup. */
void WINAPI LdrInitializeThunk( CONTEXT *context, ULONG_PTR unk2,
                                ULONG_PTR unk3, ULONG_PTR unk4 )
{
    NTSTATUS status;

    WARN( "RISCV64 LdrInitializeThunk: context=%p pc=%p sp=%p ra=%p\n",
          context,
          (void *)(ULONG_PTR)context->Pc,
          (void *)(ULONG_PTR)context->Gpr.X[2],
          (void *)(ULONG_PTR)context->Gpr.X[1] );

    loader_init( context, (void **)&context->Gpr.X[10] );

    WARN( "RISCV64 LdrInitializeThunk: after loader_init pc=%p sp=%p ra=%p\n",
          (void *)(ULONG_PTR)context->Pc,
          (void *)(ULONG_PTR)context->Gpr.X[2],
          (void *)(ULONG_PTR)context->Gpr.X[1] );

    status = NtContinue( context, TRUE );

    ERR( "RISCV64 NtContinue unexpectedly returned status=%#lx\n",
         (unsigned long)status );

    __builtin_trap();
}

void WINAPI RtlUserThreadStart( PRTL_THREAD_START_ROUTINE entry, void *arg )
{
    pBaseThreadInitThunk( 0, (LPTHREAD_START_ROUTINE)entry, arg );

    /* BaseThreadInitThunk should terminate the thread, not return. */
    RtlExitUserThread( STATUS_UNSUCCESSFUL );
    __builtin_unreachable();
}

/* Debugger entry points. */
__ASM_GLOBAL_FUNC( DbgBreakPoint, "ebreak\n\tret" )
__ASM_GLOBAL_FUNC( DbgUserBreakPoint, "ebreak\n\tret" )
__ASM_GLOBAL_FUNC( DbgUiRemoteBreakin, "ret" )

/* RISC-V64 has no Windows x86 XSAVE extended-feature state.  As on
 * ARM64, report no enabled extended context features. */
ULONG64 WINAPI RtlGetEnabledExtendedFeatures( ULONG64 feature_mask )
{
    return 0;
}

/* Processor information. */
__ASM_GLOBAL_FUNC( RtlIsProcessorFeaturePresent, "li a0, 0\n\tret" )
__ASM_GLOBAL_FUNC( RtlGetNativeSystemInformation,
                   "li a0, -1073741822\n\tret" )

/* Jump-buffer helpers. */
__ASM_GLOBAL_FUNC( NTDLL__setjmpex, "li a0, 0\n\tret" )
__ASM_GLOBAL_FUNC( NTDLL_longjmp, "ret" )

#endif /* defined(__riscv) && (__riscv_xlen == 64) */

