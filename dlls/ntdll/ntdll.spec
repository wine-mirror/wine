#note that the Zw... functions are alternate names for the
#Nt... functions.  (see www.sysinternals.com for details)
#if you change a Nt.. function DON'T FORGET to change the
#Zw one too.

@ stdcall A_SHAFinal(ptr ptr)
@ stdcall A_SHAInit(ptr)
@ stdcall A_SHAUpdate(ptr ptr long)
@ stdcall ApiSetQueryApiSetPresence(ptr ptr)
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
@ stdcall DbgUiIssueRemoteBreakin(long)
@ stdcall DbgUiRemoteBreakin(ptr)
# @ stub DbgUiSetThreadDebugObject
# @ stub DbgUiStopDebugging
@ stub DbgUiWaitStateChange
@ stdcall DbgUserBreakPoint()
@ stdcall EtwEventActivityIdControl(long ptr)
@ stdcall EtwEventEnabled(int64 ptr)
@ stdcall EtwEventProviderEnabled(int64 long int64)
@ stdcall EtwEventRegister(ptr ptr ptr ptr)
@ stdcall EtwEventSetInformation(int64 long ptr long)
@ stdcall EtwEventUnregister(int64)
@ stdcall EtwEventWrite(int64 ptr long ptr)
@ stdcall EtwEventWriteTransfer(int64 ptr ptr ptr long ptr)
@ stdcall EtwGetTraceEnableFlags(int64)
@ stdcall EtwGetTraceEnableLevel(int64)
@ stdcall -ret64 EtwGetTraceLoggerHandle(ptr)
@ stdcall EtwLogTraceEvent(int64 ptr)
@ stdcall EtwRegisterTraceGuidsA(ptr ptr ptr long ptr str str ptr)
@ stdcall EtwRegisterTraceGuidsW(ptr ptr ptr long ptr wstr wstr ptr)
@ varargs EtwTraceMessage(int64 long ptr long)
@ stdcall EtwTraceMessageVa(int64 long ptr long ptr)
@ stdcall EtwUnregisterTraceGuids(int64)
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
@ stdcall LdrEnumerateLoadedModules(ptr ptr ptr)
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
@ stdcall LdrInitializeThunk(ptr long long long)
@ stub LdrLoadAlternateResourceModule
@ stdcall LdrLoadDll(wstr long ptr ptr)
@ stdcall LdrLockLoaderLock(long ptr ptr)
@ stdcall LdrProcessRelocationBlock(ptr long ptr long)
@ stdcall LdrQueryImageFileExecutionOptions(ptr wstr long ptr long ptr)
@ stdcall LdrQueryProcessModuleInformation(ptr long ptr)
@ stdcall LdrRegisterDllNotification(long ptr ptr ptr)
@ stdcall LdrResolveDelayLoadedAPI(ptr ptr ptr ptr ptr long)
@ stub LdrSetAppCompatDllRedirectionCallback
@ stub LdrSetDllManifestProber
@ stdcall LdrShutdownProcess()
@ stdcall LdrShutdownThread()
@ stub LdrUnloadAlternateResourceModule
@ stdcall LdrUnloadDll(ptr)
@ stdcall LdrUnlockLoaderLock(long long)
@ stdcall LdrUnregisterDllNotification(ptr)
@ stub LdrVerifyImageMatchesChecksum
@ stdcall MD4Final(ptr)
@ stdcall MD4Init(ptr)
@ stdcall MD4Update(ptr ptr long)
@ stdcall MD5Final(ptr)
@ stdcall MD5Init(ptr)
@ stdcall MD5Update(ptr ptr long)
@ extern NlsAnsiCodePage
@ extern NlsMbCodePageTag
@ extern NlsMbOemCodePageTag
@ stdcall NtAcceptConnectPort(ptr long ptr long ptr ptr)
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
@ stdcall NtAdjustPrivilegesToken(long long ptr long ptr ptr)
@ stdcall NtAlertResumeThread(long ptr)
@ stdcall NtAlertThread(long)
@ stdcall NtAllocateLocallyUniqueId(ptr)
# @ stub NtAllocateUserPhysicalPages
@ stdcall NtAllocateUuids(ptr ptr ptr ptr)
@ stdcall NtAllocateVirtualMemory(long ptr long ptr long long)
@ stdcall NtAreMappedFilesTheSame(ptr ptr)
@ stdcall NtAssignProcessToJobObject(long long)
@ stub NtCallbackReturn
# @ stub NtCancelDeviceWakeupRequest
@ stdcall NtCancelIoFile(long ptr)
@ stdcall NtCancelIoFileEx(long ptr ptr)
@ stdcall NtCancelTimer(long ptr)
@ stdcall NtClearEvent(long)
@ stdcall NtClearPowerRequest(long long)
@ stdcall NtClose(long)
@ stub NtCloseObjectAuditAlarm
# @ stub NtCompactKeys
# @ stub NtCompareTokens
@ stdcall NtCompleteConnectPort(ptr)
# @ stub NtCompressKey
@ stdcall NtConnectPort(ptr ptr ptr ptr ptr ptr ptr ptr)
@ stub NtContinue
# @ stub NtCreateDebugObject
@ stdcall NtCreateDirectoryObject(ptr long ptr)
@ stdcall NtCreateEvent(ptr long ptr long long)
@ stub NtCreateEventPair
@ stdcall NtCreateFile(ptr long ptr ptr ptr long long long long ptr long)
@ stdcall NtCreateIoCompletion(ptr long ptr long)
@ stdcall NtCreateJobObject(ptr long ptr)
# @ stub NtCreateJobSet
@ stdcall NtCreateKey(ptr long ptr long ptr long ptr)
@ stdcall NtCreateKeyTransacted(ptr long ptr long ptr long long ptr)
@ stdcall NtCreateKeyedEvent(ptr long ptr long)
@ stdcall NtCreateLowBoxToken(ptr long long ptr ptr long ptr long ptr)
@ stdcall NtCreateMailslotFile(ptr long ptr ptr long long long ptr)
@ stdcall NtCreateMutant(ptr long ptr long)
@ stdcall NtCreateNamedPipeFile(ptr long ptr ptr long long long long long long long long long ptr)
@ stdcall NtCreatePagingFile(ptr ptr ptr ptr)
@ stdcall NtCreatePort(ptr ptr long long ptr)
@ stdcall NtCreatePowerRequest(ptr ptr)
@ stub NtCreateProcess
# @ stub NtCreateProcessEx
@ stub NtCreateProfile
@ stdcall NtCreateSection(ptr long ptr ptr long long long)
@ stdcall NtCreateSemaphore(ptr long ptr long long)
@ stdcall NtCreateSymbolicLinkObject(ptr long ptr ptr)
@ stub NtCreateThread
@ stdcall NtCreateThreadEx(ptr long ptr long ptr ptr long long long long ptr)
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
@ stdcall NtDeviceIoControlFile(long long ptr ptr ptr long ptr long ptr long)
@ stdcall NtDisplayString(ptr)
@ stdcall NtDuplicateObject(long long long ptr long long long)
@ stdcall NtDuplicateToken(long long ptr long long ptr)
# @ stub NtEnumerateBootEntries
@ stub NtEnumerateBus
@ stdcall NtEnumerateKey(long long long ptr long ptr)
# @ stub NtEnumerateSystemEnvironmentValuesEx
@ stdcall NtEnumerateValueKey(long long long ptr long ptr)
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
@ stdcall NtFsControlFile(long long ptr ptr ptr long ptr long ptr long)
@ stdcall -norelay NtGetContextThread(long ptr)
@ stdcall NtGetCurrentProcessorNumber()
# @ stub NtGetDevicePowerState
@ stub NtGetPlugPlayEvent
@ stdcall NtGetTickCount()
@ stdcall NtGetWriteWatch(long long ptr long ptr ptr ptr)
@ stdcall NtImpersonateAnonymousToken(long)
@ stub NtImpersonateClientOfPort
@ stub NtImpersonateThread
@ stub NtInitializeRegistry
@ stdcall NtInitiatePowerAction (long long long long)
@ stdcall NtIsProcessInJob(long long)
# @ stub NtIsSystemResumeAutomatic
@ stdcall NtListenPort(ptr ptr)
@ stdcall NtLoadDriver(ptr)
@ stdcall NtLoadKey2(ptr ptr long)
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
@ stdcall NtNotifyChangeMultipleKeys(long long ptr long ptr ptr ptr long long ptr long long)
@ stdcall NtOpenDirectoryObject(ptr long ptr)
@ stdcall NtOpenEvent(ptr long ptr)
@ stub NtOpenEventPair
@ stdcall NtOpenFile(ptr long ptr ptr long long)
@ stdcall NtOpenIoCompletion(ptr long ptr)
@ stdcall NtOpenJobObject(ptr long ptr)
@ stdcall NtOpenKey(ptr long ptr)
@ stdcall NtOpenKeyEx(ptr long ptr long)
@ stdcall NtOpenKeyTransacted(ptr long ptr long)
@ stdcall NtOpenKeyTransactedEx(ptr long ptr long long)
@ stdcall NtOpenKeyedEvent(ptr long ptr)
@ stdcall NtOpenMutant(ptr long ptr)
@ stub NtOpenObjectAuditAlarm
@ stdcall NtOpenProcess(ptr long ptr ptr)
@ stdcall NtOpenProcessToken(long long ptr)
@ stdcall NtOpenProcessTokenEx(long long long ptr)
@ stdcall NtOpenSection(ptr long ptr)
@ stdcall NtOpenSemaphore(ptr long ptr)
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
@ stdcall NtQueryLicenseValue(ptr ptr ptr long ptr)
@ stdcall NtQueryMultipleValueKey(long ptr long ptr long ptr)
@ stdcall NtQueryMutant(long long ptr long ptr)
@ stdcall NtQueryObject(long long ptr long ptr)
@ stub NtQueryOpenSubKeys
@ stdcall NtQueryPerformanceCounter(ptr ptr)
# @ stub NtQueryPortInformationProcess
# @ stub NtQueryQuotaInformationFile
@ stdcall NtQuerySection(long long ptr long ptr)
@ stdcall NtQuerySecurityObject(long long ptr long ptr)
@ stdcall NtQuerySemaphore (long long ptr long ptr)
@ stdcall NtQuerySymbolicLinkObject(long ptr ptr)
@ stdcall NtQuerySystemEnvironmentValue(ptr ptr long ptr)
@ stdcall NtQuerySystemEnvironmentValueEx(ptr ptr ptr ptr ptr)
@ stdcall NtQuerySystemInformation(long ptr long ptr)
@ stdcall NtQuerySystemInformationEx(long ptr long ptr long ptr)
@ stdcall NtQuerySystemTime(ptr)
@ stdcall NtQueryTimer(ptr long ptr long ptr)
@ stdcall NtQueryTimerResolution(ptr ptr ptr)
@ stdcall NtQueryValueKey(long ptr long ptr long ptr)
@ stdcall NtQueryVirtualMemory(long ptr long ptr long ptr)
@ stdcall NtQueryVolumeInformationFile(long ptr ptr long long)
@ stdcall NtQueueApcThread(long ptr long long long)
@ stdcall NtRaiseException(ptr ptr long)
@ stdcall NtRaiseHardError(long long ptr ptr long ptr)
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
@ stdcall NtRemoveIoCompletionEx(ptr ptr long ptr ptr long)
# @ stub NtRemoveProcessDebug
@ stdcall NtRenameKey(long ptr)
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
@ stdcall NtResumeProcess(long)
@ stdcall NtResumeThread(long ptr)
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
@ stdcall NtSetEvent(long ptr)
# @ stub NtSetEventBoostPriority
@ stub NtSetHighEventPair
@ stub NtSetHighWaitLowEventPair
@ stub NtSetHighWaitLowThread
# @ stub NtSetInformationDebugObject
@ stdcall NtSetInformationFile(long ptr ptr long long)
@ stdcall NtSetInformationJobObject(long long ptr long)
@ stdcall NtSetInformationKey(long long ptr long)
@ stdcall NtSetInformationObject(long long ptr long)
@ stdcall NtSetInformationProcess(long long ptr long)
@ stdcall NtSetInformationThread(long long ptr long)
@ stdcall NtSetInformationToken(long long ptr long)
@ stdcall NtSetIntervalProfile(long long)
@ stdcall NtSetIoCompletion(ptr long long long long)
@ stdcall NtSetLdtEntries(long long long long long long)
@ stub NtSetLowEventPair
@ stub NtSetLowWaitHighEventPair
@ stub NtSetLowWaitHighThread
@ stdcall NtSetPowerRequest(long long)
# @ stub NtSetQuotaInformationFile
@ stdcall NtSetSecurityObject(long long ptr)
@ stub NtSetSystemEnvironmentValue
# @ stub NtSetSystemEnvironmentValueEx
@ stdcall NtSetSystemInformation(long ptr long)
@ stub NtSetSystemPowerState
@ stdcall NtSetSystemTime(ptr ptr)
@ stdcall NtSetThreadExecutionState(long ptr)
@ stdcall NtSetTimer(long ptr ptr ptr long long ptr)
@ stdcall NtSetTimerResolution(long long ptr)
# @ stub NtSetUuidSeed
@ stdcall NtSetValueKey(long ptr long long ptr long)
@ stdcall NtSetVolumeInformationFile(long ptr ptr long long)
@ stdcall NtShutdownSystem(long)
@ stdcall NtSignalAndWaitForSingleObject(long long long ptr)
@ stub NtStartProfile
@ stub NtStopProfile
@ stdcall NtSuspendProcess(long)
@ stdcall NtSuspendThread(long ptr)
@ stdcall NtSystemDebugControl(long ptr long ptr long ptr)
@ stdcall NtTerminateJobObject(long long)
@ stdcall NtTerminateProcess(long long)
@ stdcall NtTerminateThread(long long)
@ stub NtTestAlert
# @ stub NtTraceEvent
# @ stub NtTranslateFilePath
@ stdcall NtUnloadDriver(ptr)
@ stdcall NtUnloadKey(ptr)
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
@ stdcall NtWaitForSingleObject(long long ptr)
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
@ stdcall RtlAddAccessAllowedObjectAce(ptr long long long ptr ptr ptr)
@ stdcall RtlAddAccessDeniedAce(ptr long long ptr)
@ stdcall RtlAddAccessDeniedAceEx(ptr long long long ptr)
@ stdcall RtlAddAccessDeniedObjectAce(ptr long long long ptr ptr ptr)
@ stdcall RtlAddAce(ptr long long ptr long)
@ stub RtlAddActionToRXact
@ stdcall RtlAddAtomToAtomTable(ptr wstr ptr)
@ stub RtlAddAttributeActionToRXact
@ stdcall RtlAddAuditAccessAce(ptr long long ptr long long)
@ stdcall RtlAddAuditAccessAceEx(ptr long long long ptr long long)
@ stdcall RtlAddAuditAccessObjectAce(ptr long long long ptr ptr ptr long long)
# @ stub RtlAddCompoundAce
@ stdcall RtlAddMandatoryAce(ptr long long long long ptr)
# @ stub RtlAddRange
@ cdecl -arch=arm,arm64,x86_64 RtlAddFunctionTable(ptr long long)
@ cdecl -arch=arm,arm64,x86_64 RtlAddGrowableFunctionTable(ptr ptr long long long long)
@ stdcall RtlAddRefActivationContext(ptr)
# @ stub RtlAddRefMemoryStream
@ stdcall RtlAddVectoredContinueHandler(long ptr)
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
@ stdcall RtlAssert(ptr ptr long str)
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
@ stdcall RtlCompareUnicodeString(ptr ptr long)
@ stdcall RtlCompareUnicodeStrings(ptr long ptr long long)
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
@ stdcall -arch=x86_64 RtlCopyMemory(ptr ptr long)
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
@ stdcall RtlCreateProcessParametersEx(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr long)
@ stub RtlCreatePropertySet
@ stdcall RtlCreateQueryDebugBuffer(long long)
@ stdcall RtlCreateRegistryKey(long wstr)
@ stdcall RtlCreateSecurityDescriptor(ptr long)
# @ stub RtlCreateSystemVolumeInformationFolder
@ stub RtlCreateTagHeap
@ stdcall RtlCreateTimer(ptr ptr ptr ptr long long long)
@ stdcall RtlCreateTimerQueue(ptr)
@ stdcall RtlCreateUnicodeString(ptr wstr)
@ stdcall RtlCreateUnicodeStringFromAsciiz(ptr str)
@ stdcall RtlCreateUserProcess(ptr long ptr ptr ptr long long long long ptr)
@ stub RtlCreateUserSecurityObject
@ stdcall RtlCreateUserStack(long long long long long ptr)
@ stdcall RtlCreateUserThread(long ptr long ptr long long ptr ptr ptr ptr)
@ stub RtlCustomCPToUnicodeN
@ stub RtlCutoverTimeToSystemTime
@ stdcall RtlDeNormalizeProcessParams(ptr)
@ stdcall RtlDeactivateActivationContext(long long)
@ stub RtlDeactivateActivationContextUnsafeFast
@ stub RtlDebugPrintTimes
@ stdcall RtlDecodePointer(ptr)
@ stdcall RtlDecodeSystemPointer(ptr) RtlDecodePointer
@ stdcall RtlDecompressBuffer(long ptr long ptr long ptr)
@ stdcall RtlDecompressFragment(long ptr long ptr long long ptr ptr)
@ stub RtlDefaultNpAcl
@ stub RtlDelete
@ stdcall RtlDeleteAce(ptr long)
@ stdcall RtlDeleteAtomFromAtomTable(ptr long)
@ stdcall RtlDeleteCriticalSection(ptr)
@ cdecl -arch=arm,arm64,x86_64 RtlDeleteGrowableFunctionTable(ptr)
@ stub RtlDeleteElementGenericTable
@ stub RtlDeleteElementGenericTableAvl
@ cdecl -arch=arm,arm64,x86_64 RtlDeleteFunctionTable(ptr)
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
@ stdcall RtlDosPathNameToNtPathName_U_WithStatus(wstr ptr ptr ptr)
@ stdcall RtlDosPathNameToRelativeNtPathName_U_WithStatus(wstr ptr ptr ptr)
@ stdcall RtlDosSearchPath_U(wstr wstr wstr long ptr ptr)
# @ stub RtlDosSearchPath_Ustr
@ stdcall RtlDowncaseUnicodeChar(long)
@ stdcall RtlDowncaseUnicodeString(ptr ptr long)
@ stdcall RtlDumpResource(ptr)
@ stdcall RtlDuplicateUnicodeString(long ptr ptr)
@ stdcall RtlEmptyAtomTable(ptr long)
# @ stub RtlEnableEarlyCriticalSectionEventCreation
@ stdcall RtlEncodePointer(ptr)
@ stdcall RtlEncodeSystemPointer(ptr) RtlEncodePointer
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
@ stdcall RtlEqualSid(ptr ptr)
@ stdcall RtlEqualString(ptr ptr long)
@ stdcall RtlEqualUnicodeString(ptr ptr long)
@ stdcall RtlEraseUnicodeString(ptr)
@ stdcall RtlExitUserProcess(long)
@ stdcall RtlExitUserThread(long)
@ stdcall RtlExpandEnvironmentStrings(ptr wstr long ptr long ptr)
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
@ stdcall RtlFindLongestRunClear(ptr ptr)
@ stdcall RtlFindLongestRunSet(ptr ptr)
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
@ stdcall RtlFreeAnsiString(ptr)
@ stdcall RtlFreeHandle(ptr ptr)
@ stdcall RtlFreeHeap(long long ptr)
@ stdcall RtlFreeOemString(ptr)
# @ stub RtlFreeRangeList
@ stdcall RtlFreeSid (ptr)
@ stdcall RtlFreeThreadActivationContextStack()
@ stdcall RtlFreeUnicodeString(ptr)
@ stdcall RtlFreeUserStack(ptr)
@ stdcall RtlGUIDFromString(ptr ptr)
@ stub RtlGenerate8dot3Name
@ stdcall RtlGetAce(ptr long ptr)
@ stdcall RtlGetActiveActivationContext(ptr)
@ stub RtlGetCallersAddress
@ stdcall RtlGetCompressionWorkSpaceSize(long ptr ptr)
@ stdcall RtlGetControlSecurityDescriptor(ptr ptr ptr)
@ stdcall RtlGetCurrentDirectory_U(long ptr)
@ stdcall RtlGetCurrentPeb()
@ stdcall RtlGetCurrentProcessorNumberEx(ptr)
@ stdcall RtlGetCurrentTransaction()
@ stdcall RtlGetDaclSecurityDescriptor(ptr ptr ptr ptr)
@ stub RtlGetElementGenericTable
# @ stub RtlGetElementGenericTableAvl
# @ stub RtlGetFirstRange
@ stdcall RtlGetFrame()
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
@ stdcall -ret64 RtlGetSystemTimePrecise()
@ stdcall RtlGetThreadErrorMode()
@ stdcall RtlGetUnloadEventTrace()
@ stdcall RtlGetUnloadEventTraceEx(ptr ptr ptr)
@ stub RtlGetUserInfoHeap
@ stdcall RtlGetVersion(ptr)
@ stdcall -arch=arm,arm64,x86_64 RtlGrowFunctionTable(ptr long)
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
@ stdcall RtlInitializeBitMap(ptr ptr long)
@ stdcall RtlInitializeConditionVariable(ptr)
@ stub RtlInitializeContext
@ stdcall RtlInitializeCriticalSection(ptr)
@ stdcall RtlInitializeCriticalSectionAndSpinCount(ptr long)
@ stdcall RtlInitializeCriticalSectionEx(ptr long long)
@ stdcall RtlInitializeGenericTable(ptr ptr ptr ptr ptr)
@ stdcall RtlInitializeGenericTableAvl(ptr ptr ptr ptr ptr)
@ stdcall RtlInitializeHandleTable(long long ptr)
@ stub RtlInitializeRXact
# @ stub RtlInitializeRangeList
@ stdcall RtlInitializeResource(ptr)
@ stdcall RtlInitializeSListHead(ptr)
@ stdcall RtlInitializeSRWLock(ptr)
@ stdcall RtlInitializeSid(ptr ptr long)
# @ stub RtlInitializeStackTraceDataBase
@ stub RtlInsertElementGenericTable
@ stdcall RtlInsertElementGenericTableAvl(ptr ptr long ptr)
@ cdecl -arch=arm,arm64,x86_64 RtlInstallFunctionTableCallback(long long long ptr ptr wstr)
@ stdcall RtlInt64ToUnicodeString(int64 long ptr)
@ stdcall RtlIntegerToChar(long long long ptr)
@ stdcall RtlIntegerToUnicodeString(long long ptr)
@ stdcall -arch=win32 -ret64 RtlInterlockedCompareExchange64(ptr int64 int64)
@ stdcall RtlInterlockedFlushSList(ptr)
@ stdcall RtlInterlockedPopEntrySList(ptr)
@ stdcall RtlInterlockedPushEntrySList(ptr ptr)
@ stdcall -fastcall RtlInterlockedPushListSList(ptr ptr ptr long)
@ stdcall RtlInterlockedPushListSListEx(ptr ptr ptr long)
# @ stub RtlInvertRangeList
@ stdcall RtlIpv4AddressToStringA(ptr ptr)
@ stdcall RtlIpv4AddressToStringExA(ptr long ptr ptr)
@ stdcall RtlIpv4AddressToStringExW(ptr long ptr ptr)
@ stdcall RtlIpv4AddressToStringW(ptr ptr)
# @ stub RtlIpv4StringToAddressA
# @ stub RtlIpv4StringToAddressExA
@ stdcall RtlIpv4StringToAddressExW(wstr long ptr ptr)
@ stdcall RtlIpv4StringToAddressW(wstr long ptr ptr)
# @ stub RtlIpv6AddressToStringA
# @ stub RtlIpv6AddressToStringExA
# @ stub RtlIpv6AddressToStringExW
# @ stub RtlIpv6AddressToStringW
# @ stub RtlIpv6StringToAddressA
# @ stub RtlIpv6StringToAddressExA
@ stdcall RtlIpv6StringToAddressExW(wstr ptr ptr ptr)
# @ stub RtlIpv6StringToAddressW
@ stdcall RtlIsActivationContextActive(ptr)
@ stdcall RtlIsCriticalSectionLocked(ptr)
@ stdcall RtlIsCriticalSectionLockedByThread(ptr)
@ stdcall RtlIsDosDeviceName_U(wstr)
@ stub RtlIsGenericTableEmpty
# @ stub RtlIsGenericTableEmptyAvl
@ stdcall RtlIsNameLegalDOS8Dot3(ptr ptr ptr)
@ stdcall RtlIsProcessorFeaturePresent(long)
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
@ stdcall RtlMapGenericMask(ptr ptr)
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
@ stdcall RtlPopFrame(ptr)
@ stdcall RtlPrefixString(ptr ptr long)
@ stdcall RtlPrefixUnicodeString(ptr ptr long)
@ stub RtlPropertySetNameToGuid
@ stub RtlProtectHeap
@ stdcall RtlPushFrame(ptr)
@ stdcall RtlQueryActivationContextApplicationSettings(long ptr wstr wstr ptr long ptr)
@ stdcall RtlQueryAtomInAtomTable(ptr long ptr ptr ptr ptr)
@ stdcall RtlQueryDepthSList(ptr)
@ stdcall RtlQueryDynamicTimeZoneInformation(ptr)
@ stdcall RtlQueryEnvironmentVariable_U(ptr ptr ptr)
@ stdcall RtlQueryHeapInformation(long long ptr long ptr)
@ stdcall RtlQueryInformationAcl(ptr ptr long long)
@ stdcall RtlQueryInformationActivationContext(long long ptr long ptr long ptr)
@ stub RtlQueryInformationActiveActivationContext
@ stub RtlQueryInterfaceMemoryStream
@ stdcall RtlQueryPackageIdentity(long ptr ptr ptr ptr ptr)
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
@ stdcall -norelay RtlRaiseException(ptr)
@ stdcall RtlRaiseStatus(long)
@ stdcall RtlRandom(ptr)
@ stdcall RtlRandomEx(ptr)
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
@ stdcall RtlReleaseRelativeName(ptr)
@ stdcall RtlReleaseResource(ptr)
@ stdcall RtlReleaseSRWLockExclusive(ptr)
@ stdcall RtlReleaseSRWLockShared(ptr)
@ stub RtlRemoteCall
@ stdcall RtlRemoveVectoredContinueHandler(ptr)
@ stdcall RtlRemoveVectoredExceptionHandler(ptr)
@ stub RtlResetRtlTranslations
@ stdcall -arch=x86_64 RtlRestoreContext(ptr ptr)
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
@ stdcall RtlSetHeapInformation(long long ptr long)
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
@ stdcall RtlSetUnhandledExceptionFilter(ptr)
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
@ stdcall RtlTimeToElapsedTimeFields(ptr ptr)
@ stdcall RtlTimeToSecondsSince1970(ptr ptr)
@ stdcall RtlTimeToSecondsSince1980(ptr ptr)
@ stdcall RtlTimeToTimeFields (ptr ptr)
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
@ stdcall -norelay RtlUnwind(ptr ptr ptr ptr)
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
@ stdcall RtlValidRelativeSecurityDescriptor(ptr long long)
@ stdcall RtlValidSecurityDescriptor(ptr)
@ stdcall RtlValidSid(ptr)
@ stdcall RtlValidateHeap(long long ptr)
@ stub RtlValidateProcessHeaps
# @ stub RtlValidateUnicodeString
@ stdcall RtlVerifyVersionInfo(ptr long int64)
@ stdcall -arch=x86_64 RtlVirtualUnwind(long long long ptr ptr ptr ptr ptr)
@ stdcall RtlWaitOnAddress(ptr ptr long ptr)
@ stdcall RtlWakeAddressAll(ptr)
@ stdcall RtlWakeAddressSingle(ptr)
@ stdcall RtlWakeAllConditionVariable(ptr)
@ stdcall RtlWakeConditionVariable(ptr)
@ stub RtlWalkFrameChain
@ stdcall RtlWalkHeap(long ptr)
@ stdcall RtlWow64EnableFsRedirection(long)
@ stdcall RtlWow64EnableFsRedirectionEx(long ptr)
@ stdcall -arch=x86_64 RtlWow64GetThreadContext(long ptr)
@ stdcall -arch=x86_64 RtlWow64SetThreadContext(long ptr)
@ stub RtlWriteMemoryStream
@ stdcall RtlWriteRegistryValue(long ptr ptr long ptr long)
@ stub RtlZeroHeap
@ stdcall RtlZeroMemory(ptr long)
@ stdcall RtlZombifyActivationContext(ptr)
# @ stub RtlpApplyLengthFunction
# @ stub RtlpEnsureBufferSize
# @ stub RtlpNotOwnerCriticalSection
@ stdcall RtlpNtCreateKey(ptr long ptr long ptr long ptr)
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
@ stdcall TpAllocCleanupGroup(ptr)
@ stdcall TpAllocPool(ptr ptr)
@ stdcall TpAllocTimer(ptr ptr ptr ptr)
@ stdcall TpAllocWait(ptr ptr ptr ptr)
@ stdcall TpAllocWork(ptr ptr ptr ptr)
@ stdcall TpCallbackLeaveCriticalSectionOnCompletion(ptr ptr)
@ stdcall TpCallbackMayRunLong(ptr)
@ stdcall TpCallbackReleaseMutexOnCompletion(ptr long)
@ stdcall TpCallbackReleaseSemaphoreOnCompletion(ptr long long)
@ stdcall TpCallbackSetEventOnCompletion(ptr long)
@ stdcall TpCallbackUnloadDllOnCompletion(ptr ptr)
@ stdcall TpDisassociateCallback(ptr)
@ stdcall TpIsTimerSet(ptr)
@ stdcall TpPostWork(ptr)
@ stdcall TpReleaseCleanupGroup(ptr)
@ stdcall TpReleaseCleanupGroupMembers(ptr long ptr)
@ stdcall TpReleasePool(ptr)
@ stdcall TpReleaseTimer(ptr)
@ stdcall TpReleaseWait(ptr)
@ stdcall TpReleaseWork(ptr)
@ stdcall TpSetPoolMaxThreads(ptr long)
@ stdcall TpSetPoolMinThreads(ptr long)
@ stdcall TpSetTimer(ptr ptr long long)
@ stdcall TpSetWait(ptr long ptr)
@ stdcall TpSimpleTryPost(ptr ptr ptr)
@ stdcall TpWaitForTimer(ptr long)
@ stdcall TpWaitForWait(ptr long)
@ stdcall TpWaitForWork(ptr long)
@ stdcall -ret64 VerSetConditionMask(int64 long long)
@ stdcall WinSqmEndSession(long)
@ stdcall WinSqmIncrementDWORD(long long long)
@ stdcall WinSqmIsOptedIn()
@ stdcall WinSqmSetDWORD(ptr long long)
@ stdcall WinSqmStartSession(ptr long long)
@ stdcall -private ZwAcceptConnectPort(ptr long ptr long ptr ptr) NtAcceptConnectPort
@ stdcall -private ZwAccessCheck(ptr long long ptr ptr ptr ptr ptr) NtAccessCheck
@ stdcall -private ZwAccessCheckAndAuditAlarm(ptr long ptr ptr ptr long ptr long ptr ptr ptr) NtAccessCheckAndAuditAlarm
# @ stub ZwAccessCheckByType
# @ stub ZwAccessCheckByTypeAndAuditAlarm
# @ stub ZwAccessCheckByTypeResultList
# @ stub ZwAccessCheckByTypeResultListAndAuditAlarm
# @ stub ZwAccessCheckByTypeResultListAndAuditAlarmByHandle
@ stdcall -private ZwAddAtom(ptr long ptr) NtAddAtom
# @ stub ZwAddBootEntry
@ stdcall -private ZwAdjustGroupsToken(long long ptr long ptr ptr) NtAdjustGroupsToken
@ stdcall -private ZwAdjustPrivilegesToken(long long ptr long ptr ptr) NtAdjustPrivilegesToken
@ stdcall -private ZwAlertResumeThread(long ptr) NtAlertResumeThread
@ stdcall -private ZwAlertThread(long) NtAlertThread
@ stdcall -private ZwAllocateLocallyUniqueId(ptr) NtAllocateLocallyUniqueId
# @ stub ZwAllocateUserPhysicalPages
@ stdcall -private ZwAllocateUuids(ptr ptr ptr ptr) NtAllocateUuids
@ stdcall -private ZwAllocateVirtualMemory(long ptr long ptr long long) NtAllocateVirtualMemory
@ stdcall -private ZwAreMappedFilesTheSame(ptr ptr) NtAreMappedFilesTheSame
@ stdcall -private ZwAssignProcessToJobObject(long long) NtAssignProcessToJobObject
@ stub ZwCallbackReturn
# @ stub ZwCancelDeviceWakeupRequest
@ stdcall -private ZwCancelIoFile(long ptr) NtCancelIoFile
@ stdcall -private ZwCancelIoFileEx(long ptr ptr) NtCancelIoFileEx
@ stdcall -private ZwCancelTimer(long ptr) NtCancelTimer
@ stdcall -private ZwClearEvent(long) NtClearEvent
@ stdcall -private ZwClearPowerRequest(long long) NtClearPowerRequest
@ stdcall -private ZwClose(long) NtClose
@ stub ZwCloseObjectAuditAlarm
# @ stub ZwCompactKeys
# @ stub ZwCompareTokens
@ stdcall -private ZwCompleteConnectPort(ptr) NtCompleteConnectPort
# @ stub ZwCompressKey
@ stdcall -private ZwConnectPort(ptr ptr ptr ptr ptr ptr ptr ptr) NtConnectPort
@ stub ZwContinue
# @ stub ZwCreateDebugObject
@ stdcall -private ZwCreateDirectoryObject(ptr long ptr) NtCreateDirectoryObject
@ stdcall -private ZwCreateEvent(ptr long ptr long long) NtCreateEvent
@ stub ZwCreateEventPair
@ stdcall -private ZwCreateFile(ptr long ptr ptr ptr long long long long ptr long) NtCreateFile
@ stdcall -private ZwCreateIoCompletion(ptr long ptr long) NtCreateIoCompletion
@ stdcall -private ZwCreateJobObject(ptr long ptr) NtCreateJobObject
# @ stub ZwCreateJobSet
@ stdcall -private ZwCreateKey(ptr long ptr long ptr long ptr) NtCreateKey
@ stdcall -private ZwCreateKeyTransacted(ptr long ptr long ptr long long ptr) NtCreateKeyTransacted
@ stdcall -private ZwCreateKeyedEvent(ptr long ptr long) NtCreateKeyedEvent
@ stdcall -private ZwCreateMailslotFile(ptr long ptr ptr long long long ptr) NtCreateMailslotFile
@ stdcall -private ZwCreateMutant(ptr long ptr long) NtCreateMutant
@ stdcall -private ZwCreateNamedPipeFile(ptr long ptr ptr long long long long long long long long long ptr) NtCreateNamedPipeFile
@ stdcall -private ZwCreatePagingFile(ptr ptr ptr ptr) NtCreatePagingFile
@ stdcall -private ZwCreatePort(ptr ptr long long ptr) NtCreatePort
@ stdcall -private ZwCreatePowerRequest(ptr ptr) NtCreatePowerRequest
@ stub ZwCreateProcess
# @ stub ZwCreateProcessEx
@ stub ZwCreateProfile
@ stdcall -private ZwCreateSection(ptr long ptr ptr long long long) NtCreateSection
@ stdcall -private ZwCreateSemaphore(ptr long ptr long long) NtCreateSemaphore
@ stdcall -private ZwCreateSymbolicLinkObject(ptr long ptr ptr) NtCreateSymbolicLinkObject
@ stub ZwCreateThread
@ stdcall -private ZwCreateTimer(ptr long ptr long) NtCreateTimer
@ stub ZwCreateToken
# @ stub ZwCreateWaitablePort
# @ stub ZwDebugActiveProcess
# @ stub ZwDebugContinue
@ stdcall -private ZwDelayExecution(long ptr) NtDelayExecution
@ stdcall -private ZwDeleteAtom(long) NtDeleteAtom
# @ stub ZwDeleteBootEntry
@ stdcall -private ZwDeleteFile(ptr) NtDeleteFile
@ stdcall -private ZwDeleteKey(long) NtDeleteKey
# @ stub ZwDeleteObjectAuditAlarm
@ stdcall -private ZwDeleteValueKey(long ptr) NtDeleteValueKey
@ stdcall -private ZwDeviceIoControlFile(long long ptr ptr ptr long ptr long ptr long) NtDeviceIoControlFile
@ stdcall -private ZwDisplayString(ptr) NtDisplayString
@ stdcall -private ZwDuplicateObject(long long long ptr long long long) NtDuplicateObject
@ stdcall -private ZwDuplicateToken(long long ptr long long ptr) NtDuplicateToken
# @ stub ZwEnumerateBootEntries
@ stub ZwEnumerateBus
@ stdcall -private ZwEnumerateKey(long long long ptr long ptr) NtEnumerateKey
# @ stub ZwEnumerateSystemEnvironmentValuesEx
@ stdcall -private ZwEnumerateValueKey(long long long ptr long ptr) NtEnumerateValueKey
@ stub ZwExtendSection
# @ stub ZwFilterToken
@ stdcall -private ZwFindAtom(ptr long ptr) NtFindAtom
@ stdcall -private ZwFlushBuffersFile(long ptr) NtFlushBuffersFile
@ stdcall -private ZwFlushInstructionCache(long ptr long) NtFlushInstructionCache
@ stdcall -private ZwFlushKey(long) NtFlushKey
@ stdcall -private ZwFlushVirtualMemory(long ptr ptr long) NtFlushVirtualMemory
@ stub ZwFlushWriteBuffer
# @ stub ZwFreeUserPhysicalPages
@ stdcall -private ZwFreeVirtualMemory(long ptr ptr long) NtFreeVirtualMemory
@ stdcall -private ZwFsControlFile(long long ptr ptr ptr long ptr long ptr long) NtFsControlFile
@ stdcall -private -norelay ZwGetContextThread(long ptr) NtGetContextThread
@ stdcall -private ZwGetCurrentProcessorNumber() NtGetCurrentProcessorNumber
# @ stub ZwGetDevicePowerState
@ stub ZwGetPlugPlayEvent
@ stdcall -private ZwGetTickCount() NtGetTickCount
@ stdcall -private ZwGetWriteWatch(long long ptr long ptr ptr ptr) NtGetWriteWatch
@ stdcall -private ZwImpersonateAnonymousToken(long) NtImpersonateAnonymousToken
@ stub ZwImpersonateClientOfPort
@ stub ZwImpersonateThread
@ stub ZwInitializeRegistry
@ stdcall -private ZwInitiatePowerAction(long long long long) NtInitiatePowerAction
@ stdcall -private ZwIsProcessInJob(long long) NtIsProcessInJob
# @ stub ZwIsSystemResumeAutomatic
@ stdcall -private ZwListenPort(ptr ptr) NtListenPort
@ stdcall -private ZwLoadDriver(ptr) NtLoadDriver
@ stdcall -private ZwLoadKey2(ptr ptr long) NtLoadKey2
@ stdcall -private ZwLoadKey(ptr ptr) NtLoadKey
@ stdcall -private ZwLockFile(long long ptr ptr ptr ptr ptr ptr long long) NtLockFile
# @ stub ZwLockProductActivationKeys
# @ stub ZwLockRegistryKey
@ stdcall -private ZwLockVirtualMemory(long ptr ptr long) NtLockVirtualMemory
# @ stub ZwMakePermanentObject
@ stdcall -private ZwMakeTemporaryObject(long) NtMakeTemporaryObject
# @ stub ZwMapUserPhysicalPages
# @ stub ZwMapUserPhysicalPagesScatter
@ stdcall -private ZwMapViewOfSection(long long ptr long long ptr ptr long long long) NtMapViewOfSection
# @ stub ZwModifyBootEntry
@ stdcall -private ZwNotifyChangeDirectoryFile(long long ptr ptr ptr ptr long long long) NtNotifyChangeDirectoryFile
@ stdcall -private ZwNotifyChangeKey(long long ptr ptr ptr long long ptr long long) NtNotifyChangeKey
@ stdcall -private ZwNotifyChangeMultipleKeys(long long ptr long ptr ptr ptr long long ptr long long) NtNotifyChangeMultipleKeys
@ stdcall -private ZwOpenDirectoryObject(ptr long ptr) NtOpenDirectoryObject
@ stdcall -private ZwOpenEvent(ptr long ptr) NtOpenEvent
@ stub ZwOpenEventPair
@ stdcall -private ZwOpenFile(ptr long ptr ptr long long) NtOpenFile
@ stdcall -private ZwOpenIoCompletion(ptr long ptr) NtOpenIoCompletion
@ stdcall -private ZwOpenJobObject(ptr long ptr) NtOpenJobObject
@ stdcall -private ZwOpenKey(ptr long ptr) NtOpenKey
@ stdcall -private ZwOpenKeyEx(ptr long ptr long) NtOpenKeyEx
@ stdcall -private ZwOpenKeyTransacted(ptr long ptr long) NtOpenKeyTransacted
@ stdcall -private ZwOpenKeyTransactedEx(ptr long ptr long long) NtOpenKeyTransactedEx
@ stdcall -private ZwOpenKeyedEvent(ptr long ptr) NtOpenKeyedEvent
@ stdcall -private ZwOpenMutant(ptr long ptr) NtOpenMutant
@ stub ZwOpenObjectAuditAlarm
@ stdcall -private ZwOpenProcess(ptr long ptr ptr) NtOpenProcess
@ stdcall -private ZwOpenProcessToken(long long ptr) NtOpenProcessToken
@ stdcall -private ZwOpenProcessTokenEx(long long long ptr) NtOpenProcessTokenEx
@ stdcall -private ZwOpenSection(ptr long ptr) NtOpenSection
@ stdcall -private ZwOpenSemaphore(ptr long ptr) NtOpenSemaphore
@ stdcall -private ZwOpenSymbolicLinkObject (ptr long ptr) NtOpenSymbolicLinkObject
@ stdcall -private ZwOpenThread(ptr long ptr ptr) NtOpenThread
@ stdcall -private ZwOpenThreadToken(long long long ptr) NtOpenThreadToken
@ stdcall -private ZwOpenThreadTokenEx(long long long long ptr) NtOpenThreadTokenEx
@ stdcall -private ZwOpenTimer(ptr long ptr) NtOpenTimer
@ stub ZwPlugPlayControl
@ stdcall -private ZwPowerInformation(long ptr long ptr long) NtPowerInformation
@ stdcall -private ZwPrivilegeCheck(ptr ptr ptr) NtPrivilegeCheck
@ stub ZwPrivilegeObjectAuditAlarm
@ stub ZwPrivilegedServiceAuditAlarm
@ stdcall -private ZwProtectVirtualMemory(long ptr ptr long ptr) NtProtectVirtualMemory
@ stdcall -private ZwPulseEvent(long ptr) NtPulseEvent
@ stdcall -private ZwQueryAttributesFile(ptr ptr) NtQueryAttributesFile
# @ stub ZwQueryBootEntryOrder
# @ stub ZwQueryBootOptions
# @ stub ZwQueryDebugFilterState
@ stdcall -private ZwQueryDefaultLocale(long ptr) NtQueryDefaultLocale
@ stdcall -private ZwQueryDefaultUILanguage(ptr) NtQueryDefaultUILanguage
@ stdcall -private ZwQueryDirectoryFile(long long ptr ptr ptr ptr long long long ptr long) NtQueryDirectoryFile
@ stdcall -private ZwQueryDirectoryObject(long ptr long long long ptr ptr) NtQueryDirectoryObject
@ stdcall -private ZwQueryEaFile(long ptr ptr long long ptr long ptr long) NtQueryEaFile
@ stdcall -private ZwQueryEvent(long long ptr long ptr) NtQueryEvent
@ stdcall -private ZwQueryFullAttributesFile(ptr ptr) NtQueryFullAttributesFile
@ stdcall -private ZwQueryInformationAtom(long long ptr long ptr) NtQueryInformationAtom
@ stdcall -private ZwQueryInformationFile(long ptr ptr long long) NtQueryInformationFile
@ stdcall -private ZwQueryInformationJobObject(long long ptr long ptr) NtQueryInformationJobObject
@ stub ZwQueryInformationPort
@ stdcall -private ZwQueryInformationProcess(long long ptr long ptr) NtQueryInformationProcess
@ stdcall -private ZwQueryInformationThread(long long ptr long ptr) NtQueryInformationThread
@ stdcall -private ZwQueryInformationToken(long long ptr long ptr) NtQueryInformationToken
@ stdcall -private ZwQueryInstallUILanguage(ptr) NtQueryInstallUILanguage
@ stub ZwQueryIntervalProfile
@ stdcall -private ZwQueryIoCompletion(long long ptr long ptr) NtQueryIoCompletion
@ stdcall -private ZwQueryKey(long long ptr long ptr) NtQueryKey
@ stdcall -private ZwQueryLicenseValue(ptr ptr ptr long ptr) NtQueryLicenseValue
@ stdcall -private ZwQueryMultipleValueKey(long ptr long ptr long ptr) NtQueryMultipleValueKey
@ stdcall -private ZwQueryMutant(long long ptr long ptr) NtQueryMutant
@ stdcall -private ZwQueryObject(long long ptr long ptr) NtQueryObject
@ stub ZwQueryOpenSubKeys
@ stdcall -private ZwQueryPerformanceCounter(ptr ptr) NtQueryPerformanceCounter
# @ stub ZwQueryPortInformationProcess
# @ stub ZwQueryQuotaInformationFile
@ stdcall -private ZwQuerySection(long long ptr long ptr) NtQuerySection
@ stdcall -private ZwQuerySecurityObject(long long ptr long ptr) NtQuerySecurityObject
@ stdcall -private ZwQuerySemaphore(long long ptr long ptr) NtQuerySemaphore
@ stdcall -private ZwQuerySymbolicLinkObject(long ptr ptr) NtQuerySymbolicLinkObject
@ stdcall -private ZwQuerySystemEnvironmentValue(ptr ptr long ptr) NtQuerySystemEnvironmentValue
@ stdcall -private ZwQuerySystemEnvironmentValueEx(ptr ptr ptr ptr ptr) NtQuerySystemEnvironmentValueEx
@ stdcall -private ZwQuerySystemInformation(long ptr long ptr) NtQuerySystemInformation
@ stdcall -private ZwQuerySystemInformationEx(long ptr long ptr long ptr) NtQuerySystemInformationEx
@ stdcall -private ZwQuerySystemTime(ptr) NtQuerySystemTime
@ stdcall -private ZwQueryTimer(ptr long ptr long ptr) NtQueryTimer
@ stdcall -private ZwQueryTimerResolution(ptr ptr ptr) NtQueryTimerResolution
@ stdcall -private ZwQueryValueKey(long ptr long ptr long ptr) NtQueryValueKey
@ stdcall -private ZwQueryVirtualMemory(long ptr long ptr long ptr) NtQueryVirtualMemory
@ stdcall -private ZwQueryVolumeInformationFile(long ptr ptr long long) NtQueryVolumeInformationFile
@ stdcall -private ZwQueueApcThread(long ptr long long long) NtQueueApcThread
@ stdcall -private ZwRaiseException(ptr ptr long) NtRaiseException
@ stdcall -private ZwRaiseHardError(long long ptr ptr long ptr) NtRaiseHardError
@ stdcall -private ZwReadFile(long long ptr ptr ptr ptr long ptr ptr) NtReadFile
@ stdcall -private ZwReadFileScatter(long long ptr ptr ptr ptr long ptr ptr) NtReadFileScatter
@ stub ZwReadRequestData
@ stdcall -private ZwReadVirtualMemory(long ptr ptr long ptr) NtReadVirtualMemory
@ stub ZwRegisterNewDevice
@ stdcall -private ZwRegisterThreadTerminatePort(ptr) NtRegisterThreadTerminatePort
@ stdcall -private ZwReleaseKeyedEvent(long ptr long ptr) NtReleaseKeyedEvent
@ stdcall -private ZwReleaseMutant(long ptr) NtReleaseMutant
@ stub ZwReleaseProcessMutant
@ stdcall -private ZwReleaseSemaphore(long long ptr) NtReleaseSemaphore
@ stdcall -private ZwRemoveIoCompletion(ptr ptr ptr ptr ptr) NtRemoveIoCompletion
@ stdcall -private ZwRemoveIoCompletionEx(ptr ptr long ptr ptr long) NtRemoveIoCompletionEx
# @ stub ZwRemoveProcessDebug
@ stdcall -private ZwRenameKey(long ptr) NtRenameKey
@ stdcall -private ZwReplaceKey(ptr long ptr) NtReplaceKey
@ stub ZwReplyPort
@ stdcall -private ZwReplyWaitReceivePort(ptr ptr ptr ptr) NtReplyWaitReceivePort
@ stub ZwReplyWaitReceivePortEx
@ stub ZwReplyWaitReplyPort
# @ stub ZwRequestDeviceWakeup
@ stub ZwRequestPort
@ stdcall -private ZwRequestWaitReplyPort(ptr ptr ptr) NtRequestWaitReplyPort
# @ stub ZwRequestWakeupLatency
@ stdcall -private ZwResetEvent(long ptr) NtResetEvent
@ stdcall -private ZwResetWriteWatch(long ptr long) NtResetWriteWatch
@ stdcall -private ZwRestoreKey(long long long) NtRestoreKey
@ stdcall -private ZwResumeProcess(long) NtResumeProcess
@ stdcall -private ZwResumeThread(long ptr) NtResumeThread
@ stdcall -private ZwSaveKey(long long) NtSaveKey
# @ stub ZwSaveKeyEx
# @ stub ZwSaveMergedKeys
@ stdcall -private ZwSecureConnectPort(ptr ptr ptr ptr ptr ptr ptr ptr ptr) NtSecureConnectPort
# @ stub ZwSetBootEntryOrder
# @ stub ZwSetBootOptions
@ stdcall -private ZwSetContextThread(long ptr) NtSetContextThread
@ stub ZwSetDebugFilterState
@ stub ZwSetDefaultHardErrorPort
@ stdcall -private ZwSetDefaultLocale(long long) NtSetDefaultLocale
@ stdcall -private ZwSetDefaultUILanguage(long) NtSetDefaultUILanguage
@ stdcall -private ZwSetEaFile(long ptr ptr long) NtSetEaFile
@ stdcall -private ZwSetEvent(long ptr) NtSetEvent
# @ stub ZwSetEventBoostPriority
@ stub ZwSetHighEventPair
@ stub ZwSetHighWaitLowEventPair
@ stub ZwSetHighWaitLowThread
# @ stub ZwSetInformationDebugObject
@ stdcall -private ZwSetInformationFile(long ptr ptr long long) NtSetInformationFile
@ stdcall -private ZwSetInformationJobObject(long long ptr long) NtSetInformationJobObject
@ stdcall -private ZwSetInformationKey(long long ptr long) NtSetInformationKey
@ stdcall -private ZwSetInformationObject(long long ptr long) NtSetInformationObject
@ stdcall -private ZwSetInformationProcess(long long ptr long) NtSetInformationProcess
@ stdcall -private ZwSetInformationThread(long long ptr long) NtSetInformationThread
@ stdcall -private ZwSetInformationToken(long long ptr long) NtSetInformationToken
@ stdcall -private ZwSetIntervalProfile(long long) NtSetIntervalProfile
@ stdcall -private ZwSetIoCompletion(ptr long long long long) NtSetIoCompletion
@ stdcall -private ZwSetLdtEntries(long long long long long long) NtSetLdtEntries
@ stub ZwSetLowEventPair
@ stub ZwSetLowWaitHighEventPair
@ stub ZwSetLowWaitHighThread
@ stdcall -private ZwSetPowerRequest(long long) NtSetPowerRequest
# @ stub ZwSetQuotaInformationFile
@ stdcall -private ZwSetSecurityObject(long long ptr) NtSetSecurityObject
@ stub ZwSetSystemEnvironmentValue
# @ stub ZwSetSystemEnvironmentValueEx
@ stdcall -private ZwSetSystemInformation(long ptr long) NtSetSystemInformation
@ stub ZwSetSystemPowerState
@ stdcall -private ZwSetSystemTime(ptr ptr) NtSetSystemTime
@ stdcall ZwSetThreadExecutionState(long ptr) NtSetThreadExecutionState
@ stdcall -private ZwSetTimer(long ptr ptr ptr long long ptr) NtSetTimer
@ stdcall -private ZwSetTimerResolution(long long ptr) NtSetTimerResolution
# @ stub ZwSetUuidSeed
@ stdcall -private ZwSetValueKey(long ptr long long ptr long) NtSetValueKey
@ stdcall -private ZwSetVolumeInformationFile(long ptr ptr long long) NtSetVolumeInformationFile
@ stdcall -private ZwShutdownSystem(long) NtShutdownSystem
@ stdcall -private ZwSignalAndWaitForSingleObject(long long long ptr) NtSignalAndWaitForSingleObject
@ stub ZwStartProfile
@ stub ZwStopProfile
@ stdcall -private ZwSuspendProcess(long) NtSuspendProcess
@ stdcall -private ZwSuspendThread(long ptr) NtSuspendThread
@ stdcall -private ZwSystemDebugControl(long ptr long ptr long ptr) NtSystemDebugControl
@ stdcall -private ZwTerminateJobObject(long long) NtTerminateJobObject
@ stdcall -private ZwTerminateProcess(long long) NtTerminateProcess
@ stdcall -private ZwTerminateThread(long long) NtTerminateThread
@ stub ZwTestAlert
# @ stub ZwTraceEvent
# @ stub ZwTranslateFilePath
@ stdcall -private ZwUnloadDriver(ptr) NtUnloadDriver
@ stdcall -private ZwUnloadKey(ptr) NtUnloadKey
@ stub ZwUnloadKeyEx
@ stdcall -private ZwUnlockFile(long ptr ptr ptr ptr) NtUnlockFile
@ stdcall -private ZwUnlockVirtualMemory(long ptr ptr long) NtUnlockVirtualMemory
@ stdcall -private ZwUnmapViewOfSection(long ptr) NtUnmapViewOfSection
@ stub ZwVdmControl
@ stub ZwW32Call
# @ stub ZwWaitForDebugEvent
@ stdcall -private ZwWaitForKeyedEvent(long ptr long ptr) NtWaitForKeyedEvent
@ stdcall -private ZwWaitForMultipleObjects(long ptr long long ptr) NtWaitForMultipleObjects
@ stub ZwWaitForProcessMutant
@ stdcall -private ZwWaitForSingleObject(long long ptr) NtWaitForSingleObject
@ stub ZwWaitHighEventPair
@ stub ZwWaitLowEventPair
@ stdcall -private ZwWriteFile(long long ptr ptr ptr ptr long ptr ptr) NtWriteFile
@ stdcall -private ZwWriteFileGather(long long ptr ptr ptr ptr long ptr ptr) NtWriteFileGather
@ stub ZwWriteRequestData
@ stdcall -private ZwWriteVirtualMemory(long ptr ptr long ptr) NtWriteVirtualMemory
@ stdcall -private ZwYieldExecution() NtYieldExecution
@ cdecl -private -arch=i386 _CIcos() NTDLL__CIcos
@ cdecl -private -arch=i386 _CIlog() NTDLL__CIlog
@ cdecl -private -arch=i386 _CIpow() NTDLL__CIpow
@ cdecl -private -arch=i386 _CIsin() NTDLL__CIsin
@ cdecl -private -arch=i386 _CIsqrt() NTDLL__CIsqrt
@ stdcall -arch=x86_64 __C_specific_handler(ptr long ptr ptr)
@ stdcall -private -arch=arm,x86_64 -norelay __chkstk()
@ cdecl __isascii(long) NTDLL___isascii
@ cdecl __iscsym(long) NTDLL___iscsym
@ cdecl __iscsymf(long) NTDLL___iscsymf
@ cdecl __toascii(long) NTDLL___toascii
@ stdcall -arch=i386 -ret64 _alldiv(int64 int64)
@ stdcall -arch=i386 -norelay _alldvrm(int64 int64)
@ stdcall -arch=i386 -ret64 _allmul(int64 int64)
@ stdcall -arch=i386 -norelay _alloca_probe()
@ stdcall -arch=i386 -ret64 _allrem(int64 int64)
@ stdcall -arch=i386 -ret64 _allshl(int64 long)
@ stdcall -arch=i386 -ret64 _allshr(int64 long)
@ cdecl -ret64 _atoi64(str)
@ stdcall -arch=i386 -ret64 _aulldiv(int64 int64)
@ stdcall -arch=i386 -norelay _aulldvrm(int64 int64)
@ stdcall -arch=i386 -ret64 _aullrem(int64 int64)
@ stdcall -arch=i386 -ret64 _aullshr(int64 long)
@ stdcall -private -arch=i386 -norelay _chkstk()
@ stub _fltused
@ cdecl -arch=i386 -ret64 _ftol() NTDLL__ftol
@ cdecl _i64toa(int64 ptr long)
@ cdecl _i64tow(int64 ptr long)
@ cdecl _itoa(long ptr long)
@ cdecl _itow(long ptr long)
@ cdecl _lfind(ptr ptr ptr long ptr)
@ stdcall -arch=x86_64 _local_unwind(ptr ptr)
@ cdecl _ltoa(long ptr long)
@ cdecl _ltow(long ptr long)
@ cdecl _memccpy(ptr ptr long long)
@ cdecl _memicmp(str str long)
@ varargs _snprintf(ptr long str) NTDLL__snprintf
@ varargs _snprintf_s(ptr long long str) _snprintf_s
@ varargs _snwprintf(ptr long wstr) NTDLL__snwprintf
@ cdecl _splitpath(str ptr ptr ptr ptr)
@ cdecl _strcmpi(str str) _stricmp
@ cdecl _stricmp(str str)
@ cdecl _strlwr(str)
@ cdecl _strnicmp(str str long)
@ cdecl _strupr(str)
@ cdecl _tolower(long) NTDLL__tolower
@ cdecl _toupper(long) NTDLL__toupper
@ cdecl _ui64toa(int64 ptr long)
@ cdecl _ui64tow(int64 ptr long)
@ cdecl _ultoa(long ptr long)
@ cdecl _ultow(long ptr long)
@ cdecl -norelay _vsnprintf(ptr long str ptr) NTDLL__vsnprintf
@ cdecl _vsnprintf_s(ptr long str ptr) _vsnprintf_s
@ cdecl _vsnwprintf(ptr long wstr ptr) NTDLL__vsnwprintf
@ cdecl _wcsicmp(wstr wstr) NTDLL__wcsicmp
@ cdecl _wcslwr(wstr) NTDLL__wcslwr
@ cdecl _wcsnicmp(wstr wstr long) NTDLL__wcsnicmp
@ cdecl _wcsupr(wstr) NTDLL__wcsupr
@ cdecl _wtoi(wstr)
@ cdecl -ret64 _wtoi64(wstr)
@ cdecl _wtol(wstr)
@ cdecl abs(long) NTDLL_abs
@ cdecl atan(double) NTDLL_atan
@ cdecl atoi(str) NTDLL_atoi
@ cdecl atol(str) NTDLL_atol
@ cdecl bsearch(ptr ptr long long ptr) NTDLL_bsearch
@ cdecl ceil(double) NTDLL_ceil
@ cdecl cos(double) NTDLL_cos
@ cdecl fabs(double) NTDLL_fabs
@ cdecl floor(double) NTDLL_floor
@ cdecl isalnum(long) NTDLL_isalnum
@ cdecl isalpha(long) NTDLL_isalpha
@ cdecl iscntrl(long) NTDLL_iscntrl
@ cdecl isdigit(long) NTDLL_isdigit
@ cdecl isgraph(long) NTDLL_isgraph
@ cdecl islower(long) NTDLL_islower
@ cdecl isprint(long) NTDLL_isprint
@ cdecl ispunct(long) NTDLL_ispunct
@ cdecl isspace(long) NTDLL_isspace
@ cdecl isupper(long) NTDLL_isupper
@ cdecl iswalpha(long) NTDLL_iswalpha
@ cdecl iswctype(long long) NTDLL_iswctype
@ cdecl iswdigit(long) NTDLL_iswdigit
@ cdecl iswlower(long) NTDLL_iswlower
@ cdecl iswspace(long) NTDLL_iswspace
@ cdecl iswxdigit(long) NTDLL_iswxdigit
@ cdecl isxdigit(long) NTDLL_isxdigit
@ cdecl labs(long) NTDLL_labs
@ cdecl log(double) NTDLL_log
@ cdecl mbstowcs(ptr str long) NTDLL_mbstowcs
@ cdecl memchr(ptr long long) NTDLL_memchr
@ cdecl memcmp(ptr ptr long) NTDLL_memcmp
@ cdecl memcpy(ptr ptr long) NTDLL_memcpy
@ cdecl memmove(ptr ptr long) NTDLL_memmove
@ cdecl memset(ptr long long) NTDLL_memset
@ cdecl pow(double double) NTDLL_pow
@ cdecl qsort(ptr long long ptr) NTDLL_qsort
@ cdecl sin(double) NTDLL_sin
@ varargs sprintf(ptr str) NTDLL_sprintf
@ cdecl sqrt(double) NTDLL_sqrt
@ varargs sscanf(str str) NTDLL_sscanf
@ cdecl strcat(str str) NTDLL_strcat
@ cdecl strchr(str long) NTDLL_strchr
@ cdecl strcmp(str str) NTDLL_strcmp
@ cdecl strcpy(ptr str) NTDLL_strcpy
@ cdecl strcspn(str str) NTDLL_strcspn
@ cdecl strlen(str) NTDLL_strlen
@ cdecl strncat(str str long) NTDLL_strncat
@ cdecl strncmp(str str long) NTDLL_strncmp
@ cdecl strncpy(ptr str long) NTDLL_strncpy
@ cdecl strnlen(ptr long) NTDLL_strnlen
@ cdecl strpbrk(str str) NTDLL_strpbrk
@ cdecl strrchr(str long) NTDLL_strrchr
@ cdecl strspn(str str) NTDLL_strspn
@ cdecl strstr(str str) NTDLL_strstr
@ cdecl strtol(str ptr long) NTDLL_strtol
@ cdecl strtoul(str ptr long) NTDLL_strtoul
@ varargs swprintf(ptr wstr) NTDLL_swprintf
@ cdecl tan(double) NTDLL_tan
@ cdecl tolower(long) NTDLL_tolower
@ cdecl toupper(long) NTDLL_toupper
@ cdecl towlower(long) NTDLL_towlower
@ cdecl towupper(long) NTDLL_towupper
@ stdcall vDbgPrintEx(long long str ptr)
@ stdcall vDbgPrintExWithPrefix(str long long str ptr)
@ cdecl vsprintf(ptr str ptr) NTDLL_vsprintf
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

##################
# Wine extensions
#
# All functions must be prefixed with '__wine_' (for internal functions)
# or 'wine_' (for user-visible functions) to avoid namespace conflicts.

# Server interface
@ cdecl -norelay wine_server_call(ptr)
@ cdecl wine_server_fd_to_handle(long long long ptr)
@ cdecl wine_server_handle_to_fd(long long ptr ptr)
@ cdecl wine_server_release_fd(long long)
@ cdecl wine_server_send_fd(long)
@ cdecl __wine_make_process_system()

# Debugging
@ cdecl -norelay __wine_dbg_get_channel_flags(ptr)
@ cdecl -norelay __wine_dbg_header(long long str)
@ cdecl -norelay __wine_dbg_output(str)
@ cdecl -norelay __wine_dbg_strdup(str)

# Virtual memory
@ cdecl __wine_locked_recvmsg(long ptr long)

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
