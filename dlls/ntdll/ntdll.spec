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
@ stdcall DbgBreakPoint()
@ varargs DbgPrint(str)
@ stub DbgPrompt
@ stub DbgSsHandleKmApiMsg
@ stub DbgSsInitialize
@ stub DbgUiConnectToDbg
@ stub DbgUiContinue
@ stub DbgUiWaitStateChange
@ stdcall DbgUserBreakPoint()
@ stub KiUserApcDispatcher
@ stub KiUserCallbackDispatcher
@ stub KiUserExceptionDispatcher
@ stdcall LdrAccessResource(long ptr ptr ptr)
@ stdcall LdrDisableThreadCalloutsForDll(long)
@ stub LdrEnumResources
@ stdcall LdrFindEntryForAddress(ptr ptr)
@ stdcall LdrFindResourceDirectory_U(long ptr long ptr)
@ stdcall LdrFindResource_U(long ptr long ptr)
@ stdcall LdrGetDllHandle(long long ptr ptr)
@ stdcall LdrGetProcedureAddress(ptr ptr long ptr)
@ stdcall LdrInitializeThunk(long long long long)
@ stdcall LdrLoadDll(wstr long ptr ptr)
@ stdcall LdrLockLoaderLock(long ptr ptr)
@ stub LdrProcessRelocationBlock
@ stub LdrQueryImageFileExecutionOptions
@ stdcall LdrQueryProcessModuleInformation(ptr long ptr)
@ stdcall LdrShutdownProcess()
@ stdcall LdrShutdownThread()
@ stdcall LdrUnloadDll(ptr)
@ stdcall LdrUnlockLoaderLock(long long)
@ stub LdrVerifyImageMatchesChecksum
@ stub NPXEMULATORTABLE
@ extern NlsAnsiCodePage
@ extern NlsMbCodePageTag
@ extern NlsMbOemCodePageTag
@ stdcall NtAcceptConnectPort(ptr long ptr long long ptr)
@ stdcall NtAccessCheck(ptr long long ptr ptr ptr ptr ptr)
@ stub NtAccessCheckAndAuditAlarm
@ stub NtAdjustGroupsToken
@ stdcall NtAdjustPrivilegesToken(long long long long long long)
@ stub NtAlertResumeThread
@ stub NtAlertThread
@ stdcall NtAllocateLocallyUniqueId(ptr)
@ stdcall NtAllocateUuids(ptr ptr ptr)
@ stdcall NtAllocateVirtualMemory(long ptr ptr ptr long long)
@ stub NtCallbackReturn
@ stub NtCancelIoFile
@ stdcall NtCancelTimer(long ptr)
@ stdcall NtClearEvent(long)
@ stdcall NtClose(long)
@ stub NtCloseObjectAuditAlarm
@ stdcall NtCompleteConnectPort(ptr)
@ stdcall NtConnectPort(ptr ptr ptr ptr ptr ptr ptr ptr)
@ stub NtContinue
@ stdcall NtCreateDirectoryObject(long long long)
@ stdcall NtCreateEvent(long long long long long)
@ stub NtCreateEventPair
@ stdcall NtCreateFile(ptr long ptr ptr long long long ptr long long ptr)
@ stub NtCreateIoCompletion
@ stdcall NtCreateKey(long long long long long long long)
@ stdcall NtCreateMailslotFile(long long long long long long long long)
@ stub NtCreateMutant
@ stub NtCreateNamedPipeFile
@ stdcall NtCreatePagingFile(long long long long)
@ stdcall NtCreatePort(ptr ptr long long long)
@ stub NtCreateProcess
@ stub NtCreateProfile
@ stdcall NtCreateSection(ptr long ptr ptr long long long)
@ stdcall NtCreateSemaphore(ptr long ptr long long)
@ stdcall NtCreateSymbolicLinkObject(ptr long ptr ptr)
@ stub NtCreateThread
@ stdcall NtCreateTimer(ptr long ptr long)
@ stub NtCreateToken
@ stdcall NtCurrentTeb()
@ stdcall NtDelayExecution(long ptr)
@ stub NtDeleteFile
@ stdcall NtDeleteKey(long)
@ stdcall NtDeleteValueKey(long ptr)
@ stdcall NtDeviceIoControlFile(long long long long long long long long long long)
@ stdcall NtDisplayString(ptr)
@ stdcall NtDuplicateObject(long long long ptr long long long)
@ stdcall NtDuplicateToken(long long long long long long)
@ stub NtEnumerateBus
@ stdcall NtEnumerateKey (long long long long long long)
@ stdcall NtEnumerateValueKey (long long long long long long)
@ stub NtExtendSection
@ stdcall NtFlushBuffersFile(long ptr)
@ stub NtFlushInstructionCache
@ stdcall NtFlushKey(long)
@ stdcall NtFlushVirtualMemory(long ptr ptr long)
@ stub NtFlushWriteBuffer
@ stdcall NtFreeVirtualMemory(long ptr ptr long)
@ stdcall NtFsControlFile(long long long long long long long long long long)
@ stdcall NtGetContextThread(long ptr)
@ stub NtGetPlugPlayEvent
@ stub NtGetTickCount
@ stub NtImpersonateClientOfPort
@ stub NtImpersonateThread
@ stub NtInitializeRegistry
@ stdcall NtListenPort(ptr ptr)
@ stub NtLoadDriver
@ stdcall NtLoadKey(ptr ptr)
@ stdcall NtLockFile(long long ptr ptr ptr ptr ptr ptr long long)
@ stdcall NtLockVirtualMemory(long ptr ptr long)
@ stub NtMakeTemporaryObject
@ stdcall NtMapViewOfSection(long long ptr long long ptr ptr long long long)
@ stub NtNotifyChangeDirectoryFile
@ stdcall NtNotifyChangeKey(long long ptr ptr ptr long long ptr long long)
@ stdcall NtOpenDirectoryObject(long long long)
@ stdcall NtOpenEvent(long long long)
@ stub NtOpenEventPair
@ stdcall NtOpenFile(ptr long ptr ptr long long)
@ stub NtOpenIoCompletion
@ stdcall NtOpenKey(ptr long ptr)
@ stub NtOpenMutant
@ stub NtOpenObjectAuditAlarm
@ stub NtOpenProcess
@ stdcall NtOpenProcessToken(long long long)
@ stdcall NtOpenSection(ptr long ptr)
@ stdcall NtOpenSemaphore(long long ptr)
@ stdcall NtOpenSymbolicLinkObject (long long long)
@ stdcall NtOpenThread(ptr long ptr ptr)
@ stdcall NtOpenThreadToken(long long long long)
@ stdcall NtOpenTimer(ptr long ptr)
@ stub NtPlugPlayControl
@ stub NtPrivilegeCheck
@ stub NtPrivilegeObjectAuditAlarm
@ stub NtPrivilegedServiceAuditAlarm
@ stdcall NtProtectVirtualMemory(long ptr ptr long ptr)
@ stdcall NtPulseEvent(long ptr)
@ stub NtQueryAttributesFile
@ stdcall NtQueryDefaultLocale(long ptr)
@ stdcall NtQueryDirectoryFile(long long  ptr ptr ptr ptr long long long ptr long)
@ stdcall NtQueryDirectoryObject(long ptr long long long ptr ptr)
@ stub NtQueryEaFile
@ stdcall NtQueryEvent(long long ptr long ptr)
@ stdcall NtQueryInformationFile(long ptr ptr long long)
@ stub NtQueryInformationPort
@ stdcall NtQueryInformationProcess(long long ptr long ptr)
@ stdcall NtQueryInformationThread(long long ptr long ptr)
@ stdcall NtQueryInformationToken(long long ptr long ptr)
@ stub NtQueryIntervalProfile
@ stub NtQueryIoCompletion
@ stdcall NtQueryKey (long long ptr long ptr)
@ stub NtQueryMutant
@ stdcall NtQueryObject(long long long long long)
@ stub NtQueryOpenSubKeys
@ stdcall NtQueryPerformanceCounter (long long)
@ stdcall NtQuerySection (long long long long long)
@ stdcall NtQuerySecurityObject (long long long long long)
@ stdcall NtQuerySemaphore (long long long long long)
@ stdcall NtQuerySymbolicLinkObject(long ptr ptr)
@ stub NtQuerySystemEnvironmentValue
@ stdcall NtQuerySystemInformation(long long long long)
@ stdcall NtQuerySystemTime(ptr)
@ stub NtQueryTimer
@ stdcall NtQueryTimerResolution(long long long)
@ stdcall NtQueryValueKey(long long long long long long)
@ stdcall NtQueryVirtualMemory(long ptr long ptr long ptr)
@ stdcall NtQueryVolumeInformationFile(long ptr ptr long long)
@ stdcall NtQueueApcThread(long ptr long long long)
@ stdcall NtRaiseException(ptr ptr long)
@ stub NtRaiseHardError
@ stdcall NtReadFile(long long long long long long long long long)
@ stub NtReadRequestData
@ stdcall NtReadVirtualMemory(long ptr ptr long ptr)
@ stub NtRegisterNewDevice
@ stdcall NtRegisterThreadTerminatePort(ptr)
@ stub NtReleaseMutant
@ stub NtReleaseProcessMutant
@ stdcall NtReleaseSemaphore(long long ptr)
@ stub NtRemoveIoCompletion
@ stdcall NtReplaceKey(ptr long ptr)
@ stub NtReplyPort
@ stdcall NtReplyWaitReceivePort(ptr ptr ptr ptr)
@ stub NtReplyWaitReceivePortEx
@ stub NtReplyWaitReplyPort
@ stub NtRequestPort
@ stdcall NtRequestWaitReplyPort(ptr ptr ptr)
@ stdcall NtResetEvent(long ptr)
@ stdcall NtRestoreKey(long long long)
@ stdcall NtResumeThread(long long)
@ stdcall NtSaveKey(long long)
@ stub NtSecureConnectPort
@ stdcall NtSetContextThread(long ptr)
@ stub NtSetDefaultHardErrorPort
@ stdcall NtSetDefaultLocale(long long)
@ stub NtSetEaFile
@ stdcall NtSetEvent(long long)
@ stub NtSetHighEventPair
@ stub NtSetHighWaitLowEventPair
@ stub NtSetHighWaitLowThread
@ stdcall NtSetInformationFile(long long long long long)
@ stdcall NtSetInformationKey(long long ptr long)
@ stdcall NtSetInformationObject(long long ptr long)
@ stdcall NtSetInformationProcess(long long long long)
@ stdcall NtSetInformationThread(long long ptr long)
@ stub NtSetInformationToken
@ stdcall NtSetIntervalProfile(long long)
@ stub NtSetIoCompletion
@ stub NtSetLdtEntries
@ stub NtSetLowEventPair
@ stub NtSetLowWaitHighEventPair
@ stub NtSetLowWaitHighThread
@ stdcall NtSetSecurityObject(long long ptr)
@ stub NtSetSystemEnvironmentValue
@ stub NtSetSystemInformation
@ stub NtSetSystemPowerState
@ stdcall NtSetSystemTime(ptr ptr)
@ stdcall NtSetTimer(long ptr ptr ptr long long ptr)
@ stdcall NtSetTimerResolution(long long ptr)
@ stdcall NtSetValueKey(long long long long long long)
@ stdcall NtSetVolumeInformationFile(long ptr ptr long long)
@ stub NtShutdownSystem
@ stub NtStartProfile
@ stub NtStopProfile
@ stdcall NtSuspendThread(long ptr)
@ stub NtSystemDebugControl
@ stdcall NtTerminateProcess(long long)
@ stdcall NtTerminateThread(long long)
@ stub NtTestAlert
@ stub NtUnloadDriver
@ stdcall NtUnloadKey(long)
@ stub NtUnloadKeyEx
@ stdcall NtUnlockFile(long ptr ptr ptr ptr)
@ stdcall NtUnlockVirtualMemory(long ptr ptr long)
@ stdcall NtUnmapViewOfSection(long ptr)
@ stub NtVdmControl
@ stub NtW32Call
@ stdcall NtWaitForMultipleObjects(long ptr long long ptr)
@ stub NtWaitForProcessMutant
@ stdcall NtWaitForSingleObject(long long long)
@ stub NtWaitHighEventPair
@ stub NtWaitLowEventPair
@ stdcall NtWriteFile(long long ptr ptr ptr ptr long ptr ptr)
@ stub NtWriteRequestData
@ stdcall NtWriteVirtualMemory(long ptr ptr long ptr)
@ stub PfxFindPrefix
@ stub PfxInitialize
@ stub PfxInsertPrefix
@ stub PfxRemovePrefix
@ stub RestoreEm87Context
@ stub RtlAbortRXact
@ stub RtlAbsoluteToSelfRelativeSD
@ stdcall RtlAcquirePebLock()
@ stdcall RtlAcquireResourceExclusive(ptr long)
@ stdcall RtlAcquireResourceShared(ptr long)
@ stdcall RtlAddAccessAllowedAce(ptr long long ptr)
@ stdcall RtlAddAccessAllowedAceEx(ptr long long long ptr)
@ stdcall RtlAddAccessDeniedAce(ptr long long ptr)
@ stdcall RtlAddAccessDeniedAceEx(ptr long long long ptr)
@ stdcall RtlAddAce(ptr long long ptr long)
@ stub RtlAddActionToRXact
@ stub RtlAddAttributeActionToRXact
@ stub RtlAddAuditAccessAce
@ stdcall RtlAddVectoredExceptionHandler(long ptr)
@ stdcall RtlAdjustPrivilege(long long long long)
@ stdcall RtlAllocateAndInitializeSid (ptr long long long long long long long long long ptr)
@ stdcall RtlAllocateHeap(long long long)
@ stub RtlAnsiCharToUnicodeChar
@ stdcall RtlAnsiStringToUnicodeSize(ptr)
@ stdcall RtlAnsiStringToUnicodeString(ptr ptr long)
@ stdcall RtlAppendAsciizToString(ptr str)
@ stdcall RtlAppendStringToString(ptr ptr)
@ stdcall RtlAppendUnicodeStringToString(ptr ptr)
@ stdcall RtlAppendUnicodeToString(ptr wstr)
@ stub RtlApplyRXact
@ stub RtlApplyRXactNoFlush
@ stdcall RtlAreAllAccessesGranted(long long)
@ stdcall RtlAreAnyAccessesGranted(long long)
@ stdcall RtlAreBitsClear(ptr long long)
@ stdcall RtlAreBitsSet(ptr long long)
@ stdcall RtlAssert(ptr ptr long long)
@ stub RtlCaptureStackBackTrace
@ stdcall RtlCharToInteger(ptr long ptr)
@ stub RtlCheckRegistryKey
@ stdcall RtlClearAllBits(ptr)
@ stdcall RtlClearBits(ptr long long)
@ stdcall RtlCompactHeap(long long)
@ stdcall RtlCompareMemory(ptr ptr long)
@ stdcall RtlCompareMemoryUlong(ptr long long)
@ stdcall RtlCompareString(ptr ptr long)
@ stdcall RtlCompareUnicodeString (ptr ptr long)
@ stub RtlCompressBuffer
@ stdcall RtlComputeCrc32(long ptr long)
@ stub RtlConsoleMultiByteToUnicodeN
@ stub RtlConvertExclusiveToShared
@ stdcall -ret64 RtlConvertLongToLargeInteger(long)
@ stub RtlConvertSharedToExclusive
@ stdcall RtlConvertSidToUnicodeString(ptr ptr long)
@ stub RtlConvertUiListToApiList
@ stdcall -ret64 RtlConvertUlongToLargeInteger(long)
@ stdcall RtlCopyLuid(ptr ptr)
@ stdcall RtlCopyLuidAndAttributesArray(long ptr ptr)
@ stub RtlCopySecurityDescriptor
@ stdcall RtlCopySid(long ptr ptr)
@ stub RtlCopySidAndAttributesArray
@ stdcall RtlCopyString(ptr ptr)
@ stdcall RtlCopyUnicodeString(ptr ptr)
@ stdcall RtlCreateAcl(ptr long long)
@ stub RtlCreateAndSetSD
@ stdcall RtlCreateEnvironment(long ptr)
@ stdcall RtlCreateHeap(long ptr long long ptr ptr)
@ stdcall RtlCreateProcessParameters(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stub RtlCreateQueryDebugBuffer
@ stub RtlCreateRegistryKey
@ stdcall RtlCreateSecurityDescriptor(ptr long)
@ stub RtlCreateTagHeap
@ stdcall RtlCreateUnicodeString(ptr wstr)
@ stdcall RtlCreateUnicodeStringFromAsciiz(ptr str)
@ stub RtlCreateUserProcess
@ stub RtlCreateUserSecurityObject
@ stdcall RtlCreateUserThread(long ptr long ptr long long ptr ptr ptr ptr)
@ stub RtlCustomCPToUnicodeN
@ stub RtlCutoverTimeToSystemTime
@ stdcall RtlDeNormalizeProcessParams(ptr)
@ stub RtlDecompressBuffer
@ stub RtlDecompressFragment
@ stub RtlDelete
@ stdcall RtlDeleteAce(ptr long)
@ stdcall RtlDeleteCriticalSection(ptr)
@ stub RtlDeleteElementGenericTable
@ stub RtlDeleteRegistryValue
@ stdcall RtlDeleteResource(ptr)
@ stdcall RtlDeleteSecurityObject(long)
@ stdcall RtlDestroyEnvironment(ptr)
@ stdcall RtlDestroyHeap(long)
@ stdcall RtlDestroyProcessParameters(ptr)
@ stub RtlDestroyQueryDebugBuffer
@ stdcall RtlDetermineDosPathNameType_U(wstr)
@ stdcall RtlDoesFileExists_U(wstr)
@ stdcall RtlDosPathNameToNtPathName_U(wstr ptr ptr ptr)
@ stdcall RtlDosSearchPath_U(wstr wstr wstr long ptr ptr)
@ stdcall RtlDowncaseUnicodeChar(long)
@ stdcall RtlDowncaseUnicodeString(ptr ptr long)
@ stdcall RtlDumpResource(ptr)
@ stdcall RtlDuplicateUnicodeString(long ptr ptr)
@ stdcall -ret64 RtlEnlargedIntegerMultiply(long long)
@ stdcall RtlEnlargedUnsignedDivide(long long long ptr)
@ stdcall -ret64 RtlEnlargedUnsignedMultiply(long long)
@ stdcall RtlEnterCriticalSection(ptr)
@ stub RtlEnumProcessHeaps
@ stub RtlEnumerateGenericTable
@ stub RtlEnumerateGenericTableWithoutSplaying
@ stdcall RtlEqualComputerName(ptr ptr)
@ stdcall RtlEqualDomainName(ptr ptr)
@ stdcall RtlEqualLuid(ptr ptr)
@ stdcall RtlEqualPrefixSid(ptr ptr)
@ stdcall RtlEqualSid (long long)
@ stdcall RtlEqualString(ptr ptr long)
@ stdcall RtlEqualUnicodeString(ptr ptr long)
@ stdcall RtlEraseUnicodeString(ptr)
@ stdcall RtlExpandEnvironmentStrings_U(ptr ptr ptr ptr)
@ stub RtlExtendHeap
@ stdcall -ret64 RtlExtendedIntegerMultiply(long long long)
@ stdcall -ret64 RtlExtendedLargeIntegerDivide(long long long ptr)
@ stdcall -ret64 RtlExtendedMagicDivide(long long long long long)
@ stdcall RtlFillMemory(ptr long long)
@ stdcall RtlFillMemoryUlong(ptr long long)
@ stdcall RtlFindCharInUnicodeString(long ptr ptr ptr)
@ stdcall RtlFindClearBits(ptr long long)
@ stdcall RtlFindClearBitsAndSet(ptr long long)
@ stdcall RtlFindClearRuns(ptr ptr long long)
@ stdcall RtlFindLastBackwardRunClear(ptr long ptr)
@ stdcall RtlFindLastBackwardRunSet(ptr long ptr)
@ stdcall RtlFindLeastSignificantBit(long long)
@ stdcall RtlFindLongestRunClear(ptr long)
@ stdcall RtlFindLongestRunSet(ptr long)
@ stdcall RtlFindMessage(long long long long ptr)
@ stdcall RtlFindMostSignificantBit(long long)
@ stdcall RtlFindNextForwardRunClear(ptr long ptr)
@ stdcall RtlFindNextForwardRunSet(ptr long ptr)
@ stdcall RtlFindSetBits(ptr long long)
@ stdcall RtlFindSetBitsAndClear(ptr long long)
@ stdcall RtlFindSetRuns(ptr ptr long long)
@ stdcall RtlFirstFreeAce(ptr ptr)
@ stdcall RtlFormatCurrentUserKeyPath(ptr)
@ stub RtlFormatMessage
@ stdcall RtlFreeAnsiString(long)
@ stdcall RtlFreeHeap(long long long)
@ stdcall RtlFreeOemString(ptr)
@ stdcall RtlFreeSid (long)
@ stdcall RtlFreeUnicodeString(ptr)
@ stub RtlGenerate8dot3Name
@ stdcall RtlGetAce(ptr long ptr)
@ stdcall RtlGetNtVersionNumbers(ptr ptr ptr)
@ stdcall RtlGetVersion(ptr)
@ stub RtlGetCallersAddress
@ stub RtlGetCompressionWorkSpaceSize
@ stdcall RtlGetControlSecurityDescriptor(ptr ptr ptr)
@ stdcall RtlGetCurrentDirectory_U(long ptr)
@ stdcall RtlGetDaclSecurityDescriptor(ptr ptr ptr ptr)
@ stub RtlGetElementGenericTable
@ stdcall RtlGetFullPathName_U(wstr long ptr ptr)
@ stdcall RtlGetGroupSecurityDescriptor(ptr ptr ptr)
@ stdcall RtlGetLongestNtPathLength()
@ stub RtlGetNtGlobalFlags
@ stdcall RtlGetNtProductType(ptr)
@ stdcall RtlGetOwnerSecurityDescriptor(ptr ptr ptr)
@ stdcall RtlGetProcessHeaps(long ptr)
@ stdcall RtlGetSaclSecurityDescriptor(ptr ptr ptr ptr)
@ stub RtlGetUserInfoHeap
@ stdcall RtlIdentifierAuthoritySid(ptr)
@ stdcall RtlImageDirectoryEntryToData(long long long ptr)
@ stdcall RtlImageNtHeader(long)
@ stdcall RtlImageRvaToSection(ptr long long)
@ stdcall RtlImageRvaToVa(ptr long long ptr)
@ stdcall RtlImpersonateSelf(long)
@ stdcall RtlInitAnsiString(ptr str)
@ stub RtlInitCodePageTable
@ stub RtlInitNlsTables
@ stdcall RtlInitString(ptr str)
@ stdcall RtlInitUnicodeString(ptr wstr)
@ stdcall RtlInitUnicodeStringEx(ptr wstr)
@ stdcall RtlInitializeBitMap(ptr long long)
@ stub RtlInitializeContext
@ stdcall RtlInitializeCriticalSection(ptr)
@ stdcall RtlInitializeCriticalSectionAndSpinCount(ptr long)
@ stdcall RtlInitializeGenericTable(ptr ptr ptr ptr ptr)
@ stub RtlInitializeRXact
@ stdcall RtlInitializeResource(ptr)
@ stdcall RtlInitializeSid(ptr ptr long)
@ stub RtlInsertElementGenericTable
@ stdcall RtlInt64ToUnicodeString(long long long ptr)
@ stdcall RtlIntegerToChar(long long long ptr)
@ stdcall RtlIntegerToUnicodeString(long long ptr)
@ stdcall RtlIsDosDeviceName_U(wstr)
@ stub RtlIsGenericTableEmpty
@ stdcall RtlIsNameLegalDOS8Dot3(ptr ptr ptr)
@ stdcall RtlIsTextUnicode(ptr long ptr)
@ stdcall -ret64 RtlLargeIntegerAdd(long long long long)
@ stdcall -ret64 RtlLargeIntegerArithmeticShift(long long long)
@ stdcall -ret64 RtlLargeIntegerDivide(long long long long ptr)
@ stdcall -ret64 RtlLargeIntegerNegate(long long)
@ stdcall -ret64 RtlLargeIntegerShiftLeft(long long long)
@ stdcall -ret64 RtlLargeIntegerShiftRight(long long long)
@ stdcall -ret64 RtlLargeIntegerSubtract(long long long long)
@ stdcall RtlLargeIntegerToChar(ptr long long ptr)
@ stdcall RtlLeaveCriticalSection(ptr)
@ stdcall RtlLengthRequiredSid(long)
@ stdcall RtlLengthSecurityDescriptor(ptr)
@ stdcall RtlLengthSid(ptr)
@ stdcall RtlLocalTimeToSystemTime(ptr ptr)
@ stdcall RtlLockHeap(long)
@ stub RtlLookupElementGenericTable
@ stdcall RtlMakeSelfRelativeSD(ptr ptr ptr)
@ stdcall RtlMapGenericMask(long ptr)
@ stdcall RtlMoveMemory(ptr ptr long)
@ stdcall RtlMultiByteToUnicodeN(ptr long ptr ptr long)
@ stdcall RtlMultiByteToUnicodeSize(ptr str long)
@ stub RtlNewInstanceSecurityObject
@ stub RtlNewSecurityGrantedAccess
@ stdcall RtlNewSecurityObject(long long long long long long)
@ stdcall RtlNormalizeProcessParams(ptr)
@ stdcall RtlNtStatusToDosError(long)
@ stub RtlNumberGenericTableElements
@ stdcall RtlNumberOfClearBits(ptr)
@ stdcall RtlNumberOfSetBits(ptr)
@ stdcall RtlOemStringToUnicodeSize(ptr)
@ stdcall RtlOemStringToUnicodeString(ptr ptr long)
@ stdcall RtlOemToUnicodeN(ptr long ptr ptr long)
@ stdcall RtlOpenCurrentUser(long ptr)
@ stub RtlPcToFileHeader
@ stdcall RtlPrefixString(ptr ptr long)
@ stdcall RtlPrefixUnicodeString(ptr ptr long)
@ stub RtlProtectHeap
@ stdcall RtlQueryEnvironmentVariable_U(ptr ptr ptr)
@ stdcall RtlQueryInformationAcl(ptr ptr long long)
@ stub RtlQueryProcessBackTraceInformation
@ stub RtlQueryProcessDebugInformation
@ stub RtlQueryProcessHeapInformation
@ stub RtlQueryProcessLockInformation
@ stub RtlQueryRegistryValues
@ stub RtlQuerySecurityObject
@ stub RtlQueryTagHeap
@ stdcall RtlQueryTimeZoneInformation(ptr)
@ stdcall RtlRaiseException(ptr)
@ stdcall RtlRaiseStatus(long)
@ stdcall RtlRandom(ptr)
@ stdcall RtlReAllocateHeap(long long ptr long)
@ stub RtlRealPredecessor
@ stub RtlRealSuccessor
@ stdcall RtlReleasePebLock()
@ stdcall RtlReleaseResource(ptr)
@ stub RtlRemoteCall
@ stdcall RtlRemoveVectoredExceptionHandler(ptr)
@ stub RtlResetRtlTranslations
@ stub RtlRunDecodeUnicodeString
@ stub RtlRunEncodeUnicodeString
@ stdcall RtlSecondsSince1970ToTime(long ptr)
@ stdcall RtlSecondsSince1980ToTime(long ptr)
@ stdcall RtlSelfRelativeToAbsoluteSD(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall RtlSetAllBits(ptr)
@ stdcall RtlSetBits(ptr long long)
@ stdcall RtlSetCurrentDirectory_U(ptr)
@ stdcall RtlSetCurrentEnvironment(wstr ptr)
@ stdcall RtlSetDaclSecurityDescriptor(ptr long ptr long)
@ stdcall RtlSetEnvironmentVariable(ptr ptr ptr)
@ stdcall RtlSetGroupSecurityDescriptor(ptr ptr long)
@ stub RtlSetInformationAcl
@ stdcall RtlSetOwnerSecurityDescriptor(ptr ptr long)
@ stdcall RtlSetSaclSecurityDescriptor(ptr long ptr long)
@ stub RtlSetSecurityObject
@ stdcall RtlSetTimeZoneInformation(ptr)
@ stub RtlSetUserFlagsHeap
@ stub RtlSetUserValueHeap
@ stdcall RtlSizeHeap(long long ptr)
@ stub RtlSplay
@ stub RtlStartRXact
@ stub RtlStringFromGUID
@ stdcall RtlSubAuthorityCountSid(ptr)
@ stdcall RtlSubAuthoritySid(ptr long)
@ stub RtlSubtreePredecessor
@ stub RtlSubtreeSuccessor
@ stdcall RtlSystemTimeToLocalTime(ptr ptr)
@ stdcall RtlTimeFieldsToTime(ptr ptr)
@ stdcall RtlTimeToElapsedTimeFields(long long)
@ stdcall RtlTimeToSecondsSince1970(ptr ptr)
@ stdcall RtlTimeToSecondsSince1980(ptr ptr)
@ stdcall RtlTimeToTimeFields (long long)
@ cdecl -i386 -norelay RtlUlongByteSwap() NTDLL_RtlUlongByteSwap
@ cdecl -ret64 RtlUlonglongByteSwap(long long)
@ stdcall RtlUnicodeStringToAnsiSize(ptr)
@ stdcall RtlUnicodeStringToAnsiString(ptr ptr long)
@ stub RtlUnicodeStringToCountedOemString
@ stdcall RtlUnicodeStringToInteger(ptr long ptr)
@ stdcall RtlUnicodeStringToOemSize(ptr)
@ stdcall RtlUnicodeStringToOemString(ptr ptr long)
@ stub RtlUnicodeToCustomCPN
@ stdcall RtlUnicodeToMultiByteN(ptr long ptr ptr long)
@ stdcall RtlUnicodeToMultiByteSize(ptr ptr long)
@ stdcall RtlUnicodeToOemN(ptr long ptr ptr long)
@ stdcall RtlUniform(ptr)
@ stdcall RtlUnlockHeap(long)
@ stdcall RtlUnwind(ptr ptr ptr long)
@ stdcall RtlUpcaseUnicodeChar(long)
@ stdcall RtlUpcaseUnicodeString(ptr ptr long)
@ stdcall RtlUpcaseUnicodeStringToAnsiString(ptr ptr long)
@ stdcall RtlUpcaseUnicodeStringToCountedOemString(ptr ptr long)
@ stdcall RtlUpcaseUnicodeStringToOemString(ptr ptr long)
@ stub RtlUpcaseUnicodeToCustomCPN
@ stdcall RtlUpcaseUnicodeToMultiByteN(ptr long ptr ptr long)
@ stdcall RtlUpcaseUnicodeToOemN(ptr long ptr ptr long)
@ stdcall RtlUpperChar(long)
@ stdcall RtlUpperString(ptr ptr)
@ stub RtlUsageHeap
@ cdecl -i386 -norelay RtlUshortByteSwap() NTDLL_RtlUshortByteSwap
@ stdcall RtlValidAcl(ptr)
@ stdcall RtlValidSecurityDescriptor(ptr)
@ stdcall RtlValidSid(ptr)
@ stdcall RtlValidateHeap(long long ptr)
@ stub RtlValidateProcessHeaps
@ stdcall RtlVerifyVersionInfo(ptr long long long)
@ stdcall RtlWalkHeap(long ptr)
@ stub RtlWriteRegistryValue
@ stub RtlZeroHeap
@ stdcall RtlZeroMemory(ptr long)
@ stub RtlpInitializeRtl
@ stub RtlpNtCreateKey
@ stub RtlpNtEnumerateSubKey
@ stub RtlpNtMakeTemporaryKey
@ stub RtlpNtOpenKey
@ stub RtlpNtQueryValueKey
@ stub RtlpNtSetValueKey
@ stdcall RtlpUnWaitCriticalSection(ptr)
@ stdcall RtlpWaitForCriticalSection(ptr)
@ stdcall RtlxAnsiStringToUnicodeSize(ptr) RtlAnsiStringToUnicodeSize
@ stdcall RtlxOemStringToUnicodeSize(ptr) RtlOemStringToUnicodeSize
@ stdcall RtlxUnicodeStringToAnsiSize(ptr) RtlUnicodeStringToAnsiSize
@ stdcall RtlxUnicodeStringToOemSize(ptr) RtlUnicodeStringToOemSize
@ stub SaveEm87Context
@ stdcall ZwAcceptConnectPort(ptr long ptr long long ptr) NtAcceptConnectPort
@ stdcall ZwAccessCheck(ptr long long ptr ptr ptr ptr ptr) NtAccessCheck
@ stub ZwAccessCheckAndAuditAlarm
@ stdcall ZwAdjustGroupsToken(long long long long long long) NtAdjustPrivilegesToken
@ stdcall ZwAdjustPrivilegesToken(long long long long long long) NtAdjustPrivilegesToken
@ stub ZwAlertResumeThread
@ stub ZwAlertThread
@ stdcall ZwAllocateLocallyUniqueId(ptr) NtAllocateLocallyUniqueId
@ stdcall ZwAllocateUuids(ptr ptr ptr) NtAllocateUuids
@ stdcall ZwAllocateVirtualMemory(long ptr ptr ptr long long) NtAllocateVirtualMemory
@ stub ZwCallbackReturn
@ stub ZwCancelIoFile
@ stdcall ZwCancelTimer(long ptr) NtCancelTimer
@ stdcall ZwClearEvent(long) NtClearEvent
@ stdcall ZwClose(long) NtClose
@ stub ZwCloseObjectAuditAlarm
@ stdcall ZwCompleteConnectPort(ptr) NtCompleteConnectPort
@ stdcall ZwConnectPort(ptr ptr ptr ptr ptr ptr ptr ptr) NtConnectPort
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
@ stdcall ZwCreatePort(ptr ptr long long long) NtCreatePort
@ stub ZwCreateProcess
@ stub ZwCreateProfile
@ stdcall ZwCreateSection(ptr long ptr ptr long long long) NtCreateSection
@ stdcall ZwCreateSemaphore(ptr long ptr long long) NtCreateSemaphore
@ stdcall ZwCreateSymbolicLinkObject(ptr long ptr ptr) NtCreateSymbolicLinkObject
@ stub ZwCreateThread
@ stdcall ZwCreateTimer(ptr long ptr long) NtCreateTimer
@ stub ZwCreateToken
@ stdcall ZwDelayExecution(long ptr) NtDelayExecution
@ stub ZwDeleteFile
@ stdcall ZwDeleteKey(long) NtDeleteKey
@ stdcall ZwDeleteValueKey(long ptr) NtDeleteValueKey
@ stdcall ZwDeviceIoControlFile(long long long long long long long long long long) NtDeviceIoControlFile
@ stdcall ZwDisplayString(ptr) NtDisplayString
@ stdcall ZwDuplicateObject(long long long ptr long long long) NtDuplicateObject
@ stdcall ZwDuplicateToken(long long long long long long) NtDuplicateToken
@ stub ZwEnumerateBus
@ stdcall ZwEnumerateKey(long long long ptr long ptr) NtEnumerateKey
@ stdcall ZwEnumerateValueKey(long long long ptr long ptr) NtEnumerateValueKey
@ stub ZwExtendSection
@ stdcall ZwFlushBuffersFile(long ptr) NtFlushBuffersFile
@ stub ZwFlushInstructionCache
@ stdcall ZwFlushKey(long) NtFlushKey
@ stdcall ZwFlushVirtualMemory(long ptr ptr long) NtFlushVirtualMemory
@ stub ZwFlushWriteBuffer
@ stdcall ZwFreeVirtualMemory(long ptr ptr long) NtFreeVirtualMemory
@ stdcall ZwFsControlFile(long long long long long long long long long long) NtFsControlFile
@ stdcall ZwGetContextThread(long ptr) NtGetContextThread
@ stub ZwGetPlugPlayEvent
@ stub ZwGetTickCount
@ stub ZwImpersonateClientOfPort
@ stub ZwImpersonateThread
@ stub ZwInitializeRegistry
@ stdcall ZwListenPort(ptr ptr) NtListenPort
@ stub ZwLoadDriver
@ stdcall ZwLoadKey(ptr ptr) NtLoadKey
@ stdcall ZwLockFile(long long ptr ptr ptr ptr ptr ptr long long) NtLockFile
@ stdcall ZwLockVirtualMemory(long ptr ptr long) NtLockVirtualMemory
@ stub ZwMakeTemporaryObject
@ stdcall ZwMapViewOfSection(long long ptr long long ptr ptr long long long) NtMapViewOfSection
@ stub ZwNotifyChangeDirectoryFile
@ stdcall ZwNotifyChangeKey(long long ptr ptr ptr long long ptr long long) NtNotifyChangeKey
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
@ stdcall ZwOpenSection(ptr long ptr) NtOpenSection
@ stdcall ZwOpenSemaphore(long long ptr) NtOpenSemaphore
@ stdcall ZwOpenSymbolicLinkObject (long long long) NtOpenSymbolicLinkObject
@ stdcall ZwOpenThread(ptr long ptr ptr) NtOpenThread
@ stdcall ZwOpenThreadToken(long long long long) NtOpenThreadToken
@ stdcall ZwOpenTimer(ptr long ptr) NtOpenTimer
@ stub ZwPlugPlayControl
@ stub ZwPrivilegeCheck
@ stub ZwPrivilegeObjectAuditAlarm
@ stub ZwPrivilegedServiceAuditAlarm
@ stdcall ZwProtectVirtualMemory(long ptr ptr long ptr) NtProtectVirtualMemory
@ stdcall ZwPulseEvent(long ptr) NtPulseEvent
@ stub ZwQueryAttributesFile
@ stdcall ZwQueryDefaultLocale(long ptr) NtQueryDefaultLocale
@ stdcall ZwQueryDirectoryFile(long long  ptr ptr ptr ptr long long long ptr long)NtQueryDirectoryFile
@ stdcall ZwQueryDirectoryObject(long ptr long long long ptr ptr) NtQueryDirectoryObject
@ stub ZwQueryEaFile
@ stdcall ZwQueryEvent(long long ptr long ptr) NtQueryEvent
@ stdcall ZwQueryInformationFile(long ptr ptr long long) NtQueryInformationFile
@ stub ZwQueryInformationPort
@ stdcall ZwQueryInformationProcess(long long ptr long ptr) NtQueryInformationProcess
@ stdcall ZwQueryInformationThread(long long ptr long ptr) NtQueryInformationThread
@ stdcall ZwQueryInformationToken(long long ptr long ptr) NtQueryInformationToken
@ stub ZwQueryIntervalProfile
@ stub ZwQueryIoCompletion
@ stdcall ZwQueryKey(long long ptr long ptr) NtQueryKey
@ stub ZwQueryMutant
@ stdcall ZwQueryObject(long long long long long) NtQueryObject
@ stub ZwQueryOpenSubKeys
@ stdcall ZwQueryPerformanceCounter (long long) NtQueryPerformanceCounter
@ stdcall ZwQuerySection (long long long long long) NtQuerySection
@ stdcall ZwQuerySecurityObject (long long long long long) NtQuerySecurityObject
@ stdcall ZwQuerySemaphore (long long long long long) NtQuerySemaphore
@ stdcall ZwQuerySymbolicLinkObject(long ptr ptr) NtQuerySymbolicLinkObject
@ stub ZwQuerySystemEnvironmentValue
@ stdcall ZwQuerySystemInformation(long long long long) NtQuerySystemInformation
@ stdcall ZwQuerySystemTime(ptr) NtQuerySystemTime
@ stub ZwQueryTimer
@ stdcall ZwQueryTimerResolution(long long long) NtQueryTimerResolution
@ stdcall ZwQueryValueKey(long ptr long ptr long ptr) NtQueryValueKey
@ stdcall ZwQueryVirtualMemory(long ptr long ptr long ptr) NtQueryVirtualMemory
@ stdcall ZwQueryVolumeInformationFile(long ptr ptr long long) NtQueryVolumeInformationFile
@ stdcall ZwRaiseException(ptr ptr long) NtRaiseException
@ stub ZwRaiseHardError
@ stdcall ZwReadFile(long long long long long long long long long) NtReadFile
@ stub ZwReadRequestData
@ stdcall ZwReadVirtualMemory(long ptr ptr long ptr) NtReadVirtualMemory
@ stub ZwRegisterNewDevice
@ stdcall ZwRegisterThreadTerminatePort(ptr) NtRegisterThreadTerminatePort
@ stub ZwReleaseMutant
@ stub ZwReleaseProcessMutant
@ stdcall ZwReleaseSemaphore(long long ptr) NtReleaseSemaphore
@ stub ZwRemoveIoCompletion
@ stdcall ZwReplaceKey(ptr long ptr) NtReplaceKey
@ stub ZwReplyPort
@ stdcall ZwReplyWaitReceivePort(ptr ptr ptr ptr) NtReplyWaitReceivePort
@ stub ZwReplyWaitReplyPort
@ stub ZwRequestPort
@ stdcall ZwRequestWaitReplyPort(ptr ptr ptr) NtRequestWaitReplyPort
@ stdcall ZwResetEvent(long ptr) NtResetEvent
@ stdcall ZwRestoreKey(long long long) NtRestoreKey
@ stdcall ZwResumeThread(long long) NtResumeThread
@ stdcall ZwSaveKey(long long) NtSaveKey
@ stdcall ZwSetContextThread(long ptr) NtSetContextThread
@ stub ZwSetDefaultHardErrorPort
@ stdcall ZwSetDefaultLocale(long long) NtSetDefaultLocale
@ stub ZwSetEaFile
@ stdcall ZwSetEvent(long long) NtSetEvent
@ stub ZwSetHighEventPair
@ stub ZwSetHighWaitLowEventPair
@ stub ZwSetHighWaitLowThread
@ stdcall ZwSetInformationFile(long long long long long) NtSetInformationFile
@ stdcall ZwSetInformationKey(long long ptr long) NtSetInformationKey
@ stdcall ZwSetInformationObject(long long ptr long) NtSetInformationObject
@ stdcall ZwSetInformationProcess(long long long long) NtSetInformationProcess
@ stdcall ZwSetInformationThread(long long ptr long) NtSetInformationThread
@ stub ZwSetInformationToken
@ stdcall ZwSetIntervalProfile(long long) NtSetIntervalProfile
@ stub ZwSetIoCompletion
@ stub ZwSetLdtEntries
@ stub ZwSetLowEventPair
@ stub ZwSetLowWaitHighEventPair
@ stub ZwSetLowWaitHighThread
@ stdcall ZwSetSecurityObject(long long ptr) NtSetSecurityObject
@ stub ZwSetSystemEnvironmentValue
@ stub ZwSetSystemInformation
@ stub ZwSetSystemPowerState
@ stdcall ZwSetSystemTime(ptr ptr) NtSetSystemTime
@ stdcall ZwSetTimer(long ptr ptr ptr long long ptr) NtSetTimer
@ stdcall ZwSetTimerResolution(long long ptr) NtSetTimerResolution
@ stdcall ZwSetValueKey(long long long long long long) NtSetValueKey
@ stdcall ZwSetVolumeInformationFile(long ptr ptr long long) NtSetVolumeInformationFile
@ stub ZwShutdownSystem
@ stub ZwStartProfile
@ stub ZwStopProfile
@ stdcall ZwSuspendThread(long ptr) NtSuspendThread
@ stub ZwSystemDebugControl
@ stdcall ZwTerminateProcess(long long) NtTerminateProcess
@ stdcall ZwTerminateThread(long long) NtTerminateThread
@ stub ZwTestAlert
@ stub ZwUnloadDriver
@ stdcall ZwUnloadKey(long) NtUnloadKey
@ stub ZwUnloadKeyEx
@ stdcall ZwUnlockFile(long ptr ptr ptr ptr) NtUnlockFile
@ stdcall ZwUnlockVirtualMemory(long ptr ptr long) NtUnlockVirtualMemory
@ stdcall ZwUnmapViewOfSection(long ptr) NtUnmapViewOfSection
@ stub ZwVdmControl
@ stub ZwW32Call
@ stdcall ZwWaitForMultipleObjects(long ptr long long ptr) NtWaitForMultipleObjects
@ stub ZwWaitForProcessMutant
@ stdcall ZwWaitForSingleObject(long long long) NtWaitForSingleObject
@ stub ZwWaitHighEventPair
@ stub ZwWaitLowEventPair
@ stdcall ZwWriteFile(long long ptr ptr ptr ptr long ptr ptr) NtWriteFile
@ stub ZwWriteRequestData
@ stdcall ZwWriteVirtualMemory(long ptr ptr long ptr) NtWriteVirtualMemory
@ cdecl _CIpow() NTDLL__CIpow
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
@ stdcall -ret64 _alldiv(long long long long)
@ stdcall -ret64 _allmul(long long long long)
@ stdcall -register -i386 _alloca_probe() NTDLL_alloca_probe
@ stdcall -ret64 _allrem(long long long long)
@ cdecl -ret64 _atoi64(str)
@ stdcall -ret64 _aulldiv(long long long long)
@ stdcall -ret64 _aullrem(long long long long)
@ stdcall -register -i386 _chkstk() NTDLL_chkstk
@ stub _fltused
@ cdecl -ret64 _ftol() NTDLL__ftol
@ cdecl _i64toa(long long ptr long)
@ cdecl _i64tow(long long ptr long)
@ cdecl _itoa(long ptr long)
@ cdecl _itow(long ptr long)
@ cdecl _ltoa(long ptr long)
@ cdecl _ltow(long ptr long)
@ cdecl _memccpy(ptr ptr long long) memccpy
@ cdecl _memicmp(str str long) NTDLL__memicmp
@ varargs _snprintf(ptr long ptr) snprintf
@ varargs _snwprintf(wstr long wstr)
@ cdecl _splitpath(str ptr ptr ptr ptr)
@ cdecl _strcmpi(str str) strcasecmp
@ cdecl _stricmp(str str) strcasecmp
@ cdecl _strlwr(str)
@ cdecl _strnicmp(str str long) strncasecmp
@ cdecl _strupr(str)
@ cdecl _ui64toa(long long ptr long)
@ cdecl _ui64tow(long long ptr long)
@ cdecl _ultoa(long ptr long)
@ cdecl _ultow(long ptr long)
@ cdecl _vsnprintf(ptr long ptr ptr) vsnprintf
@ cdecl _wcsicmp(wstr wstr) NTDLL__wcsicmp
@ cdecl _wcslwr(wstr) NTDLL__wcslwr
@ cdecl _wcsnicmp(wstr wstr long) NTDLL__wcsnicmp
@ cdecl _wcsupr(wstr) NTDLL__wcsupr
@ cdecl _wtoi(wstr)
@ cdecl _wtoi64(wstr)
@ cdecl _wtol(wstr)
@ cdecl abs(long)
@ cdecl atan(double)
@ cdecl atoi(str)
@ cdecl atol(str)
@ cdecl ceil(double)
@ cdecl cos(double)
@ cdecl fabs(double)
@ cdecl floor(double)
@ cdecl isalpha(long)
@ cdecl isdigit(long)
@ cdecl islower(long)
@ cdecl isprint(long)
@ cdecl isspace(long)
@ cdecl isupper(long)
@ cdecl iswalpha(long) NTDLL_iswalpha
@ cdecl iswctype(long long) NTDLL_iswctype
@ cdecl iswdigit(long) NTDLL_iswdigit
@ cdecl iswlower(long) NTDLL_iswlower
@ cdecl iswspace(long) NTDLL_iswspace
@ cdecl iswxdigit(long) NTDLL_iswxdigit
@ cdecl isxdigit(long)
@ cdecl labs(long)
@ cdecl log(double)
@ cdecl mbstowcs(ptr str long) NTDLL_mbstowcs
@ cdecl memchr(ptr long long)
@ cdecl memcmp(ptr ptr long)
@ cdecl memcpy(ptr ptr long)
@ cdecl memmove(ptr ptr long)
@ cdecl memset(ptr long long)
@ cdecl pow(double double)
@ cdecl qsort(ptr long long ptr)
@ cdecl sin(double)
@ varargs sprintf(str str)
@ cdecl sqrt(double)
@ varargs sscanf(str str)
@ cdecl strcat(str str)
@ cdecl strchr(str long)
@ cdecl strcmp(str str)
@ cdecl strcpy(ptr str)
@ cdecl strcspn(str str)
@ cdecl strlen(str)
@ cdecl strncat(str str long)
@ cdecl strncmp(str str long)
@ cdecl strncpy(ptr str long)
@ cdecl strpbrk(str str)
@ cdecl strrchr(str long)
@ cdecl strspn(str str)
@ cdecl strstr(str str)
@ cdecl strtol(str ptr long)
@ cdecl strtoul(str ptr long)
@ varargs swprintf(wstr wstr) NTDLL_swprintf
@ cdecl tan(double)
@ cdecl tolower(long)
@ cdecl toupper(long)
@ cdecl towlower(long) NTDLL_towlower
@ cdecl towupper(long) NTDLL_towupper
@ cdecl vsprintf(ptr str ptr)
@ cdecl wcscat(wstr wstr) NTDLL_wcscat
@ cdecl wcschr(wstr long) NTDLL_wcschr
@ cdecl wcscmp(wstr wstr) NTDLL_wcscmp
@ cdecl wcscpy(ptr wstr) NTDLL_wcscpy
@ cdecl wcscspn(wstr wstr) NTDLL_wcscspn
@ cdecl wcslen(wstr) NTDLL_wcslen
@ cdecl wcsncat(wstr wstr long) NTDLL_wcsncat
@ cdecl wcsncmp(wstr wstr long) NTDLL_wcsncmp
@ cdecl wcsncpy(ptr wstr long) NTDLL_wcsncpy
@ cdecl wcspbrk(wstr wstr) NTDLL_wcspbrk
@ cdecl wcsrchr(wstr long) NTDLL_wcsrchr
@ cdecl wcsspn(wstr wstr) NTDLL_wcsspn
@ cdecl wcsstr(wstr wstr) NTDLL_wcsstr
@ cdecl wcstok(wstr wstr) NTDLL_wcstok
@ cdecl wcstol(wstr ptr long) NTDLL_wcstol
@ cdecl wcstombs(ptr ptr long) NTDLL_wcstombs
@ cdecl wcstoul(wstr ptr long) NTDLL_wcstoul
@ stub NtAddAtom
@ stub NtDeleteAtom
@ stub NtFindAtom
@ stub NtQueryFullAttributesFile
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
@ stdcall RtlTryEnterCriticalSection(ptr)
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
@ stub RtlSetPropertySetClassId
@ stdcall NtPowerInformation(long long long long long)
@ stdcall -ret64 VerSetConditionMask(long long long long)

##################
# Wine extensions
#
# All functions must be prefixed with '__wine_' (for internal functions)
# or 'wine_' (for user-visible functions) to avoid namespace conflicts.

# Exception handling
@ cdecl -norelay __wine_exception_handler(ptr ptr ptr ptr)
@ cdecl -norelay __wine_finally_handler(ptr ptr ptr ptr)

# Relays
@ cdecl -norelay -i386 __wine_call_from_32_regs()
@ cdecl -i386 __wine_enter_vm86(ptr)

# Server interface
@ cdecl -norelay wine_server_call(ptr)
@ cdecl wine_server_fd_to_handle(long long long ptr)
@ cdecl wine_server_handle_to_fd(long long ptr ptr ptr)
@ cdecl wine_server_release_fd(long long)
@ cdecl wine_server_send_fd(long)

# Codepages
@ cdecl __wine_init_codepages(ptr ptr)

# signal handling
@ cdecl __wine_set_signal_handler(long ptr)

################################################################
# Wine dll separation hacks, these will go away, don't use them
#
@ cdecl CDROM_InitRegistry(long long str)
@ cdecl MODULE_DllThreadAttach(ptr)
@ cdecl MODULE_GetLoadOrderW(ptr wstr wstr)
@ cdecl VERSION_Init(wstr)
@ cdecl VIRTUAL_SetFaultHandler(ptr ptr ptr)
