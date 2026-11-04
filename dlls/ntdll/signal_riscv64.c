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

/* Context management. */
__ASM_GLOBAL_FUNC( RtlCaptureContext, "ret" )
__ASM_GLOBAL_FUNC( RtlRestoreContext, "ret" )

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
    loader_init( context, (void **)&context->Gpr.X[10] );
    NtContinue( context, TRUE );
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

/* Processor information. */
__ASM_GLOBAL_FUNC( RtlIsProcessorFeaturePresent, "li a0, 0\n\tret" )
__ASM_GLOBAL_FUNC( RtlGetNativeSystemInformation,
                   "li a0, -1073741822\n\tret" )

/* Jump-buffer helpers. */
__ASM_GLOBAL_FUNC( NTDLL__setjmpex, "li a0, 0\n\tret" )
__ASM_GLOBAL_FUNC( NTDLL_longjmp, "ret" )

#endif /* defined(__riscv) && (__riscv_xlen == 64) */

