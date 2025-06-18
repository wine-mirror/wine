@ stdcall EtwRegisterClassicProvider(ptr long ptr ptr ptr)
@ stdcall EtwUnregister(int64)
@ stdcall -arch=!i386 ExAcquireFastMutex(ptr)
@ stdcall -fastcall ExAcquireFastMutexUnsafe(ptr)
@ stub ExAcquireRundownProtection
@ stub ExAcquireRundownProtectionEx
@ stub ExInitializeRundownProtection
@ stub ExInterlockedAddLargeStatistic
@ stub ExInterlockedCompareExchange64
@ stdcall -fastcall -arch=i386 ExInterlockedFlushSList(ptr) NTOSKRNL_ExInterlockedFlushSList
@ stdcall -fastcall -arch=i386 ExInterlockedPopEntrySList(ptr ptr) NTOSKRNL_ExInterlockedPopEntrySList
@ stdcall -fastcall -arch=i386 ExInterlockedPushEntrySList (ptr ptr ptr) NTOSKRNL_ExInterlockedPushEntrySList
@ stub ExReInitializeRundownProtection
@ stdcall -arch=!i386 ExReleaseFastMutex(ptr)
@ stdcall -fastcall ExReleaseFastMutexUnsafe(ptr)
@ stdcall -fastcall ExReleaseResourceLite(ptr)
@ stub ExReleaseRundownProtection
@ stub ExReleaseRundownProtectionEx
@ stub ExRundownCompleted
@ stub ExWaitForRundownProtectionRelease
@ stub ExfAcquirePushLockExclusive
@ stub ExfAcquirePushLockShared
@ stub ExfInterlockedAddUlong
@ stub ExfInterlockedCompareExchange64
@ stub ExfInterlockedInsertHeadList
@ stub ExfInterlockedInsertTailList
@ stub ExfInterlockedPopEntryList
@ stub ExfInterlockedPushEntryList
@ stdcall -fastcall -arch=i386 ExfInterlockedRemoveHeadList(ptr ptr)
@ stub ExfReleasePushLock
@ stub Exfi386InterlockedDecrementLong
@ stub Exfi386InterlockedExchangeUlong
@ stub Exfi386InterlockedIncrementLong
@ stdcall -arch=win64 ExpInterlockedFlushSList(ptr) RtlInterlockedFlushSList
@ stdcall -arch=win64 ExpInterlockedPopEntrySList(ptr) RtlInterlockedPopEntrySList
@ stdcall -arch=win64 ExpInterlockedPushEntrySList(ptr ptr) RtlInterlockedPushEntrySList
@ stub HalExamineMBR
@ stdcall -arch=!i386 InitializeSListHead(ptr) RtlInitializeSListHead
@ stdcall -fastcall InterlockedCompareExchange(ptr long long) NTOSKRNL_InterlockedCompareExchange
@ stdcall -fastcall InterlockedDecrement(ptr) NTOSKRNL_InterlockedDecrement
@ stdcall -fastcall InterlockedExchange(ptr long) NTOSKRNL_InterlockedExchange
@ stdcall -fastcall InterlockedExchangeAdd(ptr long) NTOSKRNL_InterlockedExchangeAdd
@ stdcall -fastcall InterlockedIncrement(ptr) NTOSKRNL_InterlockedIncrement
@ stdcall -fastcall -arch=win32 InterlockedPopEntrySList(ptr) NTOSKRNL_InterlockedPopEntrySList
@ stdcall -fastcall -arch=win32 InterlockedPushEntrySList(ptr ptr) NTOSKRNL_InterlockedPushEntrySList
@ stdcall -arch=win64 ExQueryDepthSList(ptr) RtlQueryDepthSList
@ stub IoAssignDriveLetters
@ stub IoReadPartitionTable
@ stub IoSetPartitionInformation
@ stub IoWritePartitionTable
@ stdcall -fastcall IofCallDriver(ptr ptr)
@ stdcall -fastcall IofCompleteRequest(ptr long)
@ stdcall -arch=!i386 KeAcquireInStackQueuedSpinLock(ptr ptr)
@ stdcall -fastcall KeAcquireInStackQueuedSpinLockAtDpcLevel(ptr ptr)
@ stdcall KeEnterGuardedRegion()
@ stdcall KeExpandKernelStackAndCallout(ptr ptr long)
@ stdcall KeExpandKernelStackAndCalloutEx(ptr ptr long long ptr)
@ stdcall KeLeaveGuardedRegion()
@ stdcall -arch=!i386 KeReleaseInStackQueuedSpinLock(ptr)
@ stdcall -fastcall KeReleaseInStackQueuedSpinLockFromDpcLevel(ptr)
@ stub KeSetTimeUpdateNotifyRoutine
@ stub KefAcquireSpinLockAtDpcLevel
@ stub KefReleaseSpinLockFromDpcLevel
@ stdcall KeGenericCallDpc(ptr ptr)
@ stdcall KeSignalCallDpcDone(ptr)
@ stdcall KeSignalCallDpcSynchronize(ptr)
@ stub KiAcquireSpinLock
@ stub KiReleaseSpinLock
@ stdcall -fastcall ObfDereferenceObject(ptr)
@ stdcall -fastcall ObfReferenceObject(ptr)
@ stub RtlPrefetchMemoryNonTemporal
@ stdcall -fastcall -arch=i386 -norelay RtlUlongByteSwap(long)
@ stdcall -fastcall -arch=i386 -norelay RtlUlonglongByteSwap(int64)
@ stdcall -fastcall -arch=i386 -norelay RtlUshortByteSwap(long)
@ stub WmiGetClock
@ stub Kei386EoiHelper
@ stub Kii386SpinOnSpinLock
@ stub CcCanIWrite
@ stub CcCopyRead
@ stub CcCopyWrite
@ stub CcDeferWrite
@ stub CcFastCopyRead
@ stub CcFastCopyWrite
@ stub CcFastMdlReadWait
@ stub CcFastReadNotPossible
@ stub CcFastReadWait
@ stub CcFlushCache
@ stub CcGetDirtyPages
@ stub CcGetFileObjectFromBcb
@ stub CcGetFileObjectFromSectionPtrs
@ stub CcGetFlushedValidData
@ stub CcGetLsnForFileObject
@ stub CcInitializeCacheMap
@ stub CcIsThereDirtyData
@ stub CcMapData
@ stub CcMdlRead
@ stub CcMdlReadComplete
@ stub CcMdlWriteAbort
@ stub CcMdlWriteComplete
@ stub CcPinMappedData
@ stub CcPinRead
@ stub CcPrepareMdlWrite
@ stub CcPreparePinWrite
@ stub CcPurgeCacheSection
@ stub CcRemapBcb
@ stub CcRepinBcb
@ stub CcScheduleReadAhead
@ stub CcSetAdditionalCacheAttributes
@ stub CcSetBcbOwnerPointer
@ stub CcSetDirtyPageThreshold
@ stub CcSetDirtyPinnedData
@ stub CcSetFileSizes
@ stub CcSetLogHandleForFile
@ stub CcSetReadAheadGranularity
@ stub CcUninitializeCacheMap
@ stub CcUnpinData
@ stub CcUnpinDataForThread
@ stub CcUnpinRepinnedBcb
@ stub CcWaitForCurrentLazyWriterActivity
@ stub CcZeroData
@ stdcall CmRegisterCallback(ptr ptr ptr)
@ stdcall CmUnRegisterCallback(int64)
@ stdcall DbgBreakPoint()
@ stub DbgBreakPointWithStatus
@ stub DbgLoadImageSymbols
@ varargs DbgPrint(str)
@ varargs DbgPrintEx(long long str)
@ stub DbgPrintReturnControlC
@ stub DbgPrompt
@ stdcall DbgQueryDebugFilterState(long long)
@ stub DbgSetDebugFilterState
@ stdcall ExAcquireResourceExclusiveLite(ptr long)
@ stdcall ExAcquireResourceSharedLite(ptr long)
@ stdcall ExAcquireSharedStarveExclusive(ptr long)
@ stdcall ExAcquireSharedWaitForExclusive(ptr long)
@ stub ExAllocateFromPagedLookasideList
@ stdcall ExAllocatePool(long long)
@ stdcall ExAllocatePool2(int64 long long)
@ stdcall ExAllocatePoolWithQuota(long long)
@ stdcall ExAllocatePoolWithQuotaTag(long long long)
@ stdcall ExAllocatePoolWithTag(long long long)
@ stub ExAllocatePoolWithTagPriority
@ stub ExConvertExclusiveToSharedLite
@ stdcall ExCreateCallback(ptr ptr long long)
@ stdcall ExDeleteNPagedLookasideList(ptr)
@ stdcall ExDeletePagedLookasideList(ptr)
@ stdcall ExDeleteResourceLite(ptr)
@ stub ExDesktopObjectType
@ stub ExDisableResourceBoostLite
@ stub ExEnumHandleTable
@ extern ExEventObjectType
@ stub ExExtendZone
@ stdcall -fastcall ExfUnblockPushLock(ptr ptr)
@ stdcall ExFreePool(ptr)
@ stdcall ExFreePoolWithTag(ptr long)
@ stub ExFreeToPagedLookasideList
@ stub ExGetCurrentProcessorCounts
@ stub ExGetCurrentProcessorCpuUsage
@ stdcall ExGetExclusiveWaiterCount(ptr)
@ stdcall ExGetPreviousMode()
@ stdcall ExGetSharedWaiterCount(ptr)
@ stdcall ExInitializeNPagedLookasideList(ptr ptr ptr long long long long)
@ stdcall ExInitializePagedLookasideList(ptr ptr ptr long long long long)
@ stdcall ExInitializeResourceLite(ptr)
@ stdcall ExInitializeZone(ptr long ptr long)
@ stub ExInterlockedAddLargeInteger
@ stub ExInterlockedAddUlong
@ stub ExInterlockedDecrementLong
@ stub ExInterlockedExchangeUlong
@ stub ExInterlockedExtendZone
@ stub ExInterlockedIncrementLong
@ stub ExInterlockedInsertHeadList
@ stub ExInterlockedInsertTailList
@ stub ExInterlockedPopEntryList
@ stub ExInterlockedPushEntryList
@ stdcall ExInterlockedRemoveHeadList(ptr ptr)
@ stub ExIsProcessorFeaturePresent
@ stdcall ExIsResourceAcquiredExclusiveLite(ptr)
@ stdcall ExIsResourceAcquiredSharedLite(ptr)
@ stdcall ExLocalTimeToSystemTime(ptr ptr) RtlLocalTimeToSystemTime
@ stdcall ExNotifyCallback(ptr ptr ptr)
@ stub ExQueryPoolBlockSize
@ stub ExQueueWorkItem
@ stub ExRaiseAccessViolation
@ stub ExRaiseDatatypeMisalignment
@ stub ExRaiseException
@ stub ExRaiseHardError
@ stub ExRaiseStatus
@ stdcall ExRegisterCallback(ptr ptr ptr)
@ stub ExReinitializeResourceLite
@ stdcall ExReleaseResourceForThreadLite(ptr long)
@ extern ExSemaphoreObjectType
@ stub ExSetResourceOwnerPointer
@ stdcall ExSetTimerResolution(long long)
@ stub ExSystemExceptionFilter
@ stdcall ExSystemTimeToLocalTime(ptr ptr) RtlSystemTimeToLocalTime
@ stdcall ExUnregisterCallback(ptr)
@ stdcall ExUuidCreate(ptr)
@ stub ExVerifySuite
@ stub ExWindowStationObjectType
@ stub Exi386InterlockedDecrementLong
@ stub Exi386InterlockedExchangeUlong
@ stub Exi386InterlockedIncrementLong
@ stub FsRtlAcquireFileExclusive
@ stub FsRtlAddLargeMcbEntry
@ stub FsRtlAddMcbEntry
@ stub FsRtlAddToTunnelCache
@ stub FsRtlAllocateFileLock
@ stub FsRtlAllocatePool
@ stub FsRtlAllocatePoolWithQuota
@ stub FsRtlAllocatePoolWithQuotaTag
@ stub FsRtlAllocatePoolWithTag
@ stub FsRtlAllocateResource
@ stub FsRtlAreNamesEqual
@ stub FsRtlBalanceReads
@ stub FsRtlCheckLockForReadAccess
@ stub FsRtlCheckLockForWriteAccess
@ stub FsRtlCheckOplock
@ stub FsRtlCopyRead
@ stub FsRtlCopyWrite
@ stub FsRtlCurrentBatchOplock
@ stub FsRtlDeleteKeyFromTunnelCache
@ stub FsRtlDeleteTunnelCache
@ stub FsRtlDeregisterUncProvider
@ stub FsRtlDissectDbcs
@ stub FsRtlDissectName
@ stub FsRtlDoesDbcsContainWildCards
@ stub FsRtlDoesNameContainWildCards
@ stub FsRtlFastCheckLockForRead
@ stub FsRtlFastCheckLockForWrite
@ stub FsRtlFastUnlockAll
@ stub FsRtlFastUnlockAllByKey
@ stub FsRtlFastUnlockSingle
@ stub FsRtlFindInTunnelCache
@ stub FsRtlFreeFileLock
@ stub FsRtlGetFileSize
@ stub FsRtlGetNextFileLock
@ stub FsRtlGetNextLargeMcbEntry
@ stub FsRtlGetNextMcbEntry
@ stub FsRtlIncrementCcFastReadNoWait
@ stub FsRtlIncrementCcFastReadNotPossible
@ stub FsRtlIncrementCcFastReadResourceMiss
@ stub FsRtlIncrementCcFastReadWait
@ stub FsRtlInitializeFileLock
@ stub FsRtlInitializeLargeMcb
@ stub FsRtlInitializeMcb
@ stub FsRtlInitializeOplock
@ stub FsRtlInitializeTunnelCache
@ stub FsRtlInsertPerFileObjectContext
@ stub FsRtlInsertPerStreamContext
@ stub FsRtlIsDbcsInExpression
@ stub FsRtlIsFatDbcsLegal
@ stub FsRtlIsHpfsDbcsLegal
@ stdcall FsRtlIsNameInExpression(ptr ptr long ptr)
@ stub FsRtlIsNtstatusExpected
@ stub FsRtlIsPagingFile
@ stub FsRtlIsTotalDeviceFailure
@ stub FsRtlLegalAnsiCharacterArray
@ stub FsRtlLookupLargeMcbEntry
@ stub FsRtlLookupLastLargeMcbEntry
@ stub FsRtlLookupLastLargeMcbEntryAndIndex
@ stub FsRtlLookupLastMcbEntry
@ stub FsRtlLookupMcbEntry
@ stub FsRtlLookupPerFileObjectContext
@ stub FsRtlLookupPerStreamContextInternal
@ stub FsRtlMdlRead
@ stub FsRtlMdlReadComplete
@ stub FsRtlMdlReadCompleteDev
@ stub FsRtlMdlReadDev
@ stub FsRtlMdlWriteComplete
@ stub FsRtlMdlWriteCompleteDev
@ stub FsRtlNormalizeNtstatus
@ stub FsRtlNotifyChangeDirectory
@ stub FsRtlNotifyCleanup
@ stub FsRtlNotifyFilterChangeDirectory
@ stub FsRtlNotifyFilterReportChange
@ stub FsRtlNotifyFullChangeDirectory
@ stub FsRtlNotifyFullReportChange
@ stub FsRtlNotifyInitializeSync
@ stub FsRtlNotifyReportChange
@ stub FsRtlNotifyUninitializeSync
@ stub FsRtlNotifyVolumeEvent
@ stub FsRtlNumberOfRunsInLargeMcb
@ stub FsRtlNumberOfRunsInMcb
@ stub FsRtlOplockFsctrl
@ stub FsRtlOplockIsFastIoPossible
@ stub FsRtlPostPagingFileStackOverflow
@ stub FsRtlPostStackOverflow
@ stub FsRtlPrepareMdlWrite
@ stub FsRtlPrepareMdlWriteDev
@ stub FsRtlPrivateLock
@ stub FsRtlProcessFileLock
@ stdcall FsRtlRegisterFileSystemFilterCallbacks(ptr ptr)
@ stdcall FsRtlRegisterUncProvider(ptr ptr long)
@ stub FsRtlReleaseFile
@ stub FsRtlRemoveLargeMcbEntry
@ stub FsRtlRemoveMcbEntry
@ stub FsRtlRemovePerFileObjectContext
@ stub FsRtlRemovePerStreamContext
@ stub FsRtlResetLargeMcb
@ stub FsRtlSplitLargeMcb
@ stub FsRtlSyncVolumes
@ stub FsRtlTeardownPerStreamContexts
@ stub FsRtlTruncateLargeMcb
@ stub FsRtlTruncateMcb
@ stub FsRtlUninitializeFileLock
@ stub FsRtlUninitializeLargeMcb
@ stub FsRtlUninitializeMcb
@ stub FsRtlUninitializeOplock
@ stub HalDispatchTable
@ stub HalPrivateDispatchTable
@ stub HeadlessDispatch
@ stub InbvAcquireDisplayOwnership
@ stub InbvCheckDisplayOwnership
@ stub InbvDisplayString
@ stub InbvEnableBootDriver
@ stub InbvEnableDisplayString
@ stub InbvInstallDisplayStringFilter
@ stub InbvIsBootDriverInstalled
@ stub InbvNotifyDisplayOwnershipLost
@ stub InbvResetDisplay
@ stub InbvSetScrollRegion
@ stub InbvSetTextColor
@ stub InbvSolidColorFill
@ extern InitSafeBootMode
@ stdcall IoAcquireCancelSpinLock(ptr)
@ stdcall IoAcquireRemoveLockEx(ptr ptr str long long)
@ stub IoAcquireVpbSpinLock
@ stub IoAdapterObjectType
@ stub IoAllocateAdapterChannel
@ stub IoAllocateController
@ stdcall IoAllocateDriverObjectExtension(ptr ptr long ptr)
@ stdcall IoAllocateErrorLogEntry(ptr long)
@ stdcall IoAllocateIrp(long long)
@ stdcall IoAllocateMdl(ptr long long long ptr)
@ stdcall IoAllocateWorkItem(ptr)
@ stub IoAssignResources
@ stdcall IoAttachDevice(ptr ptr ptr)
@ stub IoAttachDeviceByPointer
@ stdcall IoAttachDeviceToDeviceStack(ptr ptr)
@ stub IoAttachDeviceToDeviceStackSafe
@ stdcall IoBuildAsynchronousFsdRequest(long ptr ptr long ptr ptr)
@ stdcall IoBuildDeviceIoControlRequest(long ptr ptr long ptr long long ptr ptr)
@ stub IoBuildPartialMdl
@ stdcall IoBuildSynchronousFsdRequest(long ptr ptr long ptr ptr ptr)
@ stdcall IoCallDriver(ptr ptr)
@ stub IoCancelFileOpen
@ stdcall IoCancelIrp(ptr)
@ stub IoCheckDesiredAccess
@ stub IoCheckEaBufferValidity
@ stub IoCheckFunctionAccess
@ stub IoCheckQuerySetFileInformation
@ stub IoCheckQuerySetVolumeInformation
@ stub IoCheckQuotaBufferValidity
@ stub IoCheckShareAccess
@ stdcall IoCompleteRequest(ptr long)
@ stub IoConnectInterrupt
@ stub IoCreateController
@ stdcall IoCreateDevice(ptr long ptr long long long ptr)
@ stdcall IoCreateDeviceSecure(ptr long ptr long long long ptr ptr ptr)
@ stub IoCreateDisk
@ stdcall IoCreateDriver(ptr ptr)
@ stdcall IoCreateFileEx(ptr long ptr ptr ptr long long long long ptr long long ptr long ptr)
@ stdcall IoCreateFile(ptr long ptr ptr ptr long long long long ptr long long ptr long)
@ stub IoCreateFileSpecifyDeviceObjectHint
@ stdcall IoCreateNotificationEvent(ptr ptr)
@ stub IoCreateStreamFileObject
@ stub IoCreateStreamFileObjectEx
@ stub IoCreateStreamFileObjectLite
@ stdcall IoCreateSymbolicLink(ptr ptr)
@ stdcall IoCreateSynchronizationEvent(ptr ptr)
@ stdcall IoCreateUnprotectedSymbolicLink(ptr ptr)
@ stdcall IoCsqInitialize(ptr ptr ptr ptr ptr ptr ptr)
@ stub IoCsqInsertIrp
@ stub IoCsqRemoveIrp
@ stub IoCsqRemoveNextIrp
@ stub IoDeleteController
@ stdcall IoDeleteDevice(ptr)
@ stdcall IoDeleteDriver(ptr)
@ stdcall IoDeleteSymbolicLink(ptr)
@ stdcall IoDetachDevice(ptr)
@ stub IoDeviceHandlerObjectSize
@ stub IoDeviceHandlerObjectType
@ extern IoDeviceObjectType
@ stub IoDisconnectInterrupt
@ extern IoDriverObjectType
@ stub IoEnqueueIrp
@ stub IoEnumerateDeviceObjectList
@ stub IoFastQueryNetworkAttributes
@ extern IoFileObjectType
@ stub IoForwardAndCatchIrp
@ stub IoForwardIrpSynchronously
@ stub IoFreeController
@ stub IoFreeErrorLogEntry
@ stdcall IoFreeIrp(ptr)
@ stdcall IoFreeMdl(ptr)
@ stdcall IoFreeWorkItem(ptr)
@ stdcall IoGetAttachedDevice(ptr)
@ stdcall IoGetAttachedDeviceReference(ptr)
@ stub IoGetBaseFileSystemDeviceObject
@ stub IoGetBootDiskInformation
@ stdcall IoGetConfigurationInformation()
@ stdcall IoGetCurrentProcess()
@ stub IoGetDeviceAttachmentBaseRef
@ stub IoGetDeviceInterfaceAlias
@ stdcall IoGetDeviceInterfacePropertyData(ptr ptr long long long ptr ptr ptr)
@ stdcall IoGetDeviceInterfaces(ptr ptr long ptr)
@ stdcall IoGetDeviceObjectPointer(ptr long ptr ptr)
@ stdcall IoGetDeviceProperty(ptr long long ptr ptr)
@ stdcall IoGetDevicePropertyData(ptr ptr long long long ptr ptr ptr)
@ stub IoGetDeviceToVerify
@ stub IoGetDiskDeviceObject
@ stub IoGetDmaAdapter
@ stdcall IoGetDriverObjectExtension(ptr ptr)
@ stub IoGetFileObjectGenericMapping
@ stub IoGetInitialStack
@ stub IoGetLowerDeviceObject
@ stdcall IoGetRelatedDeviceObject(ptr)
@ stdcall IoGetRequestorProcess(ptr)
@ stub IoGetRequestorProcessId
@ stub IoGetRequestorSessionId
@ stdcall IoGetStackLimits(ptr ptr)
@ stub IoGetTopLevelIrp
@ stdcall IoInitializeIrp(ptr long long)
@ stdcall IoInitializeRemoveLockEx(ptr long long long long)
@ stdcall IoInitializeTimer(ptr ptr ptr)
@ stdcall IoInvalidateDeviceRelations(ptr long)
@ stub IoInvalidateDeviceState
@ stdcall -arch=win64 IoIs32bitProcess(ptr)
@ stub IoIsFileOriginRemote
@ stub IoIsOperationSynchronous
@ stub IoIsSystemThread
@ stub IoIsValidNameGraftingBuffer
@ stdcall IoIsWdmVersionAvailable(long long)
@ stub IoMakeAssociatedIrp
@ stub IoOpenDeviceInterfaceRegistryKey
@ stdcall IoOpenDeviceRegistryKey(ptr long long ptr)
@ stub IoPageRead
@ stub IoPnPDeliverServicePowerNotification
@ stdcall IoQueryDeviceDescription(ptr ptr ptr ptr ptr ptr ptr ptr)
@ stub IoQueryFileDosDeviceName
@ stub IoQueryFileInformation
@ stub IoQueryVolumeInformation
@ stub IoQueueThreadIrp
@ stdcall IoQueueWorkItem(ptr ptr long ptr)
@ stub IoRaiseHardError
@ stub IoRaiseInformationalHardError
@ stub IoReadDiskSignature
@ stub IoReadOperationCount
@ stub IoReadPartitionTableEx
@ stub IoReadTransferCount
@ stdcall IoRegisterBootDriverReinitialization(ptr ptr ptr)
@ stdcall IoRegisterDeviceInterface(ptr ptr ptr ptr)
@ stdcall IoRegisterDriverReinitialization(ptr ptr ptr)
@ stdcall IoRegisterFileSystem(ptr)
@ stub IoRegisterFsRegistrationChange
@ stub IoRegisterLastChanceShutdownNotification
@ stdcall IoRegisterPlugPlayNotification(long long ptr ptr ptr ptr ptr)
@ stdcall IoRegisterShutdownNotification(ptr)
@ stdcall IoReleaseCancelSpinLock(long)
@ stdcall IoReleaseRemoveLockAndWaitEx(ptr ptr long)
@ stdcall IoReleaseRemoveLockEx(ptr ptr long)
@ stub IoReleaseVpbSpinLock
@ stub IoRemoveShareAccess
@ stub IoReportDetectedDevice
@ stub IoReportHalResourceUsage
@ stdcall IoReportResourceForDetection(ptr ptr long ptr ptr long ptr)
@ stdcall IoReportResourceUsage(ptr ptr ptr long ptr ptr long long ptr)
@ stdcall IoReportTargetDeviceChange(ptr ptr)
@ stdcall IoReportTargetDeviceChangeAsynchronous(ptr ptr ptr ptr)
@ stub IoRequestDeviceEject
@ stdcall IoReuseIrp(ptr long)
@ stub IoSetCompletionRoutineEx
@ stdcall IoSetDeviceInterfacePropertyData(ptr ptr long long long long ptr)
@ stdcall IoSetDeviceInterfaceState(ptr long)
@ stdcall IoSetDevicePropertyData(ptr ptr long long long long ptr)
@ stub IoSetDeviceToVerify
@ stub IoSetFileOrigin
@ stub IoSetHardErrorOrVerifyDevice
@ stub IoSetInformation
@ stub IoSetIoCompletion
@ stub IoSetPartitionInformationEx
@ stub IoSetShareAccess
@ stub IoSetStartIoAttributes
@ stub IoSetSystemPartition
@ stdcall IoSetThreadHardErrorMode(long)
@ stub IoSetTopLevelIrp
@ stdcall IoStartNextPacket(ptr long)
@ stub IoStartNextPacketByKey
@ stub IoStartPacket
@ stdcall IoStartTimer(ptr)
@ stub IoStatisticsLock
@ stdcall IoStopTimer(ptr)
@ stub IoSynchronousInvalidateDeviceRelations
@ stub IoSynchronousPageWrite
@ stub IoThreadToProcess
@ stdcall IoUnregisterFileSystem(ptr)
@ stub IoUnregisterFsRegistrationChange
@ stdcall IoUnregisterPlugPlayNotification(ptr)
@ stdcall IoUnregisterShutdownNotification(ptr)
@ stub IoUpdateShareAccess
@ stub IoValidateDeviceIoControlAccess
@ stub IoVerifyPartitionTable
@ stub IoVerifyVolume
@ stub IoVolumeDeviceToDosName
@ stub IoWMIAllocateInstanceIds
@ stub IoWMIDeviceObjectToInstanceName
@ stub IoWMIExecuteMethod
@ stub IoWMIHandleToInstanceName
@ stdcall IoWMIOpenBlock(ptr long ptr)
@ stub IoWMIQueryAllData
@ stub IoWMIQueryAllDataMultiple
@ stub IoWMIQuerySingleInstance
@ stub IoWMIQuerySingleInstanceMultiple
@ stdcall IoWMIRegistrationControl(ptr long)
@ stub IoWMISetNotificationCallback
@ stub IoWMISetSingleInstance
@ stub IoWMISetSingleItem
@ stub IoWMISuggestInstanceName
@ stub IoWMIWriteEvent
@ stub IoWriteErrorLogEntry
@ stub IoWriteOperationCount
@ stub IoWritePartitionTableEx
@ stub IoWriteTransferCount
@ extern KdDebuggerEnabled
@ stub KdDebuggerNotPresent
@ stdcall KdDisableDebugger()
@ stdcall KdEnableDebugger()
@ stub KdEnteredDebugger
@ stub KdPollBreakIn
@ stub KdPowerTransition
@ stdcall KdRefreshDebuggerNotPresent()
@ stub Ke386CallBios
@ stdcall Ke386IoSetAccessProcess(ptr long)
@ stdcall Ke386QueryIoAccessMap(long ptr)
@ stdcall Ke386SetIoAccessMap(long ptr)
@ stub KeAcquireInterruptSpinLock
@ stdcall KeAcquireSpinLockAtDpcLevel(ptr)
@ stdcall -arch=!i386 KeAcquireSpinLockRaiseToDpc(ptr)
@ stub KeAddSystemServiceTable
@ stdcall KeAlertThread(ptr long)
@ stdcall KeAreAllApcsDisabled()
@ stdcall KeAreApcsDisabled()
@ stub KeAttachProcess
@ stdcall KeBugCheck(long)
@ stdcall KeBugCheckEx(long long long long long)
@ stdcall KeCancelTimer(ptr)
@ stub KeCapturePersistentThreadState
@ stdcall KeClearEvent(ptr)
@ stub KeConnectInterrupt
@ stub KeDcacheFlushCount
@ stdcall KeDelayExecutionThread(long long ptr)
@ stub KeDeregisterBugCheckCallback
@ stub KeDeregisterBugCheckReasonCallback
@ stub KeDetachProcess
@ stub KeDisconnectInterrupt
@ stdcall KeEnterCriticalRegion()
@ stub KeEnterKernelDebugger
@ stub KeFindConfigurationEntry
@ stub KeFindConfigurationNextEntry
@ stub KeFlushEntireTb
@ stdcall KeFlushQueuedDpcs()
@ stdcall KeGetCurrentProcessorNumber() NtGetCurrentProcessorNumber
@ stdcall KeGetCurrentProcessorNumberEx(ptr)
@ stdcall KeGetCurrentThread()
@ stub KeGetPreviousMode
@ stub KeGetRecommendedSharedDataAlignment
@ stub KeI386AbiosCall
@ stub KeI386AllocateGdtSelectors
@ stub KeI386Call16BitCStyleFunction
@ stub KeI386Call16BitFunction
@ stub KeI386FlatToGdtSelector
@ stub KeI386GetLid
@ stub KeI386MachineType
@ stub KeI386ReleaseGdtSelectors
@ stub KeI386ReleaseLid
@ stub KeI386SetGdtSelector
@ stub KeIcacheFlushCount
@ stdcall KeInitializeApc(ptr ptr long ptr ptr ptr long ptr)
@ stdcall KeInitializeDeviceQueue(ptr)
@ stdcall KeInitializeDpc(ptr ptr ptr)
@ stdcall KeInitializeEvent(ptr long long)
@ stub KeInitializeInterrupt
@ stub KeInitializeMutant
@ stdcall KeInitializeMutex(ptr long)
@ stdcall KeInitializeGuardedMutex(ptr)
@ stub KeInitializeQueue
@ stdcall KeInitializeSemaphore(ptr long long)
@ stdcall KeInitializeSpinLock(ptr) NTOSKRNL_KeInitializeSpinLock
@ stdcall KeInitializeTimer(ptr)
@ stdcall KeInitializeTimerEx(ptr long)
@ stub KeInsertByKeyDeviceQueue
@ stdcall KeInsertDeviceQueue(ptr ptr)
@ stub KeInsertHeadQueue
@ stdcall KeInsertQueue(ptr ptr)
@ stub KeInsertQueueApc
@ stdcall KeInsertQueueDpc(ptr ptr ptr)
@ stub KeIsAttachedProcess
@ stub KeIsExecutingDpc
@ stdcall KeLeaveCriticalRegion()
@ stub KeLoaderBlock
@ stdcall -arch=x86_64 KeLowerIrql(long)
@ stub KeNumberProcessors
@ stub KeProfileInterrupt
@ stub KeProfileInterruptWithSource
@ stub KePulseEvent
@ stdcall KeQueryActiveProcessors()
@ stdcall KeQueryActiveProcessorCountEx(long)
@ stdcall KeQueryActiveProcessorCount(ptr)
@ stdcall KeQueryActiveGroupCount() GetActiveProcessorGroupCount
@ stdcall KeQueryGroupAffinity(long)
@ stdcall KeQueryInterruptTime()
@ stdcall KeQueryPriorityThread(ptr)
@ stub KeQueryRuntimeThread
@ stdcall KeQuerySystemTime(ptr)
@ stdcall KeQueryTickCount(ptr)
@ stdcall KeQueryTimeIncrement()
@ stdcall KeQueryMaximumGroupCount() GetMaximumProcessorGroupCount
@ stdcall KeQueryMaximumProcessorCountEx(long)
@ stdcall KeQueryMaximumProcessorCount()
@ stub KeRaiseUserException
@ stdcall KeReadStateEvent(ptr)
@ stub KeReadStateMutant
@ stub KeReadStateMutex
@ stub KeReadStateQueue
@ stub KeReadStateSemaphore
@ stub KeReadStateTimer
@ stub KeRegisterBugCheckCallback
@ stub KeRegisterBugCheckReasonCallback
@ stub KeReleaseInterruptSpinLock
@ stub KeReleaseMutant
@ stdcall KeReleaseMutex(ptr long)
@ stdcall KeReleaseSemaphore(ptr long long long)
@ stdcall -arch=!i386 KeReleaseSpinLock(ptr long)
@ stdcall KeReleaseSpinLockFromDpcLevel(ptr)
@ stub KeRemoveByKeyDeviceQueue
@ stub KeRemoveByKeyDeviceQueueIfBusy
@ stdcall KeRemoveDeviceQueue(ptr)
@ stub KeRemoveEntryDeviceQueue
@ stub KeRemoveQueue
@ stub KeRemoveQueueDpc
@ stub KeRemoveSystemServiceTable
@ stdcall KeResetEvent(ptr)
@ stub KeRestoreFloatingPointState
@ stdcall KeRevertToUserAffinityThread()
@ stdcall KeRevertToUserAffinityThreadEx(long)
@ stub KeRundownQueue
@ stub KeSaveFloatingPointState
@ stub KeSaveStateForHibernate
@ extern KeServiceDescriptorTable
@ stub KeSetAffinityThread
@ stub KeSetBasePriorityThread
@ stub KeSetDmaIoCoherency
@ stdcall KeSetEvent(ptr long long)
@ stub KeSetEventBoostPriority
@ stub KeSetIdealProcessorThread
@ stdcall KeSetImportanceDpc(ptr long)
@ stub KeSetKernelStackSwapEnable
@ stdcall KeSetPriorityThread(ptr long)
@ stub KeSetProfileIrql
@ stdcall KeSetSystemAffinityThread(long)
@ stdcall KeSetSystemAffinityThreadEx(long)
@ stdcall KeSetTargetProcessorDpc(ptr long)
@ stdcall KeSetTargetProcessorDpcEx(ptr ptr)
@ stub KeSetTimeIncrement
@ stdcall KeSetTimer(ptr int64 ptr)
@ stdcall KeSetTimerEx(ptr int64 long ptr)
@ stdcall KeStackAttachProcess(ptr ptr)
@ stub KeSynchronizeExecution
@ stub KeTerminateThread
@ stdcall KeTestAlertThread(long)
@ extern KeTickCount
@ stdcall KeUnstackDetachProcess(ptr)
@ stub KeUpdateRunTime
@ stub KeUpdateSystemTime
@ stub KeUserModeCallback
@ stdcall KeWaitForMultipleObjects(long ptr long long long long ptr ptr)
@ stdcall KeWaitForMutexObject(ptr long long long ptr)
@ stdcall KeWaitForSingleObject(ptr long long long ptr)
@ stdcall -arch=x86_64 KfRaiseIrql(long ptr)
@ stub KiBugCheckData
@ stub KiCoprocessorError
@ stub KiDeliverApc
@ stub KiDispatchInterrupt
@ stub KiEnableTimerWatchdog
@ stub KiIpiServiceRoutine
@ stub KiUnexpectedInterrupt
@ stdcall LdrAccessResource(long ptr ptr ptr)
@ stub LdrEnumResources
@ stdcall LdrFindResourceDirectory_U(long ptr long ptr)
@ stdcall LdrFindResource_U(long ptr long ptr)
@ stub LpcPortObjectType
@ stub LpcRequestPort
@ stub LpcRequestWaitReplyPort
@ stub LsaCallAuthenticationPackage
@ stub LsaDeregisterLogonProcess
@ stub LsaFreeReturnBuffer
@ stub LsaLogonUser
@ stub LsaLookupAuthenticationPackage
@ stub LsaRegisterLogonProcess
@ stub Mm64BitPhysicalAddress
@ stub MmAddPhysicalMemory
@ stub MmAddVerifierThunks
@ stub MmAdjustWorkingSetSize
@ stub MmAdvanceMdl
@ stdcall MmAllocateContiguousMemory(long int64)
@ stdcall MmAllocateContiguousMemorySpecifyCache(long int64 int64 int64 long)
@ stub MmAllocateMappingAddress
@ stdcall MmAllocateNonCachedMemory(long)
@ stdcall MmAllocatePagesForMdl(int64 int64 int64 long)
@ stdcall MmBuildMdlForNonPagedPool(ptr)
@ stub MmCanFileBeTruncated
@ stub MmCommitSessionMappedView
@ stdcall MmCopyVirtualMemory(ptr ptr ptr ptr long long ptr)
@ stub MmCreateMdl
@ stdcall MmCreateSection(ptr long ptr ptr long long long ptr)
@ stub MmDisableModifiedWriteOfSection
@ stub MmFlushImageSection
@ stub MmForceSectionClosed
@ stub MmFreeContiguousMemory
@ stub MmFreeContiguousMemorySpecifyCache
@ stub MmFreeMappingAddress
@ stdcall MmFreeNonCachedMemory(ptr long)
@ stub MmFreePagesFromMdl
@ stdcall MmGetPhysicalAddress(ptr)
@ stub MmGetPhysicalMemoryRanges
@ stdcall MmGetSystemRoutineAddress(ptr)
@ stub MmGetVirtualForPhysical
@ stub MmGrowKernelStack
@ stub MmHighestUserAddress
@ stdcall MmIsAddressValid(ptr)
@ stub MmIsDriverVerifying
@ stub MmIsNonPagedSystemAddressValid
@ stub MmIsRecursiveIoFault
@ stdcall MmIsThisAnNtAsSystem()
@ stub MmIsVerifierEnabled
@ stub MmLockPagableDataSection
@ stub MmLockPagableImageSection
@ stdcall MmLockPagableSectionByHandle(ptr)
@ stdcall MmMapIoSpace(int64 long long)
@ stdcall MmMapLockedPages(ptr long)
@ stdcall MmMapLockedPagesSpecifyCache(ptr long long ptr long long)
@ stub MmMapLockedPagesWithReservedMapping
@ stub MmMapMemoryDumpMdl
@ stub MmMapUserAddressesToPage
@ stub MmMapVideoDisplay
@ stub MmMapViewInSessionSpace
@ stub MmMapViewInSystemSpace
@ stub MmMapViewOfSection
@ stub MmMarkPhysicalMemoryAsBad
@ stub MmMarkPhysicalMemoryAsGood
@ stdcall MmPageEntireDriver(ptr)
@ stub MmPrefetchPages
@ stdcall MmProbeAndLockPages(ptr long long)
@ stub MmProbeAndLockProcessPages
@ stub MmProbeAndLockSelectedPages
@ stdcall MmProtectMdlSystemAddress(ptr long)
@ stdcall MmQuerySystemSize()
@ stub MmRemovePhysicalMemory
@ stdcall MmResetDriverPaging(ptr)
@ stub MmSectionObjectType
@ stub MmSecureVirtualMemory
@ stub MmSetAddressRangeModified
@ stub MmSetBankedSection
@ stub MmSizeOfMdl
@ stub MmSystemRangeStart
@ stub MmTrimAllSystemPagableMemory
@ stdcall MmUnlockPagableImageSection(ptr)
@ stdcall MmUnlockPages(ptr)
@ stdcall MmUnmapIoSpace(ptr long)
@ stdcall MmUnmapLockedPages(ptr ptr)
@ stub MmUnmapReservedMapping
@ stub MmUnmapVideoDisplay
@ stub MmUnmapViewInSessionSpace
@ stub MmUnmapViewInSystemSpace
@ stub MmUnmapViewOfSection
@ stub MmUnsecureVirtualMemory
@ stub MmUserProbeAddress
@ extern NlsAnsiCodePage ntdll.NlsAnsiCodePage
@ stub NlsLeadByteInfo
@ extern NlsMbCodePageTag ntdll.NlsMbCodePageTag
@ extern NlsMbOemCodePageTag ntdll.NlsMbOemCodePageTag
@ stub NlsOemCodePage
@ stub NlsOemLeadByteInfo
@ stdcall NtAddAtom(ptr long ptr)
@ stdcall NtAdjustPrivilegesToken(long long ptr long ptr ptr)
@ stdcall NtAllocateLocallyUniqueId(ptr)
@ stdcall NtAllocateUuids(ptr ptr ptr ptr)
@ stdcall NtAllocateVirtualMemory(long ptr long ptr long long)
@ extern NtBuildNumber
@ stdcall NtClose(long)
@ stdcall NtConnectPort(ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall NtCreateEvent(ptr long ptr long long)
@ stdcall NtCreateFile(ptr long ptr ptr ptr long long long long ptr long)
@ stdcall NtCreateSection(ptr long ptr ptr long long long)
@ stdcall NtDeleteAtom(long)
@ stdcall NtDeleteFile(ptr)
@ stdcall NtDeviceIoControlFile(long long ptr ptr ptr long ptr long ptr long)
@ stdcall NtDuplicateObject(long long long ptr long long long)
@ stdcall NtDuplicateToken(long long ptr long long ptr)
@ stdcall NtFindAtom(ptr long ptr)
@ stdcall NtFreeVirtualMemory(long ptr ptr long)
@ stdcall NtFsControlFile(long long ptr ptr ptr long ptr long ptr long)
@ stub NtGlobalFlag
@ stdcall NtLockFile(long long ptr ptr ptr ptr ptr ptr long long)
@ stdcall NtMapViewOfSection(long long ptr long long ptr ptr long long long)
@ stdcall NtNotifyChangeDirectoryFile(long long ptr ptr ptr ptr long long long)
@ stdcall NtOpenFile(ptr long ptr ptr long long)
@ stdcall NtOpenProcess(ptr long ptr ptr)
@ stdcall NtOpenProcessToken(long long ptr)
@ stdcall NtOpenProcessTokenEx(long long long ptr)
@ stdcall NtOpenThread(ptr long ptr ptr)
@ stdcall NtOpenThreadToken(long long long ptr)
@ stdcall NtOpenThreadTokenEx(long long long long ptr)
@ stdcall NtQueryDirectoryFile(long long ptr ptr ptr ptr long long long ptr long)
@ stdcall NtQueryEaFile(long ptr ptr long long ptr long ptr long)
@ stdcall NtQueryInformationAtom(long long ptr long ptr)
@ stdcall NtQueryInformationFile(long ptr ptr long long)
@ stdcall NtQueryInformationProcess(long long ptr long ptr)
@ stdcall NtQueryInformationThread(long long ptr long ptr)
@ stdcall NtQueryInformationToken(long long ptr long ptr)
@ stub NtQueryQuotaInformationFile
@ stdcall NtQuerySecurityObject(long long ptr long ptr)
@ stdcall NtQuerySystemInformation(long ptr long ptr)
@ stdcall NtQuerySystemInformationEx(long ptr long ptr long ptr)
@ stdcall NtQueryVolumeInformationFile(long ptr ptr long long)
@ stdcall NtReadFile(long long ptr ptr ptr ptr long ptr ptr)
@ stub NtRequestPort
@ stdcall NtRequestWaitReplyPort(ptr ptr ptr)
@ stdcall NtSetEaFile(long ptr ptr long)
@ stdcall NtSetEvent(long ptr)
@ stdcall NtSetInformationFile(long ptr ptr long long)
@ stdcall NtSetInformationProcess(long long ptr long)
@ stdcall NtSetInformationThread(long long ptr long)
@ stdcall NtSetInformationToken(long long ptr long)
@ stub NtSetQuotaInformationFile
@ stdcall NtSetSecurityObject(long long ptr)
@ stdcall NtSetVolumeInformationFile(long ptr ptr long long)
@ stdcall NtShutdownSystem(long)
@ stdcall NtTraceControl(long ptr long ptr long long)
@ stub NtTraceEvent
@ stdcall NtUnlockFile(long ptr ptr ptr ptr)
@ stub NtVdmControl
@ stdcall NtWaitForSingleObject(long long ptr)
@ stdcall NtWriteFile(long long ptr ptr ptr ptr long ptr ptr)
@ stub ObAssignSecurity
@ stub ObCheckCreateObjectAccess
@ stub ObCheckObjectAccess
@ stub ObCloseHandle
@ stub ObCreateObject
@ stub ObCreateObjectType
@ stdcall ObDereferenceObject(ptr)
@ stub ObDereferenceSecurityDescriptor
@ stub ObFindHandleForObject
@ stdcall ObGetFilterVersion()
@ stub ObGetObjectSecurity
@ stdcall ObGetObjectType(ptr)
@ stub ObInsertObject
@ stub ObLogSecurityDescriptor
@ stub ObMakeTemporaryObject
@ stdcall ObOpenObjectByName(ptr ptr long ptr long ptr ptr)
@ stdcall ObOpenObjectByPointer(ptr long ptr long ptr long ptr)
@ stdcall ObQueryNameString(ptr ptr long ptr)
@ stub ObQueryObjectAuditingByHandle
@ stdcall ObReferenceObjectByHandle(long long ptr long ptr ptr)
@ stdcall ObReferenceObjectByName(ptr long ptr long ptr long ptr ptr)
@ stdcall ObReferenceObjectByPointer(ptr long ptr long)
@ stub ObReferenceSecurityDescriptor
@ stdcall ObRegisterCallbacks(ptr ptr)
@ stub ObReleaseObjectSecurity
@ stub ObSetHandleAttributes
@ stub ObSetSecurityDescriptorInfo
@ stub ObSetSecurityObjectByPointer
@ stdcall ObUnRegisterCallbacks(ptr)
@ stub PfxFindPrefix
@ stub PfxInitialize
@ stub PfxInsertPrefix
@ stub PfxRemovePrefix
@ stdcall PoCallDriver(ptr ptr)
@ stub PoCancelDeviceNotify
@ stub PoQueueShutdownWorkItem
@ stub PoRegisterDeviceForIdleDetection
@ stub PoRegisterDeviceNotify
@ stub PoRegisterSystemState
@ stdcall PoRequestPowerIrp(ptr long long ptr ptr ptr)
@ stub PoRequestShutdownEvent
@ stub PoSetHiberRange
@ stdcall PoSetPowerState(ptr long long)
@ stub PoSetSystemState
@ stub PoShutdownBugCheck
@ stdcall PoStartNextPowerIrp(ptr)
@ stub PoUnregisterSystemState
@ stdcall ProbeForRead(ptr long long)
@ stdcall ProbeForWrite(ptr long long)
@ stdcall PsAcquireProcessExitSynchronization(ptr)
@ stub PsAssignImpersonationToken
@ stub PsChargePoolQuota
@ stub PsChargeProcessNonPagedPoolQuota
@ stub PsChargeProcessPagedPoolQuota
@ stub PsChargeProcessPoolQuota
@ stub PsCreateSystemProcess
@ stdcall PsCreateSystemThread(ptr long ptr long ptr ptr ptr)
@ stub PsDereferenceImpersonationToken
@ stub PsDereferencePrimaryToken
@ stub PsDisableImpersonation
@ stub PsEstablishWin32Callouts
@ stub PsGetContextThread
@ stdcall PsGetCurrentProcess() IoGetCurrentProcess
@ stdcall PsGetCurrentProcessId()
@ stdcall PsGetCurrentProcessSessionId()
@ stdcall PsGetCurrentThread() KeGetCurrentThread
@ stdcall PsGetCurrentThreadId()
@ stub PsGetCurrentThreadPreviousMode
@ stub PsGetCurrentThreadStackBase
@ stub PsGetCurrentThreadStackLimit
@ stub PsGetJobLock
@ stub PsGetJobSessionId
@ stub PsGetJobUIRestrictionsClass
@ stub PsGetProcessCreateTimeQuadPart
@ stub PsGetProcessDebugPort
@ stub PsGetProcessExitProcessCalled
@ stub PsGetProcessExitStatus
@ stub PsGetProcessExitTime
@ stdcall PsGetProcessId(ptr)
@ stub PsGetProcessImageFileName
@ stdcall PsGetProcessInheritedFromUniqueProcessId(ptr)
@ stub PsGetProcessJob
@ stub PsGetProcessPeb
@ stub PsGetProcessPriorityClass
@ stdcall PsGetProcessSectionBaseAddress(ptr)
@ stub PsGetProcessSecurityPort
@ stub PsGetProcessSessionId
@ stub PsGetProcessWin32Process
@ stub PsGetProcessWin32WindowStation
@ stdcall -arch=x86_64 PsGetProcessWow64Process(ptr)
@ stub PsGetThreadFreezeCount
@ stub PsGetThreadHardErrorsAreDisabled
@ stdcall PsGetThreadId(ptr)
@ stub PsGetThreadProcess
@ stdcall PsGetThreadProcessId(ptr)
@ stub PsGetThreadSessionId
@ stub PsGetThreadTeb
@ stub PsGetThreadWin32Thread
@ stdcall PsGetVersion(ptr ptr ptr ptr)
@ stdcall PsImpersonateClient(ptr ptr long long long)
@ extern PsInitialSystemProcess
@ stub PsIsProcessBeingDebugged
@ stdcall PsIsSystemThread(ptr)
@ stub PsIsThreadImpersonating
@ stub PsIsThreadTerminating
@ stub PsJobType
@ stdcall PsLookupProcessByProcessId(ptr ptr)
@ stub PsLookupProcessThreadByCid
@ stdcall PsLookupThreadByThreadId(ptr ptr)
@ extern PsProcessType
@ stub PsReferenceImpersonationToken
@ stub PsReferencePrimaryToken
@ stdcall PsReferenceProcessFilePointer(ptr ptr)
@ stdcall PsReleaseProcessExitSynchronization(ptr)
@ stdcall PsRemoveCreateThreadNotifyRoutine(ptr)
@ stdcall PsRemoveLoadImageNotifyRoutine(ptr)
@ stub PsRestoreImpersonation
@ stdcall PsResumeProcess(ptr)
@ stub PsReturnPoolQuota
@ stub PsReturnProcessNonPagedPoolQuota
@ stub PsReturnProcessPagedPoolQuota
@ stub PsRevertThreadToSelf
@ stdcall PsRevertToSelf()
@ stub PsSetContextThread
@ stdcall PsSetCreateProcessNotifyRoutine(ptr long)
@ stdcall PsSetCreateProcessNotifyRoutineEx(ptr long)
@ stdcall PsSetCreateThreadNotifyRoutine(ptr)
@ stub PsSetJobUIRestrictionsClass
@ stub PsSetLegoNotifyRoutine
@ stdcall PsSetLoadImageNotifyRoutine(ptr)
@ stdcall PsSetLoadImageNotifyRoutineEx(ptr long)
@ stub PsSetProcessPriorityByClass
@ stub PsSetProcessPriorityClass
@ stub PsSetProcessSecurityPort
@ stub PsSetProcessWin32Process
@ stub PsSetProcessWindowStation
@ stub PsSetThreadHardErrorsAreDisabled
@ stub PsSetThreadWin32Thread
@ stdcall PsSuspendProcess(ptr)
@ stdcall PsTerminateSystemThread(long)
@ extern PsThreadType
@ stdcall READ_REGISTER_BUFFER_UCHAR(ptr ptr long)
@ stub READ_REGISTER_BUFFER_ULONG
@ stub READ_REGISTER_BUFFER_USHORT
@ stub READ_REGISTER_UCHAR
@ stub READ_REGISTER_ULONG
@ stub READ_REGISTER_USHORT
@ stdcall RtlAbsoluteToSelfRelativeSD(ptr ptr ptr)
@ stdcall RtlAddAccessAllowedAce(ptr long long ptr)
@ stdcall RtlAddAccessAllowedAceEx(ptr long long long ptr)
@ stdcall RtlAddAccessAllowedObjectAce(ptr long long long ptr ptr ptr)
@ stdcall RtlAddAccessDeniedAceEx(ptr long long long ptr)
@ stdcall RtlAddAccessDeniedObjectAce(ptr long long long ptr ptr ptr)
@ stdcall RtlAddAce(ptr long long ptr long)
@ stdcall RtlAddAtomToAtomTable(ptr wstr ptr)
@ stdcall RtlAddAuditAccessAceEx(ptr long long long ptr long long)
@ stdcall RtlAddAuditAccessObjectAce(ptr long long long ptr ptr ptr long long)
@ stub RtlAddRange
@ stdcall RtlAllocateHeap(long long long)
@ stdcall RtlAnsiCharToUnicodeChar(ptr)
@ stdcall RtlAnsiStringToUnicodeSize(ptr)
@ stdcall RtlAnsiStringToUnicodeString(ptr ptr long)
@ stdcall RtlAppendAsciizToString(ptr str)
@ stdcall RtlAppendStringToString(ptr ptr)
@ stdcall RtlAppendUnicodeStringToString(ptr ptr)
@ stdcall RtlAppendUnicodeToString(ptr wstr)
@ stdcall RtlAreAllAccessesGranted(long long)
@ stdcall RtlAreAnyAccessesGranted(long long)
@ stdcall RtlAreBitsClear(ptr long long)
@ stdcall RtlAreBitsSet(ptr long long)
@ stdcall RtlAssert(ptr ptr long str)
@ stdcall -norelay RtlCaptureContext(ptr)
@ stdcall RtlCaptureStackBackTrace(long long ptr ptr)
@ stdcall RtlCharToInteger(ptr long ptr)
@ stdcall RtlCheckRegistryKey(long ptr)
@ stdcall RtlClearAllBits(ptr)
@ stub RtlClearBit
@ stdcall RtlClearBits(ptr long long)
@ stdcall RtlCompareMemory(ptr ptr long)
@ stdcall RtlCompareMemoryUlong(ptr long long)
@ stdcall RtlCompareString(ptr ptr long)
@ stdcall RtlCompareUnicodeString(ptr ptr long)
@ stdcall RtlCompareUnicodeStrings(ptr long ptr long long)
@ stdcall RtlCompressBuffer(long ptr long ptr long long ptr ptr)
@ stub RtlCompressChunks
@ stdcall RtlComputeCrc32(long ptr long)
@ stdcall -arch=win32 -ret64 RtlConvertLongToLargeInteger(long)
@ stdcall RtlConvertSidToUnicodeString(ptr ptr long)
@ stdcall -arch=win32 -ret64 RtlConvertUlongToLargeInteger(long)
@ stdcall RtlCopyExtendedContext(ptr long ptr)
@ stdcall RtlCopyLuid(ptr ptr)
@ stdcall RtlCopyLuidAndAttributesArray(long ptr ptr)
@ stdcall -arch=!i386 RtlCopyMemory(ptr ptr long)
@ stdcall -arch=x86_64 RtlCopyMemoryNonTemporal(ptr ptr long) RtlCopyMemory
@ stub RtlCopyRangeList
@ stdcall RtlCopySid(long ptr ptr)
@ stdcall RtlCopyString(ptr ptr)
@ stdcall RtlCopyUnicodeString(ptr ptr)
@ stdcall RtlCreateAcl(ptr long long)
@ stdcall RtlCreateAtomTable(long ptr)
@ stdcall RtlCreateHeap(long ptr long long ptr ptr)
@ stdcall RtlCreateRegistryKey(long wstr)
@ stdcall RtlCreateSecurityDescriptor(ptr long)
@ stub RtlCreateSystemVolumeInformationFolder
@ stdcall RtlCreateUnicodeString(ptr wstr)
@ stdcall RtlCreateUserThread(long ptr long ptr long long ptr ptr ptr ptr)
@ stdcall RtlCustomCPToUnicodeN(ptr ptr long ptr str long)
@ stdcall RtlDecompressBuffer(long ptr long ptr long ptr)
@ stub RtlDecompressChunks
@ stdcall RtlDecompressFragment(long ptr long ptr long long ptr ptr)
@ stdcall RtlDelete(ptr)
@ stdcall RtlDeleteAce(ptr long)
@ stdcall RtlDeleteAtomFromAtomTable(ptr long)
@ stdcall RtlDeleteElementGenericTable(ptr ptr)
@ stub RtlDeleteElementGenericTableAvl
@ stdcall RtlDeleteNoSplay(ptr ptr)
@ stub RtlDeleteOwnersRanges
@ stub RtlDeleteRange
@ stdcall RtlDeleteRegistryValue(long ptr ptr)
@ stub RtlDescribeChunk
@ stdcall RtlDestroyAtomTable(ptr)
@ stdcall RtlDestroyHeap(long)
@ stdcall RtlDowncaseUnicodeChar(long)
@ stdcall RtlDowncaseUnicodeString(ptr ptr long)
@ stdcall RtlDuplicateUnicodeString(long ptr ptr)
@ stdcall RtlEmptyAtomTable(ptr long)
@ stdcall -arch=win32 -ret64 RtlEnlargedIntegerMultiply(long long)
@ stdcall -arch=win32 RtlEnlargedUnsignedDivide(int64 long ptr)
@ stdcall -arch=win32 -ret64 RtlEnlargedUnsignedMultiply(long long)
@ stdcall RtlEnumerateGenericTable(ptr long)
@ stub RtlEnumerateGenericTableAvl
@ stub RtlEnumerateGenericTableLikeADirectory
@ stdcall RtlEnumerateGenericTableWithoutSplaying(ptr ptr)
@ stdcall RtlEnumerateGenericTableWithoutSplayingAvl(ptr ptr)
@ stdcall RtlEqualLuid(ptr ptr)
@ stdcall RtlEqualSid(ptr ptr)
@ stdcall RtlEqualString(ptr ptr long)
@ stdcall RtlEqualUnicodeString(ptr ptr long)
@ stdcall -arch=win32 -ret64 RtlExtendedIntegerMultiply(int64 long)
@ stdcall -arch=win32 -ret64 RtlExtendedLargeIntegerDivide(int64 long ptr)
@ stdcall -arch=win32 -ret64 RtlExtendedMagicDivide(int64 int64 long)
@ stdcall RtlFillMemory(ptr long long)
@ stdcall RtlFillMemoryUlong(ptr long long)
@ stdcall RtlFindClearBits(ptr long long)
@ stdcall RtlFindClearBitsAndSet(ptr long long)
@ stdcall RtlFindClearRuns(ptr ptr long long)
@ stub RtlFindFirstRunClear
@ stdcall RtlFindLastBackwardRunClear(ptr long ptr)
@ stdcall RtlFindLeastSignificantBit(int64)
@ stdcall RtlFindLongestRunClear(ptr ptr)
@ stdcall RtlFindMessage(long long long long ptr)
@ stdcall RtlFindMostSignificantBit(int64)
@ stdcall RtlFindNextForwardRunClear(ptr long ptr)
@ stub RtlFindRange
@ stdcall RtlFindSetBits(ptr long long)
@ stdcall RtlFindSetBitsAndClear(ptr long long)
@ stub RtlFindUnicodePrefix
@ stdcall RtlFirstFreeAce(ptr ptr)
@ stdcall RtlFormatCurrentUserKeyPath(ptr)
@ stdcall RtlFormatMessage(ptr long long long long ptr ptr long ptr)
@ stdcall RtlFormatMessageEx(ptr long long long long ptr ptr long ptr long)
@ stdcall RtlFreeAnsiString(ptr)
@ stdcall RtlFreeHeap(long long ptr)
@ stdcall RtlFreeOemString(ptr)
@ stub RtlFreeRangeList
@ stdcall RtlFreeUnicodeString(ptr)
@ stdcall RtlGUIDFromString(ptr ptr)
@ stub RtlGenerate8dot3Name
@ stdcall RtlGetAce(ptr long ptr)
@ stub RtlGetCallersAddress
@ stdcall RtlGetCompressionWorkSpaceSize(long ptr ptr)
@ stdcall RtlGetControlSecurityDescriptor(ptr ptr ptr)
@ stdcall RtlGetDaclSecurityDescriptor(ptr ptr ptr ptr)
@ stub RtlGetDefaultCodePage
@ stdcall RtlGetElementGenericTable(ptr long)
@ stub RtlGetElementGenericTableAvl
@ stdcall RtlGetExtendedContextLength(long ptr)
@ stdcall RtlGetExtendedContextLength2(long ptr int64)
@ stub RtlGetFirstRange
@ stdcall RtlGetGroupSecurityDescriptor(ptr ptr ptr)
@ stub RtlGetNextRange
@ stdcall RtlGetNtGlobalFlags()
@ stdcall RtlGetNtProductType(ptr)
@ stdcall RtlGetOwnerSecurityDescriptor(ptr ptr ptr)
@ stdcall RtlGetProductInfo(long long long long ptr)
@ stdcall RtlGetSaclSecurityDescriptor(ptr ptr ptr ptr)
@ stub RtlGetSetBootStatusData
@ stdcall RtlGetEnabledExtendedFeatures(int64)
@ stdcall RtlGetVersion(ptr)
@ stdcall RtlHashUnicodeString(ptr long long ptr)
@ stdcall RtlIdnToAscii(long wstr long ptr ptr)
@ stdcall RtlIdnToNameprepUnicode(long wstr long ptr ptr)
@ stdcall RtlIdnToUnicode(long wstr long ptr ptr)
@ stdcall RtlImageDirectoryEntryToData(long long long ptr)
@ stdcall RtlImageNtHeader(long)
@ stdcall RtlInitAnsiString(ptr str)
@ stdcall RtlInitAnsiStringEx(ptr str)
@ stdcall RtlInitCodePageTable(ptr ptr)
@ stdcall RtlInitString(ptr str)
@ stdcall RtlInitUnicodeString(ptr wstr)
@ stdcall RtlInitUnicodeStringEx(ptr wstr)
@ stdcall RtlInitializeBitMap(ptr ptr long)
@ stdcall RtlInitializeExtendedContext(ptr long ptr)
@ stdcall RtlInitializeExtendedContext2(ptr long ptr int64)
@ stdcall RtlInitializeGenericTable(ptr ptr ptr ptr ptr)
@ stdcall RtlInitializeGenericTableAvl(ptr ptr ptr ptr ptr)
@ stub RtlInitializeRangeList
@ stdcall RtlInitializeSid(ptr ptr long)
@ stub RtlInitializeUnicodePrefix
@ stdcall RtlInsertElementGenericTable(ptr ptr long ptr)
@ stdcall RtlInsertElementGenericTableAvl(ptr ptr long ptr)
@ stub RtlInsertElementGenericTableFull
@ stub RtlInsertElementGenericTableFullAvl
@ stub RtlInsertUnicodePrefix
@ stdcall RtlInt64ToUnicodeString(int64 long ptr)
@ stdcall RtlIntegerToChar(long long long ptr)
@ stub RtlIntegerToUnicode
@ stdcall RtlIntegerToUnicodeString(long long ptr)
@ stub RtlInvertRangeList
@ stdcall RtlIpv4AddressToStringA(ptr ptr)
@ stdcall RtlIpv4AddressToStringExA(ptr long ptr ptr)
@ stdcall RtlIpv4AddressToStringExW(ptr long ptr ptr)
@ stdcall RtlIpv4AddressToStringW(ptr ptr)
@ stdcall RtlIpv4StringToAddressA(str long ptr ptr)
@ stdcall RtlIpv4StringToAddressExA(str long ptr ptr)
@ stdcall RtlIpv4StringToAddressExW(wstr long ptr ptr)
@ stdcall RtlIpv4StringToAddressW(wstr long ptr ptr)
@ stdcall RtlIpv6AddressToStringA(ptr ptr)
@ stdcall RtlIpv6AddressToStringExA(ptr long long ptr ptr)
@ stdcall RtlIpv6AddressToStringExW(ptr long long ptr ptr)
@ stdcall RtlIpv6AddressToStringW(ptr ptr)
@ stdcall RtlIpv6StringToAddressA(str ptr ptr)
@ stdcall RtlIpv6StringToAddressExA(str ptr ptr ptr)
@ stdcall RtlIpv6StringToAddressExW(wstr ptr ptr ptr)
@ stdcall RtlIpv6StringToAddressW(wstr ptr ptr)
@ stdcall RtlIsGenericTableEmpty(ptr)
@ stub RtlIsGenericTableEmptyAvl
@ stdcall RtlIsNameLegalDOS8Dot3(ptr ptr ptr)
@ stdcall RtlIsNormalizedString(long wstr long ptr)
@ stdcall RtlIsNtDdiVersionAvailable(long)
@ stub RtlIsRangeAvailable
@ stub RtlIsValidOemCharacter
@ stdcall -arch=win32 -ret64 RtlLargeIntegerAdd(int64 int64)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerArithmeticShift(int64 long)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerDivide(int64 int64 ptr)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerNegate(int64)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerShiftLeft(int64 long)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerShiftRight(int64 long)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerSubtract(int64 int64)
@ stdcall RtlLargeIntegerToChar(ptr long long ptr)
@ stdcall RtlLengthRequiredSid(long)
@ stdcall RtlLengthSecurityDescriptor(ptr)
@ stdcall RtlLengthSid(ptr)
@ stdcall RtlLocalTimeToSystemTime(ptr ptr)
@ stdcall RtlLocateExtendedFeature(ptr long ptr)
@ stdcall RtlLocateExtendedFeature2(ptr long ptr ptr)
@ stdcall RtlLocateLegacyContext(ptr ptr)
@ stub RtlLockBootStatusData
@ stdcall RtlLookupAtomInAtomTable(ptr wstr ptr)
@ stdcall RtlLookupElementGenericTable(ptr ptr)
@ stdcall RtlLookupElementGenericTableAvl(ptr ptr)
@ stub RtlLookupElementGenericTableFull
@ stub RtlLookupElementGenericTableFullAvl
@ stdcall -arch=!i386 RtlLookupFunctionEntry(long ptr ptr)
@ stdcall RtlMapGenericMask(ptr ptr)
@ stub RtlMapSecurityErrorToNtStatus
@ stub RtlMergeRangeLists
@ stdcall RtlMoveMemory(ptr ptr long)
@ stdcall RtlMultiByteToUnicodeN(ptr long ptr ptr long)
@ stdcall RtlMultiByteToUnicodeSize(ptr str long)
@ stub RtlNextUnicodePrefix
@ stdcall RtlNormalizeString(long wstr long ptr ptr)
@ stdcall RtlNtStatusToDosError(long)
@ stdcall RtlNtStatusToDosErrorNoTeb(long)
@ stdcall RtlNumberGenericTableElements(ptr)
@ stdcall RtlNumberGenericTableElementsAvl(ptr)
@ stdcall RtlNumberOfClearBits(ptr)
@ stdcall RtlNumberOfSetBits(ptr)
@ stub RtlOemStringToCountedUnicodeString
@ stdcall RtlOemStringToUnicodeSize(ptr)
@ stdcall RtlOemStringToUnicodeString(ptr ptr long)
@ stdcall RtlOemToUnicodeN(ptr long ptr ptr long)
@ stdcall RtlOpenCurrentUser(long ptr)
@ stdcall RtlPcToFileHeader(ptr ptr)
@ stdcall RtlPinAtomInAtomTable(ptr long)
@ stdcall RtlPrefixString(ptr ptr long)
@ stdcall RtlPrefixUnicodeString(ptr ptr long)
@ stdcall RtlQueryAtomInAtomTable(ptr long ptr ptr ptr ptr)
@ stdcall RtlQueryDynamicTimeZoneInformation(ptr)
@ stdcall RtlQueryInformationAcl(ptr ptr long long)
@ stdcall RtlQueryPackageIdentity(long ptr ptr ptr ptr ptr)
@ stdcall RtlQueryProcessPlaceholderCompatibilityMode()
@ stdcall RtlQueryRegistryValues(long ptr ptr ptr ptr)
@ stdcall RtlQueryRegistryValuesEx(long ptr ptr ptr ptr) RtlQueryRegistryValues
@ stdcall RtlQueryTimeZoneInformation(ptr)
@ stdcall -norelay RtlRaiseException(ptr)
@ stdcall RtlRaiseStatus(long)
@ stdcall RtlRandom(ptr)
@ stdcall RtlRandomEx(ptr)
@ stdcall RtlRealPredecessor(ptr)
@ stdcall RtlRealSuccessor(ptr)
@ stub RtlRemoveUnicodePrefix
@ stub RtlReserveChunk
@ cdecl -arch=!i386 RtlRestoreContext(ptr ptr)
@ stdcall RtlRunOnceBeginInitialize(ptr long ptr)
@ stdcall RtlRunOnceComplete(ptr long ptr)
@ stdcall RtlRunOnceExecuteOnce(ptr ptr ptr ptr)
@ stdcall RtlRunOnceInitialize(ptr)
@ stdcall RtlSecondsSince1970ToTime(long ptr)
@ stdcall RtlSecondsSince1980ToTime(long ptr)
@ stub RtlSelfRelativeToAbsoluteSD2
@ stdcall RtlSelfRelativeToAbsoluteSD(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall RtlSetAllBits(ptr)
@ stub RtlSetBit
@ stdcall RtlSetBits(ptr long long)
@ stdcall RtlSetControlSecurityDescriptor(ptr long long)
@ stdcall RtlSetDaclSecurityDescriptor(ptr long ptr long)
@ stdcall -ret64 RtlGetExtendedFeaturesMask(ptr)
@ stdcall RtlSetExtendedFeaturesMask(ptr int64)
@ stdcall RtlSetGroupSecurityDescriptor(ptr ptr long)
@ stdcall RtlSetOwnerSecurityDescriptor(ptr ptr long)
@ stdcall RtlSetSaclSecurityDescriptor(ptr long ptr long)
@ stdcall RtlSetTimeZoneInformation(ptr)
@ stdcall RtlSizeHeap(long long ptr)
@ stdcall RtlSplay(ptr)
@ stdcall RtlStringFromGUID(ptr ptr)
@ stdcall RtlSubAuthorityCountSid(ptr)
@ stdcall RtlSubAuthoritySid(ptr long)
@ stdcall RtlSubtreePredecessor(ptr)
@ stdcall RtlSubtreeSuccessor(ptr)
@ stdcall RtlSystemTimeToLocalTime(ptr ptr)
@ stub RtlTestBit
@ stdcall RtlTimeFieldsToTime(ptr ptr)
@ stdcall RtlTimeToElapsedTimeFields(ptr ptr)
@ stdcall RtlTimeToSecondsSince1970(ptr ptr)
@ stdcall RtlTimeToSecondsSince1980(ptr ptr)
@ stdcall RtlTimeToTimeFields(ptr ptr)
@ stub RtlTraceDatabaseAdd
@ stub RtlTraceDatabaseCreate
@ stub RtlTraceDatabaseDestroy
@ stub RtlTraceDatabaseEnumerate
@ stub RtlTraceDatabaseFind
@ stub RtlTraceDatabaseLock
@ stub RtlTraceDatabaseUnlock
@ stub RtlTraceDatabaseValidate
@ stdcall RtlUTF8ToUnicodeN(ptr long ptr ptr long)
@ stdcall RtlUnicodeStringToAnsiSize(ptr)
@ stdcall RtlUnicodeStringToAnsiString(ptr ptr long)
@ stub RtlUnicodeStringToCountedOemString
@ stdcall RtlUnicodeStringToInteger(ptr long ptr)
@ stdcall RtlUnicodeStringToOemSize(ptr)
@ stdcall RtlUnicodeStringToOemString(ptr ptr long)
@ stdcall RtlUnicodeToCustomCPN(ptr ptr long ptr wstr long)
@ stdcall RtlUnicodeToMultiByteN(ptr long ptr ptr long)
@ stdcall RtlUnicodeToMultiByteSize(ptr ptr long)
@ stdcall RtlUnicodeToOemN(ptr long ptr ptr long)
@ stdcall RtlUnicodeToUTF8N(ptr long ptr ptr long)
@ stub RtlUnlockBootStatusData
@ stdcall -norelay RtlUnwind(ptr ptr ptr ptr)
@ stdcall -arch=!i386 RtlUnwindEx(ptr ptr ptr ptr ptr ptr)
@ stdcall RtlUpcaseUnicodeChar(long)
@ stdcall RtlUpcaseUnicodeString(ptr ptr long)
@ stdcall RtlUpcaseUnicodeStringToAnsiString(ptr ptr long)
@ stdcall RtlUpcaseUnicodeStringToCountedOemString(ptr ptr long)
@ stdcall RtlUpcaseUnicodeStringToOemString(ptr ptr long)
@ stdcall RtlUpcaseUnicodeToCustomCPN(ptr ptr long ptr wstr long)
@ stdcall RtlUpcaseUnicodeToMultiByteN(ptr long ptr ptr long)
@ stdcall RtlUpcaseUnicodeToOemN(ptr long ptr ptr long)
@ stdcall RtlUpperChar(long)
@ stdcall RtlUpperString(ptr ptr)
@ stdcall RtlValidAcl(ptr)
@ stdcall RtlValidRelativeSecurityDescriptor(ptr long long)
@ stdcall RtlValidSecurityDescriptor(ptr)
@ stdcall RtlValidSid(ptr)
@ stdcall RtlVerifyVersionInfo(ptr long int64)
@ stdcall -arch=!i386 RtlVirtualUnwind(long long long ptr ptr ptr ptr ptr)
@ stub RtlVolumeDeviceToDosName
@ stub RtlWalkFrameChain
@ stdcall RtlWriteRegistryValue(long ptr ptr long ptr long)
@ stub RtlZeroHeap
@ stdcall RtlZeroMemory(ptr long)
@ stdcall RtlxAnsiStringToUnicodeSize(ptr)
@ stdcall RtlxOemStringToUnicodeSize(ptr)
@ stdcall RtlxUnicodeStringToAnsiSize(ptr)
@ stdcall RtlxUnicodeStringToOemSize(ptr)
@ stub SeAccessCheck
@ stub SeAppendPrivileges
@ stub SeAssignSecurity
@ stub SeAssignSecurityEx
@ stub SeAuditHardLinkCreation
@ stub SeAuditingFileEvents
@ stub SeAuditingFileEventsWithContext
@ stub SeAuditingFileOrGlobalEvents
@ stub SeAuditingHardLinkEvents
@ stub SeAuditingHardLinkEventsWithContext
@ stub SeCaptureSecurityDescriptor
@ stub SeCaptureSubjectContext
@ stub SeCloseObjectAuditAlarm
@ stub SeCreateAccessState
@ stub SeCreateClientSecurity
@ stub SeCreateClientSecurityFromSubjectContext
@ stub SeDeassignSecurity
@ stub SeDeleteAccessState
@ stub SeDeleteObjectAuditAlarm
@ stub SeExports
@ stub SeFilterToken
@ stub SeFreePrivileges
@ stub SeImpersonateClient
@ stub SeImpersonateClientEx
@ stdcall SeLocateProcessImageName(ptr ptr)
@ stub SeLockSubjectContext
@ stub SeMarkLogonSessionForTerminationNotification
@ stub SeOpenObjectAuditAlarm
@ stub SeOpenObjectForDeleteAuditAlarm
@ stdcall SePrivilegeCheck(ptr ptr long)
@ stub SePrivilegeObjectAuditAlarm
@ stub SePublicDefaultDacl
@ stub SeQueryAuthenticationIdToken
@ stub SeQueryInformationToken
@ stub SeQuerySecurityDescriptorInfo
@ stub SeQuerySessionIdToken
@ stub SeRegisterLogonSessionTerminatedRoutine
@ stub SeReleaseSecurityDescriptor
@ stub SeReleaseSubjectContext
@ stub SeSetAccessStateGenericMapping
@ stub SeSetSecurityDescriptorInfo
@ stub SeSetSecurityDescriptorInfoEx
@ stdcall SeSinglePrivilegeCheck(int64 long)
@ stub SeSystemDefaultDacl
@ stub SeTokenImpersonationLevel
@ stub SeTokenIsAdmin
@ stub SeTokenIsRestricted
@ stub SeTokenIsWriteRestricted
@ extern SeTokenObjectType
@ stub SeTokenType
@ stub SeUnlockSubjectContext
@ stub SeUnregisterLogonSessionTerminatedRoutine
@ stub SeValidSecurityDescriptor
@ stdcall -ret64 VerSetConditionMask(int64 long long)
@ stub VfFailDeviceNode
@ stub VfFailDriver
@ stub VfFailSystemBIOS
@ stub VfIsVerificationEnabled
@ stub WRITE_REGISTER_BUFFER_UCHAR
@ stub WRITE_REGISTER_BUFFER_ULONG
@ stub WRITE_REGISTER_BUFFER_USHORT
@ stub WRITE_REGISTER_UCHAR
@ stub WRITE_REGISTER_ULONG
@ stub WRITE_REGISTER_USHORT
@ stub WmiFlushTrace
@ stub WmiQueryTrace
@ stub WmiQueryTraceInformation
@ stub WmiStartTrace
@ stub WmiStopTrace
@ stub WmiTraceMessage
@ stub WmiTraceMessageVa
@ stub WmiUpdateTrace
@ stub XIPDispatch
@ stdcall -private ZwAccessCheckAndAuditAlarm(ptr long ptr ptr ptr long ptr long ptr ptr ptr) NtAccessCheckAndAuditAlarm
@ stub ZwAddBootEntry
@ stdcall -private ZwAdjustPrivilegesToken(long long ptr long ptr ptr) NtAdjustPrivilegesToken
@ stdcall -private ZwAlertThread(long) NtAlertThread
@ stdcall -private ZwAllocateLocallyUniqueId(ptr) NtAllocateLocallyUniqueId
@ stdcall -private ZwAllocateVirtualMemory(long ptr long ptr long long) NtAllocateVirtualMemory
@ stdcall -private ZwAssignProcessToJobObject(long long) NtAssignProcessToJobObject
@ stdcall -private ZwCancelIoFile(long ptr) NtCancelIoFile
@ stdcall -private ZwCancelIoFileEx(long ptr ptr) NtCancelIoFileEx
@ stdcall -private ZwCancelTimer(long ptr) NtCancelTimer
@ stdcall -private ZwClearEvent(long) NtClearEvent
@ stdcall ZwClose(long) NtClose
@ stub ZwCloseObjectAuditAlarm
@ stdcall -private ZwConnectPort(ptr ptr ptr ptr ptr ptr ptr ptr) NtConnectPort
@ stdcall ZwCreateDirectoryObject(ptr long ptr) NtCreateDirectoryObject
@ stdcall ZwCreateEvent(ptr long ptr long long) NtCreateEvent
@ stdcall ZwCreateFile(ptr long ptr ptr ptr long long long long ptr long) NtCreateFile
@ stdcall -private ZwCreateIoCompletion(ptr long ptr long) NtCreateIoCompletion
@ stdcall -private ZwCreateJobObject(ptr long ptr) NtCreateJobObject
@ stdcall -private ZwCreateKey(ptr long ptr long ptr long ptr) NtCreateKey
@ stdcall -private ZwCreateKeyTransacted(ptr long ptr long ptr long long ptr) NtCreateKeyTransacted
@ stdcall -private ZwCreateSection(ptr long ptr ptr long long long) NtCreateSection
@ stdcall -private ZwCreateSymbolicLinkObject(ptr long ptr ptr) NtCreateSymbolicLinkObject
@ stdcall -private ZwCreateTimer(ptr long ptr long) NtCreateTimer
@ stub ZwDeleteBootEntry
@ stdcall -private ZwDeleteFile(ptr) NtDeleteFile
@ stdcall -private ZwDeleteKey(long) NtDeleteKey
@ stdcall -private ZwDeleteValueKey(long ptr) NtDeleteValueKey
@ stdcall -private ZwDeviceIoControlFile(long long ptr ptr ptr long ptr long ptr long) NtDeviceIoControlFile
@ stdcall -private ZwDisplayString(ptr) NtDisplayString
@ stdcall ZwDuplicateObject(long long long ptr long long long) NtDuplicateObject
@ stdcall -private ZwDuplicateToken(long long ptr long long ptr) NtDuplicateToken
@ stub ZwEnumerateBootEntries
@ stdcall -private ZwEnumerateKey(long long long ptr long ptr) NtEnumerateKey
@ stdcall -private ZwEnumerateValueKey(long long long ptr long ptr) NtEnumerateValueKey
@ stdcall -private ZwFlushBuffersFile(long ptr) NtFlushBuffersFile
@ stdcall -private ZwFlushInstructionCache(long ptr long) NtFlushInstructionCache
@ stdcall -private ZwFlushKey(long) NtFlushKey
@ stdcall -private ZwFlushVirtualMemory(long ptr ptr long) NtFlushVirtualMemory
@ stdcall -private ZwFreeVirtualMemory(long ptr ptr long) NtFreeVirtualMemory
@ stdcall -private ZwFsControlFile(long long ptr ptr ptr long ptr long ptr long) NtFsControlFile
@ stdcall -private ZwImpersonateAnonymousToken(long) NtImpersonateAnonymousToken
@ stdcall -private ZwInitiatePowerAction(long long long long) NtInitiatePowerAction
@ stdcall -private ZwIsProcessInJob(long long) NtIsProcessInJob
@ stdcall ZwLoadDriver(ptr)
@ stdcall -private ZwLoadKey(ptr ptr) NtLoadKey
@ stdcall -private ZwLockFile(long long ptr ptr ptr ptr ptr ptr long long) NtLockFile
@ stdcall -private ZwLockVirtualMemory(long ptr ptr long) NtLockVirtualMemory
@ stdcall ZwMakePermanentObject(long) NtMakePermanentObject
@ stdcall ZwMakeTemporaryObject(long) NtMakeTemporaryObject
@ stdcall ZwMapViewOfSection(long long ptr long long ptr ptr long long long) NtMapViewOfSection
@ stdcall -private ZwNotifyChangeDirectoryFile(long long ptr ptr ptr ptr long long long) NtNotifyChangeDirectoryFile
@ stdcall -private ZwNotifyChangeKey(long long ptr ptr ptr long long ptr long long) NtNotifyChangeKey
@ stdcall ZwOpenDirectoryObject(ptr long ptr) NtOpenDirectoryObject
@ stdcall -private ZwOpenEvent(ptr long ptr) NtOpenEvent
@ stdcall ZwOpenFile(ptr long ptr ptr long long) NtOpenFile
@ stdcall -private ZwOpenJobObject(ptr long ptr) NtOpenJobObject
@ stdcall ZwOpenKey(ptr long ptr) NtOpenKey
@ stdcall -private ZwOpenKeyEx(ptr long ptr long) NtOpenKeyEx
@ stdcall -private ZwOpenKeyTransacted(ptr long ptr long) NtOpenKeyTransacted
@ stdcall -private ZwOpenKeyTransactedEx(ptr long ptr long long) NtOpenKeyTransactedEx
@ stdcall -private ZwOpenProcess(ptr long ptr ptr) NtOpenProcess
@ stdcall -private ZwOpenProcessToken(long long ptr) NtOpenProcessToken
@ stdcall -private ZwOpenProcessTokenEx(long long long ptr) NtOpenProcessTokenEx
@ stdcall ZwOpenSection(ptr long ptr) NtOpenSection
@ stdcall -private ZwOpenSymbolicLinkObject(ptr long ptr) NtOpenSymbolicLinkObject
@ stdcall -private ZwOpenThread(ptr long ptr ptr) NtOpenThread
@ stdcall -private ZwOpenThreadToken(long long long ptr) NtOpenThreadToken
@ stdcall -private ZwOpenThreadTokenEx(long long long long ptr) NtOpenThreadTokenEx
@ stdcall -private ZwOpenTimer(ptr long ptr) NtOpenTimer
@ stdcall -private ZwPowerInformation(long ptr long ptr long) NtPowerInformation
@ stdcall -private ZwProtectVirtualMemory(long ptr ptr long ptr) NtProtectVirtualMemory
@ stdcall -private ZwPulseEvent(long ptr) NtPulseEvent
@ stub ZwQueryBootEntryOrder
@ stub ZwQueryBootOptions
@ stdcall -private ZwQueryDefaultLocale(long ptr) NtQueryDefaultLocale
@ stdcall -private ZwQueryDefaultUILanguage(ptr) NtQueryDefaultUILanguage
@ stdcall -private ZwQueryDirectoryFile(long long ptr ptr ptr ptr long long long ptr long) NtQueryDirectoryFile
@ stdcall -private ZwQueryDirectoryObject(long ptr long long long ptr ptr) NtQueryDirectoryObject
@ stdcall -private ZwQueryEaFile(long ptr ptr long long ptr long ptr long) NtQueryEaFile
@ stdcall -private ZwQueryFullAttributesFile(ptr ptr) NtQueryFullAttributesFile
@ stdcall -private ZwQueryInformationFile(long ptr ptr long long) NtQueryInformationFile
@ stdcall -private ZwQueryInformationJobObject(long long ptr long ptr) NtQueryInformationJobObject
@ stdcall ZwQueryInformationProcess(long long ptr long ptr) NtQueryInformationProcess
@ stdcall -private ZwQueryInformationThread(long long ptr long ptr) NtQueryInformationThread
@ stdcall -private ZwQueryInformationToken(long long ptr long ptr) NtQueryInformationToken
@ stdcall -private ZwQueryInstallUILanguage(ptr) NtQueryInstallUILanguage
@ stdcall -private ZwQueryKey(long long ptr long ptr) NtQueryKey
@ stdcall -private ZwQueryLicenseValue(ptr ptr ptr long ptr) NtQueryLicenseValue
@ stdcall ZwQueryObject(long long ptr long ptr) NtQueryObject
@ stdcall -private ZwQuerySection(long long ptr long ptr) NtQuerySection
@ stdcall -private ZwQuerySecurityObject(long long ptr long ptr) NtQuerySecurityObject
@ stdcall -private ZwQuerySymbolicLinkObject(long ptr ptr) NtQuerySymbolicLinkObject
@ stdcall -private ZwQuerySystemEnvironmentValueEx(ptr ptr ptr ptr ptr) NtQuerySystemEnvironmentValueEx
@ stdcall -private ZwQuerySystemInformation(long ptr long ptr) NtQuerySystemInformation
@ stdcall -private ZwQuerySystemInformationEx(long ptr long ptr long ptr) NtQuerySystemInformationEx
@ stdcall -private ZwQueryTimerResolution(ptr ptr ptr) NtQueryTimerResolution
@ stdcall ZwQueryValueKey(long ptr long ptr long ptr) NtQueryValueKey
@ stdcall -private ZwQueryVirtualMemory(long ptr long ptr long ptr) NtQueryVirtualMemory
@ stdcall -private ZwQueryVolumeInformationFile(long ptr ptr long long) NtQueryVolumeInformationFile
@ stdcall -private ZwReadFile(long long ptr ptr ptr ptr long ptr ptr) NtReadFile
@ stdcall -private ZwRemoveIoCompletion(ptr ptr ptr ptr ptr) NtRemoveIoCompletion
@ stdcall -private ZwRemoveIoCompletionEx(ptr ptr long ptr ptr long) NtRemoveIoCompletionEx
@ stdcall -private ZwRenameKey(long ptr) NtRenameKey
@ stdcall -private ZwReplaceKey(ptr long ptr) NtReplaceKey
@ stdcall -private ZwRequestWaitReplyPort(ptr ptr ptr) NtRequestWaitReplyPort
@ stdcall -private ZwResetEvent(long ptr) NtResetEvent
@ stdcall -private ZwRestoreKey(long long long) NtRestoreKey
@ stdcall -private ZwSaveKey(long long) NtSaveKey
@ stub ZwSaveKeyEx
@ stdcall -private ZwSecureConnectPort(ptr ptr ptr ptr ptr ptr ptr ptr ptr) NtSecureConnectPort
@ stub ZwSetBootEntryOrder
@ stub ZwSetBootOptions
@ stdcall -private ZwSetDefaultLocale(long long) NtSetDefaultLocale
@ stdcall -private ZwSetDefaultUILanguage(long) NtSetDefaultUILanguage
@ stdcall -private ZwSetEaFile(long ptr ptr long) NtSetEaFile
@ stdcall ZwSetEvent(long ptr) NtSetEvent
@ stdcall -private ZwSetInformationFile(long ptr ptr long long) NtSetInformationFile
@ stdcall -private ZwSetInformationJobObject(long long ptr long) NtSetInformationJobObject
@ stdcall -private ZwSetInformationKey(long long ptr long) NtSetInformationKey
@ stdcall -private ZwSetInformationObject(long long ptr long) NtSetInformationObject
@ stdcall -private ZwSetInformationProcess(long long ptr long) NtSetInformationProcess
@ stdcall -private ZwSetInformationThread(long long ptr long) NtSetInformationThread
@ stdcall -private ZwSetInformationToken(long long ptr long) NtSetInformationToken
@ stdcall -private ZwSetIntervalProfile(long long) NtSetIntervalProfile
@ stdcall -private ZwSetIoCompletion(ptr long long long long) NtSetIoCompletion
@ stdcall -private ZwSetIoCompletionEx(ptr ptr long long long long) NtSetIoCompletionEx
@ stdcall -private ZwSetSecurityObject(long long ptr) NtSetSecurityObject
@ stdcall -private ZwSetSystemInformation(long ptr long) NtSetSystemInformation
@ stdcall -private ZwSetSystemTime(ptr ptr) NtSetSystemTime
@ stdcall -private ZwSetTimer(long ptr ptr ptr long long ptr) NtSetTimer
@ stdcall -private ZwSetTimerResolution(long long ptr) NtSetTimerResolution
@ stdcall -private ZwSetValueKey(long ptr long long ptr long) NtSetValueKey
@ stdcall -private ZwSetVolumeInformationFile(long ptr ptr long long) NtSetVolumeInformationFile
@ stdcall -private ZwTerminateJobObject(long long) NtTerminateJobObject
@ stdcall -private ZwTerminateProcess(long long) NtTerminateProcess
@ stdcall -private ZwTraceControl(long ptr long ptr long long) NtTraceControl
@ stub ZwTranslateFilePath
@ stdcall ZwUnloadDriver(ptr)
@ stdcall -private ZwUnloadKey(ptr) NtUnloadKey
@ stdcall -private ZwUnlockFile(long ptr ptr ptr ptr) NtUnlockFile
@ stdcall -private ZwUnlockVirtualMemory(long ptr ptr long) NtUnlockVirtualMemory
@ stdcall ZwUnmapViewOfSection(long ptr) NtUnmapViewOfSection
@ stdcall -private ZwWaitForMultipleObjects(long ptr long long ptr) NtWaitForMultipleObjects
@ stdcall ZwWaitForSingleObject(long long ptr) NtWaitForSingleObject
@ stdcall ZwWriteFile(long long ptr ptr ptr ptr long ptr ptr) NtWriteFile
@ stdcall -private ZwYieldExecution() NtYieldExecution
@ stdcall -arch=!i386 __C_specific_handler(ptr long ptr ptr)
@ cdecl -arch=!i386 -norelay __chkstk()
@ cdecl -private -arch=i386 _CIcos()
@ cdecl -private -arch=i386 _CIsin()
@ cdecl -private -arch=i386 _CIsqrt()
@ cdecl -private _abnormal_termination()
@ cdecl -arch=i386 -norelay -ret64 _alldiv(int64 int64)
@ cdecl -arch=i386 -norelay _alldvrm(int64 int64)
@ cdecl -arch=i386 -norelay -ret64 _allmul(int64 int64)
@ cdecl -arch=i386 -norelay _alloca_probe()
@ cdecl -arch=i386 -norelay -ret64 _allrem(int64 int64)
@ cdecl -arch=i386 -norelay -ret64 _allshl(int64 long)
@ cdecl -arch=i386 -norelay -ret64 _allshr(int64 long)
@ cdecl -arch=i386 -norelay -ret64 _aulldiv(int64 int64)
@ cdecl -arch=i386 -norelay _aulldvrm(int64 int64)
@ cdecl -arch=i386 -norelay -ret64 _aullrem(int64 int64)
@ cdecl -arch=i386 -norelay -ret64 _aullshr(int64 long)
@ cdecl -arch=i386 -norelay _chkstk()
@ cdecl -arch=i386 _except_handler2(ptr ptr ptr ptr)
@ cdecl -arch=i386 _except_handler3(ptr ptr ptr ptr)
@ cdecl _finite(double)
@ cdecl -arch=i386 _global_unwind2(ptr)
@ cdecl _i64toa_s(int64 ptr long long)
@ cdecl _i64tow_s(int64 ptr long long)
@ cdecl _itoa(long ptr long)
@ cdecl _itoa_s(long ptr long long)
@ cdecl _itow(long ptr long)
@ cdecl _itow_s(long ptr long long)
@ cdecl -arch=win64 _local_unwind(ptr ptr)
@ cdecl -arch=i386 _local_unwind2(ptr long)
@ cdecl _ltoa_s(long ptr long long)
@ cdecl _ltow_s(long ptr long long)
@ cdecl _makepath_s(ptr long str str str str)
@ cdecl _purecall()
@ cdecl -norelay _setjmp(ptr)
@ cdecl -arch=!i386 -norelay _setjmpex(ptr ptr)
@ varargs _snprintf(ptr long str)
@ varargs _snprintf_s(ptr long long str)
@ varargs _snscanf_s(str long str)
@ varargs _snwprintf(ptr long wstr)
@ varargs _snwprintf_s(ptr long long wstr)
@ varargs _snwscanf_s(wstr long wstr)
@ cdecl _splitpath_s(str ptr long ptr long ptr long ptr long)
@ cdecl _stricmp(str str)
@ cdecl _strlwr(str)
@ cdecl _strnicmp(str str long)
@ cdecl _strnset(str long long)
@ cdecl _strnset_s(str long long long)
@ cdecl _strrev(str)
@ cdecl _strset(str long)
@ cdecl -ret64 _strtoui64(str ptr long)
@ cdecl _strupr(str)
@ varargs _swprintf(ptr wstr)
@ cdecl _ui64toa_s(int64 ptr long long)
@ cdecl _ui64tow_s(int64 ptr long long)
@ cdecl _ultoa_s(long ptr long long)
@ cdecl _ultow_s(long ptr long long)
@ cdecl -norelay _vsnprintf(ptr long str ptr)
@ cdecl _vsnprintf_s(ptr long long str ptr)
@ cdecl _vsnwprintf(ptr long wstr ptr)
@ cdecl _vsnwprintf_s(ptr long long wstr ptr)
@ cdecl _vswprintf(ptr wstr ptr)
@ cdecl _wcsicmp(wstr wstr)
@ cdecl _wcslwr(wstr)
@ cdecl _wcslwr_s(wstr long)
@ cdecl _wcsnicmp(wstr wstr long)
@ cdecl _wcsnset(wstr long long)
@ cdecl _wcsnset_s(wstr long long long)
@ cdecl _wcsrev(wstr)
@ cdecl _wcsset_s(wstr long long)
@ cdecl _wcsupr(wstr)
@ cdecl _wmakepath_s(ptr long wstr wstr wstr wstr)
@ cdecl _wsplitpath_s(wstr ptr long ptr long ptr long ptr long)
@ cdecl _wtoi(wstr)
@ cdecl _wtol(wstr)
@ cdecl atoi(str)
@ cdecl atol(str)
@ cdecl bsearch(ptr ptr long long ptr)
@ cdecl bsearch_s(ptr ptr long long ptr ptr)
@ cdecl isdigit(long)
@ cdecl islower(long)
@ cdecl isprint(long)
@ cdecl isspace(long)
@ cdecl isupper(long)
@ cdecl iswalnum(long)
@ cdecl iswdigit(long)
@ cdecl iswspace(long)
@ cdecl isxdigit(long)
@ cdecl longjmp(ptr long)
@ cdecl mbstowcs(ptr str long)
@ cdecl mbtowc(ptr str long)
@ cdecl memchr(ptr long long)
@ cdecl memcmp(ptr ptr long)
@ cdecl memcpy(ptr ptr long)
@ cdecl memcpy_s(ptr long ptr long)
@ cdecl memmove(ptr ptr long)
@ cdecl memmove_s(ptr long ptr long)
@ cdecl memset(ptr long long)
@ cdecl qsort(ptr long long ptr)
@ cdecl qsort_s(ptr long long ptr ptr)
@ cdecl rand()
@ varargs sprintf(ptr str)
@ varargs sprintf_s(ptr long str)
@ cdecl sqrt(double)
@ cdecl -arch=!i386 sqrtf(float)
@ cdecl srand(long)
@ varargs sscanf_s(str str)
@ cdecl strcat(str str)
@ cdecl strcat_s(str long str)
@ cdecl strchr(str long)
@ cdecl strcmp(str str)
@ cdecl strcpy(ptr str)
@ cdecl strcpy_s(ptr long str)
@ cdecl strlen(str)
@ cdecl strncat(str str long)
@ cdecl strncat_s(str long str long)
@ cdecl strncmp(str str long)
@ cdecl strncpy(ptr str long)
@ cdecl strncpy_s(ptr long str long)
@ cdecl strnlen(str long)
@ cdecl strrchr(str long)
@ cdecl strspn(str str)
@ cdecl strstr(str str)
@ cdecl strtok_s(ptr str ptr)
@ varargs swprintf(ptr wstr)
@ varargs swprintf_s(ptr long wstr)
@ varargs swscanf_s(wstr wstr)
@ cdecl tolower(long)
@ cdecl toupper(long)
@ cdecl towlower(long)
@ cdecl towupper(long)
@ stdcall vDbgPrintEx(long long str ptr)
@ stdcall vDbgPrintExWithPrefix(str long long str ptr)
@ cdecl vsprintf(ptr str ptr)
@ cdecl vsprintf_s(ptr long str ptr)
@ cdecl vswprintf_s(ptr long wstr ptr)
@ cdecl wcscat(wstr wstr)
@ cdecl wcscat_s(wstr long wstr)
@ cdecl wcschr(wstr long)
@ cdecl wcscmp(wstr wstr)
@ cdecl wcscpy(ptr wstr)
@ cdecl wcscpy_s(ptr long wstr)
@ cdecl wcscspn(wstr wstr)
@ cdecl wcslen(wstr)
@ cdecl wcsncat(wstr wstr long)
@ cdecl wcsncat_s(wstr long wstr long)
@ cdecl wcsncmp(wstr wstr long)
@ cdecl wcsncpy(ptr wstr long)
@ cdecl wcsncpy_s(ptr long wstr long)
@ cdecl wcsnlen(wstr long)
@ cdecl wcsrchr(wstr long)
@ cdecl wcsspn(wstr wstr)
@ cdecl wcsstr(wstr wstr)
@ cdecl wcstombs(ptr ptr long)
@ cdecl wcstoul(wstr ptr long)
@ cdecl wctomb(ptr long)

################################################################
# Wine internal extensions
#
# All functions must be prefixed with '__wine_' (for internal functions)
# or 'wine_' (for user-visible functions) to avoid namespace conflicts.

@ cdecl wine_ntoskrnl_main_loop(long)
@ cdecl wine_enumerate_root_devices(wstr)
