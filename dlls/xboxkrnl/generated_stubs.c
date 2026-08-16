/*
 * xboxkrnl.exe - generated FIXME stubs for xboxkrnl ordinals with no real
 * implementation yet.
 *
 * THIS FILE IS GENERATED (see the codegen notes in the project commit that
 * added it) from the real ordinal table in Cxbx-Reloaded's
 * src/core/kernel/exports/KernelThunk.cpp and the per-function signatures in
 * src/core/kernel/common/[*].h. Every export below is a real, correctly-named,
 * correctly-numbered xboxkrnl.exe ordinal; what's NOT real is any behaviour -
 * each one only logs a FIXME and returns a plausible-but-inert value
 * (STATUS_NOT_IMPLEMENTED for NTSTATUS-returning exports, NULL for
 * pointer-returning ones, 0 otherwise, nothing for void ones). None of these
 * should be mistaken for working Xbox kernel functionality.
 *
 * See dlls/xboxkrnl/main.c for the subset of ordinals that DO have a real,
 * verified-against-Cxbx-Reloaded-semantics forward to existing Wine/ntdll
 * functionality instead of living here.
 */

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "ntstatus.h"
#include "wine/asm.h"
#include "wine/debug.h"

/* debug channel retained for future stubs */
/* WINE_DEFAULT_DEBUG_CHANNEL(xboxkrnl); */

DEFINE_FASTCALL_WRAPPER(XBOXKRNL_ExInterlockedAddLargeStatistic,8)
void FASTCALL XBOXKRNL_ExInterlockedAddLargeStatistic( void *a0, LONG a1 );
DEFINE_FASTCALL_WRAPPER(XBOXKRNL_ExInterlockedCompareExchange64,12)
LONG FASTCALL XBOXKRNL_ExInterlockedCompareExchange64( void *a0, void *a1, void *a2 );
DEFINE_FASTCALL_WRAPPER(XBOXKRNL_ExfInterlockedInsertHeadList,8)
LONG FASTCALL XBOXKRNL_ExfInterlockedInsertHeadList( void *a0, void *a1 );
DEFINE_FASTCALL_WRAPPER(XBOXKRNL_ExfInterlockedInsertTailList,8)
LONG FASTCALL XBOXKRNL_ExfInterlockedInsertTailList( void *a0, void *a1 );
DEFINE_FASTCALL_WRAPPER(XBOXKRNL_ExfInterlockedRemoveHeadList,4)
LONG FASTCALL XBOXKRNL_ExfInterlockedRemoveHeadList( void *a0 );
DEFINE_FASTCALL_WRAPPER(XBOXKRNL_HalClearSoftwareInterrupt,4)
void FASTCALL XBOXKRNL_HalClearSoftwareInterrupt( LONG a0 );
DEFINE_FASTCALL_WRAPPER(XBOXKRNL_HalRequestSoftwareInterrupt,4)
void FASTCALL XBOXKRNL_HalRequestSoftwareInterrupt( LONG a0 );
DEFINE_FASTCALL_WRAPPER(XBOXKRNL_IofCallDriver,8)
LONG FASTCALL XBOXKRNL_IofCallDriver( void *a0, void *a1 );
DEFINE_FASTCALL_WRAPPER(XBOXKRNL_IofCompleteRequest,8)
void FASTCALL XBOXKRNL_IofCompleteRequest( void *a0, LONG a1 );
DEFINE_FASTCALL_WRAPPER(XBOXKRNL_KfRaiseIrql,4)
LONG FASTCALL XBOXKRNL_KfRaiseIrql( LONG a0 );
DEFINE_FASTCALL_WRAPPER(XBOXKRNL_KfLowerIrql,4)
void FASTCALL XBOXKRNL_KfLowerIrql( LONG a0 );
DEFINE_FASTCALL_WRAPPER(XBOXKRNL_KiUnlockDispatcherDatabase,4)
void FASTCALL XBOXKRNL_KiUnlockDispatcherDatabase( LONG a0 );
DEFINE_FASTCALL_WRAPPER(XBOXKRNL_ObfDereferenceObject,4)
void FASTCALL XBOXKRNL_ObfDereferenceObject( void *a0 );
DEFINE_FASTCALL_WRAPPER(XBOXKRNL_ObfReferenceObject,4)
void FASTCALL XBOXKRNL_ObfReferenceObject( void *a0 );

















































































































































































































