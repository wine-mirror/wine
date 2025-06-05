#note that the Zw... functions are alternate names for the
#Nt... functions.  (see www.sysinternals.com for details)
#if you change a Nt.. function DON'T FORGET to change the
#Zw one too.

@ stdcall A_SHAFinal(ptr ptr)
@ stdcall A_SHAInit(ptr)
@ stdcall A_SHAUpdate(ptr ptr long)
@ stdcall ApiSetQueryApiSetPresence(ptr ptr)
@ stdcall ApiSetQueryApiSetPresenceEx(ptr ptr ptr)
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
@ stdcall DbgUiConnectToDbg()
@ stdcall DbgUiContinue(ptr long)
@ stdcall DbgUiConvertStateChangeStructure(ptr ptr)
@ stdcall DbgUiDebugActiveProcess(long)
@ stdcall DbgUiGetThreadDebugObject()
@ stdcall DbgUiIssueRemoteBreakin(long)
@ stdcall DbgUiRemoteBreakin(ptr)
@ stdcall DbgUiSetThreadDebugObject(long)
@ stdcall DbgUiStopDebugging(long)
@ stdcall DbgUiWaitStateChange(ptr ptr)
@ stdcall DbgUserBreakPoint()
@ stdcall EtwEventActivityIdControl(long ptr)
@ stdcall EtwEventEnabled(int64 ptr)
@ stdcall EtwEventProviderEnabled(int64 long int64)
@ stdcall EtwEventRegister(ptr ptr ptr ptr)
@ stdcall EtwEventSetInformation(int64 long ptr long)
@ stdcall EtwEventUnregister(int64)
@ stdcall EtwEventWrite(int64 ptr long ptr)
@ stdcall EtwEventWriteString(int64 long int64 wstr)
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
@ stdcall -norelay KiRaiseUserExceptionDispatcher()
@ stdcall -norelay KiUserApcDispatcher(ptr long long long ptr)
@ stdcall -norelay KiUserCallbackDispatcher(long ptr long)
@ stdcall -norelay -arch=arm,arm64,arm64ec KiUserCallbackDispatcherReturn()
@ stdcall -norelay -arch=arm64ec KiUserEmulationDispatcher(ptr)
@ stdcall -norelay KiUserExceptionDispatcher(ptr ptr)
# @ stub LdrAccessOutOfProcessResource
@ stdcall LdrAccessResource(long ptr ptr ptr)
@ stdcall LdrAddDllDirectory(ptr ptr)
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
@ stdcall LdrGetDllDirectory(ptr)
@ stdcall LdrGetDllFullName(long ptr)
@ stdcall LdrGetDllHandle(wstr long ptr ptr)
@ stdcall LdrGetDllHandleEx(long ptr ptr ptr ptr)
# @ stub LdrGetDllHandleEx
@ stdcall LdrGetDllPath(wstr long ptr ptr)
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
@ stdcall LdrRemoveDllDirectory(ptr)
@ stdcall LdrResolveDelayLoadedAPI(ptr ptr ptr ptr ptr long)
@ stub LdrSetAppCompatDllRedirectionCallback
@ stdcall LdrSetDefaultDllDirectories(long)
@ stdcall LdrSetDllDirectory(ptr)
@ stub LdrSetDllManifestProber
@ stdcall LdrShutdownProcess()
@ stdcall LdrShutdownThread()
@ extern LdrSystemDllInitBlock
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
@ stdcall -syscall=0x0002 NtAcceptConnectPort(ptr long ptr long ptr ptr)
@ stdcall -syscall=0x0000 NtAccessCheck(ptr long long ptr ptr ptr ptr ptr)
@ stdcall -syscall=0x0029 NtAccessCheckAndAuditAlarm(ptr long ptr ptr ptr long ptr long ptr ptr ptr)
# @ stub NtAccessCheckByType
# @ stub NtAccessCheckByTypeAndAuditAlarm
# @ stub NtAccessCheckByTypeResultList
# @ stub NtAccessCheckByTypeResultListAndAuditAlarm
# @ stub NtAccessCheckByTypeResultListAndAuditAlarmByHandle
@ stdcall -syscall=0x0047 NtAddAtom(ptr long ptr)
# @ stub NtAddBootEntry
@ stdcall -syscall NtAdjustGroupsToken(long long ptr long ptr ptr)
@ stdcall -syscall=0x0041 NtAdjustPrivilegesToken(long long ptr long ptr ptr)
@ stdcall -syscall NtAlertResumeThread(long ptr)
@ stdcall -syscall NtAlertThread(long)
@ stdcall -syscall NtAlertThreadByThreadId(ptr)
@ stdcall -syscall NtAllocateLocallyUniqueId(ptr)
@ stdcall -syscall NtAllocateReserveObject(ptr ptr long)
# @ stub NtAllocateUserPhysicalPages
@ stdcall -syscall NtAllocateUuids(ptr ptr ptr ptr)
@ stdcall -syscall=0x0018 NtAllocateVirtualMemory(long ptr long ptr long long)
@ stdcall -syscall NtAllocateVirtualMemoryEx(long ptr ptr long long ptr long)
@ stdcall -syscall NtAreMappedFilesTheSame(ptr ptr)
@ stdcall -syscall NtAssignProcessToJobObject(long long)
@ stdcall -syscall=0x0005 NtCallbackReturn(ptr long long)
# @ stub NtCancelDeviceWakeupRequest
@ stdcall -syscall=0x005d NtCancelIoFile(long ptr)
@ stdcall -syscall NtCancelIoFileEx(long ptr ptr)
@ stdcall -syscall NtCancelSynchronousIoFile(long ptr ptr)
@ stdcall -syscall=0x0061 NtCancelTimer(long ptr)
@ stdcall -syscall=0x003e NtClearEvent(long)
@ stdcall -syscall=0x000f NtClose(long)
# @ stub NtCloseObjectAuditAlarm
@ stdcall -syscall NtCommitTransaction(long long)
# @ stub NtCompactKeys
@ stdcall -syscall NtCompareObjects(ptr ptr)
@ stdcall -syscall NtCompareTokens(ptr ptr ptr)
@ stdcall -syscall NtCompleteConnectPort(ptr)
# @ stub NtCompressKey
@ stdcall -syscall NtConnectPort(ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall -syscall=0x0043 NtContinue(ptr long)
@ stdcall -syscall NtContinueEx(ptr ptr)
@ stdcall -syscall NtConvertBetweenAuxiliaryCounterAndPerformanceCounter(long ptr ptr ptr)
@ stdcall -syscall=0x00a6 NtCreateDebugObject(ptr long ptr long)
@ stdcall -syscall NtCreateDirectoryObject(ptr long ptr)
@ stdcall -syscall=0x0048 NtCreateEvent(ptr long ptr long long)
# @ stub NtCreateEventPair
@ stdcall -syscall=0x0055 NtCreateFile(ptr long ptr ptr ptr long long long long ptr long)
@ stdcall -syscall NtCreateIoCompletion(ptr long ptr long)
@ stdcall -syscall NtCreateJobObject(ptr long ptr)
# @ stub NtCreateJobSet
@ stdcall -syscall=0x001d NtCreateKey(ptr long ptr long ptr long ptr)
@ stdcall -syscall NtCreateKeyTransacted(ptr long ptr long ptr long long ptr)
@ stdcall -syscall NtCreateKeyedEvent(ptr long ptr long)
@ stdcall -syscall NtCreateLowBoxToken(ptr long long ptr ptr long ptr long ptr)
@ stdcall -syscall NtCreateMailslotFile(ptr long ptr ptr long long long ptr)
@ stdcall -syscall NtCreateMutant(ptr long ptr long)
@ stdcall -syscall NtCreateNamedPipeFile(ptr long ptr ptr long long long long long long long long long ptr)
@ stdcall -syscall NtCreatePagingFile(ptr ptr ptr ptr)
@ stdcall -syscall NtCreatePort(ptr ptr long long ptr)
# @ stub NtCreateProcess
# @ stub NtCreateProcessEx
# @ stub NtCreateProfile
@ stdcall -syscall=0x004a NtCreateSection(ptr long ptr ptr long long long)
@ stdcall -syscall NtCreateSectionEx(ptr long ptr ptr long long long ptr long)
@ stdcall -syscall NtCreateSemaphore(ptr long ptr long long)
@ stdcall -syscall NtCreateSymbolicLinkObject(ptr long ptr ptr)
@ stdcall -syscall=0x004e NtCreateThread(ptr long ptr long ptr ptr ptr long)
@ stdcall -syscall NtCreateThreadEx(ptr long ptr long ptr ptr long long long long ptr)
@ stdcall -syscall NtCreateTimer(ptr long ptr long)
@ stdcall -syscall NtCreateToken(ptr long ptr long ptr ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall -syscall NtCreateTransaction(ptr long ptr ptr long long long long ptr ptr)
@ stdcall -syscall NtCreateUserProcess(ptr ptr long long ptr ptr long long ptr ptr ptr)
# @ stub NtCreateWaitablePort
@ stdcall -arch=i386 NtCurrentTeb()
@ stdcall -syscall NtDebugActiveProcess(long long)
@ stdcall -syscall NtDebugContinue(long ptr long)
@ stdcall -syscall=0x0034 NtDelayExecution(long ptr)
@ stdcall -syscall NtDeleteAtom(long)
# @ stub NtDeleteBootEntry
@ stdcall -syscall NtDeleteFile(ptr)
@ stdcall -syscall NtDeleteKey(long)
# @ stub NtDeleteObjectAuditAlarm
@ stdcall -syscall NtDeleteValueKey(long ptr)
@ stdcall -syscall=0x0007 NtDeviceIoControlFile(long long ptr ptr ptr long ptr long ptr long)
@ stdcall -syscall NtDisplayString(ptr)
@ stdcall -syscall=0x003c NtDuplicateObject(long long long ptr long long long)
@ stdcall -syscall=0x0042 NtDuplicateToken(long long ptr long long ptr)
# @ stub NtEnumerateBootEntries
# @ stub NtEnumerateBus
@ stdcall -syscall=0x0032 NtEnumerateKey(long long long ptr long ptr)
# @ stub NtEnumerateSystemEnvironmentValuesEx
@ stdcall -syscall=0x0013 NtEnumerateValueKey(long long long ptr long ptr)
# @ stub NtExtendSection
@ stdcall -syscall NtFilterToken(long long ptr ptr ptr ptr)
@ stdcall -syscall=0x0014 NtFindAtom(ptr long ptr)
@ stdcall -syscall=0x004b NtFlushBuffersFile(long ptr)
@ stdcall -syscall NtFlushBuffersFileEx(long long ptr long ptr)
@ stdcall -syscall NtFlushInstructionCache(long ptr long)
@ stdcall -syscall NtFlushKey(long)
@ stdcall -syscall NtFlushProcessWriteBuffers()
@ stdcall -syscall NtFlushVirtualMemory(long ptr ptr long)
# @ stub NtFlushWriteBuffer
# @ stub NtFreeUserPhysicalPages
@ stdcall -syscall=0x001e NtFreeVirtualMemory(long ptr ptr long)
@ stdcall -syscall=0x0039 NtFsControlFile(long long ptr ptr ptr long ptr long ptr long)
@ stdcall -norelay -syscall NtGetContextThread(long ptr)
@ stdcall -syscall NtGetCurrentProcessorNumber()
# @ stub NtGetDevicePowerState
@ stdcall -syscall NtGetNextProcess(ptr long long long ptr)
@ stdcall -syscall NtGetNextThread(ptr ptr long long long ptr)
@ stdcall -syscall NtGetNlsSectionPtr(long long long ptr ptr)
# @ stub NtGetPlugPlayEvent
@ stdcall NtGetTickCount()
@ stdcall -syscall NtGetWriteWatch(long long ptr long ptr ptr ptr)
@ stdcall -syscall NtImpersonateAnonymousToken(long)
# @ stub NtImpersonateClientOfPort
# @ stub NtImpersonateThread
@ stdcall -syscall NtInitializeNlsFiles(ptr ptr ptr)
# @ stub NtInitializeRegistry
@ stdcall -syscall NtInitiatePowerAction (long long long long)
@ stdcall -syscall=0x004f NtIsProcessInJob(long long)
# @ stub NtIsSystemResumeAutomatic
@ stdcall -syscall NtListenPort(ptr ptr)
@ stdcall -syscall NtLoadDriver(ptr)
@ stdcall -syscall NtLoadKey2(ptr ptr long)
@ stdcall -syscall NtLoadKey(ptr ptr)
@ stdcall -syscall NtLoadKeyEx(ptr ptr long long long long ptr ptr)
@ stdcall -syscall NtLockFile(long long ptr ptr ptr ptr ptr ptr long long)
# @ stub NtLockProductActivationKeys
# @ stub NtLockRegistryKey
@ stdcall -syscall NtLockVirtualMemory(long ptr ptr long)
@ stdcall -syscall NtMakePermanentObject(long)
@ stdcall -syscall NtMakeTemporaryObject(long)
# @ stub NtMapUserPhysicalPages
# @ stub NtMapUserPhysicalPagesScatter
@ stdcall -syscall=0x0028 NtMapViewOfSection(long long ptr long long ptr ptr long long long)
@ stdcall -syscall NtMapViewOfSectionEx(long long ptr ptr ptr long long ptr long)
# @ stub NtModifyBootEntry
@ stdcall -syscall NtNotifyChangeDirectoryFile(long long ptr ptr ptr ptr long long long)
@ stdcall -syscall NtNotifyChangeKey(long long ptr ptr ptr long long ptr long long)
@ stdcall -syscall NtNotifyChangeMultipleKeys(long long ptr long ptr ptr ptr long long ptr long long)
@ stdcall -syscall=0x0058 NtOpenDirectoryObject(ptr long ptr)
@ stdcall -syscall=0x0040 NtOpenEvent(ptr long ptr)
# @ stub NtOpenEventPair
@ stdcall -syscall=0x0033 NtOpenFile(ptr long ptr ptr long long)
@ stdcall -syscall NtOpenIoCompletion(ptr long ptr)
@ stdcall -syscall NtOpenJobObject(ptr long ptr)
@ stdcall -syscall=0x0012 NtOpenKey(ptr long ptr)
@ stdcall -syscall NtOpenKeyEx(ptr long ptr long)
@ stdcall -syscall NtOpenKeyTransacted(ptr long ptr long)
@ stdcall -syscall NtOpenKeyTransactedEx(ptr long ptr long long)
@ stdcall -syscall NtOpenKeyedEvent(ptr long ptr)
@ stdcall -syscall NtOpenMutant(ptr long ptr)
# @ stub NtOpenObjectAuditAlarm
@ stdcall -syscall=0x0026 NtOpenProcess(ptr long ptr ptr)
@ stdcall -syscall NtOpenProcessToken(long long ptr)
@ stdcall -syscall=0x0030 NtOpenProcessTokenEx(long long long ptr)
@ stdcall -syscall=0x0037 NtOpenSection(ptr long ptr)
@ stdcall -syscall NtOpenSemaphore(ptr long ptr)
@ stdcall -syscall NtOpenSymbolicLinkObject (ptr long ptr)
@ stdcall -syscall NtOpenThread(ptr long ptr ptr)
@ stdcall -syscall=0x0024 NtOpenThreadToken(long long long ptr)
@ stdcall -syscall=0x002f NtOpenThreadTokenEx(long long long long ptr)
@ stdcall -syscall NtOpenTimer(ptr long ptr)
# @ stub NtPlugPlayControl
@ stdcall -syscall=0x005f NtPowerInformation(long ptr long ptr long)
@ stdcall -syscall NtPrivilegeCheck(ptr ptr ptr)
# @ stub NtPrivilegeObjectAuditAlarm
# @ stub NtPrivilegedServiceAuditAlarm
@ stdcall -syscall=0x0050 NtProtectVirtualMemory(long ptr ptr long ptr)
@ stdcall -syscall NtPulseEvent(long ptr)
@ stdcall -syscall=0x003d NtQueryAttributesFile(ptr ptr)
# @ stub NtQueryBootEntryOrder
# @ stub NtQueryBootOptions
# @ stub NtQueryDebugFilterState
@ stdcall -syscall=0x0015 NtQueryDefaultLocale(long ptr)
@ stdcall -syscall=0x0044 NtQueryDefaultUILanguage(ptr)
@ stdcall -syscall=0x0035 NtQueryDirectoryFile(long long ptr ptr ptr ptr long long long ptr long)
@ stdcall -syscall NtQueryDirectoryObject(long ptr long long long ptr ptr)
@ stdcall -syscall NtQueryEaFile(long ptr ptr long long ptr long ptr long)
@ stdcall -syscall=0x0056 NtQueryEvent(long long ptr long ptr)
@ stdcall -syscall NtQueryFullAttributesFile(ptr ptr)
@ stdcall -syscall NtQueryInformationAtom(long long ptr long ptr)
@ stdcall -syscall=0x0011 NtQueryInformationFile(long ptr ptr long long)
@ stdcall -syscall NtQueryInformationJobObject(long long ptr long ptr)
# @ stub NtQueryInformationPort
@ stdcall -syscall=0x0019 NtQueryInformationProcess(long long ptr long ptr)
@ stdcall -syscall=0x0025 NtQueryInformationThread(long long ptr long ptr)
@ stdcall -syscall=0x0021 NtQueryInformationToken(long long ptr long ptr)
@ stdcall -syscall NtQueryInstallUILanguage(ptr)
# @ stub NtQueryIntervalProfile
@ stdcall -syscall NtQueryIoCompletion(long long ptr long ptr)
@ stdcall -syscall=0x0016 NtQueryKey(long long ptr long ptr)
@ stdcall -syscall NtQueryLicenseValue(ptr ptr ptr long ptr)
@ stdcall -syscall NtQueryMultipleValueKey(long ptr long ptr long ptr)
@ stdcall -syscall NtQueryMutant(long long ptr long ptr)
@ stdcall -syscall=0x0010 NtQueryObject(long long ptr long ptr)
# @ stub NtQueryOpenSubKeys
@ stdcall -syscall=0x0031 NtQueryPerformanceCounter(ptr ptr)
# @ stub NtQueryPortInformationProcess
# @ stub NtQueryQuotaInformationFile
@ stdcall -syscall=0x0051 NtQuerySection(long long ptr long ptr)
@ stdcall -syscall NtQuerySecurityObject(long long ptr long ptr)
@ stdcall -syscall NtQuerySemaphore (long long ptr long ptr)
@ stdcall -syscall NtQuerySymbolicLinkObject(long ptr ptr)
@ stdcall -syscall NtQuerySystemEnvironmentValue(ptr ptr long ptr)
@ stdcall -syscall NtQuerySystemEnvironmentValueEx(ptr ptr ptr ptr ptr)
@ stdcall -syscall=0x0036 NtQuerySystemInformation(long ptr long ptr)
@ stdcall -syscall NtQuerySystemInformationEx(long ptr long ptr long ptr)
@ stdcall -syscall=0x005a NtQuerySystemTime(ptr)
@ stdcall -syscall=0x0038 NtQueryTimer(ptr long ptr long ptr)
@ stdcall -syscall NtQueryTimerResolution(ptr ptr ptr)
@ stdcall -syscall=0x0017 NtQueryValueKey(long ptr long ptr long ptr)
@ stdcall -syscall=0x0023 NtQueryVirtualMemory(long ptr long ptr long ptr)
@ stdcall -syscall=0x0049 NtQueryVolumeInformationFile(long ptr ptr long long)
@ stdcall -syscall=0x0045 NtQueueApcThread(long ptr long long long)
@ stdcall -syscall NtQueueApcThreadEx(long long ptr long long long)
@ stdcall -syscall NtRaiseException(ptr ptr long)
@ stdcall -syscall NtRaiseHardError(long long long ptr long ptr)
@ stdcall -syscall=0x0006 NtReadFile(long long ptr ptr ptr ptr long ptr ptr)
@ stdcall -syscall=0x002e NtReadFileScatter(long long ptr ptr ptr ptr long ptr ptr)
# @ stub NtReadRequestData
@ stdcall -syscall=0x003f NtReadVirtualMemory(long ptr ptr long ptr)
# @ stub NtRegisterNewDevice
@ stdcall -syscall NtRegisterThreadTerminatePort(ptr)
@ stdcall -syscall NtReleaseKeyedEvent(long ptr long ptr)
@ stdcall -syscall=0x0020 NtReleaseMutant(long ptr)
# @ stub NtReleaseProcessMutant
@ stdcall -syscall=0x000a NtReleaseSemaphore(long long ptr)
@ stdcall -syscall=0x0009 NtRemoveIoCompletion(ptr ptr ptr ptr ptr)
@ stdcall -syscall NtRemoveIoCompletionEx(ptr ptr long ptr ptr long)
@ stdcall -syscall NtRemoveProcessDebug(long long)
@ stdcall -syscall NtRenameKey(long ptr)
@ stdcall -syscall NtReplaceKey(ptr long ptr)
# @ stub NtReplyPort
@ stdcall -syscall=0x000b NtReplyWaitReceivePort(ptr ptr ptr ptr)
# @ stub NtReplyWaitReceivePortEx
# @ stub NtReplyWaitReplyPort
# @ stub NtRequestDeviceWakeup
# @ stub NtRequestPort
@ stdcall -syscall=0x0022 NtRequestWaitReplyPort(ptr ptr ptr)
# @ stub NtRequestWakeupLatency
@ stdcall -syscall NtResetEvent(long ptr)
@ stdcall -syscall NtResetWriteWatch(long ptr long)
@ stdcall -syscall NtRestoreKey(long long long)
@ stdcall -syscall NtResumeProcess(long)
@ stdcall -syscall=0x0052 NtResumeThread(long ptr)
@ stdcall -syscall NtRollbackTransaction(long long)
@ stdcall -syscall NtSaveKey(long long)
# @ stub NtSaveKeyEx
# @ stub NtSaveMergedKeys
@ stdcall -syscall NtSecureConnectPort(ptr ptr ptr ptr ptr ptr ptr ptr ptr)
# @ stub NtSetBootEntryOrder
# @ stub NtSetBootOptions
@ stdcall -syscall NtSetContextThread(long ptr)
@ stdcall -syscall NtSetDebugFilterState(long long long)
# @ stub NtSetDefaultHardErrorPort
@ stdcall -syscall NtSetDefaultLocale(long long)
@ stdcall -syscall NtSetDefaultUILanguage(long)
@ stdcall -syscall NtSetEaFile(long ptr ptr long)
@ stdcall -syscall=0x000e NtSetEvent(long ptr)
# @ stub NtSetEventBoostPriority
# @ stub NtSetHighEventPair
# @ stub NtSetHighWaitLowEventPair
# @ stub NtSetHighWaitLowThread
@ stdcall -syscall NtSetInformationDebugObject(long long ptr long ptr)
@ stdcall -syscall=0x0027 NtSetInformationFile(long ptr ptr long long)
@ stdcall -syscall NtSetInformationJobObject(long long ptr long)
@ stdcall -syscall NtSetInformationKey(long long ptr long)
@ stdcall -syscall=0x005c NtSetInformationObject(long long ptr long)
@ stdcall -syscall=0x001c NtSetInformationProcess(long long ptr long)
@ stdcall -syscall=0x000d NtSetInformationThread(long long ptr long)
@ stdcall -syscall NtSetInformationToken(long long ptr long)
@ stdcall -syscall NtSetInformationVirtualMemory(long long ptr ptr ptr long)
@ stdcall -syscall NtSetIntervalProfile(long long)
@ stdcall -syscall NtSetIoCompletion(ptr long long long long)
@ stdcall -syscall NtSetIoCompletionEx(ptr ptr long long long long)
@ stdcall -syscall NtSetLdtEntries(long int64 long int64)
# @ stub NtSetLowEventPair
# @ stub NtSetLowWaitHighEventPair
# @ stub NtSetLowWaitHighThread
# @ stub NtSetQuotaInformationFile
@ stdcall -syscall NtSetSecurityObject(long long ptr)
# @ stub NtSetSystemEnvironmentValue
# @ stub NtSetSystemEnvironmentValueEx
@ stdcall -syscall NtSetSystemInformation(long ptr long)
# @ stub NtSetSystemPowerState
@ stdcall -syscall NtSetSystemTime(ptr ptr)
@ stdcall -syscall NtSetThreadExecutionState(long ptr)
@ stdcall -syscall=0x0062 NtSetTimer(long ptr ptr ptr long long ptr)
@ stdcall -syscall NtSetTimerResolution(long long ptr)
# @ stub NtSetUuidSeed
@ stdcall -syscall=0x0060 NtSetValueKey(long ptr long long ptr long)
@ stdcall -syscall NtSetVolumeInformationFile(long ptr ptr long long)
@ stdcall -syscall NtShutdownSystem(long)
@ stdcall -syscall NtSignalAndWaitForSingleObject(long long long ptr)
# @ stub NtStartProfile
# @ stub NtStopProfile
@ stdcall -syscall NtSuspendProcess(long)
@ stdcall -syscall NtSuspendThread(long ptr)
@ stdcall -syscall NtSystemDebugControl(long ptr long ptr long ptr)
@ stdcall -syscall NtTerminateJobObject(long long)
@ stdcall -syscall=0x002c NtTerminateProcess(long long)
@ stdcall -syscall=0x0053 NtTerminateThread(long long)
@ stdcall -syscall NtTestAlert()
@ stdcall -syscall NtTraceControl(long ptr long ptr long long)
# @ stub NtTraceEvent
# @ stub NtTranslateFilePath
@ stdcall -syscall NtUnloadDriver(ptr)
@ stdcall -syscall NtUnloadKey(ptr)
# @ stub NtUnloadKeyEx
@ stdcall -syscall NtUnlockFile(long ptr ptr ptr ptr)
@ stdcall -syscall NtUnlockVirtualMemory(long ptr ptr long)
@ stdcall -syscall=0x002a NtUnmapViewOfSection(long ptr)
@ stdcall -syscall NtUnmapViewOfSectionEx(long ptr long)
# @ stub NtVdmControl
# @ stub NtW32Call
@ stdcall -syscall NtWaitForAlertByThreadId(ptr ptr)
@ stdcall -syscall NtWaitForDebugEvent(long long ptr ptr)
@ stdcall -syscall NtWaitForKeyedEvent(long ptr long ptr)
@ stdcall -syscall=0x005b NtWaitForMultipleObjects(long ptr long long ptr)
# @ stub NtWaitForProcessMutant
@ stdcall -syscall=0x0004 NtWaitForSingleObject(long long ptr)
# @ stub NtWaitHighEventPair
# @ stub NtWaitLowEventPair
@ stdcall -syscall -arch=win32 NtWow64AllocateVirtualMemory64(long ptr int64 ptr long long)
@ stdcall -syscall -arch=win32 NtWow64GetNativeSystemInformation(long ptr long ptr)
@ stdcall -syscall -arch=win32 NtWow64IsProcessorFeaturePresent(long)
@ stdcall -syscall -arch=win32 NtWow64QueryInformationProcess64(long long ptr long ptr)
@ stdcall -syscall -arch=win32 NtWow64ReadVirtualMemory64(long int64 ptr int64 ptr)
@ stdcall -syscall -arch=win32 NtWow64WriteVirtualMemory64(long int64 ptr int64 ptr)
@ stdcall -syscall=0x0008 NtWriteFile(long long ptr ptr ptr ptr long ptr ptr)
@ stdcall -syscall=0x001b NtWriteFileGather(long long ptr ptr ptr ptr long ptr ptr)
# @ stub NtWriteRequestData
@ stdcall -syscall=0x003a NtWriteVirtualMemory(long ptr ptr long ptr)
@ stdcall -syscall=0x0046 NtYieldExecution()
@ stdcall NtdllDefWindowProc_A(long long long long)
@ stdcall NtdllDefWindowProc_W(long long long long)
@ stdcall NtdllDialogWndProc_A(long long long long)
@ stdcall NtdllDialogWndProc_W(long long long long)
@ stub PfxFindPrefix
@ stub PfxInitialize
@ stub PfxInsertPrefix
@ stub PfxRemovePrefix
@ stdcall -arch=arm64ec ProcessPendingCrossProcessEmulatorWork()
# @ stub PropertyLengthAsVariant
@ stub RtlAbortRXact
@ stdcall RtlAbsoluteToSelfRelativeSD(ptr ptr ptr)
@ stdcall RtlAcquirePebLock()
@ stdcall RtlAcquireResourceExclusive(ptr long)
@ stdcall RtlAcquireResourceShared(ptr long)
@ stdcall RtlAcquireSRWLockExclusive(ptr)
@ stdcall RtlAcquireSRWLockShared(ptr)
@ stdcall RtlActivateActivationContext(long ptr ptr)
@ stdcall RtlActivateActivationContextEx(long ptr ptr ptr)
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
@ cdecl -arch=!i386 RtlAddFunctionTable(ptr long long)
@ stdcall -arch=!i386 RtlAddGrowableFunctionTable(ptr ptr long long long long)
@ stdcall RtlAddMandatoryAce(ptr long long long long ptr)
@ stdcall RtlAddProcessTrustLabelAce(ptr long long ptr long long)
# @ stub RtlAddRange
@ stdcall RtlAddRefActivationContext(ptr)
# @ stub RtlAddRefMemoryStream
@ stdcall RtlAddVectoredContinueHandler(long ptr)
@ stdcall RtlAddVectoredExceptionHandler(long ptr)
@ stdcall RtlAddressInSectionTable(ptr long long)
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
@ stdcall RtlConvertDeviceFamilyInfoToString(ptr ptr ptr ptr)
@ stub RtlConvertExclusiveToShared
@ stdcall -arch=win32 -ret64 RtlConvertLongToLargeInteger(long)
# @ stub RtlConvertPropertyToVariant
@ stub RtlConvertSharedToExclusive
@ stdcall RtlConvertSidToUnicodeString(ptr ptr long)
@ stdcall RtlConvertToAutoInheritSecurityObject(ptr ptr ptr ptr long ptr)
@ stub RtlConvertUiListToApiList
@ stdcall -arch=win32 -ret64 RtlConvertUlongToLargeInteger(long)
# @ stub RtlConvertVariantToProperty
@ stdcall RtlCopyContext(ptr long ptr)
@ stdcall RtlCopyExtendedContext(ptr long ptr)
@ stdcall RtlCopyLuid(ptr ptr)
@ stdcall RtlCopyLuidAndAttributesArray(long ptr ptr)
@ stdcall -arch=!i386 RtlCopyMemory(ptr ptr long)
@ stdcall -arch=x86_64 RtlCopyMemoryNonTemporal(ptr ptr long) RtlCopyMemory
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
@ stdcall RtlCreateServiceSid(ptr ptr ptr)
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
@ stdcall RtlCreateUserThread(long ptr long long long long ptr ptr ptr ptr)
@ stdcall RtlCustomCPToUnicodeN(ptr ptr long ptr str long)
@ stub RtlCutoverTimeToSystemTime
@ stdcall RtlDeNormalizeProcessParams(ptr)
@ stdcall RtlDeactivateActivationContext(long long)
@ stub RtlDeactivateActivationContextUnsafeFast
@ stub RtlDebugPrintTimes
@ stdcall RtlDecodePointer(ptr)
@ stdcall RtlDecodeSystemPointer(ptr) RtlDecodePointer
@ stdcall RtlDecompressBuffer(long ptr long ptr long ptr)
@ stdcall RtlDecompressFragment(long ptr long ptr long long ptr ptr)
@ stdcall RtlDefaultNpAcl(ptr)
@ stdcall RtlDelete(ptr)
@ stdcall RtlDeleteAce(ptr long)
@ stdcall RtlDeleteAtomFromAtomTable(ptr long)
@ stdcall RtlDeleteCriticalSection(ptr)
@ stdcall -arch=!i386 RtlDeleteGrowableFunctionTable(ptr)
@ stdcall RtlDeleteElementGenericTable(ptr ptr)
@ stub RtlDeleteElementGenericTableAvl
@ cdecl -arch=!i386 RtlDeleteFunctionTable(ptr)
@ stdcall RtlDeleteNoSplay(ptr ptr)
@ stub RtlDeleteOwnersRanges
@ stub RtlDeleteRange
@ stdcall RtlDeleteRegistryValue(long ptr wstr)
@ stdcall RtlDeleteResource(ptr)
@ stdcall RtlDeleteSecurityObject(ptr)
@ stdcall RtlDeleteTimer(ptr ptr ptr)
# @ stub RtlDeleteTimerQueue
@ stdcall RtlDeleteTimerQueueEx(ptr ptr)
@ stdcall RtlDeregisterWait(ptr)
@ stdcall RtlDeregisterWaitEx(ptr ptr)
@ stdcall RtlDeriveCapabilitySidsFromName(ptr ptr ptr)
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
@ stdcall RtlDosPathNameToRelativeNtPathName_U(wstr ptr ptr ptr)
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
@ stdcall RtlEnumerateGenericTable(ptr long)
# @ stub RtlEnumerateGenericTableAvl
# @ stub RtlEnumerateGenericTableLikeADirectory
@ stdcall RtlEnumerateGenericTableWithoutSplaying(ptr ptr)
@ stdcall RtlEnumerateGenericTableWithoutSplayingAvl(ptr ptr)
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
@ stdcall RtlFindExportedRoutineByName(ptr str)
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
@ stdcall RtlFlsAlloc(ptr ptr)
@ stdcall RtlFlsFree(long)
@ stdcall RtlFlsGetValue(long ptr)
@ stdcall RtlFlsSetValue(long ptr)
@ stub RtlFlushPropertySet
# @ stub RtlFlushSecureMemoryCache
@ stdcall RtlFormatCurrentUserKeyPath(ptr)
@ stdcall RtlFormatMessage(ptr long long long long ptr ptr long ptr)
@ stdcall RtlFormatMessageEx(ptr long long long long ptr ptr long ptr long)
@ stdcall RtlFreeActivationContextStack(ptr)
@ stdcall RtlFreeAnsiString(ptr)
@ stdcall RtlFreeHandle(ptr ptr)
@ stdcall RtlFreeHeap(long long ptr)
@ stdcall RtlFreeOemString(ptr)
# @ stub RtlFreeRangeList
@ stdcall RtlFreeSid (ptr)
@ stdcall RtlFreeThreadActivationContextStack()
@ stdcall RtlFreeUnicodeString(ptr)
@ stdcall RtlFreeUserStack(ptr)
@ stdcall RtlGetDeviceFamilyInfoEnum(ptr ptr ptr)
@ stdcall RtlGUIDFromString(ptr ptr)
@ stub RtlGenerate8dot3Name
@ stdcall RtlGetAce(ptr long ptr)
@ stdcall RtlGetActiveActivationContext(ptr)
@ stdcall RtlGetCallersAddress(ptr ptr)
@ stdcall RtlGetCompressionWorkSpaceSize(long ptr ptr)
@ stdcall RtlGetControlSecurityDescriptor(ptr ptr ptr)
@ stdcall RtlGetCurrentDirectory_U(long ptr)
@ stdcall RtlGetCurrentPeb()
@ stdcall RtlGetCurrentProcessorNumberEx(ptr)
@ stdcall RtlGetCurrentTransaction()
@ stdcall RtlGetDaclSecurityDescriptor(ptr ptr ptr ptr)
@ stdcall RtlGetElementGenericTable(ptr long)
# @ stub RtlGetElementGenericTableAvl
@ stdcall RtlGetEnabledExtendedFeatures(int64)
@ stdcall RtlGetExePath(wstr ptr)
@ stdcall RtlGetExtendedContextLength(long ptr)
@ stdcall RtlGetExtendedContextLength2(long ptr int64)
@ stdcall -ret64 RtlGetExtendedFeaturesMask(ptr)
# @ stub RtlGetFirstRange
@ stdcall RtlGetFrame()
@ stdcall RtlGetFullPathName_U(wstr long ptr ptr)
@ stdcall RtlGetFullPathName_UEx(wstr long ptr ptr ptr)
@ stdcall RtlGetGroupSecurityDescriptor(ptr ptr ptr)
@ stdcall RtlGetLastNtStatus()
@ stdcall RtlGetLastWin32Error()
# @ stub RtlGetLengthWithoutLastFullDosOrNtPathElement
# Yes, Microsoft really misspelled this one!
# @ stub RtlGetLengthWithoutTrailingPathSeperators
@ stdcall RtlGetLocaleFileMappingAddress(ptr ptr ptr)
@ stdcall RtlGetLongestNtPathLength()
@ stdcall RtlGetNativeSystemInformation(long ptr long ptr)
# @ stub RtlGetNextRange
@ stdcall RtlGetNtGlobalFlags()
@ stdcall RtlGetNtProductType(ptr)
@ stdcall RtlGetNtVersionNumbers(ptr ptr ptr)
@ stdcall RtlGetOwnerSecurityDescriptor(ptr ptr ptr)
@ stdcall RtlGetProductInfo(long long long long ptr)
@ stdcall RtlGetProcessHeaps(long ptr)
@ stdcall RtlGetProcessPreferredUILanguages(long ptr ptr ptr)
@ stdcall RtlGetSaclSecurityDescriptor(ptr ptr ptr ptr)
@ stdcall RtlGetSearchPath(ptr)
# @ stub RtlGetSecurityDescriptorRMControl
# @ stub RtlGetSetBootStatusData
@ stdcall RtlGetSystemPreferredUILanguages(long long ptr ptr ptr)
@ stdcall -ret64 RtlGetSystemTimePrecise()
@ stdcall RtlGetThreadErrorMode()
@ stdcall RtlGetThreadPreferredUILanguages(long ptr ptr ptr)
@ stdcall RtlGetUnloadEventTrace()
@ stdcall RtlGetUnloadEventTraceEx(ptr ptr ptr)
@ stdcall RtlGetUserInfoHeap(ptr long ptr ptr ptr)
@ stdcall RtlGetUserPreferredUILanguages(long long ptr ptr ptr)
@ stdcall RtlGetVersion(ptr)
@ stdcall -arch=!i386 RtlGrowFunctionTable(ptr long)
@ stub RtlGuidToPropertySetName
@ stdcall RtlHashUnicodeString(ptr long long ptr)
@ stdcall RtlIdentifierAuthoritySid(ptr)
@ stdcall RtlIdnToAscii(long wstr long ptr ptr)
@ stdcall RtlIdnToNameprepUnicode(long wstr long ptr ptr)
@ stdcall RtlIdnToUnicode(long wstr long ptr ptr)
@ stdcall RtlImageDirectoryEntryToData(long long long ptr)
@ stdcall RtlImageNtHeader(long)
@ stdcall RtlImageRvaToSection(ptr long long)
@ stdcall RtlImageRvaToVa(ptr long long ptr)
@ stdcall RtlImpersonateSelf(long)
@ stdcall RtlInitAnsiString(ptr str)
@ stdcall RtlInitAnsiStringEx(ptr str)
@ stdcall RtlInitCodePageTable(ptr ptr)
# @ stub RtlInitMemoryStream
@ stdcall RtlInitNlsTables(ptr ptr ptr ptr)
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
@ stdcall RtlInitializeExtendedContext(ptr long ptr)
@ stdcall RtlInitializeExtendedContext2(ptr long ptr int64)
@ stdcall RtlInitializeGenericTable(ptr ptr ptr ptr ptr)
@ stdcall RtlInitializeGenericTableAvl(ptr ptr ptr ptr ptr)
@ stdcall RtlInitializeHandleTable(long long ptr)
@ stdcall RtlInitializeNtUserPfn(ptr long ptr long ptr long)
@ stub RtlInitializeRXact
# @ stub RtlInitializeRangeList
@ stdcall RtlInitializeResource(ptr)
@ stdcall RtlInitializeSListHead(ptr)
@ stdcall RtlInitializeSRWLock(ptr)
@ stdcall RtlInitializeSid(ptr ptr long)
# @ stub RtlInitializeStackTraceDataBase
@ stdcall RtlInsertElementGenericTable(ptr ptr long ptr)
@ stdcall RtlInsertElementGenericTableAvl(ptr ptr long ptr)
@ cdecl -arch=!i386 RtlInstallFunctionTableCallback(long long long ptr ptr wstr)
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
@ stdcall RtlIsActivationContextActive(ptr)
@ stdcall RtlIsCriticalSectionLocked(ptr)
@ stdcall RtlIsCriticalSectionLockedByThread(ptr)
@ stdcall RtlIsCurrentProcess(long)
@ stdcall RtlIsCurrentThread(long)
@ stdcall RtlIsDosDeviceName_U(wstr)
@ stdcall -arch=x86_64 -norelay RtlIsEcCode(ptr)
@ stdcall RtlIsGenericTableEmpty(ptr)
# @ stub RtlIsGenericTableEmptyAvl
@ stdcall RtlIsNameLegalDOS8Dot3(ptr ptr ptr)
@ stdcall RtlIsNormalizedString(long wstr long ptr)
@ stdcall RtlIsProcessorFeaturePresent(long)
# @ stub RtlIsRangeAvailable
@ stdcall RtlIsTextUnicode(ptr long ptr)
# @ stub RtlIsThreadWithinLoaderCallout
@ stdcall RtlIsValidHandle(ptr ptr)
@ stdcall RtlIsValidIndexHandle(ptr long ptr)
@ stdcall RtlIsValidLocaleName(wstr long)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerAdd(int64 int64)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerArithmeticShift(int64 long)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerDivide(int64 int64 ptr)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerNegate(int64)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerShiftLeft(int64 long)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerShiftRight(int64 long)
@ stdcall -arch=win32 -ret64 RtlLargeIntegerSubtract(int64 int64)
@ stdcall RtlLargeIntegerToChar(ptr long long ptr)
@ stdcall RtlLcidToLocaleName(long ptr long long)
@ stdcall RtlLeaveCriticalSection(ptr)
@ stdcall RtlLengthRequiredSid(long)
@ stdcall RtlLengthSecurityDescriptor(ptr)
@ stdcall RtlLengthSid(ptr)
@ stdcall RtlLocalTimeToSystemTime(ptr ptr)
@ stdcall RtlLocaleNameToLcid(wstr ptr long)
@ stdcall RtlLocateExtendedFeature(ptr long ptr)
@ stdcall RtlLocateExtendedFeature2(ptr long ptr ptr)
@ stdcall RtlLocateLegacyContext(ptr ptr)
# @ stub RtlLockBootStatusData
@ stdcall RtlLockHeap(long)
# @ stub RtlLockMemoryStreamRegion
# @ stub RtlLogStackBackTrace
@ stdcall RtlLookupAtomInAtomTable(ptr wstr ptr)
@ stdcall RtlLookupElementGenericTable(ptr ptr)
@ stdcall RtlLookupElementGenericTableAvl(ptr ptr)
@ stdcall -arch=!i386 RtlLookupFunctionEntry(long ptr ptr)
@ stdcall -arch=!i386 RtlLookupFunctionTable(long ptr ptr)
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
@ stdcall RtlNewSecurityObjectEx(ptr ptr ptr ptr long long long ptr)
@ stdcall RtlNewSecurityObjectWithMultipleInheritance(ptr ptr ptr ptr long long long long ptr)
@ stdcall RtlNormalizeProcessParams(ptr)
@ stdcall RtlNormalizeString(long wstr long ptr ptr)
# @ stub RtlNtPathNameToDosPathName
@ stdcall RtlNtStatusToDosError(long)
@ stdcall RtlNtStatusToDosErrorNoTeb(long)
@ stdcall RtlNumberGenericTableElements(ptr)
@ stdcall RtlNumberGenericTableElementsAvl(ptr)
@ stdcall RtlNumberOfClearBits(ptr)
@ stdcall RtlNumberOfSetBits(ptr)
@ stdcall RtlOemStringToUnicodeSize(ptr)
@ stdcall RtlOemStringToUnicodeString(ptr ptr long)
@ stdcall RtlOemToUnicodeN(ptr long ptr ptr long)
@ stdcall -arch=win64 RtlOpenCrossProcessEmulatorWorkConnection(long ptr ptr)
@ stdcall RtlOpenCurrentUser(long ptr)
@ stdcall RtlPcToFileHeader(ptr ptr)
@ stdcall RtlPinAtomInAtomTable(ptr long)
@ stdcall RtlPopFrame(ptr)
@ stdcall RtlPrefixString(ptr ptr long)
@ stdcall RtlPrefixUnicodeString(ptr ptr long)
@ stdcall RtlProcessFlsData(ptr long)
@ stub RtlPropertySetNameToGuid
@ stub RtlProtectHeap
@ stdcall RtlPushFrame(ptr)
@ stdcall RtlQueryActivationContextApplicationSettings(long ptr wstr wstr ptr long ptr)
@ stdcall RtlQueryAtomInAtomTable(ptr long ptr ptr ptr ptr)
@ stdcall RtlQueryDepthSList(ptr)
@ stdcall RtlQueryDynamicTimeZoneInformation(ptr)
@ stdcall RtlQueryEnvironmentVariable_U(ptr ptr ptr)
@ stdcall RtlQueryEnvironmentVariable(ptr ptr long ptr long ptr)
@ stdcall RtlQueryHeapInformation(long long ptr long ptr)
@ stdcall RtlQueryInformationAcl(ptr ptr long long)
@ stdcall RtlQueryInformationActivationContext(long long ptr long ptr long ptr)
@ stub RtlQueryInformationActiveActivationContext
@ stub RtlQueryInterfaceMemoryStream
@ stdcall RtlQueryPackageIdentity(long ptr ptr ptr ptr ptr)
@ stdcall RtlQueryPerformanceCounter(ptr)
@ stdcall RtlQueryPerformanceFrequency(ptr)
@ stub RtlQueryProcessBackTraceInformation
@ stdcall RtlQueryProcessDebugInformation(long long ptr)
@ stub RtlQueryProcessHeapInformation
@ stub RtlQueryProcessLockInformation
@ stdcall RtlQueryProcessPlaceholderCompatibilityMode()
@ stub RtlQueryProperties
@ stub RtlQueryPropertyNames
@ stub RtlQueryPropertySet
@ stdcall RtlQueryRegistryValues(long ptr ptr ptr ptr)
@ stdcall RtlQueryRegistryValuesEx(long ptr ptr ptr ptr) RtlQueryRegistryValues
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
@ stdcall RtlRbInsertNodeEx(ptr ptr long ptr)
@ stdcall RtlRbRemoveNode(ptr ptr)
@ stdcall RtlReAllocateHeap(long long ptr long)
@ stub RtlReadMemoryStream
@ stub RtlReadOutOfProcessMemoryStream
@ stdcall RtlRealPredecessor(ptr)
@ stdcall RtlRealSuccessor(ptr)
@ stub RtlRegisterSecureMemoryCacheCallback
@ stdcall RtlRegisterWait(ptr ptr ptr ptr long long)
@ stdcall RtlReleaseActivationContext(ptr)
@ stub RtlReleaseMemoryStream
@ stdcall RtlReleasePath(ptr)
@ stdcall RtlReleasePebLock()
@ stdcall RtlReleaseRelativeName(ptr)
@ stdcall RtlReleaseResource(ptr)
@ stdcall RtlReleaseSRWLockExclusive(ptr)
@ stdcall RtlReleaseSRWLockShared(ptr)
@ stub RtlRemoteCall
@ stdcall RtlRemoveVectoredContinueHandler(ptr)
@ stdcall RtlRemoveVectoredExceptionHandler(ptr)
@ stdcall RtlResetNtUserPfn()
@ stdcall RtlResetRtlTranslations(ptr)
@ cdecl RtlRestoreContext(ptr ptr)
@ stdcall RtlRestoreLastWin32Error(long) RtlSetLastWin32Error
@ stdcall RtlRetrieveNtUserPfn(ptr ptr ptr)
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
@ stdcall RtlSetExtendedFeaturesMask(ptr int64)
@ stdcall RtlSetGroupSecurityDescriptor(ptr ptr long)
@ stdcall RtlSetHeapInformation(long long ptr long)
@ stub RtlSetInformationAcl
@ stdcall RtlSetIoCompletionCallback(long ptr long)
@ stdcall RtlSetLastWin32Error(long)
@ stdcall RtlSetLastWin32ErrorAndNtStatusFromNtStatus(long)
# @ stub RtlSetMemoryStreamSize
@ stdcall RtlSetOwnerSecurityDescriptor(ptr ptr long)
# @ stub RtlSetProcessIsCritical
@ stdcall RtlSetProcessPreferredUILanguages(long ptr ptr)
@ stub RtlSetProperties
@ stub RtlSetPropertyClassId
@ stub RtlSetPropertyNames
@ stub RtlSetPropertySetClassId
@ stdcall RtlSetSaclSecurityDescriptor(ptr long ptr long)
@ stdcall RtlSetSearchPathMode(long)
# @ stub RtlSetSecurityDescriptorRMControl
@ stub RtlSetSecurityObject
# @ stub RtlSetSecurityObjectEx
@ stdcall RtlSetThreadErrorMode(long ptr)
# @ stub RtlSetThreadIsCritical
@ stdcall RtlSetThreadPreferredUILanguages(long ptr ptr)
# @ stub RtlSetThreadPoolStartFunc
@ stdcall RtlSetTimeZoneInformation(ptr)
# @ stub RtlSetTimer
@ stdcall RtlSetUnhandledExceptionFilter(ptr)
@ stub RtlSetUnicodeCallouts
@ stdcall RtlSetUserFlagsHeap(ptr long ptr long long)
@ stdcall RtlSetUserValueHeap(ptr long ptr ptr)
@ stdcall RtlSizeHeap(long long ptr)
@ stdcall RtlSleepConditionVariableCS(ptr ptr ptr)
@ stdcall RtlSleepConditionVariableSRW(ptr ptr ptr long)
@ stdcall RtlSplay(ptr)
@ stub RtlStartRXact
# @ stub RtlStatMemoryStream
@ stdcall RtlStringFromGUID(ptr ptr)
@ stdcall RtlSubAuthorityCountSid(ptr)
@ stdcall RtlSubAuthoritySid(ptr long)
@ stdcall RtlSubtreePredecessor(ptr)
@ stdcall RtlSubtreeSuccessor(ptr)
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
@ stdcall RtlUTF8ToUnicodeN(ptr long ptr ptr long)
@ stdcall -fastcall -arch=i386 -norelay RtlUlongByteSwap(long)
@ stdcall -fastcall -arch=i386 -norelay RtlUlonglongByteSwap(int64)
# @ stub RtlUnhandledExceptionFilter2
# @ stub RtlUnhandledExceptionFilter
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
@ stdcall RtlUniform(ptr)
# @ stub RtlUnlockBootStatusData
@ stdcall RtlUnlockHeap(long)
# @ stub RtlUnlockMemoryStreamRegion
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
@ stdcall RtlUpdateTimer(ptr ptr long long)
@ stdcall RtlUpperChar(long)
@ stdcall RtlUpperString(ptr ptr)
@ stub RtlUsageHeap
@ stdcall -norelay RtlUserThreadStart(ptr ptr)
@ stdcall -fastcall -arch=i386 -norelay RtlUshortByteSwap(long)
@ stdcall RtlValidAcl(ptr)
@ stdcall RtlValidRelativeSecurityDescriptor(ptr long long)
@ stdcall RtlValidSecurityDescriptor(ptr)
@ stdcall RtlValidSid(ptr)
@ stdcall RtlValidateHeap(long long ptr)
@ stub RtlValidateProcessHeaps
# @ stub RtlValidateUnicodeString
@ stdcall RtlVerifyVersionInfo(ptr long int64)
@ stdcall -arch=!i386 RtlVirtualUnwind(long long long ptr ptr ptr ptr ptr)
@ stdcall -arch=!i386 RtlVirtualUnwind2(long long long ptr ptr ptr ptr ptr ptr ptr ptr ptr long)
@ stdcall RtlWaitOnAddress(ptr ptr long ptr)
@ stdcall RtlWakeAddressAll(ptr)
@ stdcall RtlWakeAddressSingle(ptr)
@ stdcall RtlWakeAllConditionVariable(ptr)
@ stdcall RtlWakeConditionVariable(ptr)
@ stdcall RtlWalkFrameChain(ptr long long)
@ stdcall RtlWalkHeap(long ptr)
@ stdcall RtlWow64EnableFsRedirection(long)
@ stdcall RtlWow64EnableFsRedirectionEx(long ptr)
@ stdcall -arch=win64 RtlWow64GetCpuAreaInfo(ptr long ptr)
@ stdcall -arch=win64 RtlWow64GetCurrentCpuArea(ptr ptr ptr)
@ stdcall RtlWow64GetCurrentMachine()
@ stdcall RtlWow64GetProcessMachines(long ptr ptr)
@ stdcall RtlWow64GetSharedInfoProcess(long ptr ptr)
@ stdcall -arch=win64 RtlWow64GetThreadContext(long ptr)
@ stdcall -arch=win64 RtlWow64GetThreadSelectorEntry(long ptr long ptr)
@ stdcall RtlWow64IsWowGuestMachineSupported(long ptr)
@ stdcall -arch=win64 RtlWow64PopAllCrossProcessWorkFromWorkList(ptr ptr)
@ stdcall -arch=win64 RtlWow64PopCrossProcessWorkFromFreeList(ptr)
@ stdcall -arch=win64 RtlWow64PushCrossProcessWorkOntoFreeList(ptr ptr)
@ stdcall -arch=win64 RtlWow64PushCrossProcessWorkOntoWorkList(ptr ptr ptr)
@ stdcall -arch=win64 RtlWow64RequestCrossProcessHeavyFlush(ptr)
@ stdcall -arch=win64 RtlWow64SetThreadContext(long ptr)
@ stub RtlWriteMemoryStream
@ stdcall RtlWriteRegistryValue(long ptr wstr long ptr long)
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
@ stdcall TpAllocIoCompletion(ptr ptr ptr ptr ptr)
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
@ stdcall TpCancelAsyncIoOperation(ptr)
@ stdcall TpDisassociateCallback(ptr)
@ stdcall TpIsTimerSet(ptr)
@ stdcall TpPostWork(ptr)
@ stdcall TpQueryPoolStackInformation(ptr ptr)
@ stdcall TpReleaseCleanupGroup(ptr)
@ stdcall TpReleaseCleanupGroupMembers(ptr long ptr)
@ stdcall TpReleaseIoCompletion(ptr)
@ stdcall TpReleasePool(ptr)
@ stdcall TpReleaseTimer(ptr)
@ stdcall TpReleaseWait(ptr)
@ stdcall TpReleaseWork(ptr)
@ stdcall TpSetPoolMaxThreads(ptr long)
@ stdcall TpSetPoolMinThreads(ptr long)
@ stdcall TpSetPoolStackInformation(ptr ptr)
@ stdcall TpSetTimer(ptr ptr long long)
@ stdcall TpSetWait(ptr long ptr)
@ stdcall TpSimpleTryPost(ptr ptr ptr)
@ stdcall TpStartAsyncIoOperation(ptr)
@ stdcall TpWaitForIoCompletion(ptr long)
@ stdcall TpWaitForTimer(ptr long)
@ stdcall TpWaitForWait(ptr long)
@ stdcall TpWaitForWork(ptr long)
@ stdcall -ret64 VerSetConditionMask(int64 long long)
@ stdcall WinSqmEndSession(long)
@ stdcall WinSqmIncrementDWORD(long long long)
@ stdcall WinSqmIsOptedIn()
@ stdcall WinSqmSetDWORD(ptr long long)
@ stdcall WinSqmSetIfMaxDWORD(long long long)
@ stdcall WinSqmStartSession(ptr long long)
@ extern -arch=win32 Wow64Transition
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
@ stdcall -private ZwAlertThreadByThreadId(ptr) NtAlertThreadByThreadId
@ stdcall -private ZwAllocateLocallyUniqueId(ptr) NtAllocateLocallyUniqueId
@ stdcall -private ZwAllocateReserveObject(ptr ptr long) NtAllocateReserveObject
# @ stub ZwAllocateUserPhysicalPages
@ stdcall -private ZwAllocateUuids(ptr ptr ptr ptr) NtAllocateUuids
@ stdcall -private ZwAllocateVirtualMemory(long ptr long ptr long long) NtAllocateVirtualMemory
@ stdcall -private ZwAllocateVirtualMemoryEx(long ptr ptr long long ptr long) NtAllocateVirtualMemoryEx
@ stdcall -private ZwAreMappedFilesTheSame(ptr ptr) NtAreMappedFilesTheSame
@ stdcall -private ZwAssignProcessToJobObject(long long) NtAssignProcessToJobObject
@ stdcall -private ZwCallbackReturn(ptr long long) NtCallbackReturn
# @ stub ZwCancelDeviceWakeupRequest
@ stdcall -private ZwCancelIoFile(long ptr) NtCancelIoFile
@ stdcall -private ZwCancelIoFileEx(long ptr ptr) NtCancelIoFileEx
@ stdcall -private ZwCancelSynchronousIoFile(long ptr ptr) NtCancelSynchronousIoFile
@ stdcall -private ZwCancelTimer(long ptr) NtCancelTimer
@ stdcall -private ZwClearEvent(long) NtClearEvent
@ stdcall -private ZwClose(long) NtClose
# @ stub ZwCloseObjectAuditAlarm
@ stdcall -private ZwCommitTransaction(long long) NtCommitTransaction
# @ stub ZwCompactKeys
@ stdcall -private ZwCompareObjects(ptr ptr) NtCompareObjects
@ stdcall -private ZwCompareTokens(ptr ptr ptr) NtCompareTokens
@ stdcall -private ZwCompleteConnectPort(ptr) NtCompleteConnectPort
# @ stub ZwCompressKey
@ stdcall -private ZwConnectPort(ptr ptr ptr ptr ptr ptr ptr ptr) NtConnectPort
@ stdcall -private ZwContinue(ptr long) NtContinue
@ stdcall -private ZwContinueEx(ptr ptr) NtContinueEx
@ stdcall -private ZwConvertBetweenAuxiliaryCounterAndPerformanceCounter(long ptr ptr ptr) NtConvertBetweenAuxiliaryCounterAndPerformanceCounter
@ stdcall -private ZwCreateDebugObject(ptr long ptr long) NtCreateDebugObject
@ stdcall -private ZwCreateDirectoryObject(ptr long ptr) NtCreateDirectoryObject
@ stdcall -private ZwCreateEvent(ptr long ptr long long) NtCreateEvent
# @ stub ZwCreateEventPair
@ stdcall -private ZwCreateFile(ptr long ptr ptr ptr long long long long ptr long) NtCreateFile
@ stdcall -private ZwCreateIoCompletion(ptr long ptr long) NtCreateIoCompletion
@ stdcall -private ZwCreateJobObject(ptr long ptr) NtCreateJobObject
# @ stub ZwCreateJobSet
@ stdcall -private ZwCreateKey(ptr long ptr long ptr long ptr) NtCreateKey
@ stdcall -private ZwCreateKeyTransacted(ptr long ptr long ptr long long ptr) NtCreateKeyTransacted
@ stdcall -private ZwCreateKeyedEvent(ptr long ptr long) NtCreateKeyedEvent
@ stdcall -private ZwCreateLowBoxToken(ptr long long ptr ptr long ptr long ptr) NtCreateLowBoxToken
@ stdcall -private ZwCreateMailslotFile(ptr long ptr ptr long long long ptr) NtCreateMailslotFile
@ stdcall -private ZwCreateMutant(ptr long ptr long) NtCreateMutant
@ stdcall -private ZwCreateNamedPipeFile(ptr long ptr ptr long long long long long long long long long ptr) NtCreateNamedPipeFile
@ stdcall -private ZwCreatePagingFile(ptr ptr ptr ptr) NtCreatePagingFile
@ stdcall -private ZwCreatePort(ptr ptr long long ptr) NtCreatePort
# @ stub ZwCreateProcess
# @ stub ZwCreateProcessEx
# @ stub ZwCreateProfile
@ stdcall -private ZwCreateSection(ptr long ptr ptr long long long) NtCreateSection
@ stdcall -private ZwCreateSectionEx(ptr long ptr ptr long long long ptr long) NtCreateSectionEx
@ stdcall -private ZwCreateSemaphore(ptr long ptr long long) NtCreateSemaphore
@ stdcall -private ZwCreateSymbolicLinkObject(ptr long ptr ptr) NtCreateSymbolicLinkObject
@ stdcall -private ZwCreateThread(ptr long ptr long ptr ptr ptr long) NtCreateThread
@ stdcall -private ZwCreateThreadEx(ptr long ptr long ptr ptr long long long long ptr) NtCreateThreadEx
@ stdcall -private ZwCreateTimer(ptr long ptr long) NtCreateTimer
@ stdcall -private ZwCreateToken(ptr long ptr long ptr ptr ptr ptr ptr ptr ptr ptr ptr) NtCreateToken
@ stdcall -private ZwCreateTransaction(ptr long ptr ptr long long long long ptr ptr) NtCreateTransaction
@ stdcall -private ZwCreateUserProcess(ptr ptr long long ptr ptr long long ptr ptr ptr) NtCreateUserProcess
# @ stub ZwCreateWaitablePort
@ stdcall -private ZwDebugActiveProcess(long long) NtDebugActiveProcess
@ stdcall -private ZwDebugContinue(long ptr long) NtDebugContinue
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
# @ stub ZwEnumerateBus
@ stdcall -private ZwEnumerateKey(long long long ptr long ptr) NtEnumerateKey
# @ stub ZwEnumerateSystemEnvironmentValuesEx
@ stdcall -private ZwEnumerateValueKey(long long long ptr long ptr) NtEnumerateValueKey
# @ stub ZwExtendSection
@ stdcall -private ZwFilterToken(long long ptr ptr ptr ptr) NtFilterToken
@ stdcall -private ZwFindAtom(ptr long ptr) NtFindAtom
@ stdcall -private ZwFlushBuffersFile(long ptr) NtFlushBuffersFile
@ stdcall -private ZwFlushBuffersFileEx(long long ptr long ptr) NtFlushBuffersFileEx
@ stdcall -private ZwFlushInstructionCache(long ptr long) NtFlushInstructionCache
@ stdcall -private ZwFlushKey(long) NtFlushKey
@ stdcall -private ZwFlushProcessWriteBuffers() NtFlushProcessWriteBuffers
@ stdcall -private ZwFlushVirtualMemory(long ptr ptr long) NtFlushVirtualMemory
# @ stub ZwFlushWriteBuffer
# @ stub ZwFreeUserPhysicalPages
@ stdcall -private ZwFreeVirtualMemory(long ptr ptr long) NtFreeVirtualMemory
@ stdcall -private ZwFsControlFile(long long ptr ptr ptr long ptr long ptr long) NtFsControlFile
@ stdcall -private -norelay ZwGetContextThread(long ptr) NtGetContextThread
@ stdcall -private ZwGetCurrentProcessorNumber() NtGetCurrentProcessorNumber
# @ stub ZwGetDevicePowerState
@ stdcall -private ZwGetNextProcess(ptr long long long ptr) NtGetNextProcess
@ stdcall -private ZwGetNextThread(ptr ptr long long long ptr) NtGetNextThread
@ stdcall -private ZwGetNlsSectionPtr(long long long ptr ptr) NtGetNlsSectionPtr
# @ stub ZwGetPlugPlayEvent
@ stdcall -private ZwGetWriteWatch(long long ptr long ptr ptr ptr) NtGetWriteWatch
@ stdcall -private ZwImpersonateAnonymousToken(long) NtImpersonateAnonymousToken
# @ stub ZwImpersonateClientOfPort
# @ stub ZwImpersonateThread
@ stdcall -private ZwInitializeNlsFiles(ptr ptr ptr) NtInitializeNlsFiles
# @ stub ZwInitializeRegistry
@ stdcall -private ZwInitiatePowerAction(long long long long) NtInitiatePowerAction
@ stdcall -private ZwIsProcessInJob(long long) NtIsProcessInJob
# @ stub ZwIsSystemResumeAutomatic
@ stdcall -private ZwListenPort(ptr ptr) NtListenPort
@ stdcall -private ZwLoadDriver(ptr) NtLoadDriver
@ stdcall -private ZwLoadKey2(ptr ptr long) NtLoadKey2
@ stdcall -private ZwLoadKey(ptr ptr) NtLoadKey
@ stdcall -private ZwLoadKeyEx(ptr ptr long long long long ptr ptr) NtLoadKeyEx
@ stdcall -private ZwLockFile(long long ptr ptr ptr ptr ptr ptr long long) NtLockFile
# @ stub ZwLockProductActivationKeys
# @ stub ZwLockRegistryKey
@ stdcall -private ZwLockVirtualMemory(long ptr ptr long) NtLockVirtualMemory
@ stdcall -private ZwMakePermanentObject(long) NtMakePermanentObject
@ stdcall -private ZwMakeTemporaryObject(long) NtMakeTemporaryObject
# @ stub ZwMapUserPhysicalPages
# @ stub ZwMapUserPhysicalPagesScatter
@ stdcall -private ZwMapViewOfSection(long long ptr long long ptr ptr long long long) NtMapViewOfSection
@ stdcall -private ZwMapViewOfSectionEx(long long ptr ptr ptr long long ptr long) NtMapViewOfSectionEx
# @ stub ZwModifyBootEntry
@ stdcall -private ZwNotifyChangeDirectoryFile(long long ptr ptr ptr ptr long long long) NtNotifyChangeDirectoryFile
@ stdcall -private ZwNotifyChangeKey(long long ptr ptr ptr long long ptr long long) NtNotifyChangeKey
@ stdcall -private ZwNotifyChangeMultipleKeys(long long ptr long ptr ptr ptr long long ptr long long) NtNotifyChangeMultipleKeys
@ stdcall -private ZwOpenDirectoryObject(ptr long ptr) NtOpenDirectoryObject
@ stdcall -private ZwOpenEvent(ptr long ptr) NtOpenEvent
# @ stub ZwOpenEventPair
@ stdcall -private ZwOpenFile(ptr long ptr ptr long long) NtOpenFile
@ stdcall -private ZwOpenIoCompletion(ptr long ptr) NtOpenIoCompletion
@ stdcall -private ZwOpenJobObject(ptr long ptr) NtOpenJobObject
@ stdcall -private ZwOpenKey(ptr long ptr) NtOpenKey
@ stdcall -private ZwOpenKeyEx(ptr long ptr long) NtOpenKeyEx
@ stdcall -private ZwOpenKeyTransacted(ptr long ptr long) NtOpenKeyTransacted
@ stdcall -private ZwOpenKeyTransactedEx(ptr long ptr long long) NtOpenKeyTransactedEx
@ stdcall -private ZwOpenKeyedEvent(ptr long ptr) NtOpenKeyedEvent
@ stdcall -private ZwOpenMutant(ptr long ptr) NtOpenMutant
# @ stub ZwOpenObjectAuditAlarm
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
# @ stub ZwPlugPlayControl
@ stdcall -private ZwPowerInformation(long ptr long ptr long) NtPowerInformation
@ stdcall -private ZwPrivilegeCheck(ptr ptr ptr) NtPrivilegeCheck
# @ stub ZwPrivilegeObjectAuditAlarm
# @ stub ZwPrivilegedServiceAuditAlarm
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
# @ stub ZwQueryInformationPort
@ stdcall -private ZwQueryInformationProcess(long long ptr long ptr) NtQueryInformationProcess
@ stdcall -private ZwQueryInformationThread(long long ptr long ptr) NtQueryInformationThread
@ stdcall -private ZwQueryInformationToken(long long ptr long ptr) NtQueryInformationToken
@ stdcall -private ZwQueryInstallUILanguage(ptr) NtQueryInstallUILanguage
# @ stub ZwQueryIntervalProfile
@ stdcall -private ZwQueryIoCompletion(long long ptr long ptr) NtQueryIoCompletion
@ stdcall -private ZwQueryKey(long long ptr long ptr) NtQueryKey
@ stdcall -private ZwQueryLicenseValue(ptr ptr ptr long ptr) NtQueryLicenseValue
@ stdcall -private ZwQueryMultipleValueKey(long ptr long ptr long ptr) NtQueryMultipleValueKey
@ stdcall -private ZwQueryMutant(long long ptr long ptr) NtQueryMutant
@ stdcall -private ZwQueryObject(long long ptr long ptr) NtQueryObject
# @ stub ZwQueryOpenSubKeys
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
@ stdcall -private ZwQueueApcThreadEx(long long ptr long long long) NtQueueApcThreadEx
@ stdcall -private ZwRaiseException(ptr ptr long) NtRaiseException
@ stdcall -private ZwRaiseHardError(long long long ptr long ptr) NtRaiseHardError
@ stdcall -private ZwReadFile(long long ptr ptr ptr ptr long ptr ptr) NtReadFile
@ stdcall -private ZwReadFileScatter(long long ptr ptr ptr ptr long ptr ptr) NtReadFileScatter
# @ stub ZwReadRequestData
@ stdcall -private ZwReadVirtualMemory(long ptr ptr long ptr) NtReadVirtualMemory
# @ stub ZwRegisterNewDevice
@ stdcall -private ZwRegisterThreadTerminatePort(ptr) NtRegisterThreadTerminatePort
@ stdcall -private ZwReleaseKeyedEvent(long ptr long ptr) NtReleaseKeyedEvent
@ stdcall -private ZwReleaseMutant(long ptr) NtReleaseMutant
# @ stub ZwReleaseProcessMutant
@ stdcall -private ZwReleaseSemaphore(long long ptr) NtReleaseSemaphore
@ stdcall -private ZwRemoveIoCompletion(ptr ptr ptr ptr ptr) NtRemoveIoCompletion
@ stdcall -private ZwRemoveIoCompletionEx(ptr ptr long ptr ptr long) NtRemoveIoCompletionEx
@ stdcall -private ZwRemoveProcessDebug(long long) NtRemoveProcessDebug
@ stdcall -private ZwRenameKey(long ptr) NtRenameKey
@ stdcall -private ZwReplaceKey(ptr long ptr) NtReplaceKey
# @ stub ZwReplyPort
@ stdcall -private ZwReplyWaitReceivePort(ptr ptr ptr ptr) NtReplyWaitReceivePort
# @ stub ZwReplyWaitReceivePortEx
# @ stub ZwReplyWaitReplyPort
# @ stub ZwRequestDeviceWakeup
# @ stub ZwRequestPort
@ stdcall -private ZwRequestWaitReplyPort(ptr ptr ptr) NtRequestWaitReplyPort
# @ stub ZwRequestWakeupLatency
@ stdcall -private ZwResetEvent(long ptr) NtResetEvent
@ stdcall -private ZwResetWriteWatch(long ptr long) NtResetWriteWatch
@ stdcall -private ZwRestoreKey(long long long) NtRestoreKey
@ stdcall -private ZwResumeProcess(long) NtResumeProcess
@ stdcall -private ZwResumeThread(long ptr) NtResumeThread
@ stdcall -private ZwRollbackTransaction(long long) NtRollbackTransaction
@ stdcall -private ZwSaveKey(long long) NtSaveKey
# @ stub ZwSaveKeyEx
# @ stub ZwSaveMergedKeys
@ stdcall -private ZwSecureConnectPort(ptr ptr ptr ptr ptr ptr ptr ptr ptr) NtSecureConnectPort
# @ stub ZwSetBootEntryOrder
# @ stub ZwSetBootOptions
@ stdcall -private ZwSetContextThread(long ptr) NtSetContextThread
@ stdcall -private ZwSetDebugFilterState(long long long) NtSetDebugFilterState
# @ stub ZwSetDefaultHardErrorPort
@ stdcall -private ZwSetDefaultLocale(long long) NtSetDefaultLocale
@ stdcall -private ZwSetDefaultUILanguage(long) NtSetDefaultUILanguage
@ stdcall -private ZwSetEaFile(long ptr ptr long) NtSetEaFile
@ stdcall -private ZwSetEvent(long ptr) NtSetEvent
# @ stub ZwSetEventBoostPriority
# @ stub ZwSetHighEventPair
# @ stub ZwSetHighWaitLowEventPair
# @ stub ZwSetHighWaitLowThread
@ stdcall -private ZwSetInformationDebugObject(long long ptr long ptr) NtSetInformationDebugObject
@ stdcall -private ZwSetInformationFile(long ptr ptr long long) NtSetInformationFile
@ stdcall -private ZwSetInformationJobObject(long long ptr long) NtSetInformationJobObject
@ stdcall -private ZwSetInformationKey(long long ptr long) NtSetInformationKey
@ stdcall -private ZwSetInformationObject(long long ptr long) NtSetInformationObject
@ stdcall -private ZwSetInformationProcess(long long ptr long) NtSetInformationProcess
@ stdcall -private ZwSetInformationThread(long long ptr long) NtSetInformationThread
@ stdcall -private ZwSetInformationToken(long long ptr long) NtSetInformationToken
@ stdcall -private ZwSetInformationVirtualMemory(long long ptr ptr ptr long) NtSetInformationVirtualMemory
@ stdcall -private ZwSetIntervalProfile(long long) NtSetIntervalProfile
@ stdcall -private ZwSetIoCompletion(ptr long long long long) NtSetIoCompletion
@ stdcall -private ZwSetIoCompletionEx(ptr ptr long long long long) NtSetIoCompletionEx
@ stdcall -private ZwSetLdtEntries(long int64 long int64) NtSetLdtEntries
# @ stub ZwSetLowEventPair
# @ stub ZwSetLowWaitHighEventPair
# @ stub ZwSetLowWaitHighThread
# @ stub ZwSetQuotaInformationFile
@ stdcall -private ZwSetSecurityObject(long long ptr) NtSetSecurityObject
# @ stub ZwSetSystemEnvironmentValue
# @ stub ZwSetSystemEnvironmentValueEx
@ stdcall -private ZwSetSystemInformation(long ptr long) NtSetSystemInformation
# @ stub ZwSetSystemPowerState
@ stdcall -private ZwSetSystemTime(ptr ptr) NtSetSystemTime
@ stdcall -private ZwSetThreadExecutionState(long ptr) NtSetThreadExecutionState
@ stdcall -private ZwSetTimer(long ptr ptr ptr long long ptr) NtSetTimer
@ stdcall -private ZwSetTimerResolution(long long ptr) NtSetTimerResolution
# @ stub ZwSetUuidSeed
@ stdcall -private ZwSetValueKey(long ptr long long ptr long) NtSetValueKey
@ stdcall -private ZwSetVolumeInformationFile(long ptr ptr long long) NtSetVolumeInformationFile
@ stdcall -private ZwShutdownSystem(long) NtShutdownSystem
@ stdcall -private ZwSignalAndWaitForSingleObject(long long long ptr) NtSignalAndWaitForSingleObject
# @ stub ZwStartProfile
# @ stub ZwStopProfile
@ stdcall -private ZwSuspendProcess(long) NtSuspendProcess
@ stdcall -private ZwSuspendThread(long ptr) NtSuspendThread
@ stdcall -private ZwSystemDebugControl(long ptr long ptr long ptr) NtSystemDebugControl
@ stdcall -private ZwTerminateJobObject(long long) NtTerminateJobObject
@ stdcall -private ZwTerminateProcess(long long) NtTerminateProcess
@ stdcall -private ZwTerminateThread(long long) NtTerminateThread
@ stdcall -private ZwTestAlert() NtTestAlert
@ stdcall -private ZwTraceControl(long ptr long ptr long long) NtTraceControl
# @ stub ZwTraceEvent
# @ stub ZwTranslateFilePath
@ stdcall -private ZwUnloadDriver(ptr) NtUnloadDriver
@ stdcall -private ZwUnloadKey(ptr) NtUnloadKey
# @ stub ZwUnloadKeyEx
@ stdcall -private ZwUnlockFile(long ptr ptr ptr ptr) NtUnlockFile
@ stdcall -private ZwUnlockVirtualMemory(long ptr ptr long) NtUnlockVirtualMemory
@ stdcall -private ZwUnmapViewOfSection(long ptr) NtUnmapViewOfSection
@ stdcall -private ZwUnmapViewOfSectionEx(long ptr long) NtUnmapViewOfSectionEx
# @ stub ZwVdmControl
# @ stub ZwW32Call
@ stdcall -private ZwWaitForAlertByThreadId(ptr ptr) NtWaitForAlertByThreadId
@ stdcall -private ZwWaitForDebugEvent(long long ptr ptr) NtWaitForDebugEvent
@ stdcall -private ZwWaitForKeyedEvent(long ptr long ptr) NtWaitForKeyedEvent
@ stdcall -private ZwWaitForMultipleObjects(long ptr long long ptr) NtWaitForMultipleObjects
# @ stub ZwWaitForProcessMutant
@ stdcall -private ZwWaitForSingleObject(long long ptr) NtWaitForSingleObject
# @ stub ZwWaitHighEventPair
# @ stub ZwWaitLowEventPair
@ stdcall -private -arch=win32 ZwWow64AllocateVirtualMemory64(long ptr int64 ptr long long) NtWow64AllocateVirtualMemory64
@ stdcall -private -arch=win32 ZwWow64GetNativeSystemInformation(long ptr long ptr) NtWow64GetNativeSystemInformation
@ stdcall -private -arch=win32 ZwWow64IsProcessorFeaturePresent(long) NtWow64IsProcessorFeaturePresent
@ stdcall -private -arch=win32 ZwWow64QueryInformationProcess64(long long ptr long ptr) NtWow64QueryInformationProcess64
@ stdcall -private -arch=win32 ZwWow64ReadVirtualMemory64(long int64 ptr int64 ptr) NtWow64ReadVirtualMemory64
@ stdcall -private -arch=win32 ZwWow64WriteVirtualMemory64(long int64 ptr int64 ptr) NtWow64WriteVirtualMemory64
@ stdcall -private ZwWriteFile(long long ptr ptr ptr ptr long ptr ptr) NtWriteFile
@ stdcall -private ZwWriteFileGather(long long ptr ptr ptr ptr long ptr ptr) NtWriteFileGather
# @ stub ZwWriteRequestData
@ stdcall -private ZwWriteVirtualMemory(long ptr ptr long ptr) NtWriteVirtualMemory
@ stdcall -private ZwYieldExecution() NtYieldExecution
@ cdecl -private -arch=i386 _CIcos()
@ cdecl -private -arch=i386 _CIlog()
@ cdecl -private -arch=i386 _CIpow()
@ cdecl -private -arch=i386 _CIsin()
@ cdecl -private -arch=i386 _CIsqrt()
@ stdcall -arch=!i386 __C_specific_handler(ptr long ptr ptr)
@ cdecl -arch=!i386 -norelay __chkstk()
@ cdecl -arch=arm64ec -norelay __chkstk_arm64ec()
@ cdecl __isascii(long)
@ cdecl __iscsym(long)
@ cdecl __iscsymf(long)
@ stdcall -arch=arm __jump_unwind(ptr ptr) _local_unwind
@ cdecl __toascii(long)
@ cdecl -norelay -arch=i386 -ret64 _alldiv(int64 int64)
@ cdecl -arch=i386 -norelay _alldvrm(int64 int64)
@ cdecl -norelay -arch=i386 -ret64 _allmul(int64 int64)
@ cdecl -arch=i386 -norelay _alloca_probe()
@ cdecl -norelay -arch=i386 -ret64 _allrem(int64 int64)
@ cdecl -norelay -arch=i386 -ret64 _allshl(int64 long)
@ cdecl -norelay -arch=i386 -ret64 _allshr(int64 long)
@ cdecl -ret64 _atoi64(str)
@ cdecl -norelay -arch=i386 -ret64 _aulldiv(int64 int64)
@ cdecl -arch=i386 -norelay _aulldvrm(int64 int64)
@ cdecl -norelay -arch=i386 -ret64 _aullrem(int64 int64)
@ cdecl -norelay -arch=i386 -ret64 _aullshr(int64 long)
@ cdecl -arch=i386 -norelay _chkstk()
@ cdecl _errno()
@ stub _fltused
@ cdecl -arch=i386 -ret64 _ftol()
@ cdecl -arch=i386 -ret64 _ftol2() _ftol
@ cdecl -arch=i386 -ret64 _ftol2_sse() _ftol # FIXME
@ cdecl _i64toa(int64 ptr long)
@ cdecl _i64toa_s(int64 ptr long long)
@ cdecl _i64tow(int64 ptr long)
@ cdecl _i64tow_s(int64 ptr long long)
@ cdecl _itoa(long ptr long)
@ cdecl _itoa_s(long ptr long long)
@ cdecl _itow(long ptr long)
@ cdecl _itow_s(long ptr long long)
@ cdecl _lfind(ptr ptr ptr long ptr)
@ stdcall -arch=win64 _local_unwind(ptr ptr)
@ cdecl _ltoa(long ptr long)
@ cdecl _ltoa_s(long ptr long long)
@ cdecl _ltow(long ptr long)
@ cdecl _ltow_s(long ptr long long)
@ cdecl _makepath_s(ptr long str str str str)
@ cdecl _memccpy(ptr ptr long long)
@ cdecl _memicmp(str str long)
@ cdecl -norelay -arch=arm,x86_64 _setjmp(ptr ptr) NTDLL__setjmpex
@ cdecl -norelay -arch=!i386 _setjmpex(ptr ptr) NTDLL__setjmpex
@ varargs _snprintf(ptr long str) NTDLL__snprintf
@ varargs _snprintf_s(ptr long long str)
@ varargs _snwprintf(ptr long wstr)
@ varargs _snwprintf_s(ptr long long wstr)
@ cdecl _splitpath(str ptr ptr ptr ptr)
@ cdecl _splitpath_s(str ptr long ptr long ptr long ptr long)
@ cdecl _strcmpi(str str) _stricmp
@ cdecl _stricmp(str str)
@ cdecl _strlwr(str)
@ cdecl _strlwr_s(str long)
@ cdecl _strnicmp(str str long)
@ cdecl _strupr(str)
@ cdecl _strupr_s(str long)
@ varargs _swprintf(ptr wstr) NTDLL_swprintf
@ cdecl _tolower(long)
@ cdecl _toupper(long)
@ cdecl _ui64toa(int64 ptr long)
@ cdecl _ui64toa_s(int64 ptr long long)
@ cdecl _ui64tow(int64 ptr long)
@ cdecl _ui64tow_s(int64 ptr long long)
@ cdecl _ultoa(long ptr long)
@ cdecl _ultoa_s(long ptr long long)
@ cdecl _ultow(long ptr long)
@ cdecl _ultow_s(long ptr long long)
@ cdecl _vscprintf(str ptr)
@ cdecl _vscwprintf(wstr ptr)
@ cdecl -norelay _vsnprintf(ptr long str ptr)
@ cdecl _vsnprintf_s(ptr long str ptr)
@ cdecl _vsnwprintf(ptr long wstr ptr)
@ cdecl _vsnwprintf_s(ptr long long wstr ptr)
@ cdecl _vswprintf(ptr wstr ptr)
@ cdecl _wcsicmp(wstr wstr)
@ cdecl _wcslwr(wstr)
@ cdecl _wcslwr_s(wstr long)
@ cdecl _wcsnicmp(wstr wstr long)
@ cdecl -ret64 _wcstoi64(wstr ptr long)
@ cdecl -ret64 _wcstoui64(wstr ptr long)
@ cdecl _wcsupr(wstr)
@ cdecl _wcsupr_s(wstr long)
@ cdecl _wmakepath_s(ptr long wstr wstr wstr wstr)
@ cdecl _wsplitpath_s(wstr ptr long ptr long ptr long ptr long)
@ cdecl _wtoi(wstr)
@ cdecl -ret64 _wtoi64(wstr)
@ cdecl _wtol(wstr)
@ cdecl abs(long)
@ cdecl atan(double)
@ cdecl atan2(double double)
@ cdecl atoi(str)
@ cdecl atol(str)
@ cdecl bsearch(ptr ptr long long ptr)
@ cdecl bsearch_s(ptr ptr long long ptr ptr)
@ cdecl ceil(double)
@ cdecl cos(double)
@ cdecl fabs(double)
@ cdecl floor(double)
@ cdecl isalnum(long)
@ cdecl isalpha(long)
@ cdecl iscntrl(long)
@ cdecl isdigit(long)
@ cdecl isgraph(long)
@ cdecl islower(long)
@ cdecl isprint(long)
@ cdecl ispunct(long)
@ cdecl isspace(long)
@ cdecl isupper(long)
@ cdecl iswalnum(long)
@ cdecl iswalpha(long)
@ cdecl iswascii(long)
@ cdecl iswctype(long long)
@ cdecl iswdigit(long)
@ cdecl iswgraph(long)
@ cdecl iswlower(long)
@ cdecl iswprint(long)
@ cdecl iswspace(long)
@ cdecl iswxdigit(long)
@ cdecl isxdigit(long)
@ cdecl labs(long) abs
@ cdecl log(double)
@ cdecl -arch=!i386 longjmp(ptr long) NTDLL_longjmp
@ cdecl mbstowcs(ptr str long)
@ cdecl memchr(ptr long long)
@ cdecl memcmp(ptr ptr long)
@ cdecl memcpy(ptr ptr long)
@ cdecl memcpy_s(ptr long ptr long)
@ cdecl memmove(ptr ptr long)
@ cdecl memmove_s(ptr long ptr long)
@ cdecl memset(ptr long long)
@ cdecl pow(double double)
@ cdecl qsort(ptr long long ptr)
@ cdecl qsort_s(ptr long long ptr ptr)
@ cdecl sin(double)
@ varargs sprintf(ptr str) NTDLL_sprintf
@ varargs sprintf_s(ptr long str)
@ cdecl sqrt(double)
@ varargs sscanf(str str)
@ cdecl strcat(str str)
@ cdecl strcat_s(str long str)
@ cdecl strchr(str long)
@ cdecl strcmp(str str)
@ cdecl strcpy(ptr str)
@ cdecl strcpy_s(ptr long str)
@ cdecl strcspn(str str)
@ cdecl strlen(str)
@ cdecl strncat(str str long)
@ cdecl strncat_s(str long str long)
@ cdecl strncmp(str str long)
@ cdecl strncpy(ptr str long)
@ cdecl strncpy_s(ptr long str long)
@ cdecl strnlen(ptr long)
@ cdecl strpbrk(str str)
@ cdecl strrchr(str long)
@ cdecl strspn(str str)
@ cdecl strstr(str str)
@ cdecl strtok_s(str str ptr)
@ cdecl strtol(str ptr long)
@ cdecl strtoul(str ptr long)
@ varargs swprintf(ptr wstr) NTDLL_swprintf
@ varargs swprintf_s(ptr long wstr)
@ cdecl tan(double)
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
@ cdecl wcsnlen(ptr long)
@ cdecl wcspbrk(wstr wstr)
@ cdecl wcsrchr(wstr long)
@ cdecl wcsspn(wstr wstr)
@ cdecl wcsstr(wstr wstr)
@ cdecl wcstok(wstr wstr)
@ cdecl wcstok_s(wstr wstr ptr)
@ cdecl wcstol(wstr ptr long)
@ cdecl wcstombs(ptr ptr long)
@ cdecl wcstoul(wstr ptr long)

##################
# Wine extensions
#
# All functions must be prefixed with '__wine_' (for internal functions)
# or 'wine_' (for user-visible functions) to avoid namespace conflicts.

# Server interface
@ cdecl -norelay wine_server_call(ptr)
@ cdecl wine_server_fd_to_handle(long long long ptr)
@ cdecl wine_server_handle_to_fd(long long ptr ptr)

# Unix interface
@ stdcall __wine_unix_spawnvp(long ptr)
@ stdcall __wine_ctrl_routine(ptr)
@ extern -private __wine_syscall_dispatcher
@ extern -private __wine_unix_call_dispatcher
@ extern -private -arch=arm64ec __wine_unix_call_dispatcher_arm64ec
@ extern -private __wine_unixlib_handle

# Debugging
@ stdcall -norelay __wine_dbg_write(ptr long)
@ cdecl -norelay __wine_dbg_get_channel_flags(ptr)
@ cdecl -norelay __wine_dbg_header(long long str)
@ cdecl -norelay __wine_dbg_output(str)
@ cdecl -norelay __wine_dbg_strdup(str)

# Version
@ cdecl wine_get_version()
@ cdecl wine_get_build_id()
@ cdecl wine_get_host_version(ptr ptr)

# Filesystem
@ stdcall -syscall wine_nt_to_unix_file_name(ptr ptr ptr long)
@ stdcall -syscall wine_unix_to_nt_file_name(str ptr ptr)
