name	ntdll
type	win32

#note that the Zw... functions are alternate names for the 
#Nt... functions.  (see www.sysinternals.com for details)
#if you change a Nt.. function DON'T FORGET to change the
#Zw one too.

@ stub CsrAllocateCaptureBuffer
@ stub CsrAllocateCapturePointer
@ stub CsrAllocateMessagePointer
@ stub CsrCaptureMessageBuffer
@ stub CsrCaptureMessageString
@ stub CsrCaptureTimeout
@ stub CsrClientCallServer
@ stub CsrClientConnectToServer
@ stub CsrClientMaxMessage
@ stub CsrClientSendMessage
@ stub CsrClientThreadConnect
@ stub CsrFreeCaptureBuffer
@ stub CsrIdentifyAlertableThread
@ stub CsrNewThread
@ stub CsrProbeForRead
@ stub CsrProbeForWrite
@ stub CsrSetPriorityClass
@ stub CsrpProcessCallbackRequest
@ stdcall DbgBreakPoint() DbgBreakPoint
@ varargs DbgPrint(str) DbgPrint
@ stub DbgPrompt
@ stub DbgSsHandleKmApiMsg
@ stub DbgSsInitialize
@ stub DbgUiConnectToDbg
@ stub DbgUiContinue
@ stub DbgUiWaitStateChange
@ stdcall DbgUserBreakPoint() DbgUserBreakPoint
@ stub KiUserApcDispatcher
@ stub KiUserCallbackDispatcher
@ stub KiUserExceptionDispatcher
@ stub LdrAccessResource
@ stub LdrDisableThreadCalloutsForDll
@ stub LdrEnumResources
@ stub LdrFindEntryForAddress
@ stub LdrFindResourceDirectory_U
@ stub LdrFindResource_U
@ stub LdrGetDllHandle
@ stub LdrGetProcedureAddress
@ stub LdrInitializeThunk
@ stub LdrLoadDll
@ stub LdrProcessRelocationBlock
@ stub LdrQueryImageFileExecutionOptions
@ stub LdrQueryProcessModuleInformation
@ stub LdrShutdownProcess
@ stub LdrShutdownThread
@ stub LdrUnloadDll
@ stub LdrVerifyImageMatchesChecksum
@ stub NPXEMULATORTABLE
@ stub NlsMbCodePageTag
@ stub NlsMbOemCodePageTag
@ stdcall NtAcceptConnectPort(long long long long long long) NtAcceptConnectPort
@ stdcall NtAccessCheck(ptr long long ptr ptr ptr ptr ptr) NtAccessCheck
@ stub NtAccessCheckAndAuditAlarm
@ stub NtAdjustGroupsToken
@ stdcall NtAdjustPrivilegesToken(long long long long long long) NtAdjustPrivilegesToken
@ stub NtAlertResumeThread
@ stub NtAlertThread
@ stdcall NtAllocateLocallyUniqueId(ptr) NtAllocateLocallyUniqueId
@ stdcall NtAllocateUuids(ptr ptr ptr) NtAllocateUuids
@ stub NtAllocateVirtualMemory
@ stub NtCallbackReturn
@ stub NtCancelIoFile
@ stub NtCancelTimer
@ stdcall NtClearEvent(long) NtClearEvent
@ stdcall NtClose(long) NtClose
@ stub NtCloseObjectAuditAlarm
@ stdcall NtCompleteConnectPort(long) NtCompleteConnectPort
@ stdcall NtConnectPort(long long long long long long long long) NtConnectPort
@ stub NtContinue
@ stdcall NtCreateDirectoryObject(long long long) NtCreateDirectoryObject
@ stdcall NtCreateEvent(long long long long long) NtCreateEvent
@ stub NtCreateEventPair
@ stdcall NtCreateFile(ptr long ptr ptr long long long ptr long long ptr) NtCreateFile
@ stub NtCreateIoCompletion
@ stdcall NtCreateKey(long long long long long long long) NtCreateKey
@ stdcall NtCreateMailslotFile(long long long long long long long long) NtCreateMailslotFile
@ stub NtCreateMutant
@ stub NtCreateNamedPipeFile
@ stdcall NtCreatePagingFile(long long long long) NtCreatePagingFile
@ stdcall NtCreatePort(long long long long long) NtCreatePort
@ stub NtCreateProcess
@ stub NtCreateProfile
@ stdcall NtCreateSection(long long long long long long long) NtCreateSection
@ stdcall NtCreateSemaphore(ptr long ptr long long) NtCreateSemaphore
@ stdcall NtCreateSymbolicLinkObject(ptr long ptr ptr) NtCreateSymbolicLinkObject
@ stub NtCreateThread
@ stdcall NtCreateTimer(ptr long ptr long) NtCreateTimer
@ stub NtCreateToken
@ stdcall NtCurrentTeb() NtCurrentTeb
@ stub NtDelayExecution
@ stub NtDeleteFile
@ stdcall NtDeleteKey(long) NtDeleteKey
@ stdcall NtDeleteValueKey(long ptr) NtDeleteValueKey
@ stdcall NtDeviceIoControlFile(long long long long long long long long long long) NtDeviceIoControlFile
@ stdcall NtDisplayString(ptr)NtDisplayString
@ stdcall NtDuplicateObject(long long long long long long long) NtDuplicateObject
@ stdcall NtDuplicateToken(long long long long long long) NtDuplicateToken
@ stub NtEnumerateBus
@ stdcall NtEnumerateKey (long long long long long long) NtEnumerateKey
@ stdcall NtEnumerateValueKey (long long long long long long) NtEnumerateValueKey
@ stub NtExtendSection
@ stub NtFlushBuffersFile
@ stub NtFlushInstructionCache
@ stdcall NtFlushKey(long) NtFlushKey
@ stub NtFlushVirtualMemory
@ stub NtFlushWriteBuffer
@ stub NtFreeVirtualMemory
@ stdcall NtFsControlFile(long long long long long long long long long long) NtFsControlFile
@ stub NtGetContextThread
@ stub NtGetPlugPlayEvent
@ stub NtGetTickCount
@ stub NtImpersonateClientOfPort
@ stub NtImpersonateThread
@ stub NtInitializeRegistry
@ stdcall NtListenPort(long long) NtListenPort
@ stub NtLoadDriver
@ stdcall NtLoadKey(ptr ptr) NtLoadKey
@ stub NtLockFile
@ stub NtLockVirtualMemory
@ stub NtMakeTemporaryObject
@ stdcall NtMapViewOfSection(long long long long long long long long long long) NtMapViewOfSection
@ stub NtNotifyChangeDirectoryFile
@ stdcall NtNotifyChangeKey(long long ptr ptr ptr long long ptr long long) NtNotifyChangeKey
@ stdcall NtOpenDirectoryObject(long long long) NtOpenDirectoryObject
@ stdcall NtOpenEvent(long long long) NtOpenEvent
@ stub NtOpenEventPair
@ stdcall NtOpenFile(ptr long ptr ptr long long) NtOpenFile
@ stub NtOpenIoCompletion
@ stdcall NtOpenKey(ptr long ptr) NtOpenKey
@ stub NtOpenMutant
@ stub NtOpenObjectAuditAlarm
@ stub NtOpenProcess
@ stdcall NtOpenProcessToken(long long long) NtOpenProcessToken
@ stdcall NtOpenSection(long long long) NtOpenSection
@ stdcall NtOpenSemaphore(long long ptr) NtOpenSemaphore
@ stdcall NtOpenSymbolicLinkObject (long long long) NtOpenSymbolicLinkObject
@ stub NtOpenThread
@ stdcall NtOpenThreadToken(long long long long) NtOpenThreadToken
@ stub NtOpenTimer
@ stub NtPlugPlayControl
@ stub NtPrivilegeCheck
@ stub NtPrivilegeObjectAuditAlarm
@ stub NtPrivilegedServiceAuditAlarm
@ stub NtProtectVirtualMemory
@ stdcall NtPulseEvent(long ptr) NtPulseEvent
@ stub NtQueryAttributesFile
@ stub NtQueryDefaultLocale
@ stdcall NtQueryDirectoryFile(long long  ptr ptr ptr ptr long long long ptr long)NtQueryDirectoryFile
@ stdcall NtQueryDirectoryObject(long long long long long long long) NtQueryDirectoryObject
@ stub NtQueryEaFile
@ stdcall NtQueryEvent(long long ptr long ptr) NtQueryEvent
@ stdcall NtQueryInformationFile(long long long long long) NtQueryInformationFile
@ stub NtQueryInformationPort
@ stdcall NtQueryInformationProcess(long long long long long) NtQueryInformationProcess
@ stdcall NtQueryInformationThread (long long long long long) NtQueryInformationThread
@ stdcall NtQueryInformationToken (long long long long long) NtQueryInformationToken
@ stub NtQueryIntervalProfile
@ stub NtQueryIoCompletion
@ stdcall NtQueryKey (long long ptr long ptr) NtQueryKey
@ stub NtQueryMutant
@ stdcall NtQueryObject(long long long long long) NtQueryObject
@ stdcall NtQueryPerformanceCounter (long long) NtQueryPerformanceCounter
@ stdcall NtQuerySection (long long long long long) NtQuerySection
@ stdcall NtQuerySecurityObject (long long long long long) NtQuerySecurityObject
@ stdcall NtQuerySemaphore (long long long long long) NtQuerySemaphore
@ stdcall NtQuerySymbolicLinkObject(long ptr ptr) NtQuerySymbolicLinkObject
@ stub NtQuerySystemEnvironmentValue
@ stdcall NtQuerySystemInformation(long long long long) NtQuerySystemInformation
@ stdcall NtQuerySystemTime(ptr) GetSystemTimeAsFileTime
@ stub NtQueryTimer
@ stdcall NtQueryTimerResolution(long long long) NtQueryTimerResolution
@ stdcall NtQueryValueKey(long long long long long long) NtQueryValueKey
@ stub NtQueryVirtualMemory
@ stdcall NtRaiseException(ptr ptr long) NtRaiseException
@ stub NtRaiseHardError
@ stdcall NtReadFile(long long long long long long long long long) NtReadFile
@ stub NtReadRequestData
@ stub NtReadVirtualMemory
@ stub NtRegisterNewDevice
@ stdcall NtRegisterThreadTerminatePort(long) NtRegisterThreadTerminatePort
@ stub NtReleaseMutant
@ stub NtReleaseProcessMutant
@ stdcall NtReleaseSemaphore(long long ptr) NtReleaseSemaphore
@ stub NtRemoveIoCompletion
@ stdcall NtReplaceKey(ptr long ptr) NtReplaceKey 
@ stub NtReplyPort
@ stdcall NtReplyWaitReceivePort(long long long long) NtReplyWaitReceivePort
@ stub NtReplyWaitReplyPort
@ stub NtRequestPort
@ stdcall NtRequestWaitReplyPort(long long long) NtRequestWaitReplyPort
@ stdcall NtResetEvent(long ptr) NtResetEvent
@ stdcall NtRestoreKey(long long long) NtRestoreKey
@ stdcall NtResumeThread(long long) NtResumeThread
@ stdcall NtSaveKey(long long) NtSaveKey
@ stub NtSetContextThread
@ stub NtSetDefaultHardErrorPort
@ stub NtSetDefaultLocale
@ stub NtSetEaFile
@ stdcall NtSetEvent(long long) NtSetEvent
@ stub NtSetHighEventPair
@ stub NtSetHighWaitLowEventPair
@ stub NtSetHighWaitLowThread
@ stdcall NtSetInformationFile(long long long long long) NtSetInformationFile
@ stdcall NtSetInformationKey(long long ptr long) NtSetInformationKey
@ stub NtSetInformationObject
@ stdcall NtSetInformationProcess(long long long long) NtSetInformationProcess
@ stdcall NtSetInformationThread(long long long long) NtSetInformationThread
@ stub NtSetInformationToken
@ stdcall NtSetIntervalProfile(long long) NtSetIntervalProfile
@ stub NtSetIoCompletion
@ stub NtSetLdtEntries
@ stub NtSetLowEventPair
@ stub NtSetLowWaitHighEventPair
@ stub NtSetLowWaitHighThread
@ stdcall NtSetSecurityObject(long long ptr) NtSetSecurityObject
@ stub NtSetSystemEnvironmentValue
@ stub NtSetSystemInformation
@ stub NtSetSystemPowerState
@ stub NtSetSystemTime
@ stdcall NtSetTimer(long ptr ptr ptr long long ptr) NtSetTimer
@ stub NtSetTimerResolution
@ stdcall NtSetValueKey(long long long long long long) NtSetValueKey
@ stdcall NtSetVolumeInformationFile(long ptr ptr long long) NtSetVolumeInformationFile
@ stub NtShutdownSystem
@ stub NtStartProfile
@ stub NtStopProfile
@ stub NtSuspendThread
@ stub NtSystemDebugControl
@ stdcall NtTerminateProcess(long long)NtTerminateProcess
@ stdcall NtTerminateThread(long long) NtTerminateThread
@ stub NtTestAlert
@ stub NtUnloadDriver
@ stdcall NtUnloadKey(long) NtUnloadKey
@ stub NtUnlockFile
@ stub NtUnlockVirtualMemory
@ stub NtUnmapViewOfSection
@ stub NtVdmControl
@ stub NtW32Call
@ stub NtWaitForMultipleObjects
@ stub NtWaitForProcessMutant
@ stdcall NtWaitForSingleObject(long long long) NtWaitForSingleObject
@ stub NtWaitHighEventPair
@ stub NtWaitLowEventPair
@ stub NtWriteFile
@ stub NtWriteRequestData
@ stub NtWriteVirtualMemory
@ stub PfxFindPrefix
@ stub PfxInitialize
@ stub PfxInsertPrefix
@ stub PfxRemovePrefix
@ stub RestoreEm87Context
@ stub RtlAbortRXact
@ stub RtlAbsoluteToSelfRelativeSD
@ stdcall RtlAcquirePebLock() RtlAcquirePebLock
@ stdcall RtlAcquireResourceExclusive(ptr long) RtlAcquireResourceExclusive
@ stdcall RtlAcquireResourceShared(ptr long) RtlAcquireResourceShared
@ stdcall RtlAddAccessAllowedAce(long long long long) RtlAddAccessAllowedAce
@ stub RtlAddAccessDeniedAce
@ stdcall RtlAddAce(ptr long long ptr long) RtlAddAce
@ stub RtlAddActionToRXact
@ stub RtlAddAttributeActionToRXact
@ stub RtlAddAuditAccessAce
@ stdcall RtlAdjustPrivilege(long long long long) RtlAdjustPrivilege
@ stdcall RtlAllocateAndInitializeSid (ptr long long long long long long long long long ptr) RtlAllocateAndInitializeSid
@ stdcall RtlAllocateHeap(long long long) RtlAllocateHeap
@ stub RtlAnsiCharToUnicodeChar
@ stub RtlAnsiStringToUnicodeSize
@ stdcall RtlAnsiStringToUnicodeString(ptr ptr long) RtlAnsiStringToUnicodeString
@ stub RtlAppendAsciizToString
@ stub RtlAppendStringToString
@ stub RtlAppendUnicodeStringToString
@ stub RtlAppendUnicodeToString
@ stub RtlApplyRXact
@ stub RtlApplyRXactNoFlush
@ stub RtlAreAllAccessesGranted
@ stub RtlAreAnyAccessesGranted
@ stub RtlAreBitsClear
@ stub RtlAreBitsSet
@ stdcall RtlAssert(ptr ptr long long) RtlAssert
@ stub RtlCaptureStackBackTrace
@ stub RtlCharToInteger
@ stub RtlCheckRegistryKey
@ stub RtlClearAllBits
@ stdcall RtlClearBits(long long long) RtlClearBits
@ stub RtlCompactHeap
@ stdcall RtlCompareMemory(ptr ptr long) RtlCompareMemory
@ stub RtlCompareMemoryUlong
@ stub RtlCompareString
@ stdcall RtlCompareUnicodeString (ptr ptr long) RtlCompareUnicodeString
@ stub RtlCompressBuffer
@ stub RtlConsoleMultiByteToUnicodeN
@ stub RtlConvertExclusiveToShared
@ stub RtlConvertLongToLargeInteger
@ stub RtlConvertSharedToExclusive
@ stdcall RtlConvertSidToUnicodeString(ptr ptr)RtlConvertSidToUnicodeString
@ stub RtlConvertUiListToApiList
@ stub RtlConvertUlongToLargeInteger
@ stub RtlCopyLuid
@ stub RtlCopyLuidAndAttributesArray
@ stub RtlCopySecurityDescriptor
@ stdcall RtlCopySid(long ptr ptr) RtlCopySid
@ stub RtlCopySidAndAttributesArray
@ stub RtlCopyString
@ stub RtlCopyUnicodeString
@ stdcall RtlCreateAcl(ptr long long) RtlCreateAcl
@ stub RtlCreateAndSetSD
@ stdcall RtlCreateEnvironment(long long) RtlCreateEnvironment
@ stdcall RtlCreateHeap(long ptr long long ptr ptr) RtlCreateHeap
@ stub RtlCreateProcessParameters
@ stub RtlCreateQueryDebugBuffer
@ stub RtlCreateRegistryKey
@ stdcall RtlCreateSecurityDescriptor(ptr long) RtlCreateSecurityDescriptor
@ stub RtlCreateTagHeap
@ stub RtlCreateUnicodeString
@ stub RtlCreateUnicodeStringFromAsciiz
@ stub RtlCreateUserProcess
@ stub RtlCreateUserSecurityObject
@ stub RtlCreateUserThread
@ stub RtlCustomCPToUnicodeN
@ stub RtlCutoverTimeToSystemTime
@ stub RtlDeNormalizeProcessParams
@ stub RtlDecompressBuffer
@ stub RtlDecompressFragment
@ stub RtlDelete
@ stub RtlDeleteAce
@ stdcall RtlDeleteCriticalSection(ptr) DeleteCriticalSection
@ stub RtlDeleteElementGenericTable
@ stub RtlDeleteRegistryValue
@ stdcall RtlDeleteResource(ptr) RtlDeleteResource
@ stdcall RtlDeleteSecurityObject(long) RtlDeleteSecurityObject
@ stdcall RtlDestroyEnvironment(long) RtlDestroyEnvironment
@ stdcall RtlDestroyHeap(long) RtlDestroyHeap
@ stub RtlDestroyProcessParameters
@ stub RtlDestroyQueryDebugBuffer
@ stub RtlDetermineDosPathNameType_U
@ stub RtlDoesFileExists_U
@ stdcall RtlDosPathNameToNtPathName_U(ptr ptr long long) RtlDosPathNameToNtPathName_U
@ stub RtlDosSearchPath_U
@ stdcall RtlDumpResource(ptr) RtlDumpResource
@ stub RtlEnlargedIntegerMultiply
@ stub RtlEnlargedUnsignedDivide
@ stub RtlEnlargedUnsignedMultiply
@ stdcall RtlEnterCriticalSection(ptr) EnterCriticalSection
@ stub RtlEnumProcessHeaps
@ stub RtlEnumerateGenericTable
@ stub RtlEnumerateGenericTableWithoutSplaying
@ stub RtlEqualComputerName
@ stub RtlEqualDomainName
@ stub RtlEqualLuid
@ stdcall RtlEqualPrefixSid(ptr ptr) RtlEqualPrefixSid
@ stdcall RtlEqualSid (long long) RtlEqualSid
@ stub RtlEqualString
@ stdcall RtlEqualUnicodeString(long long long) RtlEqualUnicodeString
@ stub RtlEraseUnicodeString
@ stub RtlExpandEnvironmentStrings_U
@ stub RtlExtendHeap
@ stdcall RtlExtendedIntegerMultiply(long long long) RtlExtendedIntegerMultiply
@ stdcall RtlExtendedLargeIntegerDivide(long long long ptr) RtlExtendedLargeIntegerDivide
@ stub RtlExtendedMagicDivide
@ stdcall RtlFillMemory(ptr long long) RtlFillMemory
@ stub RtlFillMemoryUlong
@ stdcall RtlFindClearBits(long long long) RtlFindClearBits
@ stub RtlFindClearBitsAndSet
@ stub RtlFindLongestRunClear
@ stub RtlFindLongestRunSet
@ stub RtlFindMessage
@ stub RtlFindSetBits
@ stub RtlFindSetBitsAndClear
@ stdcall RtlFirstFreeAce(ptr ptr) RtlFirstFreeAce
@ stdcall RtlFormatCurrentUserKeyPath(ptr) RtlFormatCurrentUserKeyPath
@ stub RtlFormatMessage
@ stdcall RtlFreeAnsiString(long) RtlFreeAnsiString
@ stdcall RtlFreeHeap(long long long) RtlFreeHeap
@ stub RtlFreeOemString
@ stdcall RtlFreeSid (long) RtlFreeSid
@ stdcall RtlFreeUnicodeString(ptr) RtlFreeUnicodeString
@ stub RtlGenerate8dot3Name
@ stdcall RtlGetAce(ptr long ptr) RtlGetAce
@ stub RtlGetCallersAddress
@ stub RtlGetCompressionWorkSpaceSize
@ stdcall RtlGetControlSecurityDescriptor(ptr ptr ptr) RtlGetControlSecurityDescriptor
@ stub RtlGetCurrentDirectory_U
@ stdcall RtlGetDaclSecurityDescriptor(ptr ptr ptr ptr) RtlGetDaclSecurityDescriptor
@ stub RtlGetElementGenericTable
@ stub RtlGetFullPathName_U
@ stdcall RtlGetGroupSecurityDescriptor(ptr ptr ptr) RtlGetGroupSecurityDescriptor
@ stub RtlGetLongestNtPathLength
@ stub RtlGetNtGlobalFlags
@ stdcall RtlGetNtProductType(ptr) RtlGetNtProductType
@ stdcall RtlGetOwnerSecurityDescriptor(ptr ptr ptr) RtlGetOwnerSecurityDescriptor
@ stub RtlGetProcessHeaps
@ stdcall RtlGetSaclSecurityDescriptor(ptr ptr ptr ptr)RtlGetSaclSecurityDescriptor
@ stub RtlGetUserInfoHeap
@ stdcall RtlIdentifierAuthoritySid(ptr) RtlIdentifierAuthoritySid
@ stub RtlImageDirectoryEntryToData
@ stdcall RtlImageNtHeader(long) RtlImageNtHeader
@ stdcall RtlImpersonateSelf(long) RtlImpersonateSelf
@ stdcall RtlInitAnsiString(ptr str) RtlInitAnsiString
@ stub RtlInitCodePageTable
@ stub RtlInitNlsTables
@ stdcall RtlInitString(ptr str) RtlInitString
@ stdcall RtlInitUnicodeString(ptr wstr) RtlInitUnicodeString
@ stdcall RtlInitializeBitMap(long long long) RtlInitializeBitMap
@ stub RtlInitializeContext
@ stdcall RtlInitializeCriticalSection(ptr) InitializeCriticalSection
@ stdcall RtlInitializeGenericTable() RtlInitializeGenericTable
@ stub RtlInitializeRXact
@ stdcall RtlInitializeResource(ptr) RtlInitializeResource
@ stdcall RtlInitializeSid(ptr ptr long) RtlInitializeSid
@ stub RtlInsertElementGenericTable
@ stdcall RtlIntegerToChar(long long long long) RtlIntegerToChar
@ stub RtlIntegerToUnicodeString
@ stub RtlIsDosDeviceName_U
@ stub RtlIsGenericTableEmpty
@ stub RtlIsNameLegalDOS8Dot3
@ stdcall RtlIsTextUnicode(ptr long ptr) RtlIsTextUnicode
@ stub RtlLargeIntegerAdd
@ stub RtlLargeIntegerArithmeticShift
@ stub RtlLargeIntegerDivide
@ stub RtlLargeIntegerNegate
@ stub RtlLargeIntegerShiftLeft
@ stub RtlLargeIntegerShiftRight
@ stub RtlLargeIntegerSubtract
@ stub RtlLargeIntegerToChar
@ stdcall RtlLeaveCriticalSection(ptr) LeaveCriticalSection
@ stdcall RtlLengthRequiredSid(long) RtlLengthRequiredSid
@ stdcall RtlLengthSecurityDescriptor(ptr) RtlLengthSecurityDescriptor
@ stdcall RtlLengthSid(ptr) RtlLengthSid
@ stub RtlLocalTimeToSystemTime
@ stub RtlLockHeap
@ stub RtlLookupElementGenericTable
@ stdcall RtlMakeSelfRelativeSD(ptr ptr ptr) RtlMakeSelfRelativeSD
@ stub RtlMapGenericMask
@ stdcall RtlMoveMemory(ptr ptr long) RtlMoveMemory
@ stdcall RtlMultiByteToUnicodeN(ptr long ptr ptr long) RtlMultiByteToUnicodeN
@ stub RtlMultiByteToUnicodeSize
@ stub RtlNewInstanceSecurityObject
@ stub RtlNewSecurityGrantedAccess
@ stdcall RtlNewSecurityObject(long long long long long long) RtlNewSecurityObject
@ stdcall RtlNormalizeProcessParams(ptr) RtlNormalizeProcessParams
@ stdcall RtlNtStatusToDosError(long) RtlNtStatusToDosError
@ stub RtlNumberGenericTableElements
@ stub RtlNumberOfClearBits
@ stub RtlNumberOfSetBits
@ stub RtlOemStringToUnicodeSize
@ stdcall RtlOemStringToUnicodeString(ptr ptr long) RtlOemStringToUnicodeString
@ stdcall RtlOemToUnicodeN(ptr long ptr ptr long) RtlOemToUnicodeN
@ stdcall RtlOpenCurrentUser(long ptr) RtlOpenCurrentUser
@ stub RtlPcToFileHeader
@ stub RtlPrefixString
@ stdcall RtlPrefixUnicodeString(ptr ptr long) RtlPrefixUnicodeString
@ stub RtlProtectHeap
@ stdcall RtlQueryEnvironmentVariable_U(long long long) RtlQueryEnvironmentVariable_U
@ stub RtlQueryInformationAcl
@ stub RtlQueryProcessBackTraceInformation
@ stub RtlQueryProcessDebugInformation
@ stub RtlQueryProcessHeapInformation
@ stub RtlQueryProcessLockInformation
@ stub RtlQueryRegistryValues
@ stub RtlQuerySecurityObject
@ stub RtlQueryTagHeap
@ stub RtlQueryTimeZoneInformation
@ stdcall RtlRaiseException(ptr) RtlRaiseException
@ stdcall RtlRaiseStatus(long) RtlRaiseStatus
@ stub RtlRandom
@ stub RtlReAllocateHeap
@ stub RtlRealPredecessor
@ stub RtlRealSuccessor
@ stdcall RtlReleasePebLock() RtlReleasePebLock
@ stdcall RtlReleaseResource(ptr) RtlReleaseResource
@ stub RtlRemoteCall
@ stub RtlResetRtlTranslations
@ stub RtlRunDecodeUnicodeString
@ stub RtlRunEncodeUnicodeString
@ stub RtlSecondsSince1970ToTime
@ stub RtlSecondsSince1980ToTime
@ stub RtlSelfRelativeToAbsoluteSD
@ stub RtlSetAllBits
@ stdcall RtlSetBits(long long long) RtlSetBits
@ stub RtlSetCurrentDirectory_U
@ stub RtlSetCurrentEnvironment
@ stdcall RtlSetDaclSecurityDescriptor(ptr long ptr long) RtlSetDaclSecurityDescriptor
@ stdcall RtlSetEnvironmentVariable(long long long) RtlSetEnvironmentVariable
@ stdcall RtlSetGroupSecurityDescriptor(ptr ptr long) RtlSetGroupSecurityDescriptor
@ stub RtlSetInformationAcl
@ stdcall RtlSetOwnerSecurityDescriptor(ptr ptr long) RtlSetOwnerSecurityDescriptor
@ stdcall RtlSetSaclSecurityDescriptor(ptr long ptr long) RtlSetSaclSecurityDescriptor
@ stub RtlSetSecurityObject
@ stub RtlSetTimeZoneInformation
@ stub RtlSetUserFlagsHeap
@ stub RtlSetUserValueHeap
@ stdcall RtlSizeHeap(long long long) HeapSize
@ stub RtlSplay
@ stub RtlStartRXact
@ stdcall RtlSubAuthorityCountSid(ptr) RtlSubAuthorityCountSid
@ stdcall RtlSubAuthoritySid(ptr long) RtlSubAuthoritySid
@ stub RtlSubtreePredecessor
@ stub RtlSubtreeSuccessor
@ stdcall RtlSystemTimeToLocalTime (long long) RtlSystemTimeToLocalTime
@ stdcall RtlTimeFieldsToTime(ptr ptr) RtlTimeFieldsToTime
@ stdcall RtlTimeToElapsedTimeFields(long long) RtlTimeToElapsedTimeFields
@ stdcall RtlTimeToSecondsSince1970(ptr ptr) RtlTimeToSecondsSince1970
@ stdcall RtlTimeToSecondsSince1980(ptr ptr) RtlTimeToSecondsSince1980
@ stdcall RtlTimeToTimeFields (long long) RtlTimeToTimeFields
@ stub RtlUnicodeStringToAnsiSize
@ stdcall RtlUnicodeStringToAnsiString(ptr ptr long) RtlUnicodeStringToAnsiString
@ stub RtlUnicodeStringToCountedOemString
@ stub RtlUnicodeStringToInteger
@ stub RtlUnicodeStringToOemSize
@ stdcall RtlUnicodeStringToOemString(ptr ptr long) RtlUnicodeStringToOemString
@ stub RtlUnicodeToCustomCPN
@ stdcall RtlUnicodeToMultiByteN(ptr long ptr ptr long) RtlUnicodeToMultiByteN
@ stub RtlUnicodeToMultiByteSize
@ stdcall RtlUnicodeToOemN(ptr long ptr ptr long) RtlUnicodeToOemN
@ stub RtlUniform
@ stub RtlUnlockHeap
@ stdcall RtlUnwind(ptr ptr ptr long) RtlUnwind
@ stub RtlUpcaseUnicodeChar
@ stdcall RtlUpcaseUnicodeString(ptr ptr long) RtlUpcaseUnicodeString
@ stub RtlUpcaseUnicodeStringToAnsiString
@ stub RtlUpcaseUnicodeStringToCountedOemString
@ stdcall RtlUpcaseUnicodeStringToOemString(ptr ptr long) RtlUpcaseUnicodeStringToOemString
@ stub RtlUpcaseUnicodeToCustomCPN
@ stub RtlUpcaseUnicodeToMultiByteN
@ stdcall RtlUpcaseUnicodeToOemN(ptr long ptr ptr long) RtlUpcaseUnicodeToOemN
@ stub RtlUpperChar
@ stub RtlUpperString
@ stub RtlUsageHeap
@ stub RtlValidAcl
@ stdcall RtlValidSecurityDescriptor(ptr) RtlValidSecurityDescriptor
@ stdcall RtlValidSid(ptr) RtlValidSid
@ stub RtlValidateHeap
@ stub RtlValidateProcessHeaps
@ stub RtlWalkHeap
@ stub RtlWriteRegistryValue
@ stub RtlZeroHeap
@ stdcall RtlZeroMemory(ptr long) RtlZeroMemory
@ stub RtlpInitializeRtl
@ stub RtlpNtCreateKey
@ stub RtlpNtEnumerateSubKey
@ stub RtlpNtMakeTemporaryKey
@ stub RtlpNtOpenKey
@ stub RtlpNtQueryValueKey
@ stub RtlpNtSetValueKey
@ stub RtlpUnWaitCriticalSection
@ stub RtlpWaitForCriticalSection
@ stdcall RtlxAnsiStringToUnicodeSize(ptr) RtlxAnsiStringToUnicodeSize
@ stdcall RtlxOemStringToUnicodeSize(ptr) RtlxOemStringToUnicodeSize
@ stub RtlxUnicodeStringToAnsiSize
@ stdcall RtlxUnicodeStringToOemSize(ptr) RtlxUnicodeStringToOemSize
@ stub SaveEm87Context
@ stdcall ZwAcceptConnectPort(long long long long long long) NtAcceptConnectPort
@ stub ZwAccessCheck
@ stub ZwAccessCheckAndAuditAlarm
@ stdcall ZwAdjustGroupsToken(long long long long long long) NtAdjustPrivilegesToken
@ stub ZwAdjustPrivilegesToken
@ stub ZwAlertResumeThread
@ stub ZwAlertThread
@ stub ZwAllocateLocallyUniqueId
@ stub ZwAllocateUuids
@ stub ZwAllocateVirtualMemory
@ stub ZwCallbackReturn
@ stub ZwCancelIoFile
@ stub ZwCancelTimer
@ stub ZwClearEvent
@ stub ZwClose
@ stub ZwCloseObjectAuditAlarm
@ stdcall ZwCompleteConnectPort(long) NtCompleteConnectPort
@ stdcall ZwConnectPort(long long long long long long long long) NtConnectPort
@ stub ZwContinue
@ stdcall ZwCreateDirectoryObject(long long long) NtCreateDirectoryObject
@ stdcall ZwCreateEvent(long long long long long) NtCreateEvent
@ stub ZwCreateEventPair
@ stdcall ZwCreateFile(ptr long ptr ptr long long long ptr long long ptr) NtCreateFile
@ stub ZwCreateIoCompletion
@ stdcall ZwCreateKey(long long long long long long long) NtCreateKey
@ stdcall ZwCreateMailslotFile(long long long long long long long long) NtCreateMailslotFile
@ stub ZwCreateMutant
@ stub ZwCreateNamedPipeFile
@ stdcall ZwCreatePagingFile(long long long long) NtCreatePagingFile
@ stdcall ZwCreatePort(long long long long long) NtCreatePort
@ stub ZwCreateProcess
@ stub ZwCreateProfile
@ stdcall ZwCreateSection(long long long long long long long) NtCreateSection
@ stub ZwCreateSemaphore
@ stub ZwCreateSymbolicLinkObject
@ stub ZwCreateThread
@ stdcall ZwCreateTimer(ptr long ptr long) NtCreateTimer
@ stub ZwCreateToken
@ stub ZwDelayExecution
@ stub ZwDeleteFile
@ stub ZwDeleteKey
@ stub ZwDeleteValueKey
@ stdcall ZwDeviceIoControlFile(long long long long long long long long long long) NtDeviceIoControlFile
@ stub ZwDisplayString
@ stdcall ZwDuplicateObject(long long long long long long long) NtDuplicateObject
@ stdcall ZwDuplicateToken(long long long long long long) NtDuplicateToken
@ stub ZwEnumerateBus
@ stub ZwEnumerateKey
@ stub ZwEnumerateValueKey
@ stub ZwExtendSection
@ stub ZwFlushBuffersFile
@ stub ZwFlushInstructionCache
@ stub ZwFlushKey
@ stub ZwFlushVirtualMemory
@ stub ZwFlushWriteBuffer
@ stub ZwFreeVirtualMemory
@ stdcall ZwFsControlFile(long long long long long long long long long long) NtFsControlFile
@ stub ZwGetContextThread
@ stub ZwGetPlugPlayEvent
@ stub ZwGetTickCount
@ stub ZwImpersonateClientOfPort
@ stub ZwImpersonateThread
@ stub ZwInitializeRegistry
@ stdcall ZwListenPort(long long) NtListenPort
@ stub ZwLoadDriver
@ stub ZwLoadKey
@ stub ZwLockFile
@ stub ZwLockVirtualMemory
@ stub ZwMakeTemporaryObject
@ stdcall ZwMapViewOfSection(long long long long long long long long long long) NtMapViewOfSection
@ stub ZwNotifyChangeDirectoryFile
@ stub ZwNotifyChangeKey
@ stdcall ZwOpenDirectoryObject(long long long) NtOpenDirectoryObject
@ stdcall ZwOpenEvent(long long long) NtOpenEvent
@ stub ZwOpenEventPair
@ stdcall ZwOpenFile(ptr long ptr ptr long long) NtOpenFile
@ stub ZwOpenIoCompletion
@ stdcall ZwOpenKey(ptr long ptr) NtOpenKey
@ stub ZwOpenMutant
@ stub ZwOpenObjectAuditAlarm
@ stub ZwOpenProcess
@ stdcall ZwOpenProcessToken(long long long) NtOpenProcessToken
@ stdcall ZwOpenSection(long long long) NtOpenSection
@ stub ZwOpenSemaphore
@ stub ZwOpenSymbolicLinkObject
@ stub ZwOpenThread
@ stdcall ZwOpenThreadToken(long long long long) NtOpenThreadToken
@ stub ZwOpenTimer
@ stub ZwPlugPlayControl
@ stub ZwPrivilegeCheck
@ stub ZwPrivilegeObjectAuditAlarm
@ stub ZwPrivilegedServiceAuditAlarm
@ stub ZwProtectVirtualMemory
@ stub ZwPulseEvent
@ stub ZwQueryAttributesFile
@ stub ZwQueryDefaultLocale
@ stdcall ZwQueryDirectoryFile(long long  ptr ptr ptr ptr long long long ptr long)NtQueryDirectoryFile
@ stdcall ZwQueryDirectoryObject(long long long long long long long) NtQueryDirectoryObject
@ stub ZwQueryEaFile
@ stub ZwQueryEvent
@ stdcall ZwQueryInformationFile(long long long long long) NtQueryInformationFile
@ stub ZwQueryInformationPort
@ stdcall ZwQueryInformationProcess(long long long long long) NtQueryInformationProcess
@ stdcall ZwQueryInformationThread(long long long long long) NtQueryInformationThread
@ stdcall ZwQueryInformationToken(long long long long long) NtQueryInformationToken
@ stub ZwQueryIntervalProfile
@ stub ZwQueryIoCompletion
@ stub ZwQueryKey
@ stub ZwQueryMutant
@ stdcall ZwQueryObject(long long long long long) NtQueryObject
@ stub ZwQueryPerformanceCounter
@ stub ZwQuerySection
@ stub ZwQuerySecurityObject
@ stub ZwQuerySemaphore
@ stub ZwQuerySymbolicLinkObject
@ stub ZwQuerySystemEnvironmentValue
@ stdcall ZwQuerySystemInformation(long long long long) NtQuerySystemInformation
@ stdcall ZwQuerySystemTime(ptr) GetSystemTimeAsFileTime
@ stub ZwQueryTimer
@ stub ZwQueryTimerResolution
@ stub ZwQueryValueKey
@ stub ZwQueryVirtualMemory
@ stub ZwQueryVolumeInformationFile
@ stub ZwRaiseException
@ stub ZwRaiseHardError
@ stdcall ZwReadFile(long long long long long long long long long) NtReadFile
@ stub ZwReadRequestData
@ stub ZwReadVirtualMemory
@ stub ZwRegisterNewDevice
@ stdcall ZwRegisterThreadTerminatePort(long) NtRegisterThreadTerminatePort
@ stub ZwReleaseMutant
@ stub ZwReleaseProcessMutant
@ stub ZwReleaseSemaphore
@ stub ZwRemoveIoCompletion
@ stub ZwReplaceKey
@ stub ZwReplyPort
@ stdcall ZwReplyWaitReceivePort(long long long long) NtReplyWaitReceivePort
@ stub ZwReplyWaitReplyPort
@ stub ZwRequestPort
@ stdcall ZwRequestWaitReplyPort(long long long) NtRequestWaitReplyPort
@ stub ZwResetEvent
@ stub ZwRestoreKey
@ stdcall ZwResumeThread(long long) NtResumeThread
@ stub ZwSaveKey
@ stub ZwSetContextThread
@ stub ZwSetDefaultHardErrorPort
@ stub ZwSetDefaultLocale
@ stub ZwSetEaFile
@ stdcall ZwSetEvent(long long) NtSetEvent
@ stub ZwSetHighEventPair
@ stub ZwSetHighWaitLowEventPair
@ stub ZwSetHighWaitLowThread
@ stdcall ZwSetInformationFile(long long long long long) NtSetInformationFile
@ stub ZwSetInformationKey
@ stub ZwSetInformationObject
@ stdcall ZwSetInformationProcess(long long long long) NtSetInformationProcess
@ stdcall ZwSetInformationThread(long long long long) NtSetInformationThread
@ stub ZwSetInformationToken
@ stdcall ZwSetIntervalProfile(long long) NtSetIntervalProfile
@ stub ZwSetIoCompletion
@ stub ZwSetLdtEntries
@ stub ZwSetLowEventPair
@ stub ZwSetLowWaitHighEventPair
@ stub ZwSetLowWaitHighThread
@ stub ZwSetSecurityObject
@ stub ZwSetSystemEnvironmentValue
@ stub ZwSetSystemInformation
@ stub ZwSetSystemPowerState
@ stub ZwSetSystemTime
@ stdcall ZwSetTimer(long ptr ptr ptr long long ptr) NtSetTimer
@ stub ZwSetTimerResolution
@ stdcall ZwSetValueKey(long long long long long long) NtSetValueKey
@ stdcall ZwSetVolumeInformationFile(long ptr ptr long long) NtSetVolumeInformationFile
@ stub ZwShutdownSystem
@ stub ZwStartProfile
@ stub ZwStopProfile
@ stub ZwSuspendThread
@ stub ZwSystemDebugControl
@ stub ZwTerminateProcess
@ stdcall ZwTerminateThread(long long) NtTerminateThread
@ stub ZwTestAlert
@ stub ZwUnloadDriver
@ stub ZwUnloadKey
@ stub ZwUnlockFile
@ stub ZwUnlockVirtualMemory
@ stub ZwUnmapViewOfSection
@ stub ZwVdmControl
@ stub ZwW32Call
@ stub ZwWaitForMultipleObjects
@ stub ZwWaitForProcessMutant
@ stdcall ZwWaitForSingleObject(long long long) NtWaitForSingleObject
@ stub ZwWaitHighEventPair
@ stub ZwWaitLowEventPair
@ stub ZwWriteFile
@ stub ZwWriteRequestData
@ stub ZwWriteVirtualMemory
@ stub _CIpow
@ stub __eCommonExceptions
@ stub __eEmulatorInit
@ stub __eF2XM1
@ stub __eFABS
@ stub __eFADD32
@ stub __eFADD64
@ stub __eFADDPreg
@ stub __eFADDreg
@ stub __eFADDtop
@ stub __eFCHS
@ stub __eFCOM
@ stub __eFCOM32
@ stub __eFCOM64
@ stub __eFCOMP
@ stub __eFCOMP32
@ stub __eFCOMP64
@ stub __eFCOMPP
@ stub __eFCOS
@ stub __eFDECSTP
@ stub __eFDIV32
@ stub __eFDIV64
@ stub __eFDIVPreg
@ stub __eFDIVR32
@ stub __eFDIVR64
@ stub __eFDIVRPreg
@ stub __eFDIVRreg
@ stub __eFDIVRtop
@ stub __eFDIVreg
@ stub __eFDIVtop
@ stub __eFFREE
@ stub __eFIADD16
@ stub __eFIADD32
@ stub __eFICOM16
@ stub __eFICOM32
@ stub __eFICOMP16
@ stub __eFICOMP32
@ stub __eFIDIV16
@ stub __eFIDIV32
@ stub __eFIDIVR16
@ stub __eFIDIVR32
@ stub __eFILD16
@ stub __eFILD32
@ stub __eFILD64
@ stub __eFIMUL16
@ stub __eFIMUL32
@ stub __eFINCSTP
@ stub __eFINIT
@ stub __eFIST16
@ stub __eFIST32
@ stub __eFISTP16
@ stub __eFISTP32
@ stub __eFISTP64
@ stub __eFISUB16
@ stub __eFISUB32
@ stub __eFISUBR16
@ stub __eFISUBR32
@ stub __eFLD1
@ stub __eFLD32
@ stub __eFLD64
@ stub __eFLD80
@ stub __eFLDCW
@ stub __eFLDENV
@ stub __eFLDL2E
@ stub __eFLDLN2
@ stub __eFLDPI
@ stub __eFLDZ
@ stub __eFMUL32
@ stub __eFMUL64
@ stub __eFMULPreg
@ stub __eFMULreg
@ stub __eFMULtop
@ stub __eFPATAN
@ stub __eFPREM
@ stub __eFPREM1
@ stub __eFPTAN
@ stub __eFRNDINT
@ stub __eFRSTOR
@ stub __eFSAVE
@ stub __eFSCALE
@ stub __eFSIN
@ stub __eFSQRT
@ stub __eFST
@ stub __eFST32
@ stub __eFST64
@ stub __eFSTCW
@ stub __eFSTENV
@ stub __eFSTP
@ stub __eFSTP32
@ stub __eFSTP64
@ stub __eFSTP80
@ stub __eFSTSW
@ stub __eFSUB32
@ stub __eFSUB64
@ stub __eFSUBPreg
@ stub __eFSUBR32
@ stub __eFSUBR64
@ stub __eFSUBRPreg
@ stub __eFSUBRreg
@ stub __eFSUBRtop
@ stub __eFSUBreg
@ stub __eFSUBtop
@ stub __eFTST
@ stub __eFUCOM
@ stub __eFUCOMP
@ stub __eFUCOMPP
@ stub __eFXAM
@ stub __eFXCH
@ stub __eFXTRACT
@ stub __eFYL2X
@ stub __eFYL2XP1
@ stub __eGetStatusWord
@ register _alloca_probe() NTDLL_alloca_probe
@ register _chkstk() NTDLL_chkstk
@ stub _fltused
@ cdecl _ftol(double) CRTDLL__ftol
@ cdecl _itoa(long ptr long) CRTDLL__itoa
@ stub _ltoa
@ stub _memccpy
@ cdecl _memicmp(str str long) CRTDLL__memicmp
@ stub _snprintf
@ stub _snwprintf
@ stub _splitpath
@ cdecl _strcmpi(str str) CRTDLL__strcmpi
@ cdecl _stricmp(str str) CRTDLL__strcmpi
@ cdecl _strlwr(str) CRTDLL__strlwr
@ cdecl _strnicmp(str str long) CRTDLL__strnicmp
@ cdecl _strupr(str) CRTDLL__strupr
@ cdecl _ultoa(long ptr long) CRTDLL__ultoa
@ stub _vsnprintf
@ cdecl _wcsicmp(wstr wstr) CRTDLL__wcsicmp
@ cdecl _wcslwr(wstr) CRTDLL__wcslwr
@ cdecl _wcsnicmp(wstr wstr long) CRTDLL__wcsnicmp
@ cdecl _wcsupr(wstr) CRTDLL__wcsupr
@ stub abs
@ stub atan
@ cdecl atoi(str) atoi
@ cdecl atol(str) atol
@ stub ceil
@ stub cos
@ stub fabs
@ stub floor
@ cdecl isalpha(long) isalpha
@ cdecl isdigit(long) isdigit
@ cdecl islower(long) islower
@ cdecl isprint(long) isprint
@ cdecl isspace(long) isspace
@ cdecl isupper(long) isupper
@ cdecl iswalpha(long) CRTDLL_iswalpha
@ cdecl iswctype(long long) CRTDLL_iswctype
@ cdecl isxdigit(long) isxdigit
@ stub labs
@ stub log
@ cdecl mbstowcs(ptr str long) CRTDLL_mbstowcs
@ cdecl memchr(ptr long long) memchr
@ cdecl memcmp(ptr ptr long) memcmp
@ cdecl memcpy(ptr ptr long) memcpy
@ cdecl memmove(ptr ptr long) memmove
@ cdecl memset(ptr long long) memset
@ stub pow
@ stub qsort
@ stub sin
@ varargs sprintf(str str) wsprintfA
@ stub sqrt
@ varargs sscanf() sscanf
@ cdecl strcat(str str) strcat
@ cdecl strchr(str long) strchr
@ cdecl strcmp(str str) strcmp
@ cdecl strcpy(ptr str) strcpy
@ cdecl strcspn(str str) strcspn
@ cdecl strlen(str) strlen
@ cdecl strncat(str str long) strncat
@ cdecl strncmp(str str long) strncmp
@ cdecl strncpy(ptr str long) strncpy
@ cdecl strpbrk(str str long) strpbrk
@ cdecl strrchr(str long) strrchr
@ cdecl strspn(str str) strspn
@ cdecl strstr(str str) strstr
@ varargs swprintf(wstr wstr) wsprintfW
@ stub tan
@ cdecl tolower(long) tolower
@ cdecl toupper(long) toupper
@ cdecl towlower(long) towlower
@ cdecl towupper(long) towupper
@ cdecl vsprintf(ptr str ptr) CRTDLL_vsprintf
@ cdecl wcscat(wstr wstr) CRTDLL_wcscat
@ cdecl wcschr(wstr long) CRTDLL_wcschr
@ cdecl wcscmp(wstr wstr) CRTDLL_wcscmp
@ cdecl wcscpy(ptr wstr) CRTDLL_wcscpy
@ cdecl wcscspn(wstr wstr) CRTDLL_wcscspn
@ cdecl wcslen(wstr) CRTDLL_wcslen
@ cdecl wcsncat(wstr wstr long) CRTDLL_wcsncat
@ cdecl wcsncmp(wstr wstr long) CRTDLL_wcsncmp
@ cdecl wcsncpy(ptr wstr long) CRTDLL_wcsncpy
@ cdecl wcspbrk(wstr wstr) CRTDLL_wcspbrk
@ cdecl wcsrchr(wstr long) CRTDLL_wcsrchr
@ cdecl wcsspn(wstr wstr) CRTDLL_wcsspn
@ cdecl wcsstr(wstr wstr) CRTDLL_wcsstr
@ cdecl wcstok(wstr wstr) CRTDLL_wcstok
@ cdecl wcstol(wstr ptr long) CRTDLL_wcstol
@ cdecl wcstombs(ptr ptr long) CRTDLL_wcstombs
@ stub wcstoul
@ stub NtAddAtom
@ stub NtDeleteAtom
@ stub NtFindAtom
@ stub NtQueryFullAttributesFile
@ stub NtQueueApcThread
@ stub NtReadFileScatter
@ stub NtSignalAndWaitForSingleObject
@ stub NtWriteFileGather
@ stub NtYieldExecution
@ stub RtlAddAtomToAtomTable
@ stub RtlAllocateHandle
@ stub RtlCreateAtomTable
@ stub RtlDeleteAtomFromAtomTable
@ stub RtlFreeHandle
@ stub RtlInitializeHandleTable
@ stub RtlIsValidHandle
@ stub RtlLookupAtomInAtomTable
@ stub RtlQueryAtomInAtomTable
@ stdcall RtlTryEnterCriticalSection(ptr) TryEnterCriticalSection
@ stub RtlEnumerateProperties
@ stub RtlSetPropertyClassId
@ stub RtlSetPropertyNames
@ stub RtlQueryPropertyNames
@ stub RtlFlushPropertySet
@ stub RtlSetProperties
@ stub RtlQueryProperties
@ stub RtlQueryPropertySet
@ stub RtlSetUnicodeCallouts
@ stub RtlPropertySetNameToGuid
@ stub RtlGuidToPropertySetName
@ stub RtlClosePropertySet
@ stub RtlCreatePropertySet
@ stub _wtoi
@ stub _wtol
@ stub RtlSetPropertySetClassId
@ stdcall NtPowerInformation(long long long long long) NtPowerInformation
