@ stub ExAcquireFastMutexUnsafe
@ stub ExAcquireRundownProtection
@ stub ExAcquireRundownProtectionEx
@ stub ExInitializeRundownProtection
@ stub ExInterlockedAddLargeStatistic
@ stub ExInterlockedCompareExchange64
@ stub ExInterlockedFlushSList
@ stub ExInterlockedPopEntrySList
@ stub ExInterlockedPushEntrySList
@ stub ExReInitializeRundownProtection
@ stub ExReleaseFastMutexUnsafe
@ stub ExReleaseResourceLite
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
@ stub ExfInterlockedRemoveHeadList
@ stub ExfReleasePushLock
@ stub Exfi386InterlockedDecrementLong
@ stub Exfi386InterlockedExchangeUlong
@ stub Exfi386InterlockedIncrementLong
@ stub HalExamineMBR
@ stdcall InterlockedCompareExchange(ptr long long) kernel32.InterlockedCompareExchange
@ stdcall InterlockedDecrement(ptr) kernel32.InterlockedDecrement
@ stdcall InterlockedExchange(ptr long) kernel32.InterlockedExchange
@ stdcall InterlockedExchangeAdd(ptr long ) kernel32.InterlockedExchangeAdd
@ stdcall InterlockedIncrement(ptr) kernel32.InterlockedIncrement
@ stdcall InterlockedPopEntrySList(ptr) kernel32.InterlockedPopEntrySList
@ stdcall InterlockedPushEntrySList(ptr ptr) kernel32.InterlockedPushEntrySList
@ stub IoAssignDriveLetters
@ stub IoReadPartitionTable
@ stub IoSetPartitionInformation
@ stub IoWritePartitionTable
@ stub IofCallDriver
@ stub IofCompleteRequest
@ stub KeAcquireInStackQueuedSpinLockAtDpcLevel
@ stub KeReleaseInStackQueuedSpinLockFromDpcLevel
@ stub KeSetTimeUpdateNotifyRoutine
@ stub KefAcquireSpinLockAtDpcLevel
@ stub KefReleaseSpinLockFromDpcLevel
@ stub KiAcquireSpinLock
@ stub KiReleaseSpinLock
@ stub ObfDereferenceObject
@ stub ObfReferenceObject
@ stub RtlPrefetchMemoryNonTemporal
@ cdecl -i386 -norelay RtlUlongByteSwap() ntdll.RtlUlongByteSwap
@ cdecl -ret64 RtlUlonglongByteSwap(double) ntdll.RtlUlonglongByteSwap
@ cdecl -i386 -norelay RtlUshortByteSwap() ntdll.RtlUshortByteSwap
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
@ stub CmRegisterCallback
@ stub CmUnRegisterCallback
@ stdcall DbgBreakPoint() ntdll.DbgBreakPoint
@ stub DbgBreakPointWithStatus
@ stub DbgLoadImageSymbols
@ varargs DbgPrint(str) ntdll.DbgPrint
@ varargs DbgPrintEx(long long str) ntdll.DbgPrintEx
@ stub DbgPrintReturnControlC
@ stub DbgPrompt
@ stub DbgQueryDebugFilterState
@ stub DbgSetDebugFilterState
@ stub ExAcquireResourceExclusiveLite
@ stub ExAcquireResourceSharedLite
@ stub ExAcquireSharedStarveExclusive
@ stub ExAcquireSharedWaitForExclusive
@ stub ExAllocateFromPagedLookasideList
@ stub ExAllocatePool
@ stub ExAllocatePoolWithQuota
@ stub ExAllocatePoolWithQuotaTag
@ stub ExAllocatePoolWithTag
@ stub ExAllocatePoolWithTagPriority
@ stub ExConvertExclusiveToSharedLite
@ stub ExCreateCallback
@ stub ExDeleteNPagedLookasideList
@ stub ExDeletePagedLookasideList
@ stub ExDeleteResourceLite
@ stub ExDesktopObjectType
@ stub ExDisableResourceBoostLite
@ stub ExEnumHandleTable
@ stub ExEventObjectType
@ stub ExExtendZone
@ stub ExFreePool
@ stub ExFreePoolWithTag
@ stub ExFreeToPagedLookasideList
@ stub ExGetCurrentProcessorCounts
@ stub ExGetCurrentProcessorCpuUsage
@ stub ExGetExclusiveWaiterCount
@ stub ExGetPreviousMode
@ stub ExGetSharedWaiterCount
@ stub ExInitializeNPagedLookasideList
@ stub ExInitializePagedLookasideList
@ stub ExInitializeResourceLite
@ stub ExInitializeZone
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
@ stub ExInterlockedRemoveHeadList
@ stub ExIsProcessorFeaturePresent
@ stub ExIsResourceAcquiredExclusiveLite
@ stub ExIsResourceAcquiredSharedLite
@ stub ExLocalTimeToSystemTime
@ stub ExNotifyCallback
@ stub ExQueryPoolBlockSize
@ stub ExQueueWorkItem
@ stub ExRaiseAccessViolation
@ stub ExRaiseDatatypeMisalignment
@ stub ExRaiseException
@ stub ExRaiseHardError
@ stub ExRaiseStatus
@ stub ExRegisterCallback
@ stub ExReinitializeResourceLite
@ stub ExReleaseResourceForThreadLite
@ stub ExSemaphoreObjectType
@ stub ExSetResourceOwnerPointer
@ stub ExSetTimerResolution
@ stub ExSystemExceptionFilter
@ stub ExSystemTimeToLocalTime
@ stub ExUnregisterCallback
@ stub ExUuidCreate
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
@ stub FsRtlIsNameInExpression
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
@ stub FsRtlRegisterFileSystemFilterCallbacks
@ stub FsRtlRegisterUncProvider
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
@ stub InitSafeBootMode
@ stub IoAcquireCancelSpinLock
@ stub IoAcquireRemoveLockEx
@ stub IoAcquireVpbSpinLock
@ stub IoAdapterObjectType
@ stub IoAllocateAdapterChannel
@ stub IoAllocateController
@ stub IoAllocateDriverObjectExtension
@ stub IoAllocateErrorLogEntry
@ stub IoAllocateIrp
@ stub IoAllocateMdl
@ stub IoAllocateWorkItem
@ stub IoAssignResources
@ stub IoAttachDevice
@ stub IoAttachDeviceByPointer
@ stub IoAttachDeviceToDeviceStack
@ stub IoAttachDeviceToDeviceStackSafe
@ stub IoBuildAsynchronousFsdRequest
@ stub IoBuildDeviceIoControlRequest
@ stub IoBuildPartialMdl
@ stub IoBuildSynchronousFsdRequest
@ stub IoCallDriver
@ stub IoCancelFileOpen
@ stub IoCancelIrp
@ stub IoCheckDesiredAccess
@ stub IoCheckEaBufferValidity
@ stub IoCheckFunctionAccess
@ stub IoCheckQuerySetFileInformation
@ stub IoCheckQuerySetVolumeInformation
@ stub IoCheckQuotaBufferValidity
@ stub IoCheckShareAccess
@ stub IoCompleteRequest
@ stub IoConnectInterrupt
@ stub IoCreateController
@ stub IoCreateDevice
@ stub IoCreateDisk
@ stub IoCreateDriver
@ stub IoCreateFile
@ stub IoCreateFileSpecifyDeviceObjectHint
@ stub IoCreateNotificationEvent
@ stub IoCreateStreamFileObject
@ stub IoCreateStreamFileObjectEx
@ stub IoCreateStreamFileObjectLite
@ stub IoCreateSymbolicLink
@ stub IoCreateSynchronizationEvent
@ stub IoCreateUnprotectedSymbolicLink
@ stub IoCsqInitialize
@ stub IoCsqInsertIrp
@ stub IoCsqRemoveIrp
@ stub IoCsqRemoveNextIrp
@ stub IoDeleteController
@ stub IoDeleteDevice
@ stub IoDeleteDriver
@ stub IoDeleteSymbolicLink
@ stub IoDetachDevice
@ stub IoDeviceHandlerObjectSize
@ stub IoDeviceHandlerObjectType
@ stub IoDeviceObjectType
@ stub IoDisconnectInterrupt
@ stub IoDriverObjectType
@ stub IoEnqueueIrp
@ stub IoEnumerateDeviceObjectList
@ stub IoFastQueryNetworkAttributes
@ stub IoFileObjectType
@ stub IoForwardAndCatchIrp
@ stub IoForwardIrpSynchronously
@ stub IoFreeController
@ stub IoFreeErrorLogEntry
@ stub IoFreeIrp
@ stub IoFreeMdl
@ stub IoFreeWorkItem
@ stub IoGetAttachedDevice
@ stub IoGetAttachedDeviceReference
@ stub IoGetBaseFileSystemDeviceObject
@ stub IoGetBootDiskInformation
@ stub IoGetConfigurationInformation
@ stub IoGetCurrentProcess
@ stub IoGetDeviceAttachmentBaseRef
@ stub IoGetDeviceInterfaceAlias
@ stub IoGetDeviceInterfaces
@ stub IoGetDeviceObjectPointer
@ stub IoGetDeviceProperty
@ stub IoGetDeviceToVerify
@ stub IoGetDiskDeviceObject
@ stub IoGetDmaAdapter
@ stub IoGetDriverObjectExtension
@ stub IoGetFileObjectGenericMapping
@ stub IoGetInitialStack
@ stub IoGetLowerDeviceObject
@ stub IoGetRelatedDeviceObject
@ stub IoGetRequestorProcess
@ stub IoGetRequestorProcessId
@ stub IoGetRequestorSessionId
@ stub IoGetStackLimits
@ stub IoGetTopLevelIrp
@ stub IoInitializeIrp
@ stub IoInitializeRemoveLockEx
@ stub IoInitializeTimer
@ stub IoInvalidateDeviceRelations
@ stub IoInvalidateDeviceState
@ stub IoIsFileOriginRemote
@ stub IoIsOperationSynchronous
@ stub IoIsSystemThread
@ stub IoIsValidNameGraftingBuffer
@ stub IoIsWdmVersionAvailable
@ stub IoMakeAssociatedIrp
@ stub IoOpenDeviceInterfaceRegistryKey
@ stub IoOpenDeviceRegistryKey
@ stub IoPageRead
@ stub IoPnPDeliverServicePowerNotification
@ stub IoQueryDeviceDescription
@ stub IoQueryFileDosDeviceName
@ stub IoQueryFileInformation
@ stub IoQueryVolumeInformation
@ stub IoQueueThreadIrp
@ stub IoQueueWorkItem
@ stub IoRaiseHardError
@ stub IoRaiseInformationalHardError
@ stub IoReadDiskSignature
@ stub IoReadOperationCount
@ stub IoReadPartitionTableEx
@ stub IoReadTransferCount
@ stub IoRegisterBootDriverReinitialization
@ stub IoRegisterDeviceInterface
@ stub IoRegisterDriverReinitialization
@ stub IoRegisterFileSystem
@ stub IoRegisterFsRegistrationChange
@ stub IoRegisterLastChanceShutdownNotification
@ stub IoRegisterPlugPlayNotification
@ stub IoRegisterShutdownNotification
@ stub IoReleaseCancelSpinLock
@ stub IoReleaseRemoveLockAndWaitEx
@ stub IoReleaseRemoveLockEx
@ stub IoReleaseVpbSpinLock
@ stub IoRemoveShareAccess
@ stub IoReportDetectedDevice
@ stub IoReportHalResourceUsage
@ stub IoReportResourceForDetection
@ stub IoReportResourceUsage
@ stub IoReportTargetDeviceChange
@ stub IoReportTargetDeviceChangeAsynchronous
@ stub IoRequestDeviceEject
@ stub IoReuseIrp
@ stub IoSetCompletionRoutineEx
@ stub IoSetDeviceInterfaceState
@ stub IoSetDeviceToVerify
@ stub IoSetFileOrigin
@ stub IoSetHardErrorOrVerifyDevice
@ stub IoSetInformation
@ stub IoSetIoCompletion
@ stub IoSetPartitionInformationEx
@ stub IoSetShareAccess
@ stub IoSetStartIoAttributes
@ stub IoSetSystemPartition
@ stub IoSetThreadHardErrorMode
@ stub IoSetTopLevelIrp
@ stub IoStartNextPacket
@ stub IoStartNextPacketByKey
@ stub IoStartPacket
@ stub IoStartTimer
@ stub IoStatisticsLock
@ stub IoStopTimer
@ stub IoSynchronousInvalidateDeviceRelations
@ stub IoSynchronousPageWrite
@ stub IoThreadToProcess
@ stub IoUnregisterFileSystem
@ stub IoUnregisterFsRegistrationChange
@ stub IoUnregisterPlugPlayNotification
@ stub IoUnregisterShutdownNotification
@ stub IoUpdateShareAccess
@ stub IoValidateDeviceIoControlAccess
@ stub IoVerifyPartitionTable
@ stub IoVerifyVolume
@ stub IoVolumeDeviceToDosName
@ stub IoWMIAllocateInstanceIds
@ stub IoWMIDeviceObjectToInstanceName
@ stub IoWMIExecuteMethod
@ stub IoWMIHandleToInstanceName
@ stub IoWMIOpenBlock
@ stub IoWMIQueryAllData
@ stub IoWMIQueryAllDataMultiple
@ stub IoWMIQuerySingleInstance
@ stub IoWMIQuerySingleInstanceMultiple
@ stub IoWMIRegistrationControl
@ stub IoWMISetNotificationCallback
@ stub IoWMISetSingleInstance
@ stub IoWMISetSingleItem
@ stub IoWMISuggestInstanceName
@ stub IoWMIWriteEvent
@ stub IoWriteErrorLogEntry
@ stub IoWriteOperationCount
@ stub IoWritePartitionTableEx
@ stub IoWriteTransferCount
@ stub KdDebuggerEnabled
@ stub KdDebuggerNotPresent
@ stub KdDisableDebugger
@ stub KdEnableDebugger
@ stub KdEnteredDebugger
@ stub KdPollBreakIn
@ stub KdPowerTransition
@ stub Ke386CallBios
@ stub Ke386IoSetAccessProcess
@ stub Ke386QueryIoAccessMap
@ stub Ke386SetIoAccessMap
@ stub KeAcquireInterruptSpinLock
@ stub KeAcquireSpinLockAtDpcLevel
@ stub KeAddSystemServiceTable
@ stub KeAreApcsDisabled
@ stub KeAttachProcess
@ stub KeBugCheck
@ stub KeBugCheckEx
@ stub KeCancelTimer
@ stub KeCapturePersistentThreadState
@ stub KeClearEvent
@ stub KeConnectInterrupt
@ stub KeDcacheFlushCount
@ stub KeDelayExecutionThread
@ stub KeDeregisterBugCheckCallback
@ stub KeDeregisterBugCheckReasonCallback
@ stub KeDetachProcess
@ stub KeDisconnectInterrupt
@ stub KeEnterCriticalRegion
@ stub KeEnterKernelDebugger
@ stub KeFindConfigurationEntry
@ stub KeFindConfigurationNextEntry
@ stub KeFlushEntireTb
@ stub KeFlushQueuedDpcs
@ stub KeGetCurrentThread
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
@ stub KeInitializeApc
@ stub KeInitializeDeviceQueue
@ stub KeInitializeDpc
@ stub KeInitializeEvent
@ stub KeInitializeInterrupt
@ stub KeInitializeMutant
@ stub KeInitializeMutex
@ stub KeInitializeQueue
@ stub KeInitializeSemaphore
@ stub KeInitializeSpinLock
@ stub KeInitializeTimer
@ stub KeInitializeTimerEx
@ stub KeInsertByKeyDeviceQueue
@ stub KeInsertDeviceQueue
@ stub KeInsertHeadQueue
@ stub KeInsertQueue
@ stub KeInsertQueueApc
@ stub KeInsertQueueDpc
@ stub KeIsAttachedProcess
@ stub KeIsExecutingDpc
@ stub KeLeaveCriticalRegion
@ stub KeLoaderBlock
@ stub KeNumberProcessors
@ stub KeProfileInterrupt
@ stub KeProfileInterruptWithSource
@ stub KePulseEvent
@ stub KeQueryActiveProcessors
@ stub KeQueryInterruptTime
@ stub KeQueryPriorityThread
@ stub KeQueryRuntimeThread
@ stub KeQuerySystemTime
@ stub KeQueryTickCount
@ stub KeQueryTimeIncrement
@ stub KeRaiseUserException
@ stub KeReadStateEvent
@ stub KeReadStateMutant
@ stub KeReadStateMutex
@ stub KeReadStateQueue
@ stub KeReadStateSemaphore
@ stub KeReadStateTimer
@ stub KeRegisterBugCheckCallback
@ stub KeRegisterBugCheckReasonCallback
@ stub KeReleaseInterruptSpinLock
@ stub KeReleaseMutant
@ stub KeReleaseMutex
@ stub KeReleaseSemaphore
@ stub KeReleaseSpinLockFromDpcLevel
@ stub KeRemoveByKeyDeviceQueue
@ stub KeRemoveByKeyDeviceQueueIfBusy
@ stub KeRemoveDeviceQueue
@ stub KeRemoveEntryDeviceQueue
@ stub KeRemoveQueue
@ stub KeRemoveQueueDpc
@ stub KeRemoveSystemServiceTable
@ stub KeResetEvent
@ stub KeRestoreFloatingPointState
@ stub KeRevertToUserAffinityThread
@ stub KeRundownQueue
@ stub KeSaveFloatingPointState
@ stub KeSaveStateForHibernate
@ stub KeServiceDescriptorTable
@ stub KeSetAffinityThread
@ stub KeSetBasePriorityThread
@ stub KeSetDmaIoCoherency
@ stub KeSetEvent
@ stub KeSetEventBoostPriority
@ stub KeSetIdealProcessorThread
@ stub KeSetImportanceDpc
@ stub KeSetKernelStackSwapEnable
@ stub KeSetPriorityThread
@ stub KeSetProfileIrql
@ stub KeSetSystemAffinityThread
@ stub KeSetTargetProcessorDpc
@ stub KeSetTimeIncrement
@ stub KeSetTimer
@ stub KeSetTimerEx
@ stub KeStackAttachProcess
@ stub KeSynchronizeExecution
@ stub KeTerminateThread
@ stub KeTickCount
@ stub KeUnstackDetachProcess
@ stub KeUpdateRunTime
@ stub KeUpdateSystemTime
@ stub KeUserModeCallback
@ stub KeWaitForMultipleObjects
@ stub KeWaitForMutexObject
@ stub KeWaitForSingleObject
@ stub KiBugCheckData
@ stub KiCoprocessorError
@ stub KiDeliverApc
@ stub KiDispatchInterrupt
@ stub KiEnableTimerWatchdog
@ stub KiIpiServiceRoutine
@ stub KiUnexpectedInterrupt
@ stdcall LdrAccessResource(long ptr ptr ptr) ntdll.LdrAccessResource
@ stub LdrEnumResources
@ stdcall LdrFindResourceDirectory_U(long ptr long ptr) ntdll.LdrFindResourceDirectory_U
@ stdcall LdrFindResource_U(long ptr long ptr) ntdll.LdrFindResource_U
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
@ stub MmAllocateContiguousMemory
@ stub MmAllocateContiguousMemorySpecifyCache
@ stub MmAllocateMappingAddress
@ stub MmAllocateNonCachedMemory
@ stub MmAllocatePagesForMdl
@ stub MmBuildMdlForNonPagedPool
@ stub MmCanFileBeTruncated
@ stub MmCommitSessionMappedView
@ stub MmCreateMdl
@ stub MmCreateSection
@ stub MmDisableModifiedWriteOfSection
@ stub MmFlushImageSection
@ stub MmForceSectionClosed
@ stub MmFreeContiguousMemory
@ stub MmFreeContiguousMemorySpecifyCache
@ stub MmFreeMappingAddress
@ stub MmFreeNonCachedMemory
@ stub MmFreePagesFromMdl
@ stub MmGetPhysicalAddress
@ stub MmGetPhysicalMemoryRanges
@ stub MmGetSystemRoutineAddress
@ stub MmGetVirtualForPhysical
@ stub MmGrowKernelStack
@ stub MmHighestUserAddress
@ stub MmIsAddressValid
@ stub MmIsDriverVerifying
@ stub MmIsNonPagedSystemAddressValid
@ stub MmIsRecursiveIoFault
@ stub MmIsThisAnNtAsSystem
@ stub MmIsVerifierEnabled
@ stub MmLockPagableDataSection
@ stub MmLockPagableImageSection
@ stub MmLockPagableSectionByHandle
@ stub MmMapIoSpace
@ stub MmMapLockedPages
@ stub MmMapLockedPagesSpecifyCache
@ stub MmMapLockedPagesWithReservedMapping
@ stub MmMapMemoryDumpMdl
@ stub MmMapUserAddressesToPage
@ stub MmMapVideoDisplay
@ stub MmMapViewInSessionSpace
@ stub MmMapViewInSystemSpace
@ stub MmMapViewOfSection
@ stub MmMarkPhysicalMemoryAsBad
@ stub MmMarkPhysicalMemoryAsGood
@ stub MmPageEntireDriver
@ stub MmPrefetchPages
@ stub MmProbeAndLockPages
@ stub MmProbeAndLockProcessPages
@ stub MmProbeAndLockSelectedPages
@ stub MmProtectMdlSystemAddress
@ stub MmQuerySystemSize
@ stub MmRemovePhysicalMemory
@ stub MmResetDriverPaging
@ stub MmSectionObjectType
@ stub MmSecureVirtualMemory
@ stub MmSetAddressRangeModified
@ stub MmSetBankedSection
@ stub MmSizeOfMdl
@ stub MmSystemRangeStart
@ stub MmTrimAllSystemPagableMemory
@ stub MmUnlockPagableImageSection
@ stub MmUnlockPages
@ stub MmUnmapIoSpace
@ stub MmUnmapLockedPages
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
@ stdcall NtAddAtom(ptr long ptr) ntdll.NtAddAtom
@ stdcall NtAdjustPrivilegesToken(long long long long long long) ntdll.NtAdjustPrivilegesToken
@ stdcall NtAllocateLocallyUniqueId(ptr) ntdll.NtAllocateLocallyUniqueId
@ stdcall NtAllocateUuids(ptr ptr ptr) ntdll.NtAllocateUuids
@ stdcall NtAllocateVirtualMemory(long ptr ptr ptr long long) ntdll.NtAllocateVirtualMemory
@ stub NtBuildNumber
@ stdcall NtClose(long) ntdll.NtClose
@ stdcall NtConnectPort(ptr ptr ptr ptr ptr ptr ptr ptr) ntdll.NtConnectPort
@ stdcall NtCreateEvent(long long long long long) ntdll.NtCreateEvent
@ stdcall NtCreateFile(ptr long ptr ptr long long long ptr long long ptr) ntdll.NtCreateFile
@ stdcall NtCreateSection(ptr long ptr ptr long long long) ntdll.NtCreateSection
@ stdcall NtDeleteAtom(long) ntdll.NtDeleteAtom
@ stdcall NtDeleteFile(ptr) ntdll.NtDeleteFile
@ stdcall NtDeviceIoControlFile(long long long long long long long long long long) ntdll.NtDeviceIoControlFile
@ stdcall NtDuplicateObject(long long long ptr long long long) ntdll.NtDuplicateObject
@ stdcall NtDuplicateToken(long long long long long long) ntdll.NtDuplicateToken
@ stdcall NtFindAtom(ptr long ptr) ntdll.NtFindAtom
@ stdcall NtFreeVirtualMemory(long ptr ptr long) ntdll.NtFreeVirtualMemory
@ stdcall NtFsControlFile(long long long long long long long long long long) ntdll.NtFsControlFile
@ stub NtGlobalFlag
@ stdcall NtLockFile(long long ptr ptr ptr ptr ptr ptr long long) ntdll.NtLockFile
@ stub NtMakePermanentObject
@ stdcall NtMapViewOfSection(long long ptr long long ptr ptr long long long) ntdll.NtMapViewOfSection
@ stdcall NtNotifyChangeDirectoryFile(long long ptr ptr ptr ptr long long long) ntdll.NtNotifyChangeDirectoryFile
@ stdcall NtOpenFile(ptr long ptr ptr long long) ntdll.NtOpenFile
@ stdcall NtOpenProcess(ptr long ptr ptr) ntdll.NtOpenProcess
@ stdcall NtOpenProcessToken(long long long) ntdll.NtOpenProcessToken
@ stub NtOpenProcessTokenEx
@ stdcall NtOpenThread(ptr long ptr ptr) ntdll.NtOpenThread
@ stdcall NtOpenThreadToken(long long long long) ntdll.NtOpenThreadToken
@ stub NtOpenThreadTokenEx
@ stdcall NtQueryDirectoryFile(long long ptr ptr ptr ptr long long long ptr long) ntdll.NtQueryDirectoryFile
@ stub NtQueryEaFile
@ stdcall NtQueryInformationAtom(long long ptr long ptr) ntdll.NtQueryInformationAtom
@ stdcall NtQueryInformationFile(long ptr ptr long long) ntdll.NtQueryInformationFile
@ stdcall NtQueryInformationProcess(long long ptr long ptr) ntdll.NtQueryInformationProcess
@ stdcall NtQueryInformationThread(long long ptr long ptr) ntdll.NtQueryInformationThread
@ stdcall NtQueryInformationToken(long long ptr long ptr) ntdll.NtQueryInformationToken
@ stub NtQueryQuotaInformationFile
@ stdcall NtQuerySecurityObject(long long long long long) ntdll.NtQuerySecurityObject
@ stdcall NtQuerySystemInformation(long long long long) ntdll.NtQuerySystemInformation
@ stdcall NtQueryVolumeInformationFile(long ptr ptr long long) ntdll.NtQueryVolumeInformationFile
@ stdcall NtReadFile(long long long long long long long long long) ntdll.NtReadFile
@ stub NtRequestPort
@ stdcall NtRequestWaitReplyPort(ptr ptr ptr) ntdll.NtRequestWaitReplyPort
@ stub NtSetEaFile
@ stdcall NtSetEvent(long long) ntdll.NtSetEvent
@ stdcall NtSetInformationFile(long long long long long) ntdll.NtSetInformationFile
@ stdcall NtSetInformationProcess(long long long long) ntdll.NtSetInformationProcess
@ stdcall NtSetInformationThread(long long ptr long) ntdll.NtSetInformationThread
@ stub NtSetQuotaInformationFile
@ stdcall NtSetSecurityObject(long long ptr) ntdll.NtSetSecurityObject
@ stdcall NtSetVolumeInformationFile(long ptr ptr long long) ntdll.NtSetVolumeInformationFile
@ stdcall NtShutdownSystem(long) ntdll.NtShutdownSystem
@ stub NtTraceEvent
@ stdcall NtUnlockFile(long ptr ptr ptr ptr) ntdll.NtUnlockFile
@ stub NtVdmControl
@ stdcall NtWaitForSingleObject(long long long) ntdll.NtWaitForSingleObject
@ stdcall NtWriteFile(long long ptr ptr ptr ptr long ptr ptr) ntdll.NtWriteFile
@ stub ObAssignSecurity
@ stub ObCheckCreateObjectAccess
@ stub ObCheckObjectAccess
@ stub ObCloseHandle
@ stub ObCreateObject
@ stub ObCreateObjectType
@ stub ObDereferenceObject
@ stub ObDereferenceSecurityDescriptor
@ stub ObFindHandleForObject
@ stub ObGetObjectSecurity
@ stub ObInsertObject
@ stub ObLogSecurityDescriptor
@ stub ObMakeTemporaryObject
@ stub ObOpenObjectByName
@ stub ObOpenObjectByPointer
@ stub ObQueryNameString
@ stub ObQueryObjectAuditingByHandle
@ stub ObReferenceObjectByHandle
@ stub ObReferenceObjectByName
@ stub ObReferenceObjectByPointer
@ stub ObReferenceSecurityDescriptor
@ stub ObReleaseObjectSecurity
@ stub ObSetHandleAttributes
@ stub ObSetSecurityDescriptorInfo
@ stub ObSetSecurityObjectByPointer
@ stub PfxFindPrefix
@ stub PfxInitialize
@ stub PfxInsertPrefix
@ stub PfxRemovePrefix
@ stub PoCallDriver
@ stub PoCancelDeviceNotify
@ stub PoQueueShutdownWorkItem
@ stub PoRegisterDeviceForIdleDetection
@ stub PoRegisterDeviceNotify
@ stub PoRegisterSystemState
@ stub PoRequestPowerIrp
@ stub PoRequestShutdownEvent
@ stub PoSetHiberRange
@ stub PoSetPowerState
@ stub PoSetSystemState
@ stub PoShutdownBugCheck
@ stub PoStartNextPowerIrp
@ stub PoUnregisterSystemState
@ stub ProbeForRead
@ stub ProbeForWrite
@ stub PsAssignImpersonationToken
@ stub PsChargePoolQuota
@ stub PsChargeProcessNonPagedPoolQuota
@ stub PsChargeProcessPagedPoolQuota
@ stub PsChargeProcessPoolQuota
@ stub PsCreateSystemProcess
@ stub PsCreateSystemThread
@ stub PsDereferenceImpersonationToken
@ stub PsDereferencePrimaryToken
@ stub PsDisableImpersonation
@ stub PsEstablishWin32Callouts
@ stub PsGetContextThread
@ stub PsGetCurrentProcess
@ stub PsGetCurrentProcessId
@ stub PsGetCurrentProcessSessionId
@ stub PsGetCurrentThread
@ stub PsGetCurrentThreadId
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
@ stub PsGetProcessId
@ stub PsGetProcessImageFileName
@ stub PsGetProcessInheritedFromUniqueProcessId
@ stub PsGetProcessJob
@ stub PsGetProcessPeb
@ stub PsGetProcessPriorityClass
@ stub PsGetProcessSectionBaseAddress
@ stub PsGetProcessSecurityPort
@ stub PsGetProcessSessionId
@ stub PsGetProcessWin32Process
@ stub PsGetProcessWin32WindowStation
@ stub PsGetThreadFreezeCount
@ stub PsGetThreadHardErrorsAreDisabled
@ stub PsGetThreadId
@ stub PsGetThreadProcess
@ stub PsGetThreadProcessId
@ stub PsGetThreadSessionId
@ stub PsGetThreadTeb
@ stub PsGetThreadWin32Thread
@ stub PsGetVersion
@ stub PsImpersonateClient
@ stub PsInitialSystemProcess
@ stub PsIsProcessBeingDebugged
@ stub PsIsSystemThread
@ stub PsIsThreadImpersonating
@ stub PsIsThreadTerminating
@ stub PsJobType
@ stub PsLookupProcessByProcessId
@ stub PsLookupProcessThreadByCid
@ stub PsLookupThreadByThreadId
@ stub PsProcessType
@ stub PsReferenceImpersonationToken
@ stub PsReferencePrimaryToken
@ stub PsRemoveCreateThreadNotifyRoutine
@ stub PsRemoveLoadImageNotifyRoutine
@ stub PsRestoreImpersonation
@ stub PsReturnPoolQuota
@ stub PsReturnProcessNonPagedPoolQuota
@ stub PsReturnProcessPagedPoolQuota
@ stub PsRevertThreadToSelf
@ stub PsRevertToSelf
@ stub PsSetContextThread
@ stub PsSetCreateProcessNotifyRoutine
@ stub PsSetCreateThreadNotifyRoutine
@ stub PsSetJobUIRestrictionsClass
@ stub PsSetLegoNotifyRoutine
@ stub PsSetLoadImageNotifyRoutine
@ stub PsSetProcessPriorityByClass
@ stub PsSetProcessPriorityClass
@ stub PsSetProcessSecurityPort
@ stub PsSetProcessWin32Process
@ stub PsSetProcessWindowStation
@ stub PsSetThreadHardErrorsAreDisabled
@ stub PsSetThreadWin32Thread
@ stub PsTerminateSystemThread
@ stub PsThreadType
@ stub READ_REGISTER_BUFFER_UCHAR
@ stub READ_REGISTER_BUFFER_ULONG
@ stub READ_REGISTER_BUFFER_USHORT
@ stub READ_REGISTER_UCHAR
@ stub READ_REGISTER_ULONG
@ stub READ_REGISTER_USHORT
@ stdcall RtlAbsoluteToSelfRelativeSD(ptr ptr ptr) ntdll.RtlAbsoluteToSelfRelativeSD
@ stdcall RtlAddAccessAllowedAce(ptr long long ptr) ntdll.RtlAddAccessAllowedAce
@ stdcall RtlAddAccessAllowedAceEx(ptr long long long ptr) ntdll.RtlAddAccessAllowedAceEx
@ stdcall RtlAddAce(ptr long long ptr long) ntdll.RtlAddAce
@ stdcall RtlAddAtomToAtomTable(ptr wstr ptr) ntdll.RtlAddAtomToAtomTable
@ stub RtlAddRange
@ stdcall RtlAllocateHeap(long long long) ntdll.RtlAllocateHeap
@ stdcall RtlAnsiCharToUnicodeChar(ptr) ntdll.RtlAnsiCharToUnicodeChar
@ stdcall RtlAnsiStringToUnicodeSize(ptr) ntdll.RtlAnsiStringToUnicodeSize
@ stdcall RtlAnsiStringToUnicodeString(ptr ptr long) ntdll.RtlAnsiStringToUnicodeString
@ stdcall RtlAppendAsciizToString(ptr str) ntdll.RtlAppendAsciizToString
@ stdcall RtlAppendStringToString(ptr ptr) ntdll.RtlAppendStringToString
@ stdcall RtlAppendUnicodeStringToString(ptr ptr) ntdll.RtlAppendUnicodeStringToString
@ stdcall RtlAppendUnicodeToString(ptr wstr) ntdll.RtlAppendUnicodeToString
@ stdcall RtlAreAllAccessesGranted(long long) ntdll.RtlAreAllAccessesGranted
@ stdcall RtlAreAnyAccessesGranted(long long) ntdll.RtlAreAnyAccessesGranted
@ stdcall RtlAreBitsClear(ptr long long) ntdll.RtlAreBitsClear
@ stdcall RtlAreBitsSet(ptr long long) ntdll.RtlAreBitsSet
@ stdcall RtlAssert(ptr ptr long long) ntdll.RtlAssert
@ stub RtlCaptureContext
@ stub RtlCaptureStackBackTrace
@ stdcall RtlCharToInteger(ptr long ptr) ntdll.RtlCharToInteger
@ stdcall RtlCheckRegistryKey(long ptr) ntdll.RtlCheckRegistryKey
@ stdcall RtlClearAllBits(ptr) ntdll.RtlClearAllBits
@ stub RtlClearBit
@ stdcall RtlClearBits(ptr long long) ntdll.RtlClearBits
@ stdcall RtlCompareMemory(ptr ptr long) ntdll.RtlCompareMemory
@ stdcall RtlCompareMemoryUlong(ptr long long) ntdll.RtlCompareMemoryUlong
@ stdcall RtlCompareString(ptr ptr long) ntdll.RtlCompareString
@ stdcall RtlCompareUnicodeString(ptr ptr long) ntdll.RtlCompareUnicodeString
@ stub RtlCompressBuffer
@ stub RtlCompressChunks
@ stdcall -ret64 RtlConvertLongToLargeInteger(long) ntdll.RtlConvertLongToLargeInteger
@ stdcall RtlConvertSidToUnicodeString(ptr ptr long) ntdll.RtlConvertSidToUnicodeString
@ stdcall -ret64 RtlConvertUlongToLargeInteger(long) ntdll.RtlConvertUlongToLargeInteger
@ stdcall RtlCopyLuid(ptr ptr) ntdll.RtlCopyLuid
@ stub RtlCopyRangeList
@ stdcall RtlCopySid(long ptr ptr) ntdll.RtlCopySid
@ stdcall RtlCopyString(ptr ptr) ntdll.RtlCopyString
@ stdcall RtlCopyUnicodeString(ptr ptr) ntdll.RtlCopyUnicodeString
@ stdcall RtlCreateAcl(ptr long long) ntdll.RtlCreateAcl
@ stdcall RtlCreateAtomTable(long ptr) ntdll.RtlCreateAtomTable
@ stdcall RtlCreateHeap(long ptr long long ptr ptr) ntdll.RtlCreateHeap
@ stub RtlCreateRegistryKey
@ stdcall RtlCreateSecurityDescriptor(ptr long) ntdll.RtlCreateSecurityDescriptor
@ stub RtlCreateSystemVolumeInformationFolder
@ stdcall RtlCreateUnicodeString(ptr wstr) ntdll.RtlCreateUnicodeString
@ stub RtlCustomCPToUnicodeN
@ stub RtlDecompressBuffer
@ stub RtlDecompressChunks
@ stub RtlDecompressFragment
@ stub RtlDelete
@ stdcall RtlDeleteAce(ptr long) ntdll.RtlDeleteAce
@ stdcall RtlDeleteAtomFromAtomTable(ptr long) ntdll.RtlDeleteAtomFromAtomTable
@ stub RtlDeleteElementGenericTable
@ stub RtlDeleteElementGenericTableAvl
@ stub RtlDeleteNoSplay
@ stub RtlDeleteOwnersRanges
@ stub RtlDeleteRange
@ stdcall RtlDeleteRegistryValue(long ptr ptr) ntdll.RtlDeleteRegistryValue
@ stub RtlDescribeChunk
@ stdcall RtlDestroyAtomTable(ptr) ntdll.RtlDestroyAtomTable
@ stdcall RtlDestroyHeap(long) ntdll.RtlDestroyHeap
@ stdcall RtlDowncaseUnicodeString(ptr ptr long) ntdll.RtlDowncaseUnicodeString
@ stdcall RtlEmptyAtomTable(ptr long) ntdll.RtlEmptyAtomTable
@ stdcall -ret64 RtlEnlargedIntegerMultiply(long long) ntdll.RtlEnlargedIntegerMultiply
@ stdcall RtlEnlargedUnsignedDivide(double long ptr) ntdll.RtlEnlargedUnsignedDivide
@ stdcall -ret64 RtlEnlargedUnsignedMultiply(long long) ntdll.RtlEnlargedUnsignedMultiply
@ stub RtlEnumerateGenericTable
@ stub RtlEnumerateGenericTableAvl
@ stub RtlEnumerateGenericTableLikeADirectory
@ stub RtlEnumerateGenericTableWithoutSplaying
@ stub RtlEnumerateGenericTableWithoutSplayingAvl
@ stdcall RtlEqualLuid(ptr ptr) ntdll.RtlEqualLuid
@ stdcall RtlEqualSid(long long) ntdll.RtlEqualSid
@ stdcall RtlEqualString(ptr ptr long) ntdll.RtlEqualString
@ stdcall RtlEqualUnicodeString(ptr ptr long) ntdll.RtlEqualUnicodeString
@ stdcall -ret64 RtlExtendedIntegerMultiply(double long) ntdll.RtlExtendedIntegerMultiply
@ stdcall -ret64 RtlExtendedLargeIntegerDivide(double long ptr) ntdll.RtlExtendedLargeIntegerDivide
@ stdcall -ret64 RtlExtendedMagicDivide(double double long) ntdll.RtlExtendedMagicDivide
@ stdcall RtlFillMemory(ptr long long) ntdll.RtlFillMemory
@ stdcall RtlFillMemoryUlong(ptr long long) ntdll.RtlFillMemoryUlong
@ stdcall RtlFindClearBits(ptr long long) ntdll.RtlFindClearBits
@ stdcall RtlFindClearBitsAndSet(ptr long long) ntdll.RtlFindClearBitsAndSet
@ stdcall RtlFindClearRuns(ptr ptr long long) ntdll.RtlFindClearRuns
@ stub RtlFindFirstRunClear
@ stdcall RtlFindLastBackwardRunClear(ptr long ptr) ntdll.RtlFindLastBackwardRunClear
@ stdcall RtlFindLeastSignificantBit(double) ntdll.RtlFindLeastSignificantBit
@ stdcall RtlFindLongestRunClear(ptr long) ntdll.RtlFindLongestRunClear
@ stdcall RtlFindMessage(long long long long ptr) ntdll.RtlFindMessage
@ stdcall RtlFindMostSignificantBit(double) ntdll.RtlFindMostSignificantBit
@ stdcall RtlFindNextForwardRunClear(ptr long ptr) ntdll.RtlFindNextForwardRunClear
@ stub RtlFindRange
@ stdcall RtlFindSetBits(ptr long long) ntdll.RtlFindSetBits
@ stdcall RtlFindSetBitsAndClear(ptr long long) ntdll.RtlFindSetBitsAndClear
@ stub RtlFindUnicodePrefix
@ stdcall RtlFormatCurrentUserKeyPath(ptr) ntdll.RtlFormatCurrentUserKeyPath
@ stdcall RtlFreeAnsiString(long) ntdll.RtlFreeAnsiString
@ stdcall RtlFreeHeap(long long long) ntdll.RtlFreeHeap
@ stdcall RtlFreeOemString(ptr) ntdll.RtlFreeOemString
@ stub RtlFreeRangeList
@ stdcall RtlFreeUnicodeString(ptr) ntdll.RtlFreeUnicodeString
@ stdcall RtlGUIDFromString(ptr ptr) ntdll.RtlGUIDFromString
@ stub RtlGenerate8dot3Name
@ stdcall RtlGetAce(ptr long ptr) ntdll.RtlGetAce
@ stub RtlGetCallersAddress
@ stub RtlGetCompressionWorkSpaceSize
@ stdcall RtlGetDaclSecurityDescriptor(ptr ptr ptr ptr) ntdll.RtlGetDaclSecurityDescriptor
@ stub RtlGetDefaultCodePage
@ stub RtlGetElementGenericTable
@ stub RtlGetElementGenericTableAvl
@ stub RtlGetFirstRange
@ stdcall RtlGetGroupSecurityDescriptor(ptr ptr ptr) ntdll.RtlGetGroupSecurityDescriptor
@ stub RtlGetNextRange
@ stub RtlGetNtGlobalFlags
@ stdcall RtlGetOwnerSecurityDescriptor(ptr ptr ptr) ntdll.RtlGetOwnerSecurityDescriptor
@ stdcall RtlGetSaclSecurityDescriptor(ptr ptr ptr ptr) ntdll.RtlGetSaclSecurityDescriptor
@ stub RtlGetSetBootStatusData
@ stdcall RtlGetVersion(ptr) ntdll.RtlGetVersion
@ stub RtlHashUnicodeString
@ stdcall RtlImageDirectoryEntryToData(long long long ptr) ntdll.RtlImageDirectoryEntryToData
@ stdcall RtlImageNtHeader(long) ntdll.RtlImageNtHeader
@ stdcall RtlInitAnsiString(ptr str) ntdll.RtlInitAnsiString
@ stub RtlInitCodePageTable
@ stdcall RtlInitString(ptr str) ntdll.RtlInitString
@ stdcall RtlInitUnicodeString(ptr wstr) ntdll.RtlInitUnicodeString
@ stdcall RtlInitializeBitMap(ptr long long) ntdll.RtlInitializeBitMap
@ stdcall RtlInitializeGenericTable(ptr ptr ptr ptr ptr) ntdll.RtlInitializeGenericTable
@ stub RtlInitializeGenericTableAvl
@ stub RtlInitializeRangeList
@ stdcall RtlInitializeSid(ptr ptr long) ntdll.RtlInitializeSid
@ stub RtlInitializeUnicodePrefix
@ stub RtlInsertElementGenericTable
@ stub RtlInsertElementGenericTableAvl
@ stub RtlInsertElementGenericTableFull
@ stub RtlInsertElementGenericTableFullAvl
@ stub RtlInsertUnicodePrefix
@ stdcall RtlInt64ToUnicodeString(double long ptr) ntdll.RtlInt64ToUnicodeString
@ stdcall RtlIntegerToChar(long long long ptr) ntdll.RtlIntegerToChar
@ stub RtlIntegerToUnicode
@ stdcall RtlIntegerToUnicodeString(long long ptr) ntdll.RtlIntegerToUnicodeString
@ stub RtlInvertRangeList
@ stub RtlIpv4AddressToStringA
@ stub RtlIpv4AddressToStringExA
@ stdcall RtlIpv4AddressToStringExW(ptr ptr ptr ptr) ntdll.RtlIpv4AddressToStringExW
@ stub RtlIpv4AddressToStringW
@ stub RtlIpv4StringToAddressA
@ stub RtlIpv4StringToAddressExA
@ stdcall RtlIpv4StringToAddressExW(ptr ptr wstr ptr) ntdll.RtlIpv4StringToAddressExW
@ stub RtlIpv4StringToAddressW
@ stub RtlIpv6AddressToStringA
@ stub RtlIpv6AddressToStringExA
@ stub RtlIpv6AddressToStringExW
@ stub RtlIpv6AddressToStringW
@ stub RtlIpv6StringToAddressA
@ stub RtlIpv6StringToAddressExA
@ stub RtlIpv6StringToAddressExW
@ stub RtlIpv6StringToAddressW
@ stub RtlIsGenericTableEmpty
@ stub RtlIsGenericTableEmptyAvl
@ stdcall RtlIsNameLegalDOS8Dot3(ptr ptr ptr) ntdll.RtlIsNameLegalDOS8Dot3
@ stub RtlIsRangeAvailable
@ stub RtlIsValidOemCharacter
@ stdcall -ret64 RtlLargeIntegerAdd(double double) ntdll.RtlLargeIntegerAdd
@ stdcall -ret64 RtlLargeIntegerArithmeticShift(double long) ntdll.RtlLargeIntegerArithmeticShift
@ stdcall -ret64 RtlLargeIntegerDivide(double double ptr) ntdll.RtlLargeIntegerDivide
@ stdcall -ret64 RtlLargeIntegerNegate(double) ntdll.RtlLargeIntegerNegate
@ stdcall -ret64 RtlLargeIntegerShiftLeft(double long) ntdll.RtlLargeIntegerShiftLeft
@ stdcall -ret64 RtlLargeIntegerShiftRight(double long) ntdll.RtlLargeIntegerShiftRight
@ stdcall -ret64 RtlLargeIntegerSubtract(double double) ntdll.RtlLargeIntegerSubtract
@ stdcall RtlLengthRequiredSid(long) ntdll.RtlLengthRequiredSid
@ stdcall RtlLengthSecurityDescriptor(ptr) ntdll.RtlLengthSecurityDescriptor
@ stdcall RtlLengthSid(ptr) ntdll.RtlLengthSid
@ stub RtlLockBootStatusData
@ stdcall RtlLookupAtomInAtomTable(ptr wstr ptr) ntdll.RtlLookupAtomInAtomTable
@ stub RtlLookupElementGenericTable
@ stub RtlLookupElementGenericTableAvl
@ stub RtlLookupElementGenericTableFull
@ stub RtlLookupElementGenericTableFullAvl
@ stdcall RtlMapGenericMask(long ptr) ntdll.RtlMapGenericMask
@ stub RtlMapSecurityErrorToNtStatus
@ stub RtlMergeRangeLists
@ stdcall RtlMoveMemory(ptr ptr long) ntdll.RtlMoveMemory
@ stdcall RtlMultiByteToUnicodeN(ptr long ptr ptr long) ntdll.RtlMultiByteToUnicodeN
@ stdcall RtlMultiByteToUnicodeSize(ptr str long) ntdll.RtlMultiByteToUnicodeSize
@ stub RtlNextUnicodePrefix
@ stdcall RtlNtStatusToDosError(long) ntdll.RtlNtStatusToDosError
@ stdcall RtlNtStatusToDosErrorNoTeb(long) ntdll.RtlNtStatusToDosErrorNoTeb
@ stub RtlNumberGenericTableElements
@ stub RtlNumberGenericTableElementsAvl
@ stdcall RtlNumberOfClearBits(ptr) ntdll.RtlNumberOfClearBits
@ stdcall RtlNumberOfSetBits(ptr) ntdll.RtlNumberOfSetBits
@ stub RtlOemStringToCountedUnicodeString
@ stdcall RtlOemStringToUnicodeSize(ptr) ntdll.RtlOemStringToUnicodeSize
@ stdcall RtlOemStringToUnicodeString(ptr ptr long) ntdll.RtlOemStringToUnicodeString
@ stdcall RtlOemToUnicodeN(ptr long ptr ptr long) ntdll.RtlOemToUnicodeN
@ stdcall RtlPinAtomInAtomTable(ptr long) ntdll.RtlPinAtomInAtomTable
@ stdcall RtlPrefixString(ptr ptr long) ntdll.RtlPrefixString
@ stdcall RtlPrefixUnicodeString(ptr ptr long) ntdll.RtlPrefixUnicodeString
@ stdcall RtlQueryAtomInAtomTable(ptr long ptr ptr ptr ptr) ntdll.RtlQueryAtomInAtomTable
@ stdcall RtlQueryRegistryValues(long ptr ptr ptr ptr) ntdll.RtlQueryRegistryValues
@ stdcall RtlQueryTimeZoneInformation(ptr) ntdll.RtlQueryTimeZoneInformation
@ stdcall -register RtlRaiseException(ptr) ntdll.RtlRaiseException
@ stdcall RtlRandom(ptr) ntdll.RtlRandom
@ stub RtlRandomEx
@ stub RtlRealPredecessor
@ stub RtlRealSuccessor
@ stub RtlRemoveUnicodePrefix
@ stub RtlReserveChunk
@ stdcall RtlSecondsSince1970ToTime(long ptr) ntdll.RtlSecondsSince1970ToTime
@ stdcall RtlSecondsSince1980ToTime(long ptr) ntdll.RtlSecondsSince1980ToTime
@ stub RtlSelfRelativeToAbsoluteSD2
@ stdcall RtlSelfRelativeToAbsoluteSD(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) ntdll.RtlSelfRelativeToAbsoluteSD
@ stdcall RtlSetAllBits(ptr) ntdll.RtlSetAllBits
@ stub RtlSetBit
@ stdcall RtlSetBits(ptr long long) ntdll.RtlSetBits
@ stdcall RtlSetDaclSecurityDescriptor(ptr long ptr long) ntdll.RtlSetDaclSecurityDescriptor
@ stdcall RtlSetGroupSecurityDescriptor(ptr ptr long) ntdll.RtlSetGroupSecurityDescriptor
@ stdcall RtlSetOwnerSecurityDescriptor(ptr ptr long) ntdll.RtlSetOwnerSecurityDescriptor
@ stdcall RtlSetSaclSecurityDescriptor(ptr long ptr long) ntdll.RtlSetSaclSecurityDescriptor
@ stdcall RtlSetTimeZoneInformation(ptr) ntdll.RtlSetTimeZoneInformation
@ stdcall RtlSizeHeap(long long ptr) ntdll.RtlSizeHeap
@ stub RtlSplay
@ stdcall RtlStringFromGUID(ptr ptr) ntdll.RtlStringFromGUID
@ stdcall RtlSubAuthorityCountSid(ptr) ntdll.RtlSubAuthorityCountSid
@ stdcall RtlSubAuthoritySid(ptr long) ntdll.RtlSubAuthoritySid
@ stub RtlSubtreePredecessor
@ stub RtlSubtreeSuccessor
@ stub RtlTestBit
@ stdcall RtlTimeFieldsToTime(ptr ptr) ntdll.RtlTimeFieldsToTime
@ stdcall RtlTimeToElapsedTimeFields(long long) ntdll.RtlTimeToElapsedTimeFields
@ stdcall RtlTimeToSecondsSince1970(ptr ptr) ntdll.RtlTimeToSecondsSince1970
@ stdcall RtlTimeToSecondsSince1980(ptr ptr) ntdll.RtlTimeToSecondsSince1980
@ stdcall RtlTimeToTimeFields(long long) ntdll.RtlTimeToTimeFields
@ stub RtlTraceDatabaseAdd
@ stub RtlTraceDatabaseCreate
@ stub RtlTraceDatabaseDestroy
@ stub RtlTraceDatabaseEnumerate
@ stub RtlTraceDatabaseFind
@ stub RtlTraceDatabaseLock
@ stub RtlTraceDatabaseUnlock
@ stub RtlTraceDatabaseValidate
@ stdcall RtlUnicodeStringToAnsiSize(ptr) ntdll.RtlUnicodeStringToAnsiSize
@ stdcall RtlUnicodeStringToAnsiString(ptr ptr long) ntdll.RtlUnicodeStringToAnsiString
@ stub RtlUnicodeStringToCountedOemString
@ stdcall RtlUnicodeStringToInteger(ptr long ptr) ntdll.RtlUnicodeStringToInteger
@ stdcall RtlUnicodeStringToOemSize(ptr) ntdll.RtlUnicodeStringToOemSize
@ stdcall RtlUnicodeStringToOemString(ptr ptr long) ntdll.RtlUnicodeStringToOemString
@ stub RtlUnicodeToCustomCPN
@ stdcall RtlUnicodeToMultiByteN(ptr long ptr ptr long) ntdll.RtlUnicodeToMultiByteN
@ stdcall RtlUnicodeToMultiByteSize(ptr ptr long) ntdll.RtlUnicodeToMultiByteSize
@ stdcall RtlUnicodeToOemN(ptr long ptr ptr long) ntdll.RtlUnicodeToOemN
@ stub RtlUnlockBootStatusData
@ stdcall -register RtlUnwind(ptr ptr ptr ptr) ntdll.RtlUnwind
@ stdcall RtlUpcaseUnicodeChar(long) ntdll.RtlUpcaseUnicodeChar
@ stdcall RtlUpcaseUnicodeString(ptr ptr long) ntdll.RtlUpcaseUnicodeString
@ stdcall RtlUpcaseUnicodeStringToAnsiString(ptr ptr long) ntdll.RtlUpcaseUnicodeStringToAnsiString
@ stdcall RtlUpcaseUnicodeStringToCountedOemString(ptr ptr long) ntdll.RtlUpcaseUnicodeStringToCountedOemString
@ stdcall RtlUpcaseUnicodeStringToOemString(ptr ptr long) ntdll.RtlUpcaseUnicodeStringToOemString
@ stub RtlUpcaseUnicodeToCustomCPN
@ stdcall RtlUpcaseUnicodeToMultiByteN(ptr long ptr ptr long) ntdll.RtlUpcaseUnicodeToMultiByteN
@ stdcall RtlUpcaseUnicodeToOemN(ptr long ptr ptr long) ntdll.RtlUpcaseUnicodeToOemN
@ stdcall RtlUpperChar(long) ntdll.RtlUpperChar
@ stdcall RtlUpperString(ptr ptr) ntdll.RtlUpperString
@ stub RtlValidRelativeSecurityDescriptor
@ stdcall RtlValidSecurityDescriptor(ptr) ntdll.RtlValidSecurityDescriptor
@ stdcall RtlValidSid(ptr) ntdll.RtlValidSid
@ stdcall RtlVerifyVersionInfo(ptr long double) ntdll.RtlVerifyVersionInfo
@ stub RtlVolumeDeviceToDosName
@ stub RtlWalkFrameChain
@ stub RtlWriteRegistryValue
@ stub RtlZeroHeap
@ stdcall RtlZeroMemory(ptr long) ntdll.RtlZeroMemory
@ stdcall RtlxAnsiStringToUnicodeSize(ptr) ntdll.RtlxAnsiStringToUnicodeSize
@ stdcall RtlxOemStringToUnicodeSize(ptr) ntdll.RtlxOemStringToUnicodeSize
@ stdcall RtlxUnicodeStringToAnsiSize(ptr) ntdll.RtlxUnicodeStringToAnsiSize
@ stdcall RtlxUnicodeStringToOemSize(ptr) ntdll.RtlxUnicodeStringToOemSize
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
@ stub SeLockSubjectContext
@ stub SeMarkLogonSessionForTerminationNotification
@ stub SeOpenObjectAuditAlarm
@ stub SeOpenObjectForDeleteAuditAlarm
@ stub SePrivilegeCheck
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
@ stub SeSinglePrivilegeCheck
@ stub SeSystemDefaultDacl
@ stub SeTokenImpersonationLevel
@ stub SeTokenIsAdmin
@ stub SeTokenIsRestricted
@ stub SeTokenIsWriteRestricted
@ stub SeTokenObjectType
@ stub SeTokenType
@ stub SeUnlockSubjectContext
@ stub SeUnregisterLogonSessionTerminatedRoutine
@ stub SeValidSecurityDescriptor
@ stdcall -ret64 VerSetConditionMask(double long long) ntdll.VerSetConditionMask
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
@ stdcall ZwAccessCheckAndAuditAlarm(ptr long ptr ptr ptr long ptr long ptr ptr ptr) ntdll.ZwAccessCheckAndAuditAlarm
@ stub ZwAddBootEntry
@ stdcall ZwAdjustPrivilegesToken(long long long long long long) ntdll.ZwAdjustPrivilegesToken
@ stdcall ZwAlertThread(long) ntdll.ZwAlertThread
@ stdcall ZwAllocateVirtualMemory(long ptr ptr ptr long long) ntdll.ZwAllocateVirtualMemory
@ stub ZwAssignProcessToJobObject
@ stdcall ZwCancelIoFile(long ptr) ntdll.ZwCancelIoFile
@ stdcall ZwCancelTimer(long ptr) ntdll.ZwCancelTimer
@ stdcall ZwClearEvent(long) ntdll.ZwClearEvent
@ stdcall ZwClose(long) ntdll.ZwClose
@ stub ZwCloseObjectAuditAlarm
@ stdcall ZwConnectPort(ptr ptr ptr ptr ptr ptr ptr ptr) ntdll.ZwConnectPort
@ stdcall ZwCreateDirectoryObject(long long long) ntdll.ZwCreateDirectoryObject
@ stdcall ZwCreateEvent(long long long long long) ntdll.ZwCreateEvent
@ stdcall ZwCreateFile(ptr long ptr ptr long long long ptr long long ptr) ntdll.ZwCreateFile
@ stub ZwCreateJobObject
@ stdcall ZwCreateKey(ptr long ptr long ptr long long) ntdll.ZwCreateKey
@ stdcall ZwCreateSection(ptr long ptr ptr long long long) ntdll.ZwCreateSection
@ stdcall ZwCreateSymbolicLinkObject(ptr long ptr ptr) ntdll.ZwCreateSymbolicLinkObject
@ stdcall ZwCreateTimer(ptr long ptr long) ntdll.ZwCreateTimer
@ stub ZwDeleteBootEntry
@ stdcall ZwDeleteFile(ptr) ntdll.ZwDeleteFile
@ stdcall ZwDeleteKey(long) ntdll.ZwDeleteKey
@ stdcall ZwDeleteValueKey(long ptr) ntdll.ZwDeleteValueKey
@ stdcall ZwDeviceIoControlFile(long long long long long long long long long long) ntdll.ZwDeviceIoControlFile
@ stdcall ZwDisplayString(ptr) ntdll.ZwDisplayString
@ stdcall ZwDuplicateObject(long long long ptr long long long) ntdll.ZwDuplicateObject
@ stdcall ZwDuplicateToken(long long long long long long) ntdll.ZwDuplicateToken
@ stub ZwEnumerateBootEntries
@ stdcall ZwEnumerateKey(long long long ptr long ptr) ntdll.ZwEnumerateKey
@ stdcall ZwEnumerateValueKey(long long long ptr long ptr) ntdll.ZwEnumerateValueKey
@ stdcall ZwFlushInstructionCache(long ptr long) ntdll.ZwFlushInstructionCache
@ stdcall ZwFlushKey(long) ntdll.ZwFlushKey
@ stdcall ZwFlushVirtualMemory(long ptr ptr long) ntdll.ZwFlushVirtualMemory
@ stdcall ZwFreeVirtualMemory(long ptr ptr long) ntdll.ZwFreeVirtualMemory
@ stdcall ZwFsControlFile(long long long long long long long long long long) ntdll.ZwFsControlFile
@ stub ZwInitiatePowerAction
@ stub ZwIsProcessInJob
@ stdcall ZwLoadDriver(ptr) ntdll.ZwLoadDriver
@ stdcall ZwLoadKey(ptr ptr) ntdll.ZwLoadKey
@ stub ZwMakeTemporaryObject
@ stdcall ZwMapViewOfSection(long long ptr long long ptr ptr long long long) ntdll.ZwMapViewOfSection
@ stdcall ZwNotifyChangeKey(long long ptr ptr ptr long long ptr long long) ntdll.ZwNotifyChangeKey
@ stdcall ZwOpenDirectoryObject(long long long) ntdll.ZwOpenDirectoryObject
@ stdcall ZwOpenEvent(long long long) ntdll.ZwOpenEvent
@ stdcall ZwOpenFile(ptr long ptr ptr long long) ntdll.ZwOpenFile
@ stub ZwOpenJobObject
@ stdcall ZwOpenKey(ptr long ptr) ntdll.ZwOpenKey
@ stdcall ZwOpenProcess(ptr long ptr ptr) ntdll.ZwOpenProcess
@ stdcall ZwOpenProcessToken(long long long) ntdll.ZwOpenProcessToken
@ stub ZwOpenProcessTokenEx
@ stdcall ZwOpenSection(ptr long ptr) ntdll.ZwOpenSection
@ stdcall ZwOpenSymbolicLinkObject(ptr long ptr) ntdll.ZwOpenSymbolicLinkObject
@ stdcall ZwOpenThread(ptr long ptr ptr) ntdll.ZwOpenThread
@ stdcall ZwOpenThreadToken(long long long long) ntdll.ZwOpenThreadToken
@ stub ZwOpenThreadTokenEx
@ stdcall ZwOpenTimer(ptr long ptr) ntdll.ZwOpenTimer
@ stub ZwPowerInformation
@ stdcall ZwPulseEvent(long ptr) ntdll.ZwPulseEvent
@ stub ZwQueryBootEntryOrder
@ stub ZwQueryBootOptions
@ stdcall ZwQueryDefaultLocale(long ptr) ntdll.ZwQueryDefaultLocale
@ stdcall ZwQueryDefaultUILanguage(ptr) ntdll.ZwQueryDefaultUILanguage
@ stdcall ZwQueryDirectoryFile(long long ptr ptr ptr ptr long long long ptr long) ntdll.ZwQueryDirectoryFile
@ stdcall ZwQueryDirectoryObject(long ptr long long long ptr ptr) ntdll.ZwQueryDirectoryObject
@ stub ZwQueryEaFile
@ stub ZwQueryFullAttributesFile
@ stdcall ZwQueryInformationFile(long ptr ptr long long) ntdll.ZwQueryInformationFile
@ stub ZwQueryInformationJobObject
@ stdcall ZwQueryInformationProcess(long long ptr long ptr) ntdll.ZwQueryInformationProcess
@ stdcall ZwQueryInformationThread(long long ptr long ptr) ntdll.ZwQueryInformationThread
@ stdcall ZwQueryInformationToken(long long ptr long ptr) ntdll.ZwQueryInformationToken
@ stdcall ZwQueryInstallUILanguage(ptr) ntdll.ZwQueryInstallUILanguage
@ stdcall ZwQueryKey(long long ptr long ptr) ntdll.ZwQueryKey
@ stdcall ZwQueryObject(long long long long long) ntdll.ZwQueryObject
@ stdcall ZwQuerySection(long long long long long) ntdll.ZwQuerySection
@ stdcall ZwQuerySecurityObject(long long long long long) ntdll.ZwQuerySecurityObject
@ stdcall ZwQuerySymbolicLinkObject(long ptr ptr) ntdll.ZwQuerySymbolicLinkObject
@ stdcall ZwQuerySystemInformation(long long long long) ntdll.ZwQuerySystemInformation
@ stdcall ZwQueryValueKey(long ptr long ptr long ptr) ntdll.ZwQueryValueKey
@ stdcall ZwQueryVolumeInformationFile(long ptr ptr long long) ntdll.ZwQueryVolumeInformationFile
@ stdcall ZwReadFile(long long long long long long long long long) ntdll.ZwReadFile
@ stdcall ZwReplaceKey(ptr long ptr) ntdll.ZwReplaceKey
@ stdcall ZwRequestWaitReplyPort(ptr ptr ptr) ntdll.ZwRequestWaitReplyPort
@ stdcall ZwResetEvent(long ptr) ntdll.ZwResetEvent
@ stdcall ZwRestoreKey(long long long) ntdll.ZwRestoreKey
@ stdcall ZwSaveKey(long long) ntdll.ZwSaveKey
@ stub ZwSaveKeyEx
@ stub ZwSetBootEntryOrder
@ stub ZwSetBootOptions
@ stdcall ZwSetDefaultLocale(long long) ntdll.ZwSetDefaultLocale
@ stdcall ZwSetDefaultUILanguage(long) ntdll.ZwSetDefaultUILanguage
@ stub ZwSetEaFile
@ stdcall ZwSetEvent(long long) ntdll.ZwSetEvent
@ stdcall ZwSetInformationFile(long long long long long) ntdll.ZwSetInformationFile
@ stub ZwSetInformationJobObject
@ stdcall ZwSetInformationObject(long long ptr long) ntdll.ZwSetInformationObject
@ stdcall ZwSetInformationProcess(long long long long) ntdll.ZwSetInformationProcess
@ stdcall ZwSetInformationThread(long long ptr long) ntdll.ZwSetInformationThread
@ stdcall ZwSetSecurityObject(long long ptr) ntdll.ZwSetSecurityObject
@ stdcall ZwSetSystemInformation(long ptr long) ntdll.ZwSetSystemInformation
@ stdcall ZwSetSystemTime(ptr ptr) ntdll.ZwSetSystemTime
@ stdcall ZwSetTimer(long ptr ptr ptr long long ptr) ntdll.ZwSetTimer
@ stdcall ZwSetValueKey(long long long long long long) ntdll.ZwSetValueKey
@ stdcall ZwSetVolumeInformationFile(long ptr ptr long long) ntdll.ZwSetVolumeInformationFile
@ stub ZwTerminateJobObject
@ stdcall ZwTerminateProcess(long long) ntdll.ZwTerminateProcess
@ stub ZwTranslateFilePath
@ stdcall ZwUnloadDriver(ptr) ntdll.ZwUnloadDriver
@ stdcall ZwUnloadKey(long) ntdll.ZwUnloadKey
@ stdcall ZwUnmapViewOfSection(long ptr) ntdll.ZwUnmapViewOfSection
@ stdcall ZwWaitForMultipleObjects(long ptr long long ptr) ntdll.ZwWaitForMultipleObjects
@ stdcall ZwWaitForSingleObject(long long long) ntdll.ZwWaitForSingleObject
@ stdcall ZwWriteFile(long long ptr ptr ptr ptr long ptr ptr) ntdll.ZwWriteFile
@ stdcall ZwYieldExecution() ntdll.ZwYieldExecution
@ cdecl -private _CIcos() msvcrt._CIcos
@ cdecl -private _CIsin() msvcrt._CIsin
@ cdecl -private _CIsqrt() msvcrt._CIsqrt
@ cdecl -private _abnormal_termination() msvcrt._abnormal_termination
@ stdcall -private -ret64 _alldiv(double double) ntdll._alldiv
@ stub _alldvrm
@ stdcall -private -ret64 _allmul(double double) ntdll._allmul
@ stdcall -private -i386 -norelay _alloca_probe() ntdll._alloca_probe
@ stdcall -private -ret64 _allrem(double double) ntdll._allrem
@ stub _allshl
@ stub _allshr
@ stdcall -private -ret64 _aulldiv(double double) ntdll._aulldiv
@ stub _aulldvrm
@ stdcall -private -ret64 _aullrem(double double) ntdll._aullrem
@ stub _aullshr
@ cdecl -private _except_handler2(ptr ptr ptr ptr) msvcrt._except_handler2
@ cdecl -private _except_handler3(ptr ptr ptr ptr) msvcrt._except_handler3
@ cdecl -private _global_unwind2(ptr) msvcrt._global_unwind2
@ cdecl -private _itoa(long ptr long) msvcrt._itoa
@ cdecl -private _itow(long ptr long) msvcrt._itow
@ cdecl -private _local_unwind2(ptr long) msvcrt._local_unwind2
@ cdecl -private _purecall() msvcrt._purecall
@ varargs -private _snprintf(str long str) msvcrt._snprintf
@ varargs -private _snwprintf(wstr long wstr) msvcrt._snwprintf
@ cdecl -private _stricmp(str str) msvcrt._stricmp
@ cdecl -private _strlwr(str) msvcrt._strlwr
@ cdecl -private _strnicmp(str str long) msvcrt._strnicmp
@ cdecl -private _strnset(str long long) msvcrt._strnset
@ cdecl -private _strrev(str) msvcrt._strrev
@ cdecl -private _strset(str long) msvcrt._strset
@ cdecl -private _strupr(str) msvcrt._strupr
@ cdecl -private _vsnprintf(ptr long ptr ptr) msvcrt._vsnprintf
@ cdecl -private _vsnwprintf(ptr long wstr long) msvcrt._vsnwprintf
@ cdecl -private _wcsicmp(wstr wstr) msvcrt._wcsicmp
@ cdecl -private _wcslwr(wstr) msvcrt._wcslwr
@ cdecl -private _wcsnicmp(wstr wstr long) msvcrt._wcsnicmp
@ cdecl -private _wcsnset(wstr long long) msvcrt._wcsnset
@ cdecl -private _wcsrev(wstr) msvcrt._wcsrev
@ cdecl -private _wcsupr(wstr) msvcrt._wcsupr
@ cdecl -private atoi(str) msvcrt.atoi
@ cdecl -private atol(str) msvcrt.atol
@ cdecl -private isdigit(long) msvcrt.isdigit
@ cdecl -private islower(long) msvcrt.islower
@ cdecl -private isprint(long) msvcrt.isprint
@ cdecl -private isspace(long) msvcrt.isspace
@ cdecl -private isupper(long) msvcrt.isupper
@ cdecl -private isxdigit(long) msvcrt.isxdigit
@ cdecl -private mbstowcs(ptr str long) msvcrt.mbstowcs
@ cdecl -private mbtowc(wstr str long) msvcrt.mbtowc
@ cdecl -private memchr(ptr long long) msvcrt.memchr
@ cdecl -private memcpy(ptr ptr long) msvcrt.memcpy
@ cdecl -private memmove(ptr ptr long) msvcrt.memmove
@ cdecl -private memset(ptr long long) msvcrt.memset
@ cdecl -private qsort(ptr long long ptr) msvcrt.qsort
@ cdecl -private rand() msvcrt.rand
@ varargs -private sprintf(ptr str) msvcrt.sprintf
@ cdecl -private srand(long) msvcrt.srand
@ cdecl -private strcat(str str) msvcrt.strcat
@ cdecl -private strchr(str long) msvcrt.strchr
@ cdecl -private strcmp(str str) msvcrt.strcmp
@ cdecl -private strcpy(ptr str) msvcrt.strcpy
@ cdecl -private strlen(str) msvcrt.strlen
@ cdecl -private strncat(str str long) msvcrt.strncat
@ cdecl -private strncmp(str str long) msvcrt.strncmp
@ cdecl -private strncpy(ptr str long) msvcrt.strncpy
@ cdecl -private strrchr(str long) msvcrt.strrchr
@ cdecl -private strspn(str str) msvcrt.strspn
@ cdecl -private strstr(str str) msvcrt.strstr
@ varargs -private swprintf(wstr wstr) msvcrt.swprintf
@ cdecl -private tolower(long) msvcrt.tolower
@ cdecl -private toupper(long) msvcrt.toupper
@ cdecl -private towlower(long) msvcrt.towlower
@ cdecl -private towupper(long) msvcrt.towupper
@ stdcall vDbgPrintEx(long long str ptr) ntdll.vDbgPrintEx
@ stdcall vDbgPrintExWithPrefix(str long long str ptr) ntdll.vDbgPrintExWithPrefix
@ cdecl -private vsprintf(ptr str ptr) msvcrt.vsprintf
@ cdecl -private wcscat(wstr wstr) msvcrt.wcscat
@ cdecl -private wcschr(wstr long) msvcrt.wcschr
@ cdecl -private wcscmp(wstr wstr) msvcrt.wcscmp
@ cdecl -private wcscpy(ptr wstr) msvcrt.wcscpy
@ cdecl -private wcscspn(wstr wstr) msvcrt.wcscspn
@ cdecl -private wcslen(wstr) msvcrt.wcslen
@ cdecl -private wcsncat(wstr wstr long) msvcrt.wcsncat
@ cdecl -private wcsncmp(wstr wstr long) msvcrt.wcsncmp
@ cdecl -private wcsncpy(ptr wstr long) msvcrt.wcsncpy
@ cdecl -private wcsrchr(wstr long) msvcrt.wcsrchr
@ cdecl -private wcsspn(wstr wstr) msvcrt.wcsspn
@ cdecl -private wcsstr(wstr wstr) msvcrt.wcsstr
@ cdecl -private wcstombs(ptr ptr long) msvcrt.wcstombs
@ cdecl -private wctomb(ptr long) msvcrt.wctomb
