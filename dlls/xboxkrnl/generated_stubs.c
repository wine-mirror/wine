/*
 * xboxkrnl.exe - generated FIXME stubs for xboxkrnl ordinals with no real
 * implementation yet.
 *
 * THIS FILE IS GENERATED (see the codegen notes in the project commit that
 * added it) from the real ordinal table in Cxbx-Reloaded's
 * src/core/kernel/exports/KernelThunk.cpp and the per-function signatures in
 * src/core/kernel/common/*.h. Every export below is a real, correctly-named,
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

WINE_DEFAULT_DEBUG_CHANNEL(xboxkrnl);

LONG WINAPI XBOXKRNL_AvGetSavedDataAddress( void );
void WINAPI XBOXKRNL_AvSendTVEncoderOption( void *a0, LONG a1, LONG a2, void *a3 );
LONG WINAPI XBOXKRNL_AvSetDisplayMode( void *a0, LONG a1, LONG a2, LONG a3, LONG a4, LONG a5 );
void WINAPI XBOXKRNL_AvSetSavedDataAddress( void *a0 );
void WINAPI XBOXKRNL_DbgBreakPointWithStatus( LONG a0 );
LONG WINAPI XBOXKRNL_DbgLoadImageSymbols( void *a0, void *a1, LONG a2 );
LONG WINAPI XBOXKRNL_HalReadSMCTrayState( void *a0, void *a1 );
LONG WINAPI XBOXKRNL_DbgPrompt( void *a0, void *a1, LONG a2 );
void WINAPI XBOXKRNL_DbgUnLoadImageSymbols( void *a0, void *a1, LONG a2 );
void WINAPI XBOXKRNL_ExAcquireReadWriteLockExclusive( void *a0 );
void WINAPI XBOXKRNL_ExAcquireReadWriteLockShared( void *a0 );
void WINAPI XBOXKRNL_ExInitializeReadWriteLock( void *a0 );
LONG WINAPI XBOXKRNL_ExInterlockedAddLargeInteger( void *a0, INT64 a1, void *a2 );
DEFINE_FASTCALL_WRAPPER(XBOXKRNL_ExInterlockedAddLargeStatistic,8)
void FASTCALL XBOXKRNL_ExInterlockedAddLargeStatistic( void *a0, LONG a1 );
DEFINE_FASTCALL_WRAPPER(XBOXKRNL_ExInterlockedCompareExchange64,12)
LONG FASTCALL XBOXKRNL_ExInterlockedCompareExchange64( void *a0, void *a1, void *a2 );
LONG WINAPI XBOXKRNL_ExQueryPoolBlockSize( void *a0 );
LONG WINAPI XBOXKRNL_ExQueryNonVolatileSetting( LONG a0, void *a1, void *a2, LONG a3, void *a4 );
LONG WINAPI XBOXKRNL_ExReadWriteRefurbInfo( void *a0, LONG a1, LONG a2 );
void WINAPI XBOXKRNL_ExRaiseException( void *a0 );
void WINAPI XBOXKRNL_ExRaiseStatus( LONG a0 );
void WINAPI XBOXKRNL_ExReleaseReadWriteLock( void *a0 );
LONG WINAPI XBOXKRNL_ExSaveNonVolatileSetting( LONG a0, LONG a1, void *a2, LONG a3 );
DEFINE_FASTCALL_WRAPPER(XBOXKRNL_ExfInterlockedInsertHeadList,8)
LONG FASTCALL XBOXKRNL_ExfInterlockedInsertHeadList( void *a0, void *a1 );
DEFINE_FASTCALL_WRAPPER(XBOXKRNL_ExfInterlockedInsertTailList,8)
LONG FASTCALL XBOXKRNL_ExfInterlockedInsertTailList( void *a0, void *a1 );
DEFINE_FASTCALL_WRAPPER(XBOXKRNL_ExfInterlockedRemoveHeadList,4)
LONG FASTCALL XBOXKRNL_ExfInterlockedRemoveHeadList( void *a0 );
LONG WINAPI XBOXKRNL_FscGetCacheSize( void );
void WINAPI XBOXKRNL_FscInvalidateIdleBlocks( void );
LONG WINAPI XBOXKRNL_FscSetCacheSize( LONG a0 );
DEFINE_FASTCALL_WRAPPER(XBOXKRNL_HalClearSoftwareInterrupt,4)
void FASTCALL XBOXKRNL_HalClearSoftwareInterrupt( LONG a0 );
void WINAPI XBOXKRNL_HalDisableSystemInterrupt( LONG a0 );
void WINAPI XBOXKRNL_HalEnableSystemInterrupt( LONG a0, LONG a1 );
LONG WINAPI XBOXKRNL_HalGetInterruptVector( LONG a0, void *a1 );
LONG WINAPI XBOXKRNL_HalReadSMBusValue( LONG a0, LONG a1, LONG a2, void *a3 );
void WINAPI XBOXKRNL_HalReadWritePCISpace( LONG a0, LONG a1, LONG a2, void *a3, LONG a4, LONG a5 );
void WINAPI XBOXKRNL_HalRegisterShutdownNotification( void *a0, LONG a1 );
DEFINE_FASTCALL_WRAPPER(XBOXKRNL_HalRequestSoftwareInterrupt,4)
void FASTCALL XBOXKRNL_HalRequestSoftwareInterrupt( LONG a0 );
void WINAPI XBOXKRNL_HalReturnToFirmware( LONG a0 );
LONG WINAPI XBOXKRNL_HalWriteSMBusValue( LONG a0, LONG a1, LONG a2, LONG a3 );
LONG WINAPI XBOXKRNL_IoAllocateIrp( LONG a0 );
LONG WINAPI XBOXKRNL_IoBuildAsynchronousFsdRequest( LONG a0, void *a1, void *a2, LONG a3, void *a4, void *a5 );
LONG WINAPI XBOXKRNL_IoBuildDeviceIoControlRequest( LONG a0, void *a1, void *a2, LONG a3, void *a4, LONG a5, LONG a6, void *a7, void *a8 );
LONG WINAPI XBOXKRNL_IoBuildSynchronousFsdRequest( LONG a0, void *a1, void *a2, LONG a3, void *a4, void *a5, void *a6 );
LONG WINAPI XBOXKRNL_IoCheckShareAccess( LONG a0, LONG a1, void *a2, void *a3, LONG a4 );
LONG WINAPI XBOXKRNL_IoCreateDevice( void *a0, LONG a1, void *a2, LONG a3, LONG a4, void *a5 );
LONG WINAPI XBOXKRNL_IoCreateFile( void *a0, LONG a1, void *a2, void *a3, void *a4, LONG a5, LONG a6, LONG a7, LONG a8, LONG a9 );
LONG WINAPI XBOXKRNL_IoCreateSymbolicLink( void *a0, void *a1 );
void WINAPI XBOXKRNL_IoDeleteDevice( void *a0 );
LONG WINAPI XBOXKRNL_IoDeleteSymbolicLink( void *a0 );
void WINAPI XBOXKRNL_IoFreeIrp( void *a0 );
LONG WINAPI XBOXKRNL_IoInitializeIrp( void *a0, LONG a1, LONG a2 );
LONG WINAPI XBOXKRNL_IoInvalidDeviceRequest( void *a0, void *a1 );
LONG WINAPI XBOXKRNL_IoQueryFileInformation( void *a0, LONG a1, LONG a2, void *a3, void *a4 );
LONG WINAPI XBOXKRNL_IoQueryVolumeInformation( void *a0, LONG a1, LONG a2, void *a3, void *a4 );
void WINAPI XBOXKRNL_IoQueueThreadIrp( void *a0 );
void WINAPI XBOXKRNL_IoRemoveShareAccess( void *a0, void *a1 );
LONG WINAPI XBOXKRNL_IoSetIoCompletion( void *a0, void *a1, void *a2, LONG a3, LONG a4 );
void WINAPI XBOXKRNL_IoSetShareAccess( LONG a0, LONG a1, void *a2, void *a3 );
void WINAPI XBOXKRNL_IoStartNextPacket( void *a0 );
void WINAPI XBOXKRNL_IoStartNextPacketByKey( void *a0, LONG a1 );
void WINAPI XBOXKRNL_IoStartPacket( void *a0, void *a1, void *a2 );
LONG WINAPI XBOXKRNL_IoSynchronousDeviceIoControlRequest( LONG a0, void *a1, void *a2, LONG a3, void *a4, LONG a5, void *a6, LONG a7 );
LONG WINAPI XBOXKRNL_IoSynchronousFsdRequest( LONG a0, void *a1, void *a2, LONG a3, void *a4 );
DEFINE_FASTCALL_WRAPPER(XBOXKRNL_IofCallDriver,8)
LONG FASTCALL XBOXKRNL_IofCallDriver( void *a0, void *a1 );
DEFINE_FASTCALL_WRAPPER(XBOXKRNL_IofCompleteRequest,8)
void FASTCALL XBOXKRNL_IofCompleteRequest( void *a0, LONG a1 );
LONG WINAPI XBOXKRNL_IoDismountVolume( void *a0 );
LONG WINAPI XBOXKRNL_IoDismountVolumeByName( void *a0 );
LONG WINAPI XBOXKRNL_KeAlertResumeThread( void *a0, void *a1 );
LONG WINAPI XBOXKRNL_KeAlertThread( void *a0 );
LONG WINAPI XBOXKRNL_KeBoostPriorityThread( void *a0, LONG a1 );
LONG WINAPI XBOXKRNL_KeConnectInterrupt( void *a0 );
void WINAPI XBOXKRNL_KeDisconnectInterrupt( void *a0 );
void WINAPI XBOXKRNL_KeEnterCriticalRegion( LONG a0 );
LONG WINAPI XBOXKRNL_KeGetCurrentIrql( void );
LONG WINAPI XBOXKRNL_KeGetCurrentThread( void );
void WINAPI XBOXKRNL_KeInitializeApc( void *a0, void *a1, void *a2, void *a3, void *a4, LONG a5, void *a6 );
void WINAPI XBOXKRNL_KeInitializeDeviceQueue( void *a0 );
void WINAPI XBOXKRNL_KeInitializeDpc( void *a0, void *a1, void *a2 );
void WINAPI XBOXKRNL_KeInitializeInterrupt( void *a0, void *a1, void *a2, LONG a3, LONG a4, LONG a5, LONG a6 );
void WINAPI XBOXKRNL_KeInitializeQueue( void *a0, LONG a1 );
LONG WINAPI XBOXKRNL_KeInsertByKeyDeviceQueue( void *a0, void *a1, LONG a2 );
LONG WINAPI XBOXKRNL_KeInsertDeviceQueue( void *a0, void *a1 );
LONG WINAPI XBOXKRNL_KeInsertHeadQueue( void *a0, void *a1 );
LONG WINAPI XBOXKRNL_KeInsertQueue( void *a0, void *a1 );
LONG WINAPI XBOXKRNL_KeInsertQueueApc( void *a0, void *a1, void *a2, LONG a3 );
LONG WINAPI XBOXKRNL_KeInsertQueueDpc( void *a0, void *a1, void *a2 );
LONG WINAPI XBOXKRNL_KeIsExecutingDpc( void );
void WINAPI XBOXKRNL_KeLeaveCriticalRegion( LONG a0 );
LONG WINAPI XBOXKRNL_KeQueryBasePriorityThread( void *a0 );
LONG WINAPI XBOXKRNL_KeQueryInterruptTime( void );
LONG WINAPI XBOXKRNL_KeRaiseIrqlToDpcLevel( void );
LONG WINAPI XBOXKRNL_KeRaiseIrqlToSynchLevel( void );
LONG WINAPI XBOXKRNL_KeRemoveByKeyDeviceQueue( void *a0, LONG a1 );
LONG WINAPI XBOXKRNL_KeRemoveDeviceQueue( void *a0 );
LONG WINAPI XBOXKRNL_KeRemoveEntryDeviceQueue( void *a0, void *a1 );
LONG WINAPI XBOXKRNL_KeRemoveQueue( void *a0, LONG a1, void *a2 );
LONG WINAPI XBOXKRNL_KeRemoveQueueDpc( void *a0 );
LONG WINAPI XBOXKRNL_KeRestoreFloatingPointState( void *a0 );
LONG WINAPI XBOXKRNL_KeResumeThread( void *a0 );
LONG WINAPI XBOXKRNL_KeRundownQueue( void *a0 );
LONG WINAPI XBOXKRNL_KeSaveFloatingPointState( void *a0 );
LONG WINAPI XBOXKRNL_KeSetBasePriorityThread( void *a0, LONG a1 );
LONG WINAPI XBOXKRNL_KeSetDisableBoostThread( void *a0, LONG a1 );
LONG WINAPI XBOXKRNL_KeSetPriorityProcess( void *a0, LONG a1 );
void WINAPI XBOXKRNL_KeSetEventBoostPriority( void *a0, void *a1 );
LONG WINAPI XBOXKRNL_KeSetPriorityThread( void *a0, LONG a1 );
void WINAPI XBOXKRNL_KeStallExecutionProcessor( LONG a0 );
LONG WINAPI XBOXKRNL_KeSuspendThread( void *a0 );
LONG WINAPI XBOXKRNL_KeSynchronizeExecution( void *a0, void *a1, void *a2 );
LONG WINAPI XBOXKRNL_KeTestAlertThread( LONG a0 );
DEFINE_FASTCALL_WRAPPER(XBOXKRNL_KfRaiseIrql,4)
LONG FASTCALL XBOXKRNL_KfRaiseIrql( LONG a0 );
DEFINE_FASTCALL_WRAPPER(XBOXKRNL_KfLowerIrql,4)
void FASTCALL XBOXKRNL_KfLowerIrql( LONG a0 );
DEFINE_FASTCALL_WRAPPER(XBOXKRNL_KiUnlockDispatcherDatabase,4)
void FASTCALL XBOXKRNL_KiUnlockDispatcherDatabase( LONG a0 );
LONG WINAPI XBOXKRNL_MmAllocateContiguousMemory( LONG a0 );
LONG WINAPI XBOXKRNL_MmAllocateContiguousMemoryEx( LONG a0, LONG a1, LONG a2, LONG a3, LONG a4 );
LONG WINAPI XBOXKRNL_MmClaimGpuInstanceMemory( LONG a0, void *a1 );
LONG WINAPI XBOXKRNL_MmCreateKernelStack( LONG a0, LONG a1 );
void WINAPI XBOXKRNL_MmDeleteKernelStack( void *a0, void *a1 );
void WINAPI XBOXKRNL_MmFreeContiguousMemory( void *a0 );
LONG WINAPI XBOXKRNL_MmGetPhysicalAddress( void *a0 );
void WINAPI XBOXKRNL_MmLockUnlockBufferPages( void *a0, LONG a1, LONG a2 );
void WINAPI XBOXKRNL_MmLockUnlockPhysicalPage( LONG a0, LONG a1 );
LONG WINAPI XBOXKRNL_MmMapIoSpace( LONG a0, LONG a1, LONG a2 );
void WINAPI XBOXKRNL_MmPersistContiguousMemory( void *a0, LONG a1, LONG a2 );
LONG WINAPI XBOXKRNL_MmQueryAddressProtect( void *a0 );
LONG WINAPI XBOXKRNL_MmQueryStatistics( void *a0 );
void WINAPI XBOXKRNL_MmSetAddressProtect( void *a0, LONG a1, LONG a2 );
void WINAPI XBOXKRNL_MmUnmapIoSpace( void *a0, LONG a1 );
LONG WINAPI XBOXKRNL_NtQueueApcThread( void *a0, void *a1, void *a2, void *a3, void *a4 );
LONG WINAPI XBOXKRNL_NtQueryDirectoryObject( void *a0, void *a1, LONG a2, LONG a3, void *a4, void *a5 );
LONG WINAPI XBOXKRNL_NtSetSystemTime( void *a0, void *a1 );
void WINAPI XBOXKRNL_NtUserIoApcDispatcher( void *a0, void *a1, LONG a2 );
LONG WINAPI XBOXKRNL_ObCreateObject( void *a0, void *a1, LONG a2, void *a3 );
LONG WINAPI XBOXKRNL_ObInsertObject( void *a0, void *a1, LONG a2, void *a3 );
void WINAPI XBOXKRNL_ObMakeTemporaryObject( void *a0 );
LONG WINAPI XBOXKRNL_ObOpenObjectByName( void *a0, void *a1, void *a2, void *a3 );
LONG WINAPI XBOXKRNL_ObOpenObjectByPointer( void *a0, void *a1, void *a2 );
LONG WINAPI XBOXKRNL_ObReferenceObjectByHandle( void *a0, void *a1, void *a2 );
LONG WINAPI XBOXKRNL_ObReferenceObjectByName( void *a0, LONG a1, void *a2, void *a3, void *a4 );
LONG WINAPI XBOXKRNL_ObReferenceObjectByPointer( void *a0, void *a1 );
DEFINE_FASTCALL_WRAPPER(XBOXKRNL_ObfDereferenceObject,4)
void FASTCALL XBOXKRNL_ObfDereferenceObject( void *a0 );
DEFINE_FASTCALL_WRAPPER(XBOXKRNL_ObfReferenceObject,4)
void FASTCALL XBOXKRNL_ObfReferenceObject( void *a0 );
LONG WINAPI XBOXKRNL_PhyGetLinkState( LONG a0 );
LONG WINAPI XBOXKRNL_PhyInitialize( LONG a0, void *a1 );
LONG WINAPI XBOXKRNL_PsQueryStatistics( void *a0 );
LONG WINAPI XBOXKRNL_PsSetCreateThreadNotifyRoutine( void *a0 );
void WINAPI XBOXKRNL_RtlCaptureContext( void *a0 );
LONG WINAPI XBOXKRNL_RtlCaptureStackBackTrace( LONG a0, LONG a1, void *a2, void *a3 );
void WINAPI XBOXKRNL_RtlEnterCriticalSectionAndRegion( void *a0 );
LONG WINAPI XBOXKRNL_RtlExtendedMagicDivide( INT64 a0, INT64 a1, LONG a2 );
void WINAPI XBOXKRNL_RtlGetCallersAddress( void *a0, void *a1 );
void WINAPI XBOXKRNL_RtlLeaveCriticalSectionAndRegion( void *a0 );
LONG WINAPI XBOXKRNL_RtlLowerChar( LONG a0 );
void WINAPI XBOXKRNL_RtlMapGenericMask( void *a0, void *a1 );
LONG WINAPI XBOXKRNL_RtlMultiByteToUnicodeSize( void *a0, void *a1, LONG a2 );
void WINAPI XBOXKRNL_RtlRaiseException( void *a0 );
void WINAPI XBOXKRNL_RtlRaiseStatus( LONG a0 );
LONG WINAPI XBOXKRNL_RtlUnicodeToMultiByteSize( void *a0, void *a1, LONG a2 );
void WINAPI XBOXKRNL_RtlUnwind( void *a0, void *a1, void *a2, void *a3 );
LONG WINAPI XBOXKRNL_RtlUpcaseUnicodeToMultiByteN( void *a0, LONG a1, void *a2, void *a3, LONG a4 );
LONG WINAPI XBOXKRNL_RtlUpperChar( LONG a0 );
void WINAPI XBOXKRNL_RtlUpperString( void *a0, void *a1 );
LONG WINAPI XBOXKRNL_RtlWalkFrameChain( void *a0, LONG a1, LONG a2 );
LONG WINAPI XBOXKRNL_XeLoadSection( void *a0 );
LONG WINAPI XBOXKRNL_XeUnloadSection( void *a0 );
void WINAPI XBOXKRNL_READ_PORT_BUFFER_UCHAR( LONG a0, void *a1, LONG a2 );
void WINAPI XBOXKRNL_READ_PORT_BUFFER_USHORT( LONG a0, void *a1, LONG a2 );
void WINAPI XBOXKRNL_READ_PORT_BUFFER_ULONG( LONG a0, void *a1, LONG a2 );
void WINAPI XBOXKRNL_WRITE_PORT_BUFFER_UCHAR( LONG a0, void *a1, LONG a2 );
void WINAPI XBOXKRNL_WRITE_PORT_BUFFER_USHORT( LONG a0, void *a1, LONG a2 );
void WINAPI XBOXKRNL_WRITE_PORT_BUFFER_ULONG( LONG a0, void *a1, LONG a2 );
void WINAPI XBOXKRNL_XcSHAInit( void *a0 );
void WINAPI XBOXKRNL_XcSHAUpdate( void *a0, void *a1, LONG a2 );
void WINAPI XBOXKRNL_XcSHAFinal( void *a0, void *a1 );
void WINAPI XBOXKRNL_XcRC4Key( void *a0, LONG a1, void *a2 );
void WINAPI XBOXKRNL_XcRC4Crypt( void *a0, LONG a1, void *a2 );
void WINAPI XBOXKRNL_XcHMAC( void *a0, LONG a1, void *a2, LONG a3, void *a4, LONG a5, void *a6 );
LONG WINAPI XBOXKRNL_XcPKEncPublic( void *a0, void *a1, void *a2 );
LONG WINAPI XBOXKRNL_XcPKDecPrivate( void *a0, void *a1, void *a2 );
LONG WINAPI XBOXKRNL_XcPKGetKeyLen( void *a0 );
LONG WINAPI XBOXKRNL_XcVerifyPKCS1Signature( void *a0, void *a1, void *a2 );
LONG WINAPI XBOXKRNL_XcModExp( void *a0, void *a1, void *a2, void *a3, LONG a4 );
void WINAPI XBOXKRNL_XcDESKeyParity( void *a0, LONG a1 );
void WINAPI XBOXKRNL_XcKeyTable( LONG a0, void *a1, void *a2 );
void WINAPI XBOXKRNL_XcBlockCrypt( LONG a0, void *a1, void *a2, void *a3, LONG a4 );
void WINAPI XBOXKRNL_XcBlockCryptCBC( LONG a0, LONG a1, void *a2, void *a3, void *a4, LONG a5, void *a6 );
LONG WINAPI XBOXKRNL_XcCryptService( LONG a0, void *a1 );
void WINAPI XBOXKRNL_XcUpdateCrypto( void *a0, void *a1 );
void WINAPI XBOXKRNL_RtlRip( void *a0, void *a1, void *a2 );
LONG WINAPI XBOXKRNL_HalIsResetOrShutdownPending( void );
LONG WINAPI XBOXKRNL_IoMarkIrpMustComplete( void *a0 );
LONG WINAPI XBOXKRNL_HalInitiateShutdown( void );
LONG WINAPI XBOXKRNL_RtlSnprintf( void *a0, LONG a1, void *a2, LONG a3 );
LONG WINAPI XBOXKRNL_RtlSprintf( void *a0, void *a1, LONG a2 );
LONG WINAPI XBOXKRNL_RtlVsnprintf( void *a0, LONG a1, void *a2, LONG a3 );
LONG WINAPI XBOXKRNL_RtlVsprintf( void *a0, void *a1, LONG a2 );
void WINAPI XBOXKRNL_HalEnableSecureTrayEject( void );
LONG WINAPI XBOXKRNL_HalWriteSMCScratchRegister( LONG a0 );
LONG WINAPI XBOXKRNL_UnknownAPI367( void );
LONG WINAPI XBOXKRNL_UnknownAPI368( void );
LONG WINAPI XBOXKRNL_UnknownAPI369( void );
LONG WINAPI XBOXKRNL_XProfpControl( LONG a0, LONG a1 );
LONG WINAPI XBOXKRNL_XProfpGetData( void );
LONG WINAPI XBOXKRNL_IrtClientInitFast( void );
LONG WINAPI XBOXKRNL_IrtSweep( void );
LONG WINAPI XBOXKRNL_MmDbgAllocateMemory( LONG a0, LONG a1 );
LONG WINAPI XBOXKRNL_MmDbgFreeMemory( void *a0, LONG a1 );
LONG WINAPI XBOXKRNL_MmDbgQueryAvailablePages( void );
void WINAPI XBOXKRNL_MmDbgReleaseAddress( void *a0, void *a1 );
LONG WINAPI XBOXKRNL_MmDbgWriteCheck( void *a0, void *a1 );

LONG WINAPI XBOXKRNL_AvGetSavedDataAddress( void )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 1u, "AvGetSavedDataAddress", 0u );
    return 0;
}

void WINAPI XBOXKRNL_AvSendTVEncoderOption( void *a0, LONG a1, LONG a2, void *a3 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 2u, "AvSendTVEncoderOption", 4u );
}

LONG WINAPI XBOXKRNL_AvSetDisplayMode( void *a0, LONG a1, LONG a2, LONG a3, LONG a4, LONG a5 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 3u, "AvSetDisplayMode", 6u );
    return 0;
}

void WINAPI XBOXKRNL_AvSetSavedDataAddress( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 4u, "AvSetSavedDataAddress", 1u );
}

void WINAPI XBOXKRNL_DbgBreakPointWithStatus( LONG a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 6u, "DbgBreakPointWithStatus", 1u );
}

LONG WINAPI XBOXKRNL_DbgLoadImageSymbols( void *a0, void *a1, LONG a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 7u, "DbgLoadImageSymbols", 3u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_HalReadSMCTrayState( void *a0, void *a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 9u, "HalReadSMCTrayState", 2u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_DbgPrompt( void *a0, void *a1, LONG a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 10u, "DbgPrompt", 3u );
    return 0;
}

void WINAPI XBOXKRNL_DbgUnLoadImageSymbols( void *a0, void *a1, LONG a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 11u, "DbgUnLoadImageSymbols", 3u );
}

void WINAPI XBOXKRNL_ExAcquireReadWriteLockExclusive( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 12u, "ExAcquireReadWriteLockExclusive", 1u );
}

void WINAPI XBOXKRNL_ExAcquireReadWriteLockShared( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 13u, "ExAcquireReadWriteLockShared", 1u );
}

void WINAPI XBOXKRNL_ExInitializeReadWriteLock( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 18u, "ExInitializeReadWriteLock", 1u );
}

LONG WINAPI XBOXKRNL_ExInterlockedAddLargeInteger( void *a0, INT64 a1, void *a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 19u, "ExInterlockedAddLargeInteger", 3u );
    return 0;
}

void FASTCALL XBOXKRNL_ExInterlockedAddLargeStatistic( void *a0, LONG a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 20u, "ExInterlockedAddLargeStatistic", 2u );
}

LONG FASTCALL XBOXKRNL_ExInterlockedCompareExchange64( void *a0, void *a1, void *a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 21u, "ExInterlockedCompareExchange64", 3u );
    return 0;
}

LONG WINAPI XBOXKRNL_ExQueryPoolBlockSize( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 23u, "ExQueryPoolBlockSize", 1u );
    return 0;
}

LONG WINAPI XBOXKRNL_ExQueryNonVolatileSetting( LONG a0, void *a1, void *a2, LONG a3, void *a4 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 24u, "ExQueryNonVolatileSetting", 5u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_ExReadWriteRefurbInfo( void *a0, LONG a1, LONG a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 25u, "ExReadWriteRefurbInfo", 3u );
    return STATUS_NOT_IMPLEMENTED;
}

void WINAPI XBOXKRNL_ExRaiseException( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 26u, "ExRaiseException", 1u );
}

void WINAPI XBOXKRNL_ExRaiseStatus( LONG a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 27u, "ExRaiseStatus", 1u );
}

void WINAPI XBOXKRNL_ExReleaseReadWriteLock( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 28u, "ExReleaseReadWriteLock", 1u );
}

LONG WINAPI XBOXKRNL_ExSaveNonVolatileSetting( LONG a0, LONG a1, void *a2, LONG a3 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 29u, "ExSaveNonVolatileSetting", 4u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG FASTCALL XBOXKRNL_ExfInterlockedInsertHeadList( void *a0, void *a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 32u, "ExfInterlockedInsertHeadList", 2u );
    return 0;
}

LONG FASTCALL XBOXKRNL_ExfInterlockedInsertTailList( void *a0, void *a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 33u, "ExfInterlockedInsertTailList", 2u );
    return 0;
}

LONG FASTCALL XBOXKRNL_ExfInterlockedRemoveHeadList( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 34u, "ExfInterlockedRemoveHeadList", 1u );
    return 0;
}

LONG WINAPI XBOXKRNL_FscGetCacheSize( void )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 35u, "FscGetCacheSize", 0u );
    return 0;
}

void WINAPI XBOXKRNL_FscInvalidateIdleBlocks( void )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 36u, "FscInvalidateIdleBlocks", 0u );
}

LONG WINAPI XBOXKRNL_FscSetCacheSize( LONG a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 37u, "FscSetCacheSize", 1u );
    return STATUS_NOT_IMPLEMENTED;
}

void FASTCALL XBOXKRNL_HalClearSoftwareInterrupt( LONG a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 38u, "HalClearSoftwareInterrupt", 1u );
}

void WINAPI XBOXKRNL_HalDisableSystemInterrupt( LONG a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 39u, "HalDisableSystemInterrupt", 1u );
}

void WINAPI XBOXKRNL_HalEnableSystemInterrupt( LONG a0, LONG a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 43u, "HalEnableSystemInterrupt", 2u );
}

LONG WINAPI XBOXKRNL_HalGetInterruptVector( LONG a0, void *a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 44u, "HalGetInterruptVector", 2u );
    return 0;
}

LONG WINAPI XBOXKRNL_HalReadSMBusValue( LONG a0, LONG a1, LONG a2, void *a3 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 45u, "HalReadSMBusValue", 4u );
    return STATUS_NOT_IMPLEMENTED;
}

void WINAPI XBOXKRNL_HalReadWritePCISpace( LONG a0, LONG a1, LONG a2, void *a3, LONG a4, LONG a5 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 46u, "HalReadWritePCISpace", 6u );
}

void WINAPI XBOXKRNL_HalRegisterShutdownNotification( void *a0, LONG a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 47u, "HalRegisterShutdownNotification", 2u );
}

void FASTCALL XBOXKRNL_HalRequestSoftwareInterrupt( LONG a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 48u, "HalRequestSoftwareInterrupt", 1u );
}

void WINAPI XBOXKRNL_HalReturnToFirmware( LONG a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 49u, "HalReturnToFirmware", 1u );
}

LONG WINAPI XBOXKRNL_HalWriteSMBusValue( LONG a0, LONG a1, LONG a2, LONG a3 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 50u, "HalWriteSMBusValue", 4u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_IoAllocateIrp( LONG a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 59u, "IoAllocateIrp", 1u );
    return 0;
}

LONG WINAPI XBOXKRNL_IoBuildAsynchronousFsdRequest( LONG a0, void *a1, void *a2, LONG a3, void *a4, void *a5 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 60u, "IoBuildAsynchronousFsdRequest", 6u );
    return 0;
}

LONG WINAPI XBOXKRNL_IoBuildDeviceIoControlRequest( LONG a0, void *a1, void *a2, LONG a3, void *a4, LONG a5, LONG a6, void *a7, void *a8 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 61u, "IoBuildDeviceIoControlRequest", 9u );
    return 0;
}

LONG WINAPI XBOXKRNL_IoBuildSynchronousFsdRequest( LONG a0, void *a1, void *a2, LONG a3, void *a4, void *a5, void *a6 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 62u, "IoBuildSynchronousFsdRequest", 7u );
    return 0;
}

LONG WINAPI XBOXKRNL_IoCheckShareAccess( LONG a0, LONG a1, void *a2, void *a3, LONG a4 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 63u, "IoCheckShareAccess", 5u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_IoCreateDevice( void *a0, LONG a1, void *a2, LONG a3, LONG a4, void *a5 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 65u, "IoCreateDevice", 6u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_IoCreateFile( void *a0, LONG a1, void *a2, void *a3, void *a4, LONG a5, LONG a6, LONG a7, LONG a8, LONG a9 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 66u, "IoCreateFile", 10u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_IoCreateSymbolicLink( void *a0, void *a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 67u, "IoCreateSymbolicLink", 2u );
    return STATUS_NOT_IMPLEMENTED;
}

void WINAPI XBOXKRNL_IoDeleteDevice( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 68u, "IoDeleteDevice", 1u );
}

LONG WINAPI XBOXKRNL_IoDeleteSymbolicLink( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 69u, "IoDeleteSymbolicLink", 1u );
    return STATUS_NOT_IMPLEMENTED;
}

void WINAPI XBOXKRNL_IoFreeIrp( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 72u, "IoFreeIrp", 1u );
}

LONG WINAPI XBOXKRNL_IoInitializeIrp( void *a0, LONG a1, LONG a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 73u, "IoInitializeIrp", 3u );
    return 0;
}

LONG WINAPI XBOXKRNL_IoInvalidDeviceRequest( void *a0, void *a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 74u, "IoInvalidDeviceRequest", 2u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_IoQueryFileInformation( void *a0, LONG a1, LONG a2, void *a3, void *a4 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 75u, "IoQueryFileInformation", 5u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_IoQueryVolumeInformation( void *a0, LONG a1, LONG a2, void *a3, void *a4 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 76u, "IoQueryVolumeInformation", 5u );
    return STATUS_NOT_IMPLEMENTED;
}

void WINAPI XBOXKRNL_IoQueueThreadIrp( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 77u, "IoQueueThreadIrp", 1u );
}

void WINAPI XBOXKRNL_IoRemoveShareAccess( void *a0, void *a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 78u, "IoRemoveShareAccess", 2u );
}

LONG WINAPI XBOXKRNL_IoSetIoCompletion( void *a0, void *a1, void *a2, LONG a3, LONG a4 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 79u, "IoSetIoCompletion", 5u );
    return STATUS_NOT_IMPLEMENTED;
}

void WINAPI XBOXKRNL_IoSetShareAccess( LONG a0, LONG a1, void *a2, void *a3 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 80u, "IoSetShareAccess", 4u );
}

void WINAPI XBOXKRNL_IoStartNextPacket( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 81u, "IoStartNextPacket", 1u );
}

void WINAPI XBOXKRNL_IoStartNextPacketByKey( void *a0, LONG a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 82u, "IoStartNextPacketByKey", 2u );
}

void WINAPI XBOXKRNL_IoStartPacket( void *a0, void *a1, void *a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 83u, "IoStartPacket", 3u );
}

LONG WINAPI XBOXKRNL_IoSynchronousDeviceIoControlRequest( LONG a0, void *a1, void *a2, LONG a3, void *a4, LONG a5, void *a6, LONG a7 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 84u, "IoSynchronousDeviceIoControlRequest", 8u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_IoSynchronousFsdRequest( LONG a0, void *a1, void *a2, LONG a3, void *a4 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 85u, "IoSynchronousFsdRequest", 5u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG FASTCALL XBOXKRNL_IofCallDriver( void *a0, void *a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 86u, "IofCallDriver", 2u );
    return STATUS_NOT_IMPLEMENTED;
}

void FASTCALL XBOXKRNL_IofCompleteRequest( void *a0, LONG a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 87u, "IofCompleteRequest", 2u );
}

LONG WINAPI XBOXKRNL_IoDismountVolume( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 90u, "IoDismountVolume", 1u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_IoDismountVolumeByName( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 91u, "IoDismountVolumeByName", 1u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_KeAlertResumeThread( void *a0, void *a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 92u, "KeAlertResumeThread", 2u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_KeAlertThread( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 93u, "KeAlertThread", 1u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_KeBoostPriorityThread( void *a0, LONG a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 94u, "KeBoostPriorityThread", 2u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_KeConnectInterrupt( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 98u, "KeConnectInterrupt", 1u );
    return 0;
}

void WINAPI XBOXKRNL_KeDisconnectInterrupt( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 100u, "KeDisconnectInterrupt", 1u );
}

void WINAPI XBOXKRNL_KeEnterCriticalRegion( LONG a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 101u, "KeEnterCriticalRegion", 1u );
}

LONG WINAPI XBOXKRNL_KeGetCurrentIrql( void )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 103u, "KeGetCurrentIrql", 0u );
    return 0;
}

LONG WINAPI XBOXKRNL_KeGetCurrentThread( void )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 104u, "KeGetCurrentThread", 0u );
    return 0;
}

void WINAPI XBOXKRNL_KeInitializeApc( void *a0, void *a1, void *a2, void *a3, void *a4, LONG a5, void *a6 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 105u, "KeInitializeApc", 7u );
}

void WINAPI XBOXKRNL_KeInitializeDeviceQueue( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 106u, "KeInitializeDeviceQueue", 1u );
}

void WINAPI XBOXKRNL_KeInitializeDpc( void *a0, void *a1, void *a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 107u, "KeInitializeDpc", 3u );
}

void WINAPI XBOXKRNL_KeInitializeInterrupt( void *a0, void *a1, void *a2, LONG a3, LONG a4, LONG a5, LONG a6 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 109u, "KeInitializeInterrupt", 7u );
}

void WINAPI XBOXKRNL_KeInitializeQueue( void *a0, LONG a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 111u, "KeInitializeQueue", 2u );
}

LONG WINAPI XBOXKRNL_KeInsertByKeyDeviceQueue( void *a0, void *a1, LONG a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 114u, "KeInsertByKeyDeviceQueue", 3u );
    return 0;
}

LONG WINAPI XBOXKRNL_KeInsertDeviceQueue( void *a0, void *a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 115u, "KeInsertDeviceQueue", 2u );
    return 0;
}

LONG WINAPI XBOXKRNL_KeInsertHeadQueue( void *a0, void *a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 116u, "KeInsertHeadQueue", 2u );
    return 0;
}

LONG WINAPI XBOXKRNL_KeInsertQueue( void *a0, void *a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 117u, "KeInsertQueue", 2u );
    return 0;
}

LONG WINAPI XBOXKRNL_KeInsertQueueApc( void *a0, void *a1, void *a2, LONG a3 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 118u, "KeInsertQueueApc", 4u );
    return 0;
}

LONG WINAPI XBOXKRNL_KeInsertQueueDpc( void *a0, void *a1, void *a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 119u, "KeInsertQueueDpc", 3u );
    return 0;
}

LONG WINAPI XBOXKRNL_KeIsExecutingDpc( void )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 121u, "KeIsExecutingDpc", 0u );
    return 0;
}

void WINAPI XBOXKRNL_KeLeaveCriticalRegion( LONG a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 122u, "KeLeaveCriticalRegion", 1u );
}

LONG WINAPI XBOXKRNL_KeQueryBasePriorityThread( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 124u, "KeQueryBasePriorityThread", 1u );
    return 0;
}

LONG WINAPI XBOXKRNL_KeQueryInterruptTime( void )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 125u, "KeQueryInterruptTime", 0u );
    return 0;
}

LONG WINAPI XBOXKRNL_KeRaiseIrqlToDpcLevel( void )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 129u, "KeRaiseIrqlToDpcLevel", 0u );
    return 0;
}

LONG WINAPI XBOXKRNL_KeRaiseIrqlToSynchLevel( void )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 130u, "KeRaiseIrqlToSynchLevel", 0u );
    return 0;
}

LONG WINAPI XBOXKRNL_KeRemoveByKeyDeviceQueue( void *a0, LONG a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 133u, "KeRemoveByKeyDeviceQueue", 2u );
    return 0;
}

LONG WINAPI XBOXKRNL_KeRemoveDeviceQueue( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 134u, "KeRemoveDeviceQueue", 1u );
    return 0;
}

LONG WINAPI XBOXKRNL_KeRemoveEntryDeviceQueue( void *a0, void *a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 135u, "KeRemoveEntryDeviceQueue", 2u );
    return 0;
}

LONG WINAPI XBOXKRNL_KeRemoveQueue( void *a0, LONG a1, void *a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 136u, "KeRemoveQueue", 3u );
    return 0;
}

LONG WINAPI XBOXKRNL_KeRemoveQueueDpc( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 137u, "KeRemoveQueueDpc", 1u );
    return 0;
}

LONG WINAPI XBOXKRNL_KeRestoreFloatingPointState( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 139u, "KeRestoreFloatingPointState", 1u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_KeResumeThread( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 140u, "KeResumeThread", 1u );
    return 0;
}

LONG WINAPI XBOXKRNL_KeRundownQueue( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 141u, "KeRundownQueue", 1u );
    return 0;
}

LONG WINAPI XBOXKRNL_KeSaveFloatingPointState( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 142u, "KeSaveFloatingPointState", 1u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_KeSetBasePriorityThread( void *a0, LONG a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 143u, "KeSetBasePriorityThread", 2u );
    return 0;
}

LONG WINAPI XBOXKRNL_KeSetDisableBoostThread( void *a0, LONG a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 144u, "KeSetDisableBoostThread", 2u );
    return 0;
}

LONG WINAPI XBOXKRNL_KeSetPriorityProcess( void *a0, LONG a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 147u, "KeSetPriorityProcess", 2u );
    return 0;
}

void WINAPI XBOXKRNL_KeSetEventBoostPriority( void *a0, void *a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 146u, "KeSetEventBoostPriority", 2u );
}

LONG WINAPI XBOXKRNL_KeSetPriorityThread( void *a0, LONG a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 148u, "KeSetPriorityThread", 2u );
    return 0;
}

void WINAPI XBOXKRNL_KeStallExecutionProcessor( LONG a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 151u, "KeStallExecutionProcessor", 1u );
}

LONG WINAPI XBOXKRNL_KeSuspendThread( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 152u, "KeSuspendThread", 1u );
    return 0;
}

LONG WINAPI XBOXKRNL_KeSynchronizeExecution( void *a0, void *a1, void *a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 153u, "KeSynchronizeExecution", 3u );
    return 0;
}

LONG WINAPI XBOXKRNL_KeTestAlertThread( LONG a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 155u, "KeTestAlertThread", 1u );
    return 0;
}

LONG FASTCALL XBOXKRNL_KfRaiseIrql( LONG a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 160u, "KfRaiseIrql", 1u );
    return 0;
}

void FASTCALL XBOXKRNL_KfLowerIrql( LONG a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 161u, "KfLowerIrql", 1u );
}

void FASTCALL XBOXKRNL_KiUnlockDispatcherDatabase( LONG a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 163u, "KiUnlockDispatcherDatabase", 1u );
}

LONG WINAPI XBOXKRNL_MmAllocateContiguousMemory( LONG a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 165u, "MmAllocateContiguousMemory", 1u );
    return 0;
}

LONG WINAPI XBOXKRNL_MmAllocateContiguousMemoryEx( LONG a0, LONG a1, LONG a2, LONG a3, LONG a4 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 166u, "MmAllocateContiguousMemoryEx", 5u );
    return 0;
}

LONG WINAPI XBOXKRNL_MmClaimGpuInstanceMemory( LONG a0, void *a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 168u, "MmClaimGpuInstanceMemory", 2u );
    return 0;
}

LONG WINAPI XBOXKRNL_MmCreateKernelStack( LONG a0, LONG a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 169u, "MmCreateKernelStack", 2u );
    return 0;
}

void WINAPI XBOXKRNL_MmDeleteKernelStack( void *a0, void *a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 170u, "MmDeleteKernelStack", 2u );
}

void WINAPI XBOXKRNL_MmFreeContiguousMemory( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 171u, "MmFreeContiguousMemory", 1u );
}

LONG WINAPI XBOXKRNL_MmGetPhysicalAddress( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 173u, "MmGetPhysicalAddress", 1u );
    return 0;
}

void WINAPI XBOXKRNL_MmLockUnlockBufferPages( void *a0, LONG a1, LONG a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 175u, "MmLockUnlockBufferPages", 3u );
}

void WINAPI XBOXKRNL_MmLockUnlockPhysicalPage( LONG a0, LONG a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 176u, "MmLockUnlockPhysicalPage", 2u );
}

LONG WINAPI XBOXKRNL_MmMapIoSpace( LONG a0, LONG a1, LONG a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 177u, "MmMapIoSpace", 3u );
    return 0;
}

void WINAPI XBOXKRNL_MmPersistContiguousMemory( void *a0, LONG a1, LONG a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 178u, "MmPersistContiguousMemory", 3u );
}

LONG WINAPI XBOXKRNL_MmQueryAddressProtect( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 179u, "MmQueryAddressProtect", 1u );
    return 0;
}

LONG WINAPI XBOXKRNL_MmQueryStatistics( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 181u, "MmQueryStatistics", 1u );
    return STATUS_NOT_IMPLEMENTED;
}

void WINAPI XBOXKRNL_MmSetAddressProtect( void *a0, LONG a1, LONG a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 182u, "MmSetAddressProtect", 3u );
}

void WINAPI XBOXKRNL_MmUnmapIoSpace( void *a0, LONG a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 183u, "MmUnmapIoSpace", 2u );
}

LONG WINAPI XBOXKRNL_NtQueueApcThread( void *a0, void *a1, void *a2, void *a3, void *a4 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 206u, "NtQueueApcThread", 5u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_NtQueryDirectoryObject( void *a0, void *a1, LONG a2, LONG a3, void *a4, void *a5 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 208u, "NtQueryDirectoryObject", 6u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_NtSetSystemTime( void *a0, void *a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 228u, "NtSetSystemTime", 2u );
    return STATUS_NOT_IMPLEMENTED;
}

void WINAPI XBOXKRNL_NtUserIoApcDispatcher( void *a0, void *a1, LONG a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 232u, "NtUserIoApcDispatcher", 3u );
}

LONG WINAPI XBOXKRNL_ObCreateObject( void *a0, void *a1, LONG a2, void *a3 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 239u, "ObCreateObject", 4u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_ObInsertObject( void *a0, void *a1, LONG a2, void *a3 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 241u, "ObInsertObject", 4u );
    return STATUS_NOT_IMPLEMENTED;
}

void WINAPI XBOXKRNL_ObMakeTemporaryObject( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 242u, "ObMakeTemporaryObject", 1u );
}

LONG WINAPI XBOXKRNL_ObOpenObjectByName( void *a0, void *a1, void *a2, void *a3 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 243u, "ObOpenObjectByName", 4u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_ObOpenObjectByPointer( void *a0, void *a1, void *a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 244u, "ObOpenObjectByPointer", 3u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_ObReferenceObjectByHandle( void *a0, void *a1, void *a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 246u, "ObReferenceObjectByHandle", 3u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_ObReferenceObjectByName( void *a0, LONG a1, void *a2, void *a3, void *a4 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 247u, "ObReferenceObjectByName", 5u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_ObReferenceObjectByPointer( void *a0, void *a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 248u, "ObReferenceObjectByPointer", 2u );
    return STATUS_NOT_IMPLEMENTED;
}

void FASTCALL XBOXKRNL_ObfDereferenceObject( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 250u, "ObfDereferenceObject", 1u );
}

void FASTCALL XBOXKRNL_ObfReferenceObject( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 251u, "ObfReferenceObject", 1u );
}

LONG WINAPI XBOXKRNL_PhyGetLinkState( LONG a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 252u, "PhyGetLinkState", 1u );
    return 0;
}

LONG WINAPI XBOXKRNL_PhyInitialize( LONG a0, void *a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 253u, "PhyInitialize", 2u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_PsQueryStatistics( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 256u, "PsQueryStatistics", 1u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_PsSetCreateThreadNotifyRoutine( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 257u, "PsSetCreateThreadNotifyRoutine", 1u );
    return STATUS_NOT_IMPLEMENTED;
}

void WINAPI XBOXKRNL_RtlCaptureContext( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 265u, "RtlCaptureContext", 1u );
}

LONG WINAPI XBOXKRNL_RtlCaptureStackBackTrace( LONG a0, LONG a1, void *a2, void *a3 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 266u, "RtlCaptureStackBackTrace", 4u );
    return 0;
}

void WINAPI XBOXKRNL_RtlEnterCriticalSectionAndRegion( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 278u, "RtlEnterCriticalSectionAndRegion", 1u );
}

LONG WINAPI XBOXKRNL_RtlExtendedMagicDivide( INT64 a0, INT64 a1, LONG a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 283u, "RtlExtendedMagicDivide", 3u );
    return 0;
}

void WINAPI XBOXKRNL_RtlGetCallersAddress( void *a0, void *a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 288u, "RtlGetCallersAddress", 2u );
}

void WINAPI XBOXKRNL_RtlLeaveCriticalSectionAndRegion( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 295u, "RtlLeaveCriticalSectionAndRegion", 1u );
}

LONG WINAPI XBOXKRNL_RtlLowerChar( LONG a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 296u, "RtlLowerChar", 1u );
    return 0;
}

void WINAPI XBOXKRNL_RtlMapGenericMask( void *a0, void *a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 297u, "RtlMapGenericMask", 2u );
}

LONG WINAPI XBOXKRNL_RtlMultiByteToUnicodeSize( void *a0, void *a1, LONG a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 300u, "RtlMultiByteToUnicodeSize", 3u );
    return STATUS_NOT_IMPLEMENTED;
}

void WINAPI XBOXKRNL_RtlRaiseException( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 302u, "RtlRaiseException", 1u );
}

void WINAPI XBOXKRNL_RtlRaiseStatus( LONG a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 303u, "RtlRaiseStatus", 1u );
}

LONG WINAPI XBOXKRNL_RtlUnicodeToMultiByteSize( void *a0, void *a1, LONG a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 311u, "RtlUnicodeToMultiByteSize", 3u );
    return STATUS_NOT_IMPLEMENTED;
}

void WINAPI XBOXKRNL_RtlUnwind( void *a0, void *a1, void *a2, void *a3 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 312u, "RtlUnwind", 4u );
}

LONG WINAPI XBOXKRNL_RtlUpcaseUnicodeToMultiByteN( void *a0, LONG a1, void *a2, void *a3, LONG a4 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 315u, "RtlUpcaseUnicodeToMultiByteN", 5u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_RtlUpperChar( LONG a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 316u, "RtlUpperChar", 1u );
    return 0;
}

void WINAPI XBOXKRNL_RtlUpperString( void *a0, void *a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 317u, "RtlUpperString", 2u );
}

LONG WINAPI XBOXKRNL_RtlWalkFrameChain( void *a0, LONG a1, LONG a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 319u, "RtlWalkFrameChain", 3u );
    return 0;
}

LONG WINAPI XBOXKRNL_XeLoadSection( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 327u, "XeLoadSection", 1u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_XeUnloadSection( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 328u, "XeUnloadSection", 1u );
    return STATUS_NOT_IMPLEMENTED;
}

void WINAPI XBOXKRNL_READ_PORT_BUFFER_UCHAR( LONG a0, void *a1, LONG a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 329u, "READ_PORT_BUFFER_UCHAR", 3u );
}

void WINAPI XBOXKRNL_READ_PORT_BUFFER_USHORT( LONG a0, void *a1, LONG a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 330u, "READ_PORT_BUFFER_USHORT", 3u );
}

void WINAPI XBOXKRNL_READ_PORT_BUFFER_ULONG( LONG a0, void *a1, LONG a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 331u, "READ_PORT_BUFFER_ULONG", 3u );
}

void WINAPI XBOXKRNL_WRITE_PORT_BUFFER_UCHAR( LONG a0, void *a1, LONG a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 332u, "WRITE_PORT_BUFFER_UCHAR", 3u );
}

void WINAPI XBOXKRNL_WRITE_PORT_BUFFER_USHORT( LONG a0, void *a1, LONG a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 333u, "WRITE_PORT_BUFFER_USHORT", 3u );
}

void WINAPI XBOXKRNL_WRITE_PORT_BUFFER_ULONG( LONG a0, void *a1, LONG a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 334u, "WRITE_PORT_BUFFER_ULONG", 3u );
}

void WINAPI XBOXKRNL_XcSHAInit( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 335u, "XcSHAInit", 1u );
}

void WINAPI XBOXKRNL_XcSHAUpdate( void *a0, void *a1, LONG a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 336u, "XcSHAUpdate", 3u );
}

void WINAPI XBOXKRNL_XcSHAFinal( void *a0, void *a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 337u, "XcSHAFinal", 2u );
}

void WINAPI XBOXKRNL_XcRC4Key( void *a0, LONG a1, void *a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 338u, "XcRC4Key", 3u );
}

void WINAPI XBOXKRNL_XcRC4Crypt( void *a0, LONG a1, void *a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 339u, "XcRC4Crypt", 3u );
}

void WINAPI XBOXKRNL_XcHMAC( void *a0, LONG a1, void *a2, LONG a3, void *a4, LONG a5, void *a6 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 340u, "XcHMAC", 7u );
}

LONG WINAPI XBOXKRNL_XcPKEncPublic( void *a0, void *a1, void *a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 341u, "XcPKEncPublic", 3u );
    return 0;
}

LONG WINAPI XBOXKRNL_XcPKDecPrivate( void *a0, void *a1, void *a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 342u, "XcPKDecPrivate", 3u );
    return 0;
}

LONG WINAPI XBOXKRNL_XcPKGetKeyLen( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 343u, "XcPKGetKeyLen", 1u );
    return 0;
}

LONG WINAPI XBOXKRNL_XcVerifyPKCS1Signature( void *a0, void *a1, void *a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 344u, "XcVerifyPKCS1Signature", 3u );
    return 0;
}

LONG WINAPI XBOXKRNL_XcModExp( void *a0, void *a1, void *a2, void *a3, LONG a4 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 345u, "XcModExp", 5u );
    return 0;
}

void WINAPI XBOXKRNL_XcDESKeyParity( void *a0, LONG a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 346u, "XcDESKeyParity", 2u );
}

void WINAPI XBOXKRNL_XcKeyTable( LONG a0, void *a1, void *a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 347u, "XcKeyTable", 3u );
}

void WINAPI XBOXKRNL_XcBlockCrypt( LONG a0, void *a1, void *a2, void *a3, LONG a4 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 348u, "XcBlockCrypt", 5u );
}

void WINAPI XBOXKRNL_XcBlockCryptCBC( LONG a0, LONG a1, void *a2, void *a3, void *a4, LONG a5, void *a6 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 349u, "XcBlockCryptCBC", 7u );
}

LONG WINAPI XBOXKRNL_XcCryptService( LONG a0, void *a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 350u, "XcCryptService", 2u );
    return 0;
}

void WINAPI XBOXKRNL_XcUpdateCrypto( void *a0, void *a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 351u, "XcUpdateCrypto", 2u );
}

void WINAPI XBOXKRNL_RtlRip( void *a0, void *a1, void *a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 352u, "RtlRip", 3u );
}

LONG WINAPI XBOXKRNL_HalIsResetOrShutdownPending( void )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 358u, "HalIsResetOrShutdownPending", 0u );
    return 0;
}

LONG WINAPI XBOXKRNL_IoMarkIrpMustComplete( void *a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 359u, "IoMarkIrpMustComplete", 1u );
    return 0;
}

LONG WINAPI XBOXKRNL_HalInitiateShutdown( void )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 360u, "HalInitiateShutdown", 0u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_RtlSnprintf( void *a0, LONG a1, void *a2, LONG a3 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 361u, "RtlSnprintf", 4u );
    return 0;
}

LONG WINAPI XBOXKRNL_RtlSprintf( void *a0, void *a1, LONG a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 362u, "RtlSprintf", 3u );
    return 0;
}

LONG WINAPI XBOXKRNL_RtlVsnprintf( void *a0, LONG a1, void *a2, LONG a3 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 363u, "RtlVsnprintf", 4u );
    return 0;
}

LONG WINAPI XBOXKRNL_RtlVsprintf( void *a0, void *a1, LONG a2 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 364u, "RtlVsprintf", 3u );
    return 0;
}

void WINAPI XBOXKRNL_HalEnableSecureTrayEject( void )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 365u, "HalEnableSecureTrayEject", 0u );
}

LONG WINAPI XBOXKRNL_HalWriteSMCScratchRegister( LONG a0 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 366u, "HalWriteSMCScratchRegister", 1u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_UnknownAPI367( void )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 367u, "UnknownAPI367", 0u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_UnknownAPI368( void )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 368u, "UnknownAPI368", 0u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_UnknownAPI369( void )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 369u, "UnknownAPI369", 0u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_XProfpControl( LONG a0, LONG a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 370u, "XProfpControl", 2u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_XProfpGetData( void )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 371u, "XProfpGetData", 0u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_IrtClientInitFast( void )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 372u, "IrtClientInitFast", 0u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_IrtSweep( void )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 373u, "IrtSweep", 0u );
    return STATUS_NOT_IMPLEMENTED;
}

LONG WINAPI XBOXKRNL_MmDbgAllocateMemory( LONG a0, LONG a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 374u, "MmDbgAllocateMemory", 2u );
    return 0;
}

LONG WINAPI XBOXKRNL_MmDbgFreeMemory( void *a0, LONG a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 375u, "MmDbgFreeMemory", 2u );
    return 0;
}

LONG WINAPI XBOXKRNL_MmDbgQueryAvailablePages( void )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 376u, "MmDbgQueryAvailablePages", 0u );
    return 0;
}

void WINAPI XBOXKRNL_MmDbgReleaseAddress( void *a0, void *a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 377u, "MmDbgReleaseAddress", 2u );
}

LONG WINAPI XBOXKRNL_MmDbgWriteCheck( void *a0, void *a1 )
{
    FIXME( "ordinal %u (%s): stub, %u arg(s) ignored\n", 378u, "MmDbgWriteCheck", 2u );
    return 0;
}
