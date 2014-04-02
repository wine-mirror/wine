#note that the Zw... functions are alternate names for the
#Nt... functions.  (see www.sysinternals.com for details)
#if you change a Nt.. function DON'T FORGET to change the
#Zw one too.

@ stub CsrAllocateCaptureBuffer
@ stub CsrAllocateCapturePointer
@ stub CsrAllocateMessagePointer
@ stub CsrCaptureMessageBuffer
# @ stub CsrCaptureMessageMultiUnicodeStringsInPlace
@ stub CsrCaptureMessageString
@ stub CsrCaptureTimeout
@ stub CsrClientCallServer
@ stub CsrClientConnectToServer
@ stub CsrClientMaxMessage
@ stub CsrClientSendMessage
@ stub CsrClientThreadConnect
@ stub CsrFreeCaptureBuffer
# @ stub CsrGetProcessId
@ stub CsrIdentifyAlertableThread
@ stub CsrNewThread
@ stub CsrProbeForRead
@ stub CsrProbeForWrite
@ stub CsrSetPriorityClass
@ stub CsrpProcessCallbackRequest
@ stdcall DbgBreakPoint()
@ varargs DbgPrint(str)
@ varargs DbgPrintEx(long long str)
# @ stub DbgPrintReturnControlC
@ stub DbgPrompt
# @ stub DbgQueryDebugFilterState
# @ stub DbgSetDebugFilterState
@ stub DbgUiConnectToDbg
@ stub DbgUiContinue
@ stub DbgUiConvertStateChangeStructure
# @ stub DbgUiDebugActiveProcess
# @ stub DbgUiGetThreadDebugObject
# @ stub DbgUiIssueRemoteBreakin
# @ stub DbgUiRemoteBreakin
# @ stub DbgUiSetThreadDebugObject
# @ stub DbgUiStopDebugging
@ stub DbgUiWaitStateChange
@ stdcall DbgUserBreakPoint()
# @ stub KiFastSystemCall
# @ stub KiFastSystemCallRet
# @ stub KiIntSystemCall
@ stub KiRaiseUserExceptionDispatcher
@ stub KiUserApcDispatcher
@ stub KiUserCallbackDispatcher
@ stub KiUserExceptionDispatcher
# @ stub LdrAccessOutOfProcessResource
@ stdcall LdrAccessResource(long ptr ptr ptr)
@ stdcall LdrAddRefDll(long ptr)
# @ stub LdrAlternateResourcesEnabled
# @ stub LdrCreateOutOfProcessImage
# @ stub LdrDestroyOutOfProcessImage
@ stdcall LdrDisableThreadCalloutsForDll(long)
@ stub LdrEnumResources
# @ stub LdrEnumerateLoadedModules
# @ stub LdrFindCreateProcessManifest
@ stdcall LdrFindEntryForAddress(ptr ptr)
@ stdcall LdrFindResourceDirectory_U(long ptr long ptr)
# @ stub LdrFindResourceEx_U
@ stdcall LdrFindResource_U(long ptr long ptr)
@ stub LdrFlushAlternateResourceModules
@ stdcall LdrGetDllHandle(wstr long ptr ptr)
# @ stub LdrGetDllHandleEx
@ stdcall LdrGetProcedureAddress(ptr ptr long ptr)
# @ stub LdrHotPatchRoutine
@ stub LdrInitShimEngineDynamic
@ stdcall LdrInitializeThunk(long long long long)
@ stub LdrLoadAlternateResourceModule
@ stdcall LdrLoadDll(wstr long ptr ptr)
@ stdcall LdrLockLoaderLock(long ptr ptr)
@ stdcall LdrProcessRelocationBlock(ptr long ptr long)
@ stdcall LdrQueryImageFileExecutionOptions(ptr wstr long ptr long ptr)
@ stdcall LdrQueryProcessModuleInformation(ptr long ptr)
@ stdcall LdrResolveDelayLoadedAPI(ptr ptr ptr ptr ptr long)
@ stub LdrSetAppCompatDllRedirectionCallback
@ stub LdrSetDllManifestProber
@ stdcall LdrShutdownProcess()
@ stdcall LdrShutdownThread()
@ stub LdrUnloadAlternateResourceModule
@ stdcall LdrUnloadDll(ptr)
@ stdcall LdrUnlockLoaderLock(long long)
@ stub LdrVerifyImageMatchesChecksum
@ extern NlsAnsiCodePage
@ extern NlsMbCodePageTag
@ extern NlsMbOemCodePageTag
@ stdcall NtAcceptConnectPort(ptr long ptr long long ptr)
@ stdcall NtAccessCheck(ptr long long ptr ptr ptr ptr ptr)
@ stdcall NtAccessCheckAndAuditAlarm(ptr long ptr ptr ptr long ptr long ptr ptr ptr)
# @ stub NtAccessCheckByType
# @ stub NtAccessCheckByTypeAndAuditAlarm
# @ stub NtAccessCheckByTypeResultList
# @ stub NtAccessCheckByTypeResultListAndAuditAlarm
# @ stub NtAccessCheckByTypeResultListAndAuditAlarmByHandle
@ stdcall NtAddAtom(ptr long ptr)
# @ stub NtAddBootEntry
@ stdcall NtAdjustGroupsToken(long long ptr long ptr ptr)
@ stdcall NtAdjustPrivilegesToken(long long long long long long)
@ stdcall NtAlertResumeThread(long ptr)
@ stdcall NtAlertThread(long)
@ stdcall NtAllocateLocallyUniqueId(ptr)
# @ stub NtAllocateUserPhysicalPages
@ stdcall NtAllocateUuids(ptr ptr ptr)
@ stdcall NtAllocateVirtualMemory(long ptr ptr ptr long long)
@ stdcall NtAreMappedFilesTheSame(ptr ptr)
@ stdcall NtAssignProcessToJobObject(long long)
@ stub NtCallbackReturn
# @ stub NtCancelDeviceWakeupRequest
@ stdcall NtCancelIoFile(long ptr)
@ stdcall NtCancelIoFileEx(long ptr ptr)
@ stdcall NtCancelTimer(long ptr)
@ stdcall NtClearEvent(long)
@ stdcall NtClose(long)
@ stub NtCloseObjectAuditAlarm
# @ stub NtCompactKeys
# @ stub NtCompareTokens
@ stdcall NtCompleteConnectPort(ptr)
# @ stub NtCompressKey
@ stdcall NtConnectPort(ptr ptr ptr ptr ptr ptr ptr ptr)
@ stub NtContinue
# @ stub NtCreateDebugObject
@ stdcall NtCreateDirectoryObject(long long long)
@ stdcall NtCreateEvent(long long long long long)
@ stub NtCreateEventPair
@ stdcall NtCreateFile(ptr long ptr ptr long long long ptr long long ptr)
@ stdcall NtCreateIoCompletion(ptr long ptr long)
@ stdcall NtCreateJobObject(ptr long ptr)
# @ stub NtCreateJobSet
@ stdcall NtCreateKey(ptr long ptr long ptr long long)
@ stdcall NtCreateKeyedEvent(ptr long ptr long)
@ stdcall NtCreateMailslotFile(long long long long long long long long)
@ stdcall NtCreateMutant(ptr long ptr long)
@ stdcall NtCreateNamedPipeFile(ptr long ptr ptr long long long long long long long long long ptr)
@ stdcall NtCreatePagingFile(long long long long)
@ stdcall NtCreatePort(ptr ptr long long ptr)
@ stub NtCreateProcess
# @ stub NtCreateProcessEx
@ stub NtCreateProfile
@ stdcall NtCreateSection(ptr long ptr ptr long long long)
@ stdcall NtCreateSemaphore(ptr long ptr long long)
@ stdcall NtCreateSymbolicLinkObject(ptr long ptr ptr)
@ stub NtCreateThread
@ stdcall NtCreateTimer(ptr long ptr long)
@ stub NtCreateToken
# @ stub NtCreateWaitablePort
@ stdcall -arch=win32,arm64 NtCurrentTeb()
# @ stub NtDebugActiveProcess
# @ stub NtDebugContinue
@ stdcall NtDelayExecution(long ptr)
@ stdcall NtDeleteAtom(long)
# @ stub NtDeleteBootEntry
@ stdcall NtDeleteFile(ptr)
@ stdcall NtDeleteKey(long)
# @ stub NtDeleteObjectAuditAlarm
@ stdcall NtDeleteValueKey(long ptr)
@ stdcall NtDeviceIoControlFile(long long long long long long long long long long)
@ stdcall NtDisplayString(ptr)
@ stdcall NtDuplicateObject(long long long ptr long long long)
@ stdcall NtDuplicateToken(long long long long long long)
# @ stub NtEnumerateBootEntries
@ stub NtEnumerateBus
@ stdcall NtEnumerateKey (long long long long long long)
# @ stub NtEnumerateSystemEnvironmentValuesEx
@ stdcall NtEnumerateValueKey (long long long long long long)
@ stub NtExtendSection
# @ stub NtFilterToken
@ stdcall NtFindAtom(ptr long ptr)
@ stdcall NtFlushBuffersFile(long ptr)
@ stdcall NtFlushInstructionCache(long ptr long)
@ stdcall NtFlushKey(long)
@ stdcall NtFlushVirtualMemory(long ptr ptr long)
@ stub NtFlushWriteBuffer
# @ stub NtFreeUserPhysicalPages
@ stdcall NtFreeVirtualMemory(long ptr ptr long)
@ stdcall NtFsControlFile(long long long long long long long long long long)
@ stdcall NtGetContextThread(long ptr)
@ stdcall NtGetCurrentProcessorNumber()
# @ stub NtGetDevicePowerState
@ stub NtGetPlugPlayEvent
@ stdcall NtGetTickCount()
@ stdcall NtGetWriteWatch(long long ptr long ptr ptr ptr)
@ stub NtImpersonateAnonymousToken
@ stub NtImpersonateClientOfPort
@ stub NtImpersonateThread
@ stub NtInitializeRegistry
@ stdcall NtInitiatePowerAction (long long long long)
@ stdcall NtIsProcessInJob(long long)
# @ stub NtIsSystemResumeAutomatic
@ stdcall NtListenPort(ptr ptr)
@ stdcall NtLoadDriver(ptr)
# @ stub NtLoadKey2
@ stdcall NtLoadKey(ptr ptr)
@ stdcall NtLockFile(long long ptr ptr ptr ptr ptr ptr long long)
# @ stub NtLockProductActivationKeys
# @ stub NtLockRegistryKey
@ stdcall NtLockVirtualMemory(long ptr ptr long)
# @ stub NtMakePermanentObject
@ stdcall NtMakeTemporaryObject(long)
# @ stub NtMapUserPhysicalPages
# @ stub NtMapUserPhysicalPagesScatter
@ stdcall NtMapViewOfSection(long long ptr long long ptr ptr long long long)
# @ stub NtModifyBootEntry
@ stdcall NtNotifyChangeDirectoryFile(long long ptr ptr ptr ptr long long long)
@ stdcall NtNotifyChangeKey(long long ptr ptr ptr long long ptr long long)
# @ stub NtNotifyChangeMultipleKeys
@ stdcall NtOpenDirectoryObject(long long long)
@ stdcall NtOpenEvent(long long long)
@ stub NtOpenEventPair
@ stdcall NtOpenFile(ptr long ptr ptr long long)
@ stdcall NtOpenIoCompletion(ptr long ptr)
@ stdcall NtOpenJobObject(ptr long ptr)
@ stdcall NtOpenKey(ptr long ptr)
@ stdcall NtOpenKeyedEvent(ptr long ptr)
@ stdcall NtOpenMutant(ptr long ptr)
@ stub NtOpenObjectAuditAlarm
@ stdcall NtOpenProcess(ptr long ptr ptr)
@ stdcall NtOpenProcessToken(long long ptr)
@ stdcall NtOpenProcessTokenEx(long long long ptr)
@ stdcall NtOpenSection(ptr long ptr)
@ stdcall NtOpenSemaphore(long long ptr)
@ stdcall NtOpenSymbolicLinkObject (ptr long ptr)
@ stdcall NtOpenThread(ptr long ptr ptr)
@ stdcall NtOpenThreadToken(long long long ptr)
@ stdcall NtOpenThreadTokenEx(long long long long ptr)
@ stdcall NtOpenTimer(ptr long ptr)
@ stub NtPlugPlayControl
@ stdcall NtPowerInformation(long ptr long ptr long)
@ stdcall NtPrivilegeCheck(ptr ptr ptr)
@ stub NtPrivilegeObjectAuditAlarm
@ stub NtPrivilegedServiceAuditAlarm
@ stdcall NtProtectVirtualMemory(long ptr ptr long ptr)
@ stdcall NtPulseEvent(long ptr)
@ stdcall NtQueryAttributesFile(ptr ptr)
# @ stub NtQueryBootEntryOrder
# @ stub NtQueryBootOptions
# @ stub NtQueryDebugFilterState
@ stdcall NtQueryDefaultLocale(long ptr)
@ stdcall NtQueryDefaultUILanguage(ptr)
@ stdcall NtQueryDirectoryFile(long long ptr ptr ptr ptr long long long ptr long)
@ stdcall NtQueryDirectoryObject(long ptr long long long ptr ptr)
@ stdcall NtQueryEaFile(long ptr ptr long long ptr long ptr long)
@ stdcall NtQueryEvent(long long ptr long ptr)
@ stdcall NtQueryFullAttributesFile(ptr ptr)
@ stdcall NtQueryInformationAtom(long long ptr long ptr)
@ stdcall NtQueryInformationFile(long ptr ptr long long)
@ stdcall NtQueryInformationJobObject(long long ptr long ptr)
@ stub NtQueryInformationPort
@ stdcall NtQueryInformationProcess(long long ptr long ptr)
@ stdcall NtQueryInformationThread(long long ptr long ptr)
@ stdcall NtQueryInformationToken(long long ptr long ptr)
@ stdcall NtQueryInstallUILanguage(ptr)
@ stub NtQueryIntervalProfile
@ stdcall NtQueryIoCompletion(long long ptr long ptr)
@ stdcall NtQueryKey (long long ptr long ptr)
@ stdcall NtQueryMultipleValueKey(long ptr long ptr long ptr)
@ stdcall NtQueryMutant(long long ptr long ptr)
@ stdcall NtQueryObject(long long long long long)
@ stub NtQueryOpenSubKeys
@ stdcall NtQueryPerformanceCounter(ptr ptr)
# @ stub NtQueryPortInformationProcess
# @ stub NtQueryQuotaInformationFile
@ stdcall NtQuerySection (long long long long long)
@ stdcall NtQuerySecurityObject (long long long long long)
@ stdcall NtQuerySemaphore (long long ptr long ptr)
@ stdcall NtQuerySymbolicLinkObject(long ptr ptr)
@ stdcall NtQuerySystemEnvironmentValue(ptr ptr long ptr)
@ stdcall NtQuerySystemEnvironmentValueEx(ptr ptr ptr ptr ptr)
@ stdcall NtQuerySystemInformation(long long long long)
@ stdcall NtQuerySystemTime(ptr)
@ stdcall NtQueryTimer(ptr long ptr long ptr)
@ stdcall NtQueryTimerResolution(long long long)
@ stdcall NtQueryValueKey(long long long long long long)
@ stdcall NtQueryVirtualMemory(long ptr long ptr long ptr)
@ stdcall NtQueryVolumeInformationFile(long ptr ptr long long)
@ stdcall NtQueueApcThread(long ptr long long long)
@ stdcall NtRaiseException(ptr ptr long)
@ stdcall NtRaiseHardError(long long ptr ptr long long)
@ stdcall NtReadFile(long long ptr ptr ptr ptr long ptr ptr)
@ stdcall NtReadFileScatter(long long ptr ptr ptr ptr long ptr ptr)
@ stub NtReadRequestData
@ stdcall NtReadVirtualMemory(long ptr ptr long ptr)
@ stub NtRegisterNewDevice
@ stdcall NtRegisterThreadTerminatePort(ptr)
@ stdcall NtReleaseKeyedEvent(long ptr long ptr)
@ stdcall NtReleaseMutant(long ptr)
@ stub NtReleaseProcessMutant
@ stdcall NtReleaseSemaphore(long long ptr)
@ stdcall NtRemoveIoCompletion(ptr ptr ptr ptr ptr)
# @ stub NtRemoveProcessDebug
# @ stub NtRenameKey
@ stdcall NtReplaceKey(ptr long ptr)
@ stub NtReplyPort
@ stdcall NtReplyWaitReceivePort(ptr ptr ptr ptr)
@ stub NtReplyWaitReceivePortEx
@ stub NtReplyWaitReplyPort
# @ stub NtRequestDeviceWakeup
@ stub NtRequestPort
@ stdcall NtRequestWaitReplyPort(ptr ptr ptr)
# @ stub NtRequestWakeupLatency
@ stdcall NtResetEvent(long ptr)
@ stdcall NtResetWriteWatch(long ptr long)
@ stdcall NtRestoreKey(long long long)
# @ stub NtResumeProcess
@ stdcall NtResumeThread(long long)
@ stdcall NtSaveKey(long long)
# @ stub NtSaveKeyEx
# @ stub NtSaveMergedKeys
@ stdcall NtSecureConnectPort(ptr ptr ptr ptr ptr ptr ptr ptr ptr)
# @ stub NtSetBootEntryOrder
# @ stub NtSetBootOptions
@ stdcall NtSetContextThread(long ptr)
@ stub NtSetDebugFilterState
@ stub NtSetDefaultHardErrorPort
@ stdcall NtSetDefaultLocale(long long)
@ stdcall NtSetDefaultUILanguage(long)
@ stdcall NtSetEaFile(long ptr ptr long)
@ stdcall NtSetEvent(long long)
# @ stub NtSetEventBoostPriority
@ stub NtSetHighEventPair
@ stub NtSetHighWaitLowEventPair
@ stub NtSetHighWaitLowThread
# @ stub NtSetInformationDebugObject
@ stdcall NtSetInformationFile(long long long long long)
@ stdcall NtSetInformationJobObject(long long ptr long)
@ stdcall NtSetInformationKey(long long ptr long)
@ stdcall NtSetInformationObject(long long ptr long)
@ stdcall NtSetInformationProcess(long long long long)
@ stdcall NtSetInformationThread(long long ptr long)
@ stdcall NtSetInformationToken(long long ptr long)
@ stdcall NtSetIntervalProfile(long long)
@ stdcall NtSetIoCompletion(ptr long ptr long long)
@ stub NtSetLdtEntries
@ stub NtSetLowEventPair
@ stub NtSetLowWaitHighEventPair
@ stub NtSetLowWaitHighThread
# @ stub NtSetQuotaInformationFile
@ stdcall NtSetSecurityObject(long long ptr)
@ stub NtSetSystemEnvironmentValue
# @ stub NtSetSystemEnvironmentValueEx
@ stdcall NtSetSystemInformation(long ptr long)
@ stub NtSetSystemPowerState
@ stdcall NtSetSystemTime(ptr ptr)
# @ stub NtSetThreadExecutionState
@ stdcall NtSetTimer(long ptr ptr ptr long long ptr)
@ stdcall NtSetTimerResolution(long long ptr)
# @ stub NtSetUuidSeed
@ stdcall NtSetValueKey(long long long long long long)
@ stdcall NtSetVolumeInformationFile(long ptr ptr long long)
@ stdcall NtShutdownSystem(long)
@ stdcall NtSignalAndWaitForSingleObject(long long long ptr)
@ stub NtStartProfile
@ stub NtStopProfile
# @ stub NtSuspendProcess
@ stdcall NtSuspendThread(long ptr)
@ stdcall NtSystemDebugControl(long ptr long ptr long ptr)
@ stdcall NtTerminateJobObject(long long)
@ stdcall NtTerminateProcess(long long)
@ stdcall NtTerminateThread(long long)
@ stub NtTestAlert
# @ stub NtTraceEvent
# @ stub NtTranslateFilePath
@ stdcall NtUnloadDriver(ptr)
@ stdcall NtUnloadKey(long)
@ stub NtUnloadKeyEx
@ stdcall NtUnlockFile(long ptr ptr ptr ptr)
@ stdcall NtUnlockVirtualMemory(long ptr ptr long)
@ stdcall NtUnmapViewOfSection(long ptr)
@ stub NtVdmControl
@ stub NtW32Call
# @ stub NtWaitForDebugEvent
@ stdcall NtWaitForKeyedEvent(long ptr long ptr)
@ stdcall NtWaitForMultipleObjects(long ptr long long ptr)
@ stub NtWaitForProcessMutant
@ stdcall NtWaitForSingleObject(long long long)
@ stub NtWaitHighEventPair
@ stub NtWaitLowEventPair
@ stdcall NtWriteFile(long long ptr ptr ptr ptr long ptr ptr)
@ stdcall NtWriteFileGather(long long ptr ptr ptr ptr long ptr ptr)
@ stub NtWriteRequestData
@ stdcall NtWriteVirtualMemory(long ptr ptr long ptr)
@ stdcall NtYieldExecution()
@ stub PfxFindPrefix
@ stub PfxInitialize
@ stub PfxInsertPrefix
@ stub PfxRemovePrefix
# @ stub PropertyLengthAsVariant
@ stub RtlAbortRXact
@ stdcall RtlAbsoluteToSelfRelativeSD(ptr ptr ptr)
@ stdcall RtlAcquirePebLock()
@ stdcall RtlAcquireResourceExclusive(ptr long)
@ stdcall RtlAcquireResourceShared(ptr long)
@ stdcall RtlAcquireSRWLockExclusive(ptr)
@ stdcall RtlAcquireSRWLockShared(ptr)
@ stdcall RtlActivateActivationContext(long ptr ptr)
@ stub RtlActivateActivationContextEx
@ stub RtlActivateActivationContextUnsafeFast
@ stdcall RtlAddAccessAllowedAce(ptr long long ptr)
@ stdcall RtlAddAccessAllowedAceEx(ptr long long long ptr)
# @ stub RtlAddAccessAllowedObjectAce
@ stdcall RtlAddAccessDeniedAce(ptr long long ptr)
@ stdcall RtlAddAccessDeniedAceEx(ptr long long long ptr)
# @ stub RtlAddAccessDeniedObjectAce
@ stdcall RtlAddAce(ptr long long ptr long)
@ stub RtlAddActionToRXact
@ stdcall RtlAddAtomToAtomTable(ptr wstr ptr)
@ stub RtlAddAttributeActionToRXact
@ stdcall RtlAddAuditAccessAce(ptr long long ptr long long)
@ stdcall RtlAddAuditAccessAceEx(ptr long long long ptr long long)
# @ stub RtlAddAuditAccessObjectAce
# @ stub RtlAddCompoundAce
# @ stub RtlAddRange
@ cdecl -arch=arm,x86_64 RtlAddFunctionTable(ptr long long)
@ stdcall RtlAddRefActivationContext(ptr)
# @ stub RtlAddRefMemoryStream
@ stdcall RtlAddVectoredExceptionHandler(long ptr)
# @ stub RtlAddressInSectionTable
@ stdcall RtlAdjustPrivilege(long long long ptr)
@ stdcall RtlAllocateAndInitializeSid (ptr long long long long long long long long long ptr)
@ stdcall RtlAllocateHandle(ptr ptr)
@ stdcall RtlAllocateHeap(long long long)
@ stdcall RtlAnsiCharToUnicodeChar(ptr)
@ stdcall RtlAnsiStringToUnicodeSize(ptr)
@ stdcall RtlAnsiStringToUnicodeString(ptr ptr long)
@ stdcall RtlAppendAsciizToString(ptr str)
# @ stub RtlAppendPathElement
@ stdcall RtlAppendStringToString(ptr ptr)
@ stdcall RtlAppendUnicodeStringToString(ptr ptr)
@ stdcall RtlAppendUnicodeToString(ptr wstr)
# @ stub RtlApplicationVerifierStop
@ stub RtlApplyRXact
@ stub RtlApplyRXactNoFlush
@ stdcall RtlAreAllAccessesGranted(long long)
@ stdcall RtlAreAnyAccessesGranted(long long)
@ stdcall RtlAreBitsClear(ptr long long)
@ stdcall RtlAreBitsSet(ptr long long)
# @ stub RtlAssert2
@ stdcall RtlAssert(ptr ptr long long)
# @ stub RtlCancelTimer
@ stdcall -norelay RtlCaptureContext(ptr)
@ stdcall RtlCaptureStackBackTrace(long long ptr ptr)
# @ stub RtlCaptureStackContext
@ stdcall RtlCharToInteger(ptr long ptr)
# @ stub RtlCheckForOrphanedCriticalSections
# @ stub RtlCheckProcessParameters
@ stdcall RtlCheckRegistryKey(long ptr)
@ stdcall RtlClearAllBits(ptr)
@ stdcall RtlClearBits(ptr long long)
# @ stub RtlCloneMemoryStream
@ stub RtlClosePropertySet
# @ stub RtlCommitMemoryStream
@ stdcall RtlCompactHeap(long long)
@ stdcall RtlCompareMemory(ptr ptr long)
@ stdcall RtlCompareMemoryUlong(ptr long long)
@ stdcall RtlCompareString(ptr ptr long)
@ stdcall RtlCompareUnicodeString (ptr ptr long)
@ stdcall RtlCompressBuffer(long ptr long ptr long long ptr ptr)
@ stdcall RtlComputeCrc32(long ptr long)
# @ stub RtlComputeImportTableHash
# @ stub RtlComputePrivatizedDllName_U
@ stub RtlConsoleMultiByteToUnicodeN
@ stub RtlConvertExclusiveToShared
@ stdcall -arch=win32 -ret64 RtlConvertLongToLargeInteger(long)
# @ stub RtlConvertPropertyToVariant
@ stub RtlConvertSharedToExclusive
@ stdcall RtlConvertSidToUnicodeString(ptr ptr long)
@ stdcall RtlConvertToAutoInheritSecurityObject(ptr ptr ptr ptr long ptr)
@ stub RtlConvertUiListToApiList
@ stdcall -arch=win32 -ret64 RtlConvertUlongToLargeInteger(long)
# @ stub RtlConvertVariantToProperty
@ stdcall RtlCopyLuid(ptr ptr)
@ stdcall RtlCopyLuidAndAttributesArray(long ptr ptr)
# @ stub RtlCopyMemoryStreamTo
# @ stub RtlCopyOutOfProcessMemoryStreamTo
# @ stub RtlCopyRangeList
@ stdcall RtlCopySecurityDescriptor(ptr ptr)
@ stdcall RtlCopySid(long ptr ptr)
@ stub RtlCopySidAndAttributesArray
@ stdcall RtlCopyString(ptr ptr)
@ stdcall RtlCopyUnicodeString(ptr ptr)
@ stdcall RtlCreateAcl(ptr long long)
@ stdcall RtlCreateActivationContext(ptr ptr)
@ stub RtlCreateAndSetSD
@ stdcall RtlCreateAtomTable(long ptr)
# @ stub RtlCreateBootStatusDataFile
@ stdcall RtlCreateEnvironment(long ptr)
@ stdcall RtlCreateHeap(long ptr long long ptr ptr)
@ stdcall RtlCreateProcessParameters(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stub RtlCreatePropertySet
@ stdcall RtlCreateQueryDebugBuffer(long long)
@ stub RtlCreateRegistryKey
@ stdcall RtlCreateSecurityDescriptor(ptr long)
# @ stub RtlCreateSystemVolumeInformationFolder
@ stub RtlCreateTagHeap
@ stdcall RtlCreateTimer(ptr ptr ptr ptr long long long)
@ stdcall RtlCreateTimerQueue(ptr)
@ stdcall RtlCreateUnicodeString(ptr wstr)
@ stdcall RtlCreateUnicodeStringFromAsciiz(ptr str)
@ stub RtlCreateUserProcess
@ stub RtlCreateUserSecurityObject
@ stdcall RtlCreateUserThread(long ptr long ptr long long ptr ptr ptr ptr)
@ stub RtlCustomCPToUnicodeN
@ stub RtlCutoverTimeToSystemTime
@ stdcall RtlDeNormalizeProcessParams(ptr)
@ stdcall RtlDeactivateActivationContext(long long)
@ stub RtlDeactivateActivationContextUnsafeFast
@ stub RtlDebugPrintTimes
@ stdcall RtlDecodePointer(ptr)
# @ stub RtlDecodeSystemPointer
@ stdcall RtlDecompressBuffer(long ptr long ptr long ptr)
@ stub RtlDecompressFragment
@ stub RtlDefaultNpAcl
@ stub RtlDelete
@ stdcall RtlDeleteAce(ptr long)
@ stdcall RtlDeleteAtomFromAtomTable(ptr long)
@ stdcall RtlDeleteCriticalSection(ptr)
@ stub RtlDeleteElementGenericTable
@ stub RtlDeleteElementGenericTableAvl
@ cdecl -arch=arm,x86_64 RtlDeleteFunctionTable(ptr)
@ stub RtlDeleteNoSplay
@ stub RtlDeleteOwnersRanges
@ stub RtlDeleteRange
@ stdcall RtlDeleteRegistryValue(long ptr ptr)
@ stdcall RtlDeleteResource(ptr)
@ stdcall RtlDeleteSecurityObject(ptr)
@ stdcall RtlDeleteTimer(ptr ptr ptr)
# @ stub RtlDeleteTimerQueue
@ stdcall RtlDeleteTimerQueueEx(ptr ptr)
@ stdcall RtlDeregisterWait(ptr)
@ stdcall RtlDeregisterWaitEx(ptr ptr)
@ stdcall RtlDestroyAtomTable(ptr)
@ stdcall RtlDestroyEnvironment(ptr)
@ stdcall RtlDestroyHandleTable(ptr)
@ stdcall RtlDestroyHeap(long)
@ stdcall RtlDestroyProcessParameters(ptr)
@ stdcall RtlDestroyQueryDebugBuffer(ptr)
@ stdcall RtlDetermineDosPathNameType_U(wstr)
@ stdcall RtlDllShutdownInProgress()
# @ stub RtlDnsHostNameToComputerName
@ stdcall RtlDoesFileExists_U(wstr)
# @ stub RtlDosApplyFileIsolationRedirection_Ustr
@ stdcall RtlDosPathNameToNtPathName_U(wstr ptr ptr ptr)
@ stdcall RtlDosSearchPath_U(wstr wstr wstr long ptr ptr)
# @ stub RtlDosSearchPath_Ustr
@ stdcall RtlDowncaseUnicodeChar(long)
@ stdcall RtlDowncaseUnicodeString(ptr ptr long)
@ stdcall RtlDumpResource(ptr)
@ stdcall RtlDuplicateUnicodeString(long ptr ptr)
@ stdcall RtlEmptyAtomTable(ptr long)
# @ stub RtlEnableEarlyCriticalSectionEventCreation
@ stdcall RtlEncodePointer(ptr)
# @ stub RtlEncodeSystemPointer
@ stdcall -arch=win32 -ret64 RtlEnlargedIntegerMultiply(long long)
@ stdcall -arch=win32 RtlEnlargedUnsignedDivide(int64 long ptr)
@ stdcall -arch=win32 -ret64 RtlEnlargedUnsignedMultiply(long long)
@ stdcall RtlEnterCriticalSection(ptr)
@ stub RtlEnumProcessHeaps
@ stub RtlEnumerateGenericTable
# @ stub RtlEnumerateGenericTableAvl
# @ stub RtlEnumerateGenericTableLikeADirectory
@ stdcall RtlEnumerateGenericTableWithoutSplaying(ptr ptr)
# @ stub RtlEnumerateGenericTableWithoutSplayingAvl
@ stub RtlEnumerateProperties
@ stdcall RtlEqualComputerName(ptr ptr)
@ stdcall RtlEqualDomainName(ptr ptr)
@ stdcall RtlEqualLuid(ptr ptr)
@ stdcall RtlEqualPrefixSid(ptr ptr)
@ stdcall RtlEqualSid(long long)
@ stdcall RtlEqualString(ptr ptr long)
@ stdcall RtlEqualUnicodeString(ptr ptr long)
@ stdcall RtlEraseUnicodeString(ptr)
@ stdcall RtlExitUserProcess(long)
@ stdcall RtlExitUserThread(long)
@ stdcall RtlExpandEnvironmentStrings_U(ptr ptr ptr ptr)
@ stub RtlExtendHeap
@ stdcall -arch=win32 -ret64 RtlExtendedIntegerMultiply(int64 long)
@ stdcall -arch=win32 -ret64 RtlExtendedLargeIntegerDivide(int64 long ptr)
@ stdcall -arch=win32 -ret64 RtlExtendedMagicDivide(int64 int64 long)
@ stdcall RtlFillMemory(ptr long long)
@ stdcall RtlFillMemoryUlong(ptr long long)
@ stub RtlFinalReleaseOutOfProcessMemoryStream
@ stdcall RtlFindActivationContextSectionGuid(long ptr long ptr ptr)
@ stdcall RtlFindActivationContextSectionString(long ptr long ptr ptr)
@ stdcall RtlFindCharInUnicodeString(long ptr ptr ptr)
@ stdcall RtlFindClearBits(ptr long long)
@ stdcall RtlFindClearBitsAndSet(ptr long long)
@ stdcall RtlFindClearRuns(ptr ptr long long)
@ stdcall RtlFindLastBackwardRunClear(ptr long ptr)
@ stdcall RtlFindLastBackwardRunSet(ptr long ptr)
@ stdcall RtlFindLeastSignificantBit(int64)
@ stdcall RtlFindLongestRunClear(ptr long)
@ stdcall RtlFindLongestRunSet(ptr long)
@ stdcall RtlFindMessage(long long long long ptr)
@ stdcall RtlFindMostSignificantBit(int64)
@ stdcall RtlFindNextForwardRunClear(ptr long ptr)
@ stdcall RtlFindNextForwardRunSet(ptr long ptr)
@ stub RtlFindRange
@ stdcall RtlFindSetBits(ptr long long)
@ stdcall RtlFindSetBitsAndClear(ptr long long)
@ stdcall RtlFindSetRuns(ptr ptr long long)
@ stdcall RtlFirstEntrySList(ptr)
@ stdcall RtlFirstFreeAce(ptr ptr)
@ stub RtlFlushPropertySet
# @ stub RtlFlushSecureMemoryCache
@ stdcall RtlFormatCurrentUserKeyPath(ptr)
@ stdcall RtlFormatMessage(ptr long long long long ptr ptr long)
@ stdcall RtlFreeAnsiString(long)
@ stdcall RtlFreeHandle(ptr ptr)
@ stdcall RtlFreeHeap(long long ptr)
@ stdcall RtlFreeOemString(ptr)
# @ stub RtlFreeRangeList
@ stdcall RtlFreeSid (long)
@ stdcall RtlFreeThreadActivationContextStack()
@ stdcall RtlFreeUnicodeString(ptr)
@ stub RtlFreeUserThreadStack
@ stdcall RtlGUIDFromString(ptr ptr)
@ stub RtlGenerate8dot3Name
@ stdcall RtlGetAce(ptr long ptr)
@ stdcall RtlGetActiveActivationContext(ptr)
@ stub RtlGetCallersAddress
@ stdcall RtlGetCompressionWorkSpaceSize(long ptr ptr)
@ stdcall RtlGetControlSecurityDescriptor(ptr ptr ptr)
@ stdcall RtlGetCurrentDirectory_U(long ptr)
@ stdcall RtlGetCurrentPeb()
@ stdcall RtlGetCurrentTransaction()
@ stdcall RtlGetDaclSecurityDescriptor(ptr ptr ptr ptr)
@ stub RtlGetElementGenericTable
# @ stub RtlGetElementGenericTableAvl
# @ stub RtlGetFirstRange
# @ stub RtlGetFrame
@ stdcall RtlGetFullPathName_U(wstr long ptr ptr)
@ stdcall RtlGetGroupSecurityDescriptor(ptr ptr ptr)
@ stdcall RtlGetLastNtStatus()
@ stdcall RtlGetLastWin32Error()
# @ stub RtlGetLengthWithoutLastFullDosOrNtPathElement
# Yes, Microsoft really misspelled this one!
# @ stub RtlGetLengthWithoutTrailingPathSeperators
@ stdcall RtlGetLongestNtPathLength()
# @ stub RtlGetNativeSystemInformation
# @ stub RtlGetNextRange
@ stdcall RtlGetNtGlobalFlags()
@ stdcall RtlGetNtProductType(ptr)
@ stdcall RtlGetNtVersionNumbers(ptr ptr ptr)
@ stdcall RtlGetOwnerSecurityDescriptor(ptr ptr ptr)
@ stdcall RtlGetProductInfo(long long long long ptr)
@ stdcall RtlGetProcessHeaps(long ptr)
@ stdcall RtlGetSaclSecurityDescriptor(ptr ptr ptr ptr)
# @ stub RtlGetSecurityDescriptorRMControl
# @ stub RtlGetSetBootStatusData
@ stdcall RtlGetThreadErrorMode()
# @ stub RtlGetUnloadEventTrace
@ stub RtlGetUserInfoHeap
@ stdcall RtlGetVersion(ptr)
@ stub RtlGuidToPropertySetName
@ stdcall RtlHashUnicodeString(ptr long long ptr)
@ stdcall RtlIdentifierAuthoritySid(ptr)
@ stdcall RtlImageDirectoryEntryToData(long long long ptr)
@ stdcall RtlImageNtHeader(long)
@ stdcall RtlImageRvaToSection(ptr long long)
@ stdcall RtlImageRvaToVa(ptr long long ptr)
@ stdcall RtlImpersonateSelf(long)
@ stdcall RtlInitAnsiString(ptr str)
@ stdcall RtlInitAnsiStringEx(ptr str)
@ stub RtlInitCodePageTable
# @ stub RtlInitMemoryStream
@ stub RtlInitNlsTables
# @ stub RtlInitOutOfProcessMemoryStream
@ stdcall RtlInitString(ptr str)
@ stdcall RtlInitUnicodeString(ptr wstr)
@ stdcall RtlInitUnicodeStringEx(ptr wstr)
# @ stub RtlInitializeAtomPackage
@ stdcall RtlInitializeBitMap(ptr long long)
@ stdcall RtlInitializeConditionVariable(ptr)
@ stub RtlInitializeContext
@ stdcall RtlInitializeCriticalSection(ptr)
@ stdcall RtlInitializeCriticalSectionAndSpinCount(ptr long)
@ stdcall RtlInitializeCriticalSectionEx(ptr long long)
@ stdcall RtlInitializeGenericTable(ptr ptr ptr ptr ptr)
# @ stub RtlInitializeGenericTableAvl
@ stdcall RtlInitializeHandleTable(long long ptr)
@ stub RtlInitializeRXact
# @ stub RtlInitializeRangeList
@ stdcall RtlInitializeResource(ptr)
@ stdcall RtlInitializeSListHead(ptr)
@ stdcall RtlInitializeSRWLock(ptr)
@ stdcall RtlInitializeSid(ptr ptr long)
# @ stub RtlInitializeStackTraceDataBase
@ stub RtlInsertElementGenericTable
# @ stub RtlInsertElementGenericTableAvl
@ stdcall RtlInt64ToUnicodeString(int64 long ptr)
@ stdcall RtlIntegerToChar(long long long ptr)
@ stdcall RtlIntegerToUnicodeString(long long ptr)
@ stdcall -arch=win32 -ret64 RtlInterlockedCompareExchange64(ptr int64 int64)
@ stdcall RtlInterlockedFlushSList(ptr)
@ stdcall RtlInterlockedPopEntrySList(ptr)
@ stdcall RtlInterlockedPushEntrySList(ptr ptr)
@ stdcall RtlInterlockedPushListSList(ptr ptr ptr long)
# @ stub RtlInvertRangeList
@ stdcall RtlIpv4AddressToStringA(ptr ptr)
@ stdcall RtlIpv4AddressToStringExA(ptr long ptr ptr)
@ stdcall RtlIpv4AddressToStringExW(ptr long ptr ptr)
@ stdcall RtlIpv4AddressToStringW(ptr ptr)
# @ stub RtlIpv4StringToAddressA
# @ stub RtlIpv4StringToAddressExA
@ stdcall RtlIpv4StringToAddressExW(ptr ptr wstr ptr)
# @ stub RtlIpv4StringToAddressW
# @ stub RtlIpv6AddressToStringA
# @ stub RtlIpv6AddressToStringExA
# @ stub RtlIpv6AddressToStringExW
# @ stub RtlIpv6AddressToStringW
# @ stub RtlIpv6StringToAddressA
# @ stub RtlIpv6StringToAddressExA
# @ stub RtlIpv6StringToAddressExW
# @ stub RtlIpv6StringToAddressW
@ stdcall RtlIsActivationContextActive(ptr)
@ stdcall RtlIsDosDeviceName_U(wstr)
@ stub RtlIsGenericTableEmpty
# @ stub RtlIsGenericTableEmptyAvl
@ stdcall RtlIsNameLegalDOS8Dot3(ptr ptr ptr)
# @ stub RtlIsRangeAvailable
@ stdcall RtlIsTextUnicode(ptr long ptr)
# @ stub RtlIsThreadWithinLoaderCallout
@ stdcall RtlIsValidHandle(ptr ptr)
@ stdcall RtlIsValidIndexHandle(ptr long ptr)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerAdd(int64 int64)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerArithmeticShift(int64 long)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerDivide(int64 int64 ptr)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerNegate(int64)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerShiftLeft(int64 long)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerShiftRight(int64 long)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerSubtract(int64 int64)
@ stdcall RtlLargeIntegerToChar(ptr long long ptr)
@ stdcall RtlLeaveCriticalSection(ptr)
@ stdcall RtlLengthRequiredSid(long)
@ stdcall RtlLengthSecurityDescriptor(ptr)
@ stdcall RtlLengthSid(ptr)
@ stdcall RtlLocalTimeToSystemTime(ptr ptr)
# @ stub RtlLockBootStatusData
@ stdcall RtlLockHeap(long)
# @ stub RtlLockMemoryStreamRegion
# @ stub RtlLogStackBackTrace
@ stdcall RtlLookupAtomInAtomTable(ptr wstr ptr)
@ stub RtlLookupElementGenericTable
# @ stub RtlLookupElementGenericTableAvl
@ stdcall -arch=arm,x86_64 RtlLookupFunctionEntry(long ptr ptr)
@ stdcall RtlMakeSelfRelativeSD(ptr ptr ptr)
@ stdcall RtlMapGenericMask(long ptr)
# @ stub RtlMapSecurityErrorToNtStatus
# @ stub RtlMergeRangeLists
@ stdcall RtlMoveMemory(ptr ptr long)
# @ stub RtlMultiAppendUnicodeStringBuffer
@ stdcall RtlMultiByteToUnicodeN(ptr long ptr ptr long)
@ stdcall RtlMultiByteToUnicodeSize(ptr str long)
@ stub RtlNewInstanceSecurityObject
@ stub RtlNewSecurityGrantedAccess
@ stdcall RtlNewSecurityObject(ptr ptr ptr long ptr ptr)
# @ stub RtlNewSecurityObjectEx
# @ stub RtlNewSecurityObjectWithMultipleInheritance
@ stdcall RtlNormalizeProcessParams(ptr)
# @ stub RtlNtPathNameToDosPathName
@ stdcall RtlNtStatusToDosError(long)
@ stdcall RtlNtStatusToDosErrorNoTeb(long)
@ stdcall RtlNumberGenericTableElements(ptr)
# @ stub RtlNumberGenericTableElementsAvl
@ stdcall RtlNumberOfClearBits(ptr)
@ stdcall RtlNumberOfSetBits(ptr)
@ stdcall RtlOemStringToUnicodeSize(ptr)
@ stdcall RtlOemStringToUnicodeString(ptr ptr long)
@ stdcall RtlOemToUnicodeN(ptr long ptr ptr long)
@ stdcall RtlOpenCurrentUser(long ptr)
@ stdcall RtlPcToFileHeader(ptr ptr)
@ stdcall RtlPinAtomInAtomTable(ptr long)
# @ stub RtlPopFrame
@ stdcall RtlPrefixString(ptr ptr long)
@ stdcall RtlPrefixUnicodeString(ptr ptr long)
@ stub RtlPropertySetNameToGuid
@ stub RtlProtectHeap
# @ stub RtlPushFrame
@ stdcall RtlQueryAtomInAtomTable(ptr long ptr ptr ptr ptr)
@ stdcall RtlQueryDepthSList(ptr)
@ stdcall RtlQueryEnvironmentVariable_U(ptr ptr ptr)
@ stdcall RtlQueryHeapInformation(long long ptr long ptr)
@ stdcall RtlQueryInformationAcl(ptr ptr long long)
@ stdcall RtlQueryInformationActivationContext(long long ptr long ptr long ptr)
@ stub RtlQueryInformationActiveActivationContext
@ stub RtlQueryInterfaceMemoryStream
@ stub RtlQueryProcessBackTraceInformation
@ stdcall RtlQueryProcessDebugInformation(long long ptr)
@ stub RtlQueryProcessHeapInformation
@ stub RtlQueryProcessLockInformation
@ stub RtlQueryProperties
@ stub RtlQueryPropertyNames
@ stub RtlQueryPropertySet
@ stdcall RtlQueryRegistryValues(long ptr ptr ptr ptr)
@ stub RtlQuerySecurityObject
@ stub RtlQueryTagHeap
@ stdcall RtlQueryTimeZoneInformation(ptr)
@ stdcall RtlQueryUnbiasedInterruptTime(ptr)
@ stub RtlQueueApcWow64Thread
@ stdcall RtlQueueWorkItem(ptr ptr long)
@ stdcall -register RtlRaiseException(ptr)
@ stdcall RtlRaiseStatus(long)
@ stdcall RtlRandom(ptr)
@ stub RtlRandomEx
@ stdcall RtlReAllocateHeap(long long ptr long)
@ stub RtlReadMemoryStream
@ stub RtlReadOutOfProcessMemoryStream
@ stub RtlRealPredecessor
@ stub RtlRealSuccessor
@ stub RtlRegisterSecureMemoryCacheCallback
@ stdcall RtlRegisterWait(ptr ptr ptr ptr long long)
@ stdcall RtlReleaseActivationContext(ptr)
@ stub RtlReleaseMemoryStream
@ stdcall RtlReleasePebLock()
@ stdcall RtlReleaseResource(ptr)
@ stdcall RtlReleaseSRWLockExclusive(ptr)
@ stdcall RtlReleaseSRWLockShared(ptr)
@ stub RtlRemoteCall
@ stdcall RtlRemoveVectoredExceptionHandler(ptr)
@ stub RtlResetRtlTranslations
@ stdcall RtlRestoreLastWin32Error(long) RtlSetLastWin32Error
@ stub RtlRevertMemoryStream
@ stub RtlRunDecodeUnicodeString
@ stub RtlRunEncodeUnicodeString
@ stdcall RtlRunOnceBeginInitialize(ptr long ptr)
@ stdcall RtlRunOnceComplete(ptr long ptr)
@ stdcall RtlRunOnceExecuteOnce(ptr ptr ptr ptr)
@ stdcall RtlRunOnceInitialize(ptr)
@ stdcall RtlSecondsSince1970ToTime(long ptr)
@ stdcall RtlSecondsSince1980ToTime(long ptr)
# @ stub RtlSeekMemoryStream
# @ stub RtlSelfRelativeToAbsoluteSD2
@ stdcall RtlSelfRelativeToAbsoluteSD(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall RtlSetAllBits(ptr)
# @ stub RtlSetAttributesSecurityDescriptor
@ stdcall RtlSetBits(ptr long long)
@ stdcall RtlSetControlSecurityDescriptor(ptr long long)
@ stdcall RtlSetCriticalSectionSpinCount(ptr long)
@ stdcall RtlSetCurrentDirectory_U(ptr)
@ stdcall RtlSetCurrentEnvironment(wstr ptr)
@ stdcall RtlSetCurrentTransaction(ptr)
@ stdcall RtlSetDaclSecurityDescriptor(ptr long ptr long)
@ stdcall RtlSetEnvironmentVariable(ptr ptr ptr)
@ stdcall RtlSetGroupSecurityDescriptor(ptr ptr long)
# @ stub RtlSetHeapInformation
@ stub RtlSetInformationAcl
@ stdcall RtlSetIoCompletionCallback(long ptr long)
@ stdcall RtlSetLastWin32Error(long)
@ stdcall RtlSetLastWin32ErrorAndNtStatusFromNtStatus(long)
# @ stub RtlSetMemoryStreamSize
@ stdcall RtlSetOwnerSecurityDescriptor(ptr ptr long)
# @ stub RtlSetProcessIsCritical
@ stub RtlSetProperties
@ stub RtlSetPropertyClassId
@ stub RtlSetPropertyNames
@ stub RtlSetPropertySetClassId
@ stdcall RtlSetSaclSecurityDescriptor(ptr long ptr long)
# @ stub RtlSetSecurityDescriptorRMControl
@ stub RtlSetSecurityObject
# @ stub RtlSetSecurityObjectEx
@ stdcall RtlSetThreadErrorMode(long ptr)
# @ stub RtlSetThreadIsCritical
# @ stub RtlSetThreadPoolStartFunc
@ stdcall RtlSetTimeZoneInformation(ptr)
# @ stub RtlSetTimer
@ stub RtlSetUnicodeCallouts
@ stub RtlSetUserFlagsHeap
@ stub RtlSetUserValueHeap
@ stdcall RtlSizeHeap(long long ptr)
@ stdcall RtlSleepConditionVariableCS(ptr ptr ptr)
@ stdcall RtlSleepConditionVariableSRW(ptr ptr ptr long)
@ stub RtlSplay
@ stub RtlStartRXact
# @ stub RtlStatMemoryStream
@ stdcall RtlStringFromGUID(ptr ptr)
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
# @ stub RtlTraceDatabaseAdd
# @ stub RtlTraceDatabaseCreate
# @ stub RtlTraceDatabaseDestroy
# @ stub RtlTraceDatabaseEnumerate
# @ stub RtlTraceDatabaseFind
# @ stub RtlTraceDatabaseLock
# @ stub RtlTraceDatabaseUnlock
# @ stub RtlTraceDatabaseValidate
@ stdcall RtlTryAcquireSRWLockExclusive(ptr)
@ stdcall RtlTryAcquireSRWLockShared(ptr)
@ stdcall RtlTryEnterCriticalSection(ptr)
@ cdecl -i386 -norelay RtlUlongByteSwap() NTDLL_RtlUlongByteSwap
@ cdecl -ret64 RtlUlonglongByteSwap(int64)
# @ stub RtlUnhandledExceptionFilter2
# @ stub RtlUnhandledExceptionFilter
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
# @ stub RtlUnlockBootStatusData
@ stdcall RtlUnlockHeap(long)
# @ stub RtlUnlockMemoryStreamRegion
@ stdcall -register RtlUnwind(ptr ptr ptr ptr)
@ stdcall -arch=x86_64 RtlUnwindEx(ptr ptr ptr ptr ptr ptr)
@ stdcall RtlUpcaseUnicodeChar(long)
@ stdcall RtlUpcaseUnicodeString(ptr ptr long)
@ stdcall RtlUpcaseUnicodeStringToAnsiString(ptr ptr long)
@ stdcall RtlUpcaseUnicodeStringToCountedOemString(ptr ptr long)
@ stdcall RtlUpcaseUnicodeStringToOemString(ptr ptr long)
@ stub RtlUpcaseUnicodeToCustomCPN
@ stdcall RtlUpcaseUnicodeToMultiByteN(ptr long ptr ptr long)
@ stdcall RtlUpcaseUnicodeToOemN(ptr long ptr ptr long)
@ stdcall RtlUpdateTimer(ptr ptr long long)
@ stdcall RtlUpperChar(long)
@ stdcall RtlUpperString(ptr ptr)
@ stub RtlUsageHeap
@ cdecl -i386 -norelay RtlUshortByteSwap() NTDLL_RtlUshortByteSwap
@ stdcall RtlValidAcl(ptr)
# @ stub RtlValidRelativeSecurityDescriptor
@ stdcall RtlValidSecurityDescriptor(ptr)
@ stdcall RtlValidSid(ptr)
@ stdcall RtlValidateHeap(long long ptr)
@ stub RtlValidateProcessHeaps
# @ stub RtlValidateUnicodeString
@ stdcall RtlVerifyVersionInfo(ptr long int64)
@ stdcall -arch=x86_64 RtlVirtualUnwind(long long long ptr ptr ptr ptr ptr)
@ stdcall RtlWakeAllConditionVariable(ptr)
@ stdcall RtlWakeConditionVariable(ptr)
@ stub RtlWalkFrameChain
@ stdcall RtlWalkHeap(long ptr)
@ stdcall RtlWow64EnableFsRedirection(long)
@ stdcall RtlWow64EnableFsRedirectionEx(long ptr)
@ stub RtlWriteMemoryStream
@ stdcall RtlWriteRegistryValue(long ptr ptr long ptr long)
@ stub RtlZeroHeap
@ stdcall RtlZeroMemory(ptr long)
@ stdcall RtlZombifyActivationContext(ptr)
# @ stub RtlpApplyLengthFunction
# @ stub RtlpEnsureBufferSize
# @ stub RtlpNotOwnerCriticalSection
@ stdcall RtlpNtCreateKey(ptr long ptr long ptr long long)
@ stdcall RtlpNtEnumerateSubKey(ptr ptr long)
@ stdcall RtlpNtMakeTemporaryKey(ptr)
@ stdcall RtlpNtOpenKey(ptr long ptr)
@ stdcall RtlpNtQueryValueKey(long ptr ptr ptr ptr)
@ stdcall RtlpNtSetValueKey(ptr long ptr long)
@ stdcall RtlpUnWaitCriticalSection(ptr)
@ stdcall RtlpWaitForCriticalSection(ptr)
@ stdcall RtlxAnsiStringToUnicodeSize(ptr) RtlAnsiStringToUnicodeSize
@ stdcall RtlxOemStringToUnicodeSize(ptr) RtlOemStringToUnicodeSize
@ stdcall RtlxUnicodeStringToAnsiSize(ptr) RtlUnicodeStringToAnsiSize
@ stdcall RtlxUnicodeStringToOemSize(ptr) RtlUnicodeStringToOemSize
@ stdcall -ret64 VerSetConditionMask(int64 long long)
@ stdcall ZwAcceptConnectPort(ptr long ptr long long ptr) NtAcceptConnectPort
@ stdcall ZwAccessCheck(ptr long long ptr ptr ptr ptr ptr) NtAccessCheck
@ stdcall ZwAccessCheckAndAuditAlarm(ptr long ptr ptr ptr long ptr long ptr ptr ptr) NtAccessCheckAndAuditAlarm
# @ stub ZwAccessCheckByType
# @ stub ZwAccessCheckByTypeAndAuditAlarm
# @ stub ZwAccessCheckByTypeResultList
# @ stub ZwAccessCheckByTypeResultListAndAuditAlarm
# @ stub ZwAccessCheckByTypeResultListAndAuditAlarmByHandle
@ stdcall ZwAddAtom(ptr long ptr) NtAddAtom
# @ stub ZwAddBootEntry
@ stdcall ZwAdjustGroupsToken(long long long long long long) NtAdjustGroupsToken
@ stdcall ZwAdjustPrivilegesToken(long long long long long long) NtAdjustPrivilegesToken
@ stdcall ZwAlertResumeThread(long ptr) NtAlertResumeThread
@ stdcall ZwAlertThread(long) NtAlertThread
@ stdcall ZwAllocateLocallyUniqueId(ptr) NtAllocateLocallyUniqueId
# @ stub ZwAllocateUserPhysicalPages
@ stdcall ZwAllocateUuids(ptr ptr ptr) NtAllocateUuids
@ stdcall ZwAllocateVirtualMemory(long ptr ptr ptr long long) NtAllocateVirtualMemory
@ stdcall ZwAreMappedFilesTheSame(ptr ptr) NtAreMappedFilesTheSame
@ stdcall ZwAssignProcessToJobObject(long long) NtAssignProcessToJobObject
@ stub ZwCallbackReturn
# @ stub ZwCancelDeviceWakeupRequest
@ stdcall ZwCancelIoFile(long ptr) NtCancelIoFile
@ stdcall ZwCancelIoFileEx(long ptr ptr) NtCancelIoFileEx
@ stdcall ZwCancelTimer(long ptr) NtCancelTimer
@ stdcall ZwClearEvent(long) NtClearEvent
@ stdcall ZwClose(long) NtClose
@ stub ZwCloseObjectAuditAlarm
# @ stub ZwCompactKeys
# @ stub ZwCompareTokens
@ stdcall ZwCompleteConnectPort(ptr) NtCompleteConnectPort
# @ stub ZwCompressKey
@ stdcall ZwConnectPort(ptr ptr ptr ptr ptr ptr ptr ptr) NtConnectPort
@ stub ZwContinue
# @ stub ZwCreateDebugObject
@ stdcall ZwCreateDirectoryObject(long long long) NtCreateDirectoryObject
@ stdcall ZwCreateEvent(long long long long long) NtCreateEvent
@ stub ZwCreateEventPair
@ stdcall ZwCreateFile(ptr long ptr ptr long long long ptr long long ptr) NtCreateFile
@ stdcall ZwCreateIoCompletion(ptr long ptr long) NtCreateIoCompletion
@ stdcall ZwCreateJobObject(ptr long ptr) NtCreateJobObject
# @ stub ZwCreateJobSet
@ stdcall ZwCreateKey(ptr long ptr long ptr long long) NtCreateKey
@ stdcall ZwCreateKeyedEvent(ptr long ptr long) NtCreateKeyedEvent
@ stdcall ZwCreateMailslotFile(long long long long long long long long) NtCreateMailslotFile
@ stdcall ZwCreateMutant(ptr long ptr long) NtCreateMutant
@ stdcall ZwCreateNamedPipeFile(ptr long ptr ptr long long long long long long long long long ptr) NtCreateNamedPipeFile
@ stdcall ZwCreatePagingFile(long long long long) NtCreatePagingFile
@ stdcall ZwCreatePort(ptr ptr long long long) NtCreatePort
@ stub ZwCreateProcess
# @ stub ZwCreateProcessEx
@ stub ZwCreateProfile
@ stdcall ZwCreateSection(ptr long ptr ptr long long long) NtCreateSection
@ stdcall ZwCreateSemaphore(ptr long ptr long long) NtCreateSemaphore
@ stdcall ZwCreateSymbolicLinkObject(ptr long ptr ptr) NtCreateSymbolicLinkObject
@ stub ZwCreateThread
@ stdcall ZwCreateTimer(ptr long ptr long) NtCreateTimer
@ stub ZwCreateToken
# @ stub ZwCreateWaitablePort
# @ stub ZwDebugActiveProcess
# @ stub ZwDebugContinue
@ stdcall ZwDelayExecution(long ptr) NtDelayExecution
@ stdcall ZwDeleteAtom(long) NtDeleteAtom
# @ stub ZwDeleteBootEntry
@ stdcall ZwDeleteFile(ptr) NtDeleteFile
@ stdcall ZwDeleteKey(long) NtDeleteKey
# @ stub ZwDeleteObjectAuditAlarm
@ stdcall ZwDeleteValueKey(long ptr) NtDeleteValueKey
@ stdcall ZwDeviceIoControlFile(long long long long long long long long long long) NtDeviceIoControlFile
@ stdcall ZwDisplayString(ptr) NtDisplayString
@ stdcall ZwDuplicateObject(long long long ptr long long long) NtDuplicateObject
@ stdcall ZwDuplicateToken(long long long long long long) NtDuplicateToken
# @ stub ZwEnumerateBootEntries
@ stub ZwEnumerateBus
@ stdcall ZwEnumerateKey(long long long ptr long ptr) NtEnumerateKey
# @ stub ZwEnumerateSystemEnvironmentValuesEx
@ stdcall ZwEnumerateValueKey(long long long ptr long ptr) NtEnumerateValueKey
@ stub ZwExtendSection
# @ stub ZwFilterToken
@ stdcall ZwFindAtom(ptr long ptr) NtFindAtom
@ stdcall ZwFlushBuffersFile(long ptr) NtFlushBuffersFile
@ stdcall ZwFlushInstructionCache(long ptr long) NtFlushInstructionCache
@ stdcall ZwFlushKey(long) NtFlushKey
@ stdcall ZwFlushVirtualMemory(long ptr ptr long) NtFlushVirtualMemory
@ stub ZwFlushWriteBuffer
# @ stub ZwFreeUserPhysicalPages
@ stdcall ZwFreeVirtualMemory(long ptr ptr long) NtFreeVirtualMemory
@ stdcall ZwFsControlFile(long long long long long long long long long long) NtFsControlFile
@ stdcall ZwGetContextThread(long ptr) NtGetContextThread
# @ stub ZwGetDevicePowerState
@ stub ZwGetPlugPlayEvent
@ stdcall ZwGetTickCount() NtGetTickCount
@ stdcall ZwGetWriteWatch(long long ptr long ptr ptr ptr) NtGetWriteWatch
# @ stub ZwImpersonateAnonymousToken
@ stub ZwImpersonateClientOfPort
@ stub ZwImpersonateThread
@ stub ZwInitializeRegistry
@ stdcall ZwInitiatePowerAction(long long long long) NtInitiatePowerAction
@ stdcall ZwIsProcessInJob(long long) NtIsProcessInJob
# @ stub ZwIsSystemResumeAutomatic
@ stdcall ZwListenPort(ptr ptr) NtListenPort
@ stdcall ZwLoadDriver(ptr) NtLoadDriver
# @ stub ZwLoadKey2
@ stdcall ZwLoadKey(ptr ptr) NtLoadKey
@ stdcall ZwLockFile(long long ptr ptr ptr ptr ptr ptr long long) NtLockFile
# @ stub ZwLockProductActivationKeys
# @ stub ZwLockRegistryKey
@ stdcall ZwLockVirtualMemory(long ptr ptr long) NtLockVirtualMemory
# @ stub ZwMakePermanentObject
@ stdcall ZwMakeTemporaryObject(long) NtMakeTemporaryObject
# @ stub ZwMapUserPhysicalPages
# @ stub ZwMapUserPhysicalPagesScatter
@ stdcall ZwMapViewOfSection(long long ptr long long ptr ptr long long long) NtMapViewOfSection
# @ stub ZwModifyBootEntry
@ stdcall ZwNotifyChangeDirectoryFile(long long ptr ptr ptr ptr long long long) NtNotifyChangeDirectoryFile
@ stdcall ZwNotifyChangeKey(long long ptr ptr ptr long long ptr long long) NtNotifyChangeKey
# @ stub ZwNotifyChangeMultipleKeys
@ stdcall ZwOpenDirectoryObject(long long long) NtOpenDirectoryObject
@ stdcall ZwOpenEvent(long long long) NtOpenEvent
@ stub ZwOpenEventPair
@ stdcall ZwOpenFile(ptr long ptr ptr long long) NtOpenFile
@ stdcall ZwOpenIoCompletion(ptr long ptr) NtOpenIoCompletion
@ stdcall ZwOpenJobObject(ptr long ptr) NtOpenJobObject
@ stdcall ZwOpenKey(ptr long ptr) NtOpenKey
@ stdcall ZwOpenKeyedEvent(ptr long ptr) NtOpenKeyedEvent
@ stdcall ZwOpenMutant(ptr long ptr) NtOpenMutant
@ stub ZwOpenObjectAuditAlarm
@ stdcall ZwOpenProcess(ptr long ptr ptr) NtOpenProcess
@ stdcall ZwOpenProcessToken(long long ptr) NtOpenProcessToken
@ stdcall ZwOpenProcessTokenEx(long long long ptr) NtOpenProcessTokenEx
@ stdcall ZwOpenSection(ptr long ptr) NtOpenSection
@ stdcall ZwOpenSemaphore(long long ptr) NtOpenSemaphore
@ stdcall ZwOpenSymbolicLinkObject (ptr long ptr) NtOpenSymbolicLinkObject
@ stdcall ZwOpenThread(ptr long ptr ptr) NtOpenThread
@ stdcall ZwOpenThreadToken(long long long ptr) NtOpenThreadToken
@ stdcall ZwOpenThreadTokenEx(long long long long ptr) NtOpenThreadTokenEx
@ stdcall ZwOpenTimer(ptr long ptr) NtOpenTimer
@ stub ZwPlugPlayControl
@ stdcall ZwPowerInformation(long ptr long ptr long) NtPowerInformation
@ stdcall ZwPrivilegeCheck(ptr ptr ptr) NtPrivilegeCheck
@ stub ZwPrivilegeObjectAuditAlarm
@ stub ZwPrivilegedServiceAuditAlarm
@ stdcall ZwProtectVirtualMemory(long ptr ptr long ptr) NtProtectVirtualMemory
@ stdcall ZwPulseEvent(long ptr) NtPulseEvent
@ stdcall ZwQueryAttributesFile(ptr ptr) NtQueryAttributesFile
# @ stub ZwQueryBootEntryOrder
# @ stub ZwQueryBootOptions
# @ stub ZwQueryDebugFilterState
@ stdcall ZwQueryDefaultLocale(long ptr) NtQueryDefaultLocale
@ stdcall ZwQueryDefaultUILanguage(ptr) NtQueryDefaultUILanguage
@ stdcall ZwQueryDirectoryFile(long long ptr ptr ptr ptr long long long ptr long) NtQueryDirectoryFile
@ stdcall ZwQueryDirectoryObject(long ptr long long long ptr ptr) NtQueryDirectoryObject
@ stdcall ZwQueryEaFile(long ptr ptr long long ptr long ptr long) NtQueryEaFile
@ stdcall ZwQueryEvent(long long ptr long ptr) NtQueryEvent
@ stdcall ZwQueryFullAttributesFile(ptr ptr) NtQueryFullAttributesFile
@ stdcall ZwQueryInformationAtom(long long ptr long ptr) NtQueryInformationAtom
@ stdcall ZwQueryInformationFile(long ptr ptr long long) NtQueryInformationFile
@ stdcall ZwQueryInformationJobObject(long long ptr long ptr) NtQueryInformationJobObject
@ stub ZwQueryInformationPort
@ stdcall ZwQueryInformationProcess(long long ptr long ptr) NtQueryInformationProcess
@ stdcall ZwQueryInformationThread(long long ptr long ptr) NtQueryInformationThread
@ stdcall ZwQueryInformationToken(long long ptr long ptr) NtQueryInformationToken
@ stdcall ZwQueryInstallUILanguage(ptr) NtQueryInstallUILanguage
@ stub ZwQueryIntervalProfile
@ stdcall ZwQueryIoCompletion(long long ptr long ptr) NtQueryIoCompletion
@ stdcall ZwQueryKey(long long ptr long ptr) NtQueryKey
@ stdcall ZwQueryMultipleValueKey(long ptr long ptr long ptr) NtQueryMultipleValueKey
@ stdcall ZwQueryMutant(long long ptr long ptr) NtQueryMutant
@ stdcall ZwQueryObject(long long long long long) NtQueryObject
@ stub ZwQueryOpenSubKeys
@ stdcall ZwQueryPerformanceCounter (long long) NtQueryPerformanceCounter
# @ stub ZwQueryPortInformationProcess
# @ stub ZwQueryQuotaInformationFile
@ stdcall ZwQuerySection (long long long long long) NtQuerySection
@ stdcall ZwQuerySecurityObject (long long long long long) NtQuerySecurityObject
@ stdcall ZwQuerySemaphore (long long long long long) NtQuerySemaphore
@ stdcall ZwQuerySymbolicLinkObject(long ptr ptr) NtQuerySymbolicLinkObject
@ stub ZwQuerySystemEnvironmentValue
# @ stub ZwQuerySystemEnvironmentValueEx
@ stdcall ZwQuerySystemInformation(long long long long) NtQuerySystemInformation
@ stdcall ZwQuerySystemTime(ptr) NtQuerySystemTime
@ stdcall ZwQueryTimer(ptr long ptr long ptr) NtQueryTimer
@ stdcall ZwQueryTimerResolution(long long long) NtQueryTimerResolution
@ stdcall ZwQueryValueKey(long ptr long ptr long ptr) NtQueryValueKey
@ stdcall ZwQueryVirtualMemory(long ptr long ptr long ptr) NtQueryVirtualMemory
@ stdcall ZwQueryVolumeInformationFile(long ptr ptr long long) NtQueryVolumeInformationFile
@ stdcall ZwQueueApcThread(long ptr long long long) NtQueueApcThread
@ stdcall ZwRaiseException(ptr ptr long) NtRaiseException
@ stub ZwRaiseHardError
@ stdcall ZwReadFile(long long ptr ptr ptr ptr long ptr ptr) NtReadFile
@ stdcall ZwReadFileScatter(long long ptr ptr ptr ptr long ptr ptr) NtReadFileScatter
@ stub ZwReadRequestData
@ stdcall ZwReadVirtualMemory(long ptr ptr long ptr) NtReadVirtualMemory
@ stub ZwRegisterNewDevice
@ stdcall ZwRegisterThreadTerminatePort(ptr) NtRegisterThreadTerminatePort
@ stdcall ZwReleaseKeyedEvent(long ptr long ptr) NtReleaseKeyedEvent
@ stdcall ZwReleaseMutant(long ptr) NtReleaseMutant
@ stub ZwReleaseProcessMutant
@ stdcall ZwReleaseSemaphore(long long ptr) NtReleaseSemaphore
@ stdcall ZwRemoveIoCompletion(ptr ptr ptr ptr ptr) NtRemoveIoCompletion
# @ stub ZwRemoveProcessDebug
# @ stub ZwRenameKey
@ stdcall ZwReplaceKey(ptr long ptr) NtReplaceKey
@ stub ZwReplyPort
@ stdcall ZwReplyWaitReceivePort(ptr ptr ptr ptr) NtReplyWaitReceivePort
# @ stub ZwReplyWaitReceivePortEx
@ stub ZwReplyWaitReplyPort
# @ stub ZwRequestDeviceWakeup
@ stub ZwRequestPort
@ stdcall ZwRequestWaitReplyPort(ptr ptr ptr) NtRequestWaitReplyPort
# @ stub ZwRequestWakeupLatency
@ stdcall ZwResetEvent(long ptr) NtResetEvent
@ stdcall ZwResetWriteWatch(long ptr long) NtResetWriteWatch
@ stdcall ZwRestoreKey(long long long) NtRestoreKey
# @ stub ZwResumeProcess
@ stdcall ZwResumeThread(long long) NtResumeThread
@ stdcall ZwSaveKey(long long) NtSaveKey
# @ stub ZwSaveKeyEx
# @ stub ZwSaveMergedKeys
@ stdcall ZwSecureConnectPort(ptr ptr ptr ptr ptr ptr ptr ptr ptr) NtSecureConnectPort
# @ stub ZwSetBootEntryOrder
# @ stub ZwSetBootOptions
@ stdcall ZwSetContextThread(long ptr) NtSetContextThread
# @ stub ZwSetDebugFilterState
@ stub ZwSetDefaultHardErrorPort
@ stdcall ZwSetDefaultLocale(long long) NtSetDefaultLocale
@ stdcall ZwSetDefaultUILanguage(long) NtSetDefaultUILanguage
@ stdcall ZwSetEaFile(long ptr ptr long) NtSetEaFile
@ stdcall ZwSetEvent(long long) NtSetEvent
# @ stub ZwSetEventBoostPriority
@ stub ZwSetHighEventPair
@ stub ZwSetHighWaitLowEventPair
@ stub ZwSetHighWaitLowThread
# @ stub ZwSetInformationDebugObject
@ stdcall ZwSetInformationFile(long long long long long) NtSetInformationFile
@ stdcall ZwSetInformationJobObject(long long ptr long) NtSetInformationJobObject
@ stdcall ZwSetInformationKey(long long ptr long) NtSetInformationKey
@ stdcall ZwSetInformationObject(long long ptr long) NtSetInformationObject
@ stdcall ZwSetInformationProcess(long long long long) NtSetInformationProcess
@ stdcall ZwSetInformationThread(long long ptr long) NtSetInformationThread
@ stdcall ZwSetInformationToken(long long ptr long) NtSetInformationToken
@ stdcall ZwSetIntervalProfile(long long) NtSetIntervalProfile
@ stdcall ZwSetIoCompletion(ptr long ptr long long) NtSetIoCompletion
@ stub ZwSetLdtEntries
@ stub ZwSetLowEventPair
@ stub ZwSetLowWaitHighEventPair
@ stub ZwSetLowWaitHighThread
# @ stub ZwSetQuotaInformationFile
@ stdcall ZwSetSecurityObject(long long ptr) NtSetSecurityObject
@ stub ZwSetSystemEnvironmentValue
# @ stub ZwSetSystemEnvironmentValueEx
@ stdcall ZwSetSystemInformation(long ptr long) NtSetSystemInformation
@ stub ZwSetSystemPowerState
@ stdcall ZwSetSystemTime(ptr ptr) NtSetSystemTime
# @ stub ZwSetThreadExecutionState
@ stdcall ZwSetTimer(long ptr ptr ptr long long ptr) NtSetTimer
@ stdcall ZwSetTimerResolution(long long ptr) NtSetTimerResolution
# @ stub ZwSetUuidSeed
@ stdcall ZwSetValueKey(long long long long long long) NtSetValueKey
@ stdcall ZwSetVolumeInformationFile(long ptr ptr long long) NtSetVolumeInformationFile
@ stdcall ZwShutdownSystem(long) NtShutdownSystem
@ stdcall ZwSignalAndWaitForSingleObject(long long long ptr) NtSignalAndWaitForSingleObject
@ stub ZwStartProfile
@ stub ZwStopProfile
# @ stub ZwSuspendProcess
@ stdcall ZwSuspendThread(long ptr) NtSuspendThread
@ stdcall ZwSystemDebugControl(long ptr long ptr long ptr) NtSystemDebugControl
@ stdcall ZwTerminateJobObject(long long) NtTerminateJobObject
@ stdcall ZwTerminateProcess(long long) NtTerminateProcess
@ stdcall ZwTerminateThread(long long) NtTerminateThread
@ stub ZwTestAlert
# @ stub ZwTraceEvent
# @ stub ZwTranslateFilePath
@ stdcall ZwUnloadDriver(ptr) NtUnloadDriver
@ stdcall ZwUnloadKey(long) NtUnloadKey
@ stub ZwUnloadKeyEx
@ stdcall ZwUnlockFile(long ptr ptr ptr ptr) NtUnlockFile
@ stdcall ZwUnlockVirtualMemory(long ptr ptr long) NtUnlockVirtualMemory
@ stdcall ZwUnmapViewOfSection(long ptr) NtUnmapViewOfSection
@ stub ZwVdmControl
@ stub ZwW32Call
# @ stub ZwWaitForDebugEvent
@ stdcall ZwWaitForKeyedEvent(long ptr long ptr) NtWaitForKeyedEvent
@ stdcall ZwWaitForMultipleObjects(long ptr long long ptr) NtWaitForMultipleObjects
@ stub ZwWaitForProcessMutant
@ stdcall ZwWaitForSingleObject(long long long) NtWaitForSingleObject
@ stub ZwWaitHighEventPair
@ stub ZwWaitLowEventPair
@ stdcall ZwWriteFile(long long ptr ptr ptr ptr long ptr ptr) NtWriteFile
@ stdcall ZwWriteFileGather(long long ptr ptr ptr ptr long ptr ptr) NtWriteFileGather
@ stub ZwWriteRequestData
@ stdcall ZwWriteVirtualMemory(long ptr ptr long ptr) NtWriteVirtualMemory
@ stdcall ZwYieldExecution() NtYieldExecution
@ cdecl -private -arch=i386 _CIcos() NTDLL__CIcos
@ cdecl -private -arch=i386 _CIlog() NTDLL__CIlog
@ cdecl -private -arch=i386 _CIpow() NTDLL__CIpow
@ cdecl -private -arch=i386 _CIsin() NTDLL__CIsin
@ cdecl -private -arch=i386 _CIsqrt() NTDLL__CIsqrt
@ stdcall -arch=x86_64 __C_specific_handler(ptr long ptr ptr)
@ stdcall -private -arch=x86_64 -norelay __chkstk()
@ cdecl -private __isascii(long) NTDLL___isascii
@ cdecl -private __iscsym(long) NTDLL___iscsym
@ cdecl -private __iscsymf(long) NTDLL___iscsymf
@ cdecl -private __toascii(long) NTDLL___toascii
@ stdcall -private -arch=i386 -ret64 _alldiv(int64 int64)
# @ stub _alldvrm
@ stdcall -private -arch=i386 -ret64 _allmul(int64 int64)
@ stdcall -private -arch=i386 -norelay _alloca_probe()
@ stdcall -private -arch=i386 -ret64 _allrem(int64 int64)
# @ stub _allshl
# @ stub _allshr
@ cdecl -private -ret64 _atoi64(str)
@ stdcall -private -arch=i386 -ret64 _aulldiv(int64 int64)
# @ stub _aulldvrm
@ stdcall -private -arch=i386 -ret64 _aullrem(int64 int64)
# @ stub _aullshr
@ stdcall -private -arch=i386 -norelay _chkstk()
@ stub _fltused
@ cdecl -private -arch=i386 -ret64 _ftol() NTDLL__ftol
@ cdecl -private _i64toa(int64 ptr long)
@ cdecl -private _i64tow(int64 ptr long)
@ cdecl -private _itoa(long ptr long)
@ cdecl -private _itow(long ptr long)
@ cdecl -private _lfind(ptr ptr ptr long ptr)
@ stdcall -arch=x86_64 _local_unwind(ptr ptr)
@ cdecl -private _ltoa(long ptr long)
@ cdecl -private _ltow(long ptr long)
@ cdecl -private _memccpy(ptr ptr long long)
@ cdecl -private _memicmp(str str long)
@ varargs -private _snprintf(ptr long str) NTDLL__snprintf
@ varargs -private _snwprintf(ptr long wstr) NTDLL__snwprintf
@ cdecl -private _splitpath(str ptr ptr ptr ptr)
@ cdecl -private _strcmpi(str str) _stricmp
@ cdecl -private _stricmp(str str)
@ cdecl -private _strlwr(str)
@ cdecl -private _strnicmp(str str long)
@ cdecl -private _strupr(str)
@ cdecl -private _tolower(long) NTDLL__tolower
@ cdecl -private _toupper(long) NTDLL__toupper
@ cdecl -private _ui64toa(int64 ptr long)
@ cdecl -private _ui64tow(int64 ptr long)
@ cdecl -private _ultoa(long ptr long)
@ cdecl -private _ultow(long ptr long)
@ cdecl -private _vsnprintf(ptr long str ptr) NTDLL__vsnprintf
@ cdecl -private _vsnwprintf(ptr long wstr ptr) NTDLL__vsnwprintf
@ cdecl -private _wcsicmp(wstr wstr) NTDLL__wcsicmp
@ cdecl -private _wcslwr(wstr) NTDLL__wcslwr
@ cdecl -private _wcsnicmp(wstr wstr long) NTDLL__wcsnicmp
@ cdecl -private _wcsupr(wstr) NTDLL__wcsupr
@ cdecl -private _wtoi(wstr)
@ cdecl -private -ret64 _wtoi64(wstr)
@ cdecl -private _wtol(wstr)
@ cdecl -private abs(long) NTDLL_abs
@ cdecl -private atan(double) NTDLL_atan
@ cdecl -private atoi(str) NTDLL_atoi
@ cdecl -private atol(str) NTDLL_atol
@ cdecl -private bsearch(ptr ptr long long ptr) NTDLL_bsearch
@ cdecl -private ceil(double) NTDLL_ceil
@ cdecl -private cos(double) NTDLL_cos
@ cdecl -private fabs(double) NTDLL_fabs
@ cdecl -private floor(double) NTDLL_floor
@ cdecl -private isalnum(long) NTDLL_isalnum
@ cdecl -private isalpha(long) NTDLL_isalpha
@ cdecl -private iscntrl(long) NTDLL_iscntrl
@ cdecl -private isdigit(long) NTDLL_isdigit
@ cdecl -private isgraph(long) NTDLL_isgraph
@ cdecl -private islower(long) NTDLL_islower
@ cdecl -private isprint(long) NTDLL_isprint
@ cdecl -private ispunct(long) NTDLL_ispunct
@ cdecl -private isspace(long) NTDLL_isspace
@ cdecl -private isupper(long) NTDLL_isupper
@ cdecl -private iswalpha(long) NTDLL_iswalpha
@ cdecl -private iswctype(long long) NTDLL_iswctype
@ cdecl -private iswdigit(long) NTDLL_iswdigit
@ cdecl -private iswlower(long) NTDLL_iswlower
@ cdecl -private iswspace(long) NTDLL_iswspace
@ cdecl -private iswxdigit(long) NTDLL_iswxdigit
@ cdecl -private isxdigit(long) NTDLL_isxdigit
@ cdecl -private labs(long) NTDLL_labs
@ cdecl -private log(double) NTDLL_log
@ cdecl -private mbstowcs(ptr str long) NTDLL_mbstowcs
@ cdecl -private memchr(ptr long long) NTDLL_memchr
@ cdecl -private memcmp(ptr ptr long) NTDLL_memcmp
@ cdecl -private memcpy(ptr ptr long) NTDLL_memcpy
@ cdecl -private memmove(ptr ptr long) NTDLL_memmove
@ cdecl -private memset(ptr long long) NTDLL_memset
@ cdecl -private pow(double double) NTDLL_pow
@ cdecl -private qsort(ptr long long ptr) NTDLL_qsort
@ cdecl -private sin(double) NTDLL_sin
@ varargs -private sprintf(ptr str) NTDLL_sprintf
@ cdecl -private sqrt(double) NTDLL_sqrt
@ varargs -private sscanf(str str) NTDLL_sscanf
@ cdecl -private strcat(str str) NTDLL_strcat
@ cdecl -private strchr(str long) NTDLL_strchr
@ cdecl -private strcmp(str str) NTDLL_strcmp
@ cdecl -private strcpy(ptr str) NTDLL_strcpy
@ cdecl -private strcspn(str str) NTDLL_strcspn
@ cdecl -private strlen(str) NTDLL_strlen
@ cdecl -private strncat(str str long) NTDLL_strncat
@ cdecl -private strncmp(str str long) NTDLL_strncmp
@ cdecl -private strncpy(ptr str long) NTDLL_strncpy
@ cdecl -private strpbrk(str str) NTDLL_strpbrk
@ cdecl -private strrchr(str long) NTDLL_strrchr
@ cdecl -private strspn(str str) NTDLL_strspn
@ cdecl -private strstr(str str) NTDLL_strstr
@ cdecl -private strtol(str ptr long) NTDLL_strtol
@ cdecl -private strtoul(str ptr long) NTDLL_strtoul
@ varargs -private swprintf(ptr wstr) NTDLL_swprintf
@ cdecl -private tan(double) NTDLL_tan
@ cdecl -private tolower(long) NTDLL_tolower
@ cdecl -private toupper(long) NTDLL_toupper
@ cdecl -private towlower(long) NTDLL_towlower
@ cdecl -private towupper(long) NTDLL_towupper
@ stdcall vDbgPrintEx(long long str ptr)
@ stdcall vDbgPrintExWithPrefix(str long long str ptr)
@ cdecl -private vsprintf(ptr str ptr) NTDLL_vsprintf
@ cdecl -private wcscat(wstr wstr) NTDLL_wcscat
@ cdecl -private wcschr(wstr long) NTDLL_wcschr
@ cdecl -private wcscmp(wstr wstr) NTDLL_wcscmp
@ cdecl -private wcscpy(ptr wstr) NTDLL_wcscpy
@ cdecl -private wcscspn(wstr wstr) NTDLL_wcscspn
@ cdecl -private wcslen(wstr) NTDLL_wcslen
@ cdecl -private wcsncat(wstr wstr long) NTDLL_wcsncat
@ cdecl -private wcsncmp(wstr wstr long) NTDLL_wcsncmp
@ cdecl -private wcsncpy(ptr wstr long) NTDLL_wcsncpy
@ cdecl -private wcspbrk(wstr wstr) NTDLL_wcspbrk
@ cdecl -private wcsrchr(wstr long) NTDLL_wcsrchr
@ cdecl -private wcsspn(wstr wstr) NTDLL_wcsspn
@ cdecl -private wcsstr(wstr wstr) NTDLL_wcsstr
@ cdecl -private wcstok(wstr wstr) NTDLL_wcstok
@ cdecl -private wcstol(wstr ptr long) NTDLL_wcstol
@ cdecl -private wcstombs(ptr ptr long) NTDLL_wcstombs
@ cdecl -private wcstoul(wstr ptr long) NTDLL_wcstoul

##################
# Wine extensions
#
# All functions must be prefixed with '__wine_' (for internal functions)
# or 'wine_' (for user-visible functions) to avoid namespace conflicts.

# Relays
@ cdecl -i386 __wine_enter_vm86(ptr)

# Server interface
@ cdecl -norelay wine_server_call(ptr)
@ cdecl wine_server_fd_to_handle(long long long ptr)
@ cdecl wine_server_handle_to_fd(long long ptr ptr)
@ cdecl wine_server_release_fd(long long)
@ cdecl wine_server_send_fd(long)
@ cdecl __wine_make_process_system()

# Version
@ cdecl wine_get_version() NTDLL_wine_get_version
@ cdecl wine_get_build_id() NTDLL_wine_get_build_id
@ cdecl wine_get_host_version(ptr ptr) NTDLL_wine_get_host_version

# Codepages
@ cdecl __wine_init_codepages(ptr ptr ptr)

# signal handling
@ cdecl __wine_set_signal_handler(long ptr)

# Filesystem
@ cdecl wine_nt_to_unix_file_name(ptr ptr long long)
@ cdecl wine_unix_to_nt_file_name(ptr ptr)
@ cdecl __wine_init_windows_dir(wstr wstr)
