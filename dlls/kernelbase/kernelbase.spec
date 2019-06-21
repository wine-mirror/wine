@ stdcall AccessCheck(ptr long long ptr ptr ptr ptr ptr) advapi32.AccessCheck
@ stdcall AccessCheckAndAuditAlarmW(wstr ptr wstr wstr ptr long ptr long ptr ptr ptr) advapi32.AccessCheckAndAuditAlarmW
@ stdcall AccessCheckByType(ptr ptr long long ptr long ptr ptr ptr ptr ptr) advapi32.AccessCheckByType
@ stub AccessCheckByTypeAndAuditAlarmW
@ stub AccessCheckByTypeResultList
@ stub AccessCheckByTypeResultListAndAuditAlarmByHandleW
@ stub AccessCheckByTypeResultListAndAuditAlarmW
@ stdcall AcquireSRWLockExclusive(ptr) kernel32.AcquireSRWLockExclusive
@ stdcall AcquireSRWLockShared(ptr) kernel32.AcquireSRWLockShared
# @ stub AcquireStateLock
@ stdcall ActivateActCtx(ptr ptr) kernel32.ActivateActCtx
@ stdcall AddAccessAllowedAce(ptr long long ptr) advapi32.AddAccessAllowedAce
@ stdcall AddAccessAllowedAceEx(ptr long long long ptr) advapi32.AddAccessAllowedAceEx
@ stdcall AddAccessAllowedObjectAce(ptr long long long ptr ptr ptr) advapi32.AddAccessAllowedObjectAce
@ stdcall AddAccessDeniedAce(ptr long long ptr) advapi32.AddAccessDeniedAce
@ stdcall AddAccessDeniedAceEx(ptr long long long ptr) advapi32.AddAccessDeniedAceEx
@ stdcall AddAccessDeniedObjectAce(ptr long long long ptr ptr ptr) advapi32.AddAccessDeniedObjectAce
@ stdcall AddAce(ptr long long ptr long) advapi32.AddAce
@ stdcall AddAuditAccessAce(ptr long long ptr long long) advapi32.AddAuditAccessAce
@ stdcall AddAuditAccessAceEx(ptr long long long ptr long long) advapi32.AddAuditAccessAceEx
@ stdcall AddAuditAccessObjectAce(ptr long long long ptr ptr ptr long long) advapi32.AddAuditAccessObjectAce
@ stdcall AddDllDirectory(wstr) kernel32.AddDllDirectory
@ stdcall AddMandatoryAce(ptr long long long ptr) advapi32.AddMandatoryAce
@ stdcall AddRefActCtx(ptr) kernel32.AddRefActCtx
# @ stub AddResourceAttributeAce
# @ stub AddSIDToBoundaryDescriptor
# @ stub AddScopedPolicyIDAce
@ stdcall AddVectoredContinueHandler(long ptr) kernel32.AddVectoredContinueHandler
@ stdcall AddVectoredExceptionHandler(long ptr) kernel32.AddVectoredExceptionHandler
@ stdcall AdjustTokenGroups(long long ptr long ptr ptr) advapi32.AdjustTokenGroups
@ stdcall AdjustTokenPrivileges(long long ptr long ptr ptr) advapi32.AdjustTokenPrivileges
@ stdcall AllocConsole() kernel32.AllocConsole
@ stdcall AllocateAndInitializeSid(ptr long long long long long long long long long ptr) advapi32.AllocateAndInitializeSid
@ stdcall AllocateLocallyUniqueId(ptr) advapi32.AllocateLocallyUniqueId
@ stdcall AllocateUserPhysicalPages(long ptr ptr) kernel32.AllocateUserPhysicalPages
# @ stub AllocateUserPhysicalPagesNuma
# @ stub AppContainerDeriveSidFromMoniker
# @ stub AppContainerFreeMemory
# @ stub AppContainerLookupDisplayNameMrtReference
# @ stub AppContainerLookupMoniker
# @ stub AppContainerRegisterSid
# @ stub AppContainerUnregisterSid
# @ stub AppPolicyGetClrCompat
# @ stub AppPolicyGetCreateFileAccess
# @ stub AppPolicyGetLifecycleManagement
# @ stub AppPolicyGetMediaFoundationCodecLoading
@ stdcall AppPolicyGetProcessTerminationMethod(ptr ptr)
@ stdcall AppPolicyGetShowDeveloperDiagnostic(ptr ptr)
@ stdcall AppPolicyGetThreadInitializationType(ptr ptr)
@ stdcall AppPolicyGetWindowingModel(ptr ptr)
# @ stub AppXFreeMemory
# @ stub AppXGetApplicationData
# @ stub AppXGetDevelopmentMode
# @ stub AppXGetOSMaxVersionTested
# @ stub AppXGetOSMinVersion
# @ stub AppXGetPackageCapabilities
# @ stub AppXGetPackageSid
# @ stub AppXLookupDisplayName
# @ stub AppXLookupMoniker
# @ stub AppXPostSuccessExtension
# @ stub AppXPreCreationExtension
# @ stub AppXReleaseAppXContext
# @ stub AppXUpdatePackageCapabilities
# @ stub ApplicationUserModelIdFromProductId
@ stdcall AreAllAccessesGranted(long long) advapi32.AreAllAccessesGranted
@ stdcall AreAnyAccessesGranted(long long) advapi32.AreAnyAccessesGranted
@ stdcall AreFileApisANSI() kernel32.AreFileApisANSI
# @ stub AreThereVisibleLogoffScriptsInternal
# @ stub AreThereVisibleShutdownScriptsInternal
@ stdcall AttachConsole(long) kernel32.AttachConsole
@ stub BaseCheckAppcompatCache
# @ stub BaseCheckAppcompatCacheEx
@ stub BaseCleanupAppcompatCacheSupport
@ stub BaseDllFreeResourceId
@ stub BaseDllMapResourceIdW
@ stub BaseDumpAppcompatCache
@ stdcall BaseFlushAppcompatCache() kernel32.BaseFlushAppcompatCache
# @ stub BaseFormatObjectAttributes
# @ stub BaseFreeAppCompatDataForProcess
# @ stub BaseGetNamedObjectDirectory
@ stub BaseGetProcessDllPath
@ stub BaseGetProcessExePath
@ stub BaseInitAppcompatCacheSupport
@ stub BaseInvalidateDllSearchPathCache
@ stub BaseInvalidateProcessSearchPathCache
# @ stub BaseIsAppcompatInfrastructureDisabled
# @ stub BaseMarkFileForDelete
# @ stub BaseReadAppCompatDataForProcess
@ stub BaseReleaseProcessDllPath
@ stub BaseReleaseProcessExePath
@ stub BaseUpdateAppcompatCache
# @ stub BasepAdjustObjectAttributesForPrivateNamespace
# @ stub BasepCopyFileCallback
# @ stub BasepCopyFileExW
# @ stub BasepNotifyTrackingService
@ stdcall Beep(long long) kernel32.Beep
@ stub BemCopyReference
@ stub BemCreateContractFrom
@ stub BemCreateReference
@ stub BemFreeContract
@ stub BemFreeReference
# @ stub CLOSE_LOCAL_HANDLE_INTERNAL
@ stdcall CallNamedPipeW(wstr ptr long ptr long ptr long) kernel32.CallNamedPipeW
@ stdcall CallbackMayRunLong(ptr) kernel32.CallbackMayRunLong
@ stdcall CancelIo(long) kernel32.CancelIo
@ stdcall CancelIoEx(long ptr) kernel32.CancelIoEx
@ stdcall CancelSynchronousIo(long) kernel32.CancelSynchronousIo
@ stub CancelThreadpoolIo
@ stdcall CancelWaitableTimer(long) kernel32.CancelWaitableTimer
# @ stub CeipIsOptedIn
@ stdcall ChangeTimerQueueTimer(ptr ptr long long) kernel32.ChangeTimerQueueTimer
@ stdcall CharLowerA(str)
@ stdcall CharLowerBuffA(str long)
@ stdcall CharLowerBuffW(wstr long)
@ stdcall CharLowerW(wstr)
@ stdcall CharNextA(str)
@ stdcall CharNextExA(long str long)
@ stdcall CharNextW(wstr)
@ stdcall CharPrevA(str str)
@ stdcall CharPrevExA(long str str long)
@ stdcall CharPrevW(wstr wstr)
@ stdcall CharUpperA(str)
@ stdcall CharUpperBuffA(str long)
@ stdcall CharUpperBuffW(wstr long)
@ stdcall CharUpperW(wstr)
# @ stub CheckAllowDecryptedRemoteDestinationPolicy
@ stub CheckGroupPolicyEnabled
# @ stub CheckIfStateChangeNotificationExists
@ stdcall CheckRemoteDebuggerPresent(long ptr) kernel32.CheckRemoteDebuggerPresent
# @ stub CheckTokenCapability
@ stdcall CheckTokenMembership(long ptr ptr) advapi32.CheckTokenMembership
# @ stub CheckTokenMembershipEx
@ stdcall ChrCmpIA(long long)
@ stdcall ChrCmpIW(long long)
@ stdcall ClearCommBreak(long) kernel32.ClearCommBreak
@ stdcall ClearCommError(long ptr ptr) kernel32.ClearCommError
# @ stub CloseGlobalizationUserSettingsKey
@ stdcall CloseHandle(long) kernel32.CloseHandle
# @ stub ClosePackageInfo
# @ stub ClosePrivateNamespace
# @ stub CloseState
# @ stub CloseStateAtom
# @ stub CloseStateChangeNotification
# @ stub CloseStateContainer
# @ stub CloseStateLock
@ stdcall CloseThreadpool(ptr) kernel32.CloseThreadpool
@ stdcall CloseThreadpoolCleanupGroup(ptr) kernel32.CloseThreadpoolCleanupGroup
@ stdcall CloseThreadpoolCleanupGroupMembers(ptr long ptr) kernel32.CloseThreadpoolCleanupGroupMembers
@ stub CloseThreadpoolIo
@ stdcall CloseThreadpoolTimer(ptr) kernel32.CloseThreadpoolTimer
@ stdcall CloseThreadpoolWait(ptr) kernel32.CloseThreadpoolWait
@ stdcall CloseThreadpoolWork(ptr) kernel32.CloseThreadpoolWork
# @ stub CommitStateAtom
@ stdcall CompareFileTime(ptr ptr) kernel32.CompareFileTime
# @ stub CompareObjectHandles
@ stdcall CompareStringA(long long str long str long) kernel32.CompareStringA
@ stdcall CompareStringEx(wstr long wstr long wstr long ptr ptr long) kernel32.CompareStringEx
@ stdcall CompareStringOrdinal(wstr long wstr long long) kernel32.CompareStringOrdinal
@ stdcall CompareStringW(long long wstr long wstr long) kernel32.CompareStringW
@ stdcall ConnectNamedPipe(long ptr) kernel32.ConnectNamedPipe
@ stdcall ContinueDebugEvent(long long long) kernel32.ContinueDebugEvent
@ stdcall ConvertDefaultLocale(long) kernel32.ConvertDefaultLocale
@ stdcall ConvertFiberToThread() kernel32.ConvertFiberToThread
@ stdcall ConvertThreadToFiber(ptr) kernel32.ConvertThreadToFiber
@ stdcall ConvertThreadToFiberEx(ptr long) kernel32.ConvertThreadToFiberEx
@ stdcall ConvertToAutoInheritPrivateObjectSecurity(ptr ptr ptr ptr long ptr) advapi32.ConvertToAutoInheritPrivateObjectSecurity
# @ stub CopyContext
# @ stub CopyFile2
@ stdcall CopyFileExW(wstr wstr ptr ptr ptr long) kernel32.CopyFileExW
@ stdcall CopyFileW(wstr wstr long) kernel32.CopyFileW
# @ stub -arch=x86_64 CopyMemoryNonTemporal
@ stdcall CopySid(long ptr ptr) advapi32.CopySid
# @ stub CouldMultiUserAppsBehaviorBePossibleForPackage
@ stdcall CreateActCtxW(ptr) kernel32.CreateActCtxW
# @ stub CreateAppContainerToken
# @ stub CreateBoundaryDescriptorW
@ stdcall CreateConsoleScreenBuffer(long long ptr long ptr) kernel32.CreateConsoleScreenBuffer
@ stdcall CreateDirectoryA(str ptr) kernel32.CreateDirectoryA
@ stdcall CreateDirectoryExW(wstr wstr ptr) kernel32.CreateDirectoryExW
@ stdcall CreateDirectoryW(wstr ptr) kernel32.CreateDirectoryW
# @ stub CreateEnclave
@ stdcall CreateEventA(ptr long long str) kernel32.CreateEventA
@ stdcall CreateEventExA(ptr str long long) kernel32.CreateEventExA
@ stdcall CreateEventExW(ptr wstr long long) kernel32.CreateEventExW
@ stdcall CreateEventW(ptr long long wstr) kernel32.CreateEventW
@ stdcall CreateFiber(long ptr ptr) kernel32.CreateFiber
@ stdcall CreateFiberEx(long long long ptr ptr) kernel32.CreateFiberEx
@ stdcall CreateFile2(wstr long long long ptr) kernel32.CreateFile2
@ stdcall CreateFileA(str long long ptr long long long) kernel32.CreateFileA
# @ stub CreateFileMappingFromApp
@ stub CreateFileMappingNumaW
@ stdcall CreateFileMappingW(long ptr long long long wstr) kernel32.CreateFileMappingW
@ stdcall CreateFileW(wstr long long ptr long long long) kernel32.CreateFileW
@ stdcall CreateHardLinkA(str str ptr) kernel32.CreateHardLinkA
@ stdcall CreateHardLinkW(wstr wstr ptr) kernel32.CreateHardLinkW
@ stdcall CreateIoCompletionPort(long long long long) kernel32.CreateIoCompletionPort
@ stdcall CreateMemoryResourceNotification(long) kernel32.CreateMemoryResourceNotification
@ stdcall CreateMutexA(ptr long str) kernel32.CreateMutexA
@ stdcall CreateMutexExA(ptr str long long) kernel32.CreateMutexExA
@ stdcall CreateMutexExW(ptr wstr long long) kernel32.CreateMutexExW
@ stdcall CreateMutexW(ptr long wstr) kernel32.CreateMutexW
@ stdcall CreateNamedPipeW(wstr long long long long long long ptr) kernel32.CreateNamedPipeW
@ stdcall CreatePipe(ptr ptr ptr long) kernel32.CreatePipe
# @ stub CreatePrivateNamespaceW
@ stdcall CreatePrivateObjectSecurity(ptr ptr ptr long long ptr) advapi32.CreatePrivateObjectSecurity
@ stdcall CreatePrivateObjectSecurityEx(ptr ptr ptr ptr long long long ptr) advapi32.CreatePrivateObjectSecurityEx
@ stdcall CreatePrivateObjectSecurityWithMultipleInheritance(ptr ptr ptr ptr long long long long ptr) advapi32.CreatePrivateObjectSecurityWithMultipleInheritance
@ stdcall CreateProcessA(str str ptr ptr long long ptr str ptr ptr) kernel32.CreateProcessA
@ stdcall CreateProcessAsUserA(long str str ptr ptr long long ptr str ptr ptr) kernel32.CreateProcessAsUserA
@ stdcall CreateProcessAsUserW(long wstr wstr ptr ptr long long ptr wstr ptr ptr) kernel32.CreateProcessAsUserW
@ stdcall CreateProcessInternalA(long str str ptr ptr long long ptr str ptr ptr ptr) kernel32.CreateProcessInternalA
@ stdcall CreateProcessInternalW(long wstr wstr ptr ptr long long ptr wstr ptr ptr ptr) kernel32.CreateProcessInternalW
@ stdcall CreateProcessW(wstr wstr ptr ptr long long ptr wstr ptr ptr) kernel32.CreateProcessW
@ stdcall CreateRemoteThread(long ptr long ptr long long ptr) kernel32.CreateRemoteThread
@ stdcall CreateRemoteThreadEx(long ptr long ptr ptr long ptr ptr) kernel32.CreateRemoteThreadEx
@ stdcall CreateRestrictedToken(long long long ptr long ptr long ptr ptr) advapi32.CreateRestrictedToken
@ stdcall CreateSemaphoreExW(ptr long long wstr long long) kernel32.CreateSemaphoreExW
@ stdcall CreateSemaphoreW(ptr long long wstr) kernel32.CreateSemaphoreW
# @ stub CreateStateAtom
# @ stub CreateStateChangeNotification
# @ stub CreateStateContainer
# @ stub CreateStateLock
# @ stub CreateStateSubcontainer
@ stdcall CreateSymbolicLinkW(wstr wstr long) kernel32.CreateSymbolicLinkW
@ stdcall CreateThread(ptr long ptr long long ptr) kernel32.CreateThread
@ stdcall CreateThreadpool(ptr) kernel32.CreateThreadpool
@ stdcall CreateThreadpoolCleanupGroup() kernel32.CreateThreadpoolCleanupGroup
@ stdcall CreateThreadpoolIo(ptr ptr ptr ptr) kernel32.CreateThreadpoolIo
@ stdcall CreateThreadpoolTimer(ptr ptr ptr) kernel32.CreateThreadpoolTimer
@ stdcall CreateThreadpoolWait(ptr ptr ptr) kernel32.CreateThreadpoolWait
@ stdcall CreateThreadpoolWork(ptr ptr ptr) kernel32.CreateThreadpoolWork
@ stdcall CreateTimerQueue() kernel32.CreateTimerQueue
@ stdcall CreateTimerQueueTimer(ptr long ptr ptr long long long) kernel32.CreateTimerQueueTimer
@ stdcall CreateWaitableTimerExW(ptr wstr long long) kernel32.CreateWaitableTimerExW
@ stdcall CreateWaitableTimerW(ptr long wstr) kernel32.CreateWaitableTimerW
@ stdcall CreateWellKnownSid(long ptr ptr ptr) advapi32.CreateWellKnownSid
# @ stub CtrlRoutine
# @ stub CveEventWrite
@ stdcall DeactivateActCtx(long long) kernel32.DeactivateActCtx
@ stdcall DebugActiveProcess(long) kernel32.DebugActiveProcess
@ stdcall DebugActiveProcessStop(long) kernel32.DebugActiveProcessStop
@ stdcall DebugBreak() kernel32.DebugBreak
@ stdcall DecodePointer(ptr) kernel32.DecodePointer
# @ stub DecodeRemotePointer
@ stdcall DecodeSystemPointer(ptr) kernel32.DecodeSystemPointer
@ stdcall DefineDosDeviceW(long wstr wstr) kernel32.DefineDosDeviceW
@ stdcall DelayLoadFailureHook(str str) kernel32.DelayLoadFailureHook
# @ stub DelayLoadFailureHookLookup
@ stdcall DeleteAce(ptr long) advapi32.DeleteAce
# @ stub DeleteBoundaryDescriptor
@ stdcall DeleteCriticalSection(ptr) kernel32.DeleteCriticalSection
@ stdcall DeleteFiber(ptr) kernel32.DeleteFiber
@ stdcall DeleteFileA(str) kernel32.DeleteFileA
@ stdcall DeleteFileW(wstr) kernel32.DeleteFileW
@ stdcall DeleteProcThreadAttributeList(ptr) kernel32.DeleteProcThreadAttributeList
# @ stub DeleteStateAtomValue
# @ stub DeleteStateContainer
# @ stub DeleteStateContainerValue
# @ stub DeleteSynchronizationBarrier
@ stdcall DeleteTimerQueueEx(long long) kernel32.DeleteTimerQueueEx
@ stdcall DeleteTimerQueueTimer(long long long) kernel32.DeleteTimerQueueTimer
@ stdcall DeleteVolumeMountPointW(wstr) kernel32.DeleteVolumeMountPointW
@ stdcall DestroyPrivateObjectSecurity(ptr) advapi32.DestroyPrivateObjectSecurity
@ stdcall DeviceIoControl(long long ptr long ptr long ptr ptr) kernel32.DeviceIoControl
# @ stub DisablePredefinedHandleTableInternal
@ stdcall DisableThreadLibraryCalls(long) kernel32.DisableThreadLibraryCalls
@ stdcall DisassociateCurrentThreadFromCallback(ptr) kernel32.DisassociateCurrentThreadFromCallback
# @ stub DiscardVirtualMemory
@ stdcall DisconnectNamedPipe(long) kernel32.DisconnectNamedPipe
# @ stub DnsHostnameToComputerNameExW
# @ stub DsBindWithSpnExW
# @ stub DsCrackNamesW
# @ stub DsFreeDomainControllerInfoW
# @ stub DsFreeNameResultW
# @ stub DsFreeNgcKey
# @ stub DsFreePasswordCredentials
# @ stub DsGetDomainControllerInfoW
# @ stub DsMakePasswordCredentialsW
# @ stub DsReadNgcKeyW
# @ stub DsUnBindW
# @ stub DsWriteNgcKeyW
@ stdcall DuplicateHandle(long long long ptr long long long) kernel32.DuplicateHandle
# @ stub DuplicateStateContainerHandle
@ stdcall DuplicateToken(long long ptr) advapi32.DuplicateToken
@ stdcall DuplicateTokenEx(long long ptr long long ptr) advapi32.DuplicateTokenEx
# @ stub EmptyWorkingSet
@ stdcall EncodePointer(ptr) kernel32.EncodePointer
# @ stub EncodeRemotePointer
@ stdcall EncodeSystemPointer(ptr) kernel32.EncodeSystemPointer
# @ stub EnterCriticalPolicySectionInternal
@ stdcall EnterCriticalSection(ptr) kernel32.EnterCriticalSection
# @ stub EnterSynchronizationBarrier
@ stdcall EnumCalendarInfoExEx(ptr wstr long wstr long long) kernel32.EnumCalendarInfoExEx
@ stdcall EnumCalendarInfoExW(ptr long long long) kernel32.EnumCalendarInfoExW
@ stdcall EnumCalendarInfoW(ptr long long long) kernel32.EnumCalendarInfoW
@ stdcall EnumDateFormatsExEx(ptr wstr long long) kernel32.EnumDateFormatsExEx
@ stdcall EnumDateFormatsExW(ptr long long) kernel32.EnumDateFormatsExW
@ stdcall EnumDateFormatsW(ptr long long) kernel32.EnumDateFormatsW
# @ stub EnumDeviceDrivers
@ stdcall EnumDynamicTimeZoneInformation(long ptr) advapi32.EnumDynamicTimeZoneInformation
@ stdcall EnumLanguageGroupLocalesW(ptr long long ptr) kernel32.EnumLanguageGroupLocalesW
# @ stub EnumPageFilesA
# @ stub EnumPageFilesW
# @ stub EnumProcessModules
# @ stub EnumProcessModulesEx
# @ stub EnumProcesses
@ stdcall EnumResourceLanguagesExA(long str str ptr long long long) kernel32.EnumResourceLanguagesExA
@ stdcall EnumResourceLanguagesExW(long wstr wstr ptr long long long) kernel32.EnumResourceLanguagesExW
# @ stub EnumResourceNamesExA
# @ stub EnumResourceNamesExW
@ stdcall EnumResourceNamesW(long wstr ptr long) kernel32.EnumResourceNamesW
# @ stub EnumResourceTypesExA
# @ stub EnumResourceTypesExW
@ stdcall EnumSystemCodePagesW(ptr long) kernel32.EnumSystemCodePagesW
# @ stub EnumSystemFirmwareTables
@ stdcall EnumSystemGeoID(long long ptr) kernel32.EnumSystemGeoID
@ stdcall EnumSystemLanguageGroupsW(ptr long ptr) kernel32.EnumSystemLanguageGroupsW
@ stdcall EnumSystemLocalesA(ptr long) kernel32.EnumSystemLocalesA
@ stdcall EnumSystemLocalesEx(ptr long long ptr) kernel32.EnumSystemLocalesEx
@ stdcall EnumSystemLocalesW(ptr long) kernel32.EnumSystemLocalesW
@ stdcall EnumTimeFormatsEx(ptr wstr long long) kernel32.EnumTimeFormatsEx
@ stdcall EnumTimeFormatsW(ptr long long) kernel32.EnumTimeFormatsW
@ stdcall EnumUILanguagesW(ptr long long) kernel32.EnumUILanguagesW
# @ stub EnumerateStateAtomValues
# @ stub EnumerateStateContainerItems
@ stub EqualDomainSid
@ stdcall EqualPrefixSid(ptr ptr) advapi32.EqualPrefixSid
@ stdcall EqualSid(ptr ptr) advapi32.EqualSid
@ stdcall EscapeCommFunction(long long) kernel32.EscapeCommFunction
@ stdcall EventActivityIdControl(long ptr) advapi32.EventActivityIdControl
@ stdcall EventEnabled(int64 ptr) advapi32.EventEnabled
@ stdcall EventProviderEnabled(int64 long int64) advapi32.EventProviderEnabled
@ stdcall EventRegister(ptr ptr ptr ptr) advapi32.EventRegister
@ stdcall EventSetInformation(int64 long ptr long) advapi32.EventSetInformation
@ stdcall EventUnregister(int64) advapi32.EventUnregister
@ stdcall EventWrite(int64 ptr long ptr) advapi32.EventWrite
# @ stub EventWriteEx
# @ stub EventWriteString
@ stdcall EventWriteTransfer(int64 ptr ptr ptr long ptr) advapi32.EventWriteTransfer
@ stdcall ExitProcess(long) kernel32.ExitProcess
@ stdcall ExitThread(long) kernel32.ExitThread
@ stdcall ExpandEnvironmentStringsA(str ptr long) kernel32.ExpandEnvironmentStringsA
@ stdcall ExpandEnvironmentStringsW(wstr ptr long) kernel32.ExpandEnvironmentStringsW
@ stdcall FatalAppExitA(long str) kernel32.FatalAppExitA
@ stdcall FatalAppExitW(long wstr) kernel32.FatalAppExitW
@ stdcall FileTimeToLocalFileTime(ptr ptr) kernel32.FileTimeToLocalFileTime
@ stdcall FileTimeToSystemTime(ptr ptr) kernel32.FileTimeToSystemTime
@ stdcall FillConsoleOutputAttribute(long long long long ptr) kernel32.FillConsoleOutputAttribute
@ stdcall FillConsoleOutputCharacterA(long long long long ptr) kernel32.FillConsoleOutputCharacterA
@ stdcall FillConsoleOutputCharacterW(long long long long ptr) kernel32.FillConsoleOutputCharacterW
@ stdcall FindActCtxSectionGuid(long ptr long ptr ptr) kernel32.FindActCtxSectionGuid
@ stdcall FindActCtxSectionStringW(long ptr long wstr ptr) kernel32.FindActCtxSectionStringW
@ stdcall FindClose(long) kernel32.FindClose
@ stdcall FindCloseChangeNotification(long) kernel32.FindCloseChangeNotification
@ stdcall FindFirstChangeNotificationA(str long long) kernel32.FindFirstChangeNotificationA
@ stdcall FindFirstChangeNotificationW(wstr long long) kernel32.FindFirstChangeNotificationW
@ stdcall FindFirstFileA(str ptr) kernel32.FindFirstFileA
@ stdcall FindFirstFileExA(str long ptr long ptr long) kernel32.FindFirstFileExA
@ stdcall FindFirstFileExW(wstr long ptr long ptr long) kernel32.FindFirstFileExW
# @ stub FindFirstFileNameW
@ stdcall FindFirstFileW(wstr ptr) kernel32.FindFirstFileW
@ stdcall FindFirstFreeAce(ptr ptr) advapi32.FindFirstFreeAce
@ stdcall FindFirstStreamW(wstr long ptr long) kernel32.FindFirstStreamW
@ stdcall FindFirstVolumeW(ptr long) kernel32.FindFirstVolumeW
@ stub FindNLSString
@ stdcall FindNLSStringEx(wstr long wstr long wstr long ptr ptr ptr long) kernel32.FindNLSStringEx
@ stdcall FindNextChangeNotification(long) kernel32.FindNextChangeNotification
@ stdcall FindNextFileA(long ptr) kernel32.FindNextFileA
# @ stub FindNextFileNameW
@ stdcall FindNextFileW(long ptr) kernel32.FindNextFileW
@ stdcall FindNextStreamW(long ptr) kernel32.FindNextStreamW
@ stdcall FindNextVolumeW(long ptr long) kernel32.FindNextVolumeW
# @ stub FindPackagesByPackageFamily
@ stdcall FindResourceExW(long wstr wstr long) kernel32.FindResourceExW
@ stdcall FindResourceW(long wstr wstr) kernel32.FindResourceW
@ stdcall FindStringOrdinal(long wstr long wstr long long) kernel32.FindStringOrdinal
@ stdcall FindVolumeClose(ptr) kernel32.FindVolumeClose
@ stdcall FlsAlloc(ptr) kernel32.FlsAlloc
@ stdcall FlsFree(long) kernel32.FlsFree
@ stdcall FlsGetValue(long) kernel32.FlsGetValue
@ stdcall FlsSetValue(long ptr) kernel32.FlsSetValue
@ stdcall FlushConsoleInputBuffer(long) kernel32.FlushConsoleInputBuffer
@ stdcall FlushFileBuffers(long) kernel32.FlushFileBuffers
@ stdcall FlushInstructionCache(long long long) kernel32.FlushInstructionCache
@ stdcall FlushProcessWriteBuffers() kernel32.FlushProcessWriteBuffers
@ stdcall FlushViewOfFile(ptr long) kernel32.FlushViewOfFile
@ stdcall FoldStringW(long wstr long ptr long) kernel32.FoldStringW
# @ stub ForceSyncFgPolicyInternal
# @ stub FormatApplicationUserModelId
@ stdcall FormatMessageA(long ptr long long ptr long ptr) kernel32.FormatMessageA
@ stdcall FormatMessageW(long ptr long long ptr long ptr) kernel32.FormatMessageW
@ stdcall FreeConsole() kernel32.FreeConsole
@ stdcall FreeEnvironmentStringsA(ptr) kernel32.FreeEnvironmentStringsA
@ stdcall FreeEnvironmentStringsW(ptr) kernel32.FreeEnvironmentStringsW
# @ stub FreeGPOListInternalA
# @ stub FreeGPOListInternalW
@ stdcall FreeLibrary(long) kernel32.FreeLibrary
@ stdcall FreeLibraryAndExitThread(long long) kernel32.FreeLibraryAndExitThread
@ stdcall FreeLibraryWhenCallbackReturns(ptr ptr) kernel32.FreeLibraryWhenCallbackReturns
@ stdcall FreeResource(long) kernel32.FreeResource
@ stdcall FreeSid(ptr) advapi32.FreeSid
@ stdcall FreeUserPhysicalPages(long ptr ptr) kernel32.FreeUserPhysicalPages
@ stdcall GenerateConsoleCtrlEvent(long long) kernel32.GenerateConsoleCtrlEvent
# @ stub GenerateGPNotificationInternal
@ stdcall GetACP() kernel32.GetACP
@ stdcall GetAcceptLanguagesA(ptr ptr)
@ stdcall GetAcceptLanguagesW(ptr ptr)
@ stdcall GetAce(ptr long ptr) advapi32.GetAce
@ stdcall GetAclInformation(ptr ptr long long) advapi32.GetAclInformation
# @ stub GetAdjustObjectAttributesForPrivateNamespaceRoutine
# @ stub GetAlternatePackageRoots
# @ stub GetAppContainerAce
# @ stub GetAppContainerNamedObjectPath
# @ stub GetAppDataFolder
# @ stub GetAppModelVersion
# @ stub GetApplicationRecoveryCallback
@ stdcall GetApplicationRestartSettings(long ptr ptr ptr) kernel32.GetApplicationRestartSettings
# @ stub GetApplicationUserModelId
# @ stub GetApplicationUserModelIdFromToken
# @ stub GetAppliedGPOListInternalA
# @ stub GetAppliedGPOListInternalW
@ stub GetCPFileNameFromRegistry
@ stub GetCPHashNode
@ stdcall GetCPInfo(long ptr) kernel32.GetCPInfo
@ stdcall GetCPInfoExW(long long ptr) kernel32.GetCPInfoExW
# @ stub GetCachedSigningLevel
@ stub GetCalendar
@ stdcall GetCalendarInfoEx(wstr long ptr long ptr long ptr) kernel32.GetCalendarInfoEx
@ stdcall GetCalendarInfoW(long long long ptr long ptr) kernel32.GetCalendarInfoW
@ stdcall GetCommConfig(long ptr ptr) kernel32.GetCommConfig
@ stdcall GetCommMask(long ptr) kernel32.GetCommMask
@ stdcall GetCommModemStatus(long ptr) kernel32.GetCommModemStatus
@ stdcall GetCommProperties(long ptr) kernel32.GetCommProperties
@ stdcall GetCommState(long ptr) kernel32.GetCommState
@ stdcall GetCommTimeouts(long ptr) kernel32.GetCommTimeouts
@ stdcall GetCommandLineA() kernel32.GetCommandLineA
@ stdcall GetCommandLineW() kernel32.GetCommandLineW
@ stdcall GetCompressedFileSizeA(long ptr) kernel32.GetCompressedFileSizeA
@ stdcall GetCompressedFileSizeW(long ptr) kernel32.GetCompressedFileSizeW
@ stdcall GetComputerNameExA(long ptr ptr) kernel32.GetComputerNameExA
@ stdcall GetComputerNameExW(long ptr ptr) kernel32.GetComputerNameExW
@ stdcall GetConsoleCP() kernel32.GetConsoleCP
@ stdcall GetConsoleCursorInfo(long ptr) kernel32.GetConsoleCursorInfo
@ stdcall GetConsoleInputExeNameA(long ptr) kernel32.GetConsoleInputExeNameA
@ stdcall GetConsoleInputExeNameW(long ptr) kernel32.GetConsoleInputExeNameW
@ stdcall GetConsoleMode(long ptr) kernel32.GetConsoleMode
@ stdcall GetConsoleOutputCP() kernel32.GetConsoleOutputCP
@ stdcall GetConsoleScreenBufferInfo(long ptr) kernel32.GetConsoleScreenBufferInfo
@ stdcall GetConsoleScreenBufferInfoEx(long ptr) kernel32.GetConsoleScreenBufferInfoEx
@ stdcall GetConsoleTitleW(ptr long) kernel32.GetConsoleTitleW
@ stdcall GetCurrencyFormatEx(wstr long wstr ptr ptr long) kernel32.GetCurrencyFormatEx
@ stdcall GetCurrencyFormatW(long long wstr ptr ptr long) kernel32.GetCurrencyFormatW
@ stdcall GetCurrentActCtx(ptr) kernel32.GetCurrentActCtx
# @ stub GetCurrentApplicationUserModelId
@ stdcall GetCurrentDirectoryA(long ptr) kernel32.GetCurrentDirectoryA
@ stdcall GetCurrentDirectoryW(long ptr) kernel32.GetCurrentDirectoryW
# @ stub GetCurrentPackageApplicationContext
# @ stub GetCurrentPackageApplicationResourcesContext
# @ stub GetCurrentPackageContext
@ stdcall GetCurrentPackageFamilyName(ptr ptr) kernel32.GetCurrentPackageFamilyName
@ stdcall GetCurrentPackageFullName(ptr ptr) kernel32.GetCurrentPackageFullName
@ stdcall GetCurrentPackageId(ptr ptr) kernel32.GetCurrentPackageId
# @ stub GetCurrentPackageInfo
# @ stub GetCurrentPackagePath
# @ stub GetCurrentPackageResourcesContext
# @ stub GetCurrentPackageSecurityContext
@ stdcall -norelay GetCurrentProcess() kernel32.GetCurrentProcess
@ stdcall -norelay GetCurrentProcessId() kernel32.GetCurrentProcessId
@ stdcall GetCurrentProcessorNumber() kernel32.GetCurrentProcessorNumber
@ stdcall GetCurrentProcessorNumberEx(ptr) kernel32.GetCurrentProcessorNumberEx
# @ stub GetCurrentTargetPlatformContext
@ stdcall -norelay GetCurrentThread() kernel32.GetCurrentThread
@ stdcall -norelay GetCurrentThreadId() kernel32.GetCurrentThreadId
@ stdcall GetCurrentThreadStackLimits(ptr ptr) kernel32.GetCurrentThreadStackLimits
@ stdcall GetDateFormatA(long long ptr str ptr long) kernel32.GetDateFormatA
@ stdcall GetDateFormatEx(wstr long ptr wstr ptr long wstr) kernel32.GetDateFormatEx
@ stdcall GetDateFormatW(long long ptr wstr ptr long) kernel32.GetDateFormatW
# @ stub GetDeviceDriverBaseNameA
# @ stub GetDeviceDriverBaseNameW
# @ stub GetDeviceDriverFileNameA
# @ stub GetDeviceDriverFileNameW
@ stdcall GetDiskFreeSpaceA(str ptr ptr ptr ptr) kernel32.GetDiskFreeSpaceA
@ stdcall GetDiskFreeSpaceExA(str ptr ptr ptr) kernel32.GetDiskFreeSpaceExA
@ stdcall GetDiskFreeSpaceExW(wstr ptr ptr ptr) kernel32.GetDiskFreeSpaceExW
@ stdcall GetDiskFreeSpaceW(wstr ptr ptr ptr ptr) kernel32.GetDiskFreeSpaceW
@ stdcall GetDriveTypeA(str) kernel32.GetDriveTypeA
@ stdcall GetDriveTypeW(wstr) kernel32.GetDriveTypeW
# @ stub GetDurationFormatEx
@ stdcall GetDynamicTimeZoneInformation(ptr) kernel32.GetDynamicTimeZoneInformation
@ stdcall GetDynamicTimeZoneInformationEffectiveYears(ptr ptr ptr) kernel32.GetDynamicTimeZoneInformationEffectiveYears
# @ stub GetEffectivePackageStatusForUser
# @ stub GetEightBitStringToUnicodeSizeRoutine
# @ stub GetEightBitStringToUnicodeStringRoutine
@ stdcall -ret64 -arch=i386,x86_64 GetEnabledXStateFeatures() kernel32.GetEnabledXStateFeatures
@ stdcall GetEnvironmentStrings() kernel32.GetEnvironmentStrings
@ stdcall GetEnvironmentStringsA() kernel32.GetEnvironmentStringsA
@ stdcall GetEnvironmentStringsW() kernel32.GetEnvironmentStringsW
@ stdcall GetEnvironmentVariableA(str ptr long) kernel32.GetEnvironmentVariableA
@ stdcall GetEnvironmentVariableW(wstr ptr long) kernel32.GetEnvironmentVariableW
@ stub GetEraNameCountedString
@ stdcall GetErrorMode() kernel32.GetErrorMode
@ stdcall GetExitCodeProcess(long ptr) kernel32.GetExitCodeProcess
@ stdcall GetExitCodeThread(long ptr) kernel32.GetExitCodeThread
@ stub GetFallbackDisplayName
@ stdcall GetFileAttributesA(str) kernel32.GetFileAttributesA
@ stdcall GetFileAttributesExA(str long ptr) kernel32.GetFileAttributesExA
@ stdcall GetFileAttributesExW(wstr long ptr) kernel32.GetFileAttributesExW
@ stdcall GetFileAttributesW(wstr) kernel32.GetFileAttributesW
@ stdcall GetFileInformationByHandle(long ptr) kernel32.GetFileInformationByHandle
@ stdcall GetFileInformationByHandleEx(long long ptr long) kernel32.GetFileInformationByHandleEx
@ stdcall GetFileMUIInfo(long wstr ptr ptr) kernel32.GetFileMUIInfo
@ stdcall GetFileMUIPath(long wstr wstr ptr ptr ptr ptr) kernel32.GetFileMUIPath
@ stdcall GetFileSecurityW(wstr long ptr long ptr) advapi32.GetFileSecurityW
@ stdcall GetFileSize(long ptr) kernel32.GetFileSize
@ stdcall GetFileSizeEx(long ptr) kernel32.GetFileSizeEx
@ stdcall GetFileTime(long ptr ptr ptr) kernel32.GetFileTime
@ stdcall GetFileType(long) kernel32.GetFileType
@ stdcall GetFileVersionInfoA(str long long ptr)
# @ stub GetFileVersionInfoByHandle
@ stdcall GetFileVersionInfoExA(long str long long ptr)
@ stdcall GetFileVersionInfoExW(long wstr long long ptr)
@ stdcall GetFileVersionInfoSizeA(str ptr)
@ stdcall GetFileVersionInfoSizeExA(long str ptr)
@ stdcall GetFileVersionInfoSizeExW(long wstr ptr)
@ stdcall GetFileVersionInfoSizeW(wstr ptr)
@ stdcall GetFileVersionInfoW(wstr long long ptr)
@ stdcall GetFinalPathNameByHandleA(long ptr long long) kernel32.GetFinalPathNameByHandleA
@ stdcall GetFinalPathNameByHandleW(long ptr long long) kernel32.GetFinalPathNameByHandleW
@ stdcall GetFullPathNameA(str long ptr ptr) kernel32.GetFullPathNameA
@ stdcall GetFullPathNameW(wstr long ptr ptr) kernel32.GetFullPathNameW
# @ stub GetGPOListInternalA
# @ stub GetGPOListInternalW
@ stdcall GetGeoInfoW(long long ptr long long) kernel32.GetGeoInfoW
@ stdcall GetHandleInformation(long ptr) kernel32.GetHandleInformation
# @ stub GetHivePath
# @ stub GetIntegratedDisplaySize
# @ stub GetIsEdpEnabled
@ stdcall GetKernelObjectSecurity(long long ptr long ptr) advapi32.GetKernelObjectSecurity
@ stdcall GetLargePageMinimum() kernel32.GetLargePageMinimum
@ stdcall GetLargestConsoleWindowSize(long) kernel32.GetLargestConsoleWindowSize
@ stdcall GetLastError() kernel32.GetLastError
@ stdcall GetLengthSid(ptr) advapi32.GetLengthSid
@ stdcall GetLocalTime(ptr) kernel32.GetLocalTime
@ stdcall GetLocaleInfoA(long long ptr long) kernel32.GetLocaleInfoA
@ stdcall GetLocaleInfoEx(wstr long ptr long) kernel32.GetLocaleInfoEx
@ stub GetLocaleInfoHelper
@ stdcall GetLocaleInfoW(long long ptr long) kernel32.GetLocaleInfoW
@ stdcall GetLogicalDriveStringsW(long ptr) kernel32.GetLogicalDriveStringsW
@ stdcall GetLogicalDrives() kernel32.GetLogicalDrives
@ stdcall GetLogicalProcessorInformation(ptr ptr) kernel32.GetLogicalProcessorInformation
@ stdcall GetLogicalProcessorInformationEx(long ptr ptr) kernel32.GetLogicalProcessorInformationEx
@ stdcall GetLongPathNameA(str long long) kernel32.GetLongPathNameA
@ stdcall GetLongPathNameW(wstr long long) kernel32.GetLongPathNameW
# @ stub GetMappedFileNameA
# @ stub GetMappedFileNameW
# @ stub GetMemoryErrorHandlingCapabilities
# @ stub GetModuleBaseNameA
# @ stub GetModuleBaseNameW
@ stdcall GetModuleFileNameA(long ptr long) kernel32.GetModuleFileNameA
# @ stub GetModuleFileNameExA
# @ stub GetModuleFileNameExW
@ stdcall GetModuleFileNameW(long ptr long) kernel32.GetModuleFileNameW
@ stdcall GetModuleHandleA(str) kernel32.GetModuleHandleA
@ stdcall GetModuleHandleExA(long ptr ptr) kernel32.GetModuleHandleExA
@ stdcall GetModuleHandleExW(long ptr ptr) kernel32.GetModuleHandleExW
@ stdcall GetModuleHandleW(wstr) kernel32.GetModuleHandleW
# @ stub GetModuleInformation
@ stub GetNLSVersion
@ stub GetNLSVersionEx
@ stub GetNamedLocaleHashNode
@ stub GetNamedPipeAttribute
@ stub GetNamedPipeClientComputerNameW
@ stdcall GetNamedPipeHandleStateW(long ptr ptr ptr ptr wstr long) kernel32.GetNamedPipeHandleStateW
@ stdcall GetNamedPipeInfo(long ptr ptr ptr ptr) kernel32.GetNamedPipeInfo
@ stdcall GetNativeSystemInfo(ptr) kernel32.GetNativeSystemInfo
# @ stub GetNextFgPolicyRefreshInfoInternal
@ stdcall GetNumaHighestNodeNumber(ptr) kernel32.GetNumaHighestNodeNumber
@ stdcall GetNumaNodeProcessorMaskEx(long ptr) kernel32.GetNumaNodeProcessorMaskEx
@ stdcall GetNumaProximityNode(long ptr) kernel32.GetNumaProximityNode
@ stdcall GetNumaProximityNodeEx(long ptr) kernel32.GetNumaProximityNodeEx
@ stdcall GetNumberFormatEx(wstr long wstr ptr ptr long) kernel32.GetNumberFormatEx
@ stdcall GetNumberFormatW(long long wstr ptr ptr long) kernel32.GetNumberFormatW
@ stdcall GetNumberOfConsoleInputEvents(long ptr) kernel32.GetNumberOfConsoleInputEvents
@ stdcall GetOEMCP() kernel32.GetOEMCP
# @ stub GetOsManufacturingMode
# @ stub GetOsSafeBootMode
@ stdcall GetOverlappedResult(long ptr ptr long) kernel32.GetOverlappedResult
# @ stub GetOverlappedResultEx
# @ stub GetPackageApplicationContext
# @ stub GetPackageApplicationIds
# @ stub GetPackageApplicationProperty
# @ stub GetPackageApplicationPropertyString
# @ stub GetPackageApplicationResourcesContext
# @ stub GetPackageContext
# @ stub GetPackageFamilyName
# @ stub GetPackageFamilyNameFromToken
@ stdcall GetPackageFullName(long ptr ptr) kernel32.GetPackageFullName
# @ stub GetPackageFullNameFromToken
# @ stub GetPackageId
# @ stub GetPackageInfo
# @ stub GetPackageInstallTime
# @ stub GetPackageOSMaxVersionTested
# @ stub GetPackagePath
# @ stub GetPackagePathByFullName
# @ stub GetPackagePathOnVolume
# @ stub GetPackageProperty
# @ stub GetPackagePropertyString
# @ stub GetPackageResourcesContext
# @ stub GetPackageResourcesProperty
# @ stub GetPackageSecurityContext
# @ stub GetPackageSecurityProperty
# @ stub GetPackageStatus
# @ stub GetPackageStatusForUser
# @ stub GetPackageTargetPlatformProperty
# @ stub GetPackageVolumeSisPath
# @ stub GetPackagesByPackageFamily
# @ stub GetPerformanceInfo
@ stdcall GetPhysicallyInstalledSystemMemory(ptr) kernel32.GetPhysicallyInstalledSystemMemory
# @ stub GetPreviousFgPolicyRefreshInfoInternal
@ stdcall GetPriorityClass(long) kernel32.GetPriorityClass
@ stdcall GetPrivateObjectSecurity(ptr long ptr long ptr) advapi32.GetPrivateObjectSecurity
@ stdcall GetProcAddress(long str) kernel32.GetProcAddress
# @ stub GetProcAddressForCaller
# @ stub GetProcessDefaultCpuSets
# @ stub GetProcessGroupAffinity
@ stdcall GetProcessHandleCount(long ptr) kernel32.GetProcessHandleCount
@ stdcall -norelay GetProcessHeap() kernel32.GetProcessHeap
@ stdcall GetProcessHeaps(long ptr) kernel32.GetProcessHeaps
@ stdcall GetProcessId(long) kernel32.GetProcessId
@ stdcall GetProcessIdOfThread(long) kernel32.GetProcessIdOfThread
# @ stub GetProcessImageFileNameA
# @ stub GetProcessImageFileNameW
# @ stub GetProcessInformation
# @ stub GetProcessMemoryInfo
@ stdcall GetProcessMitigationPolicy(long long ptr long) kernel32.GetProcessMitigationPolicy
@ stdcall GetProcessPreferredUILanguages(long ptr ptr ptr) kernel32.GetProcessPreferredUILanguages
@ stdcall GetProcessPriorityBoost(long ptr) kernel32.GetProcessPriorityBoost
@ stdcall GetProcessShutdownParameters(ptr ptr) kernel32.GetProcessShutdownParameters
@ stdcall GetProcessTimes(long ptr ptr ptr ptr) kernel32.GetProcessTimes
@ stdcall GetProcessVersion(long) kernel32.GetProcessVersion
@ stdcall GetProcessWorkingSetSizeEx(long ptr ptr ptr) kernel32.GetProcessWorkingSetSizeEx
# @ stub GetProcessorSystemCycleTime
@ stdcall GetProductInfo(long long long long ptr) kernel32.GetProductInfo
@ stub GetPtrCalData
@ stub GetPtrCalDataArray
# @ stub GetPublisherCacheFolder
# @ stub GetPublisherRootFolder
@ stdcall GetQueuedCompletionStatus(long ptr ptr ptr long) kernel32.GetQueuedCompletionStatus
@ stdcall GetQueuedCompletionStatusEx(ptr ptr long ptr long long) kernel32.GetQueuedCompletionStatusEx
# @ stub GetRegistryExtensionFlags
# @ stub GetRoamingLastObservedChangeTime
@ stdcall GetSecurityDescriptorControl(ptr ptr ptr) advapi32.GetSecurityDescriptorControl
@ stdcall GetSecurityDescriptorDacl(ptr ptr ptr ptr) advapi32.GetSecurityDescriptorDacl
@ stdcall GetSecurityDescriptorGroup(ptr ptr ptr) advapi32.GetSecurityDescriptorGroup
@ stdcall GetSecurityDescriptorLength(ptr) advapi32.GetSecurityDescriptorLength
@ stdcall GetSecurityDescriptorOwner(ptr ptr ptr) advapi32.GetSecurityDescriptorOwner
@ stub GetSecurityDescriptorRMControl
@ stdcall GetSecurityDescriptorSacl(ptr ptr ptr ptr) advapi32.GetSecurityDescriptorSacl
# @ stub GetSerializedAtomBytes
# @ stub GetSharedLocalFolder
@ stdcall GetShortPathNameW(wstr ptr long) kernel32.GetShortPathNameW
@ stdcall GetSidIdentifierAuthority(ptr) advapi32.GetSidIdentifierAuthority
@ stdcall GetSidLengthRequired(long) advapi32.GetSidLengthRequired
@ stdcall GetSidSubAuthority(ptr long) advapi32.GetSidSubAuthority
@ stdcall GetSidSubAuthorityCount(ptr) advapi32.GetSidSubAuthorityCount
# @ stub GetStagedPackageOrigin
# @ stub GetStagedPackagePathByFullName
@ stdcall GetStartupInfoW(ptr) kernel32.GetStartupInfoW
# @ stub GetStateContainerDepth
# @ stub GetStateFolder
# @ stub GetStateRootFolder
# @ stub GetStateRootFolderBase
# @ stub GetStateSettingsFolder
# @ stub GetStateVersion
@ stdcall GetStdHandle(long) kernel32.GetStdHandle
# @ stub GetStringScripts
@ stub GetStringTableEntry
@ stdcall GetStringTypeA(long long str long ptr) kernel32.GetStringTypeA
@ stdcall GetStringTypeExW(long long wstr long ptr) kernel32.GetStringTypeExW
@ stdcall GetStringTypeW(long wstr long ptr) kernel32.GetStringTypeW
# @ stub GetSystemAppDataFolder
# @ stub GetSystemAppDataKey
# @ stub GetSystemCpuSetInformation
@ stdcall GetSystemDefaultLCID() kernel32.GetSystemDefaultLCID
@ stdcall GetSystemDefaultLangID() kernel32.GetSystemDefaultLangID
@ stdcall GetSystemDefaultLocaleName(ptr long) kernel32.GetSystemDefaultLocaleName
@ stdcall GetSystemDefaultUILanguage() kernel32.GetSystemDefaultUILanguage
@ stdcall GetSystemDirectoryA(ptr long) kernel32.GetSystemDirectoryA
@ stdcall GetSystemDirectoryW(ptr long) kernel32.GetSystemDirectoryW
@ stdcall GetSystemFileCacheSize(ptr ptr ptr) kernel32.GetSystemFileCacheSize
@ stdcall GetSystemFirmwareTable(long long ptr long) kernel32.GetSystemFirmwareTable
@ stdcall GetSystemInfo(ptr) kernel32.GetSystemInfo
# @ stub GetSystemMetadataPath
# @ stub GetSystemMetadataPathForPackage
# @ stub GetSystemMetadataPathForPackageFamily
@ stdcall GetSystemPreferredUILanguages(long ptr ptr ptr) kernel32.GetSystemPreferredUILanguages
# @ stub GetSystemStateRootFolder
@ stdcall GetSystemTime(ptr) kernel32.GetSystemTime
@ stdcall GetSystemTimeAdjustment(ptr ptr ptr) kernel32.GetSystemTimeAdjustment
@ stdcall GetSystemTimeAsFileTime(ptr) kernel32.GetSystemTimeAsFileTime
@ stdcall GetSystemTimePreciseAsFileTime(ptr) kernel32.GetSystemTimePreciseAsFileTime
@ stdcall GetSystemTimes(ptr ptr ptr) kernel32.GetSystemTimes
@ stdcall GetSystemWindowsDirectoryA(ptr long) kernel32.GetSystemWindowsDirectoryA
@ stdcall GetSystemWindowsDirectoryW(ptr long) kernel32.GetSystemWindowsDirectoryW
# @ stub GetSystemWow64Directory2A
# @ stub GetSystemWow64Directory2W
@ stdcall GetSystemWow64DirectoryA(ptr long) kernel32.GetSystemWow64DirectoryA
@ stdcall GetSystemWow64DirectoryW(ptr long) kernel32.GetSystemWow64DirectoryW
# @ stub GetTargetPlatformContext
@ stdcall GetTempFileNameA(str str long ptr) kernel32.GetTempFileNameA
@ stdcall GetTempFileNameW(wstr wstr long ptr) kernel32.GetTempFileNameW
@ stdcall GetTempPathA(long ptr) kernel32.GetTempPathA
@ stdcall GetTempPathW(long ptr) kernel32.GetTempPathW
@ stdcall GetThreadContext(long ptr) kernel32.GetThreadContext
# @ stub GetThreadDescription
@ stdcall GetThreadErrorMode() kernel32.GetThreadErrorMode
@ stdcall GetThreadGroupAffinity(long ptr) kernel32.GetThreadGroupAffinity
@ stdcall GetThreadIOPendingFlag(long ptr) kernel32.GetThreadIOPendingFlag
@ stdcall GetThreadId(ptr) kernel32.GetThreadId
# @ stub GetThreadIdealProcessorEx
# @ stub GetThreadInformation
@ stdcall GetThreadLocale() kernel32.GetThreadLocale
@ stdcall GetThreadPreferredUILanguages(long ptr ptr ptr) kernel32.GetThreadPreferredUILanguages
@ stdcall GetThreadPriority(long) kernel32.GetThreadPriority
@ stdcall GetThreadPriorityBoost(long ptr) kernel32.GetThreadPriorityBoost
# @ stub GetThreadSelectedCpuSets
@ stdcall GetThreadTimes(long ptr ptr ptr ptr) kernel32.GetThreadTimes
@ stdcall GetThreadUILanguage() kernel32.GetThreadUILanguage
@ stdcall GetTickCount() kernel32.GetTickCount
@ stdcall -ret64 GetTickCount64() kernel32.GetTickCount64
@ stdcall GetTimeFormatA(long long ptr str ptr long) kernel32.GetTimeFormatA
@ stdcall GetTimeFormatEx(wstr long ptr wstr ptr long) kernel32.GetTimeFormatEx
@ stdcall GetTimeFormatW(long long ptr wstr ptr long) kernel32.GetTimeFormatW
@ stdcall GetTimeZoneInformation(ptr) kernel32.GetTimeZoneInformation
@ stdcall GetTimeZoneInformationForYear(long ptr ptr) kernel32.GetTimeZoneInformationForYear
@ stdcall GetTokenInformation(long long ptr long ptr) advapi32.GetTokenInformation
@ stdcall GetTraceEnableFlags(int64) advapi32.GetTraceEnableFlags
@ stdcall GetTraceEnableLevel(int64) advapi32.GetTraceEnableLevel
@ stdcall -ret64 GetTraceLoggerHandle(ptr) advapi32.GetTraceLoggerHandle
@ stub GetUILanguageInfo
# @ stub GetUnicodeStringToEightBitSizeRoutine
# @ stub GetUnicodeStringToEightBitStringRoutine
@ stdcall GetUserDefaultLCID() kernel32.GetUserDefaultLCID
@ stdcall GetUserDefaultLangID() kernel32.GetUserDefaultLangID
@ stdcall GetUserDefaultLocaleName(ptr long) kernel32.GetUserDefaultLocaleName
@ stdcall GetUserDefaultUILanguage() kernel32.GetUserDefaultUILanguage
@ stdcall GetUserGeoID(long) kernel32.GetUserGeoID
@ stub GetUserInfo
@ stub GetUserInfoWord
# @ stub GetUserOverrideString
# @ stub GetUserOverrideWord
@ stdcall GetUserPreferredUILanguages(long ptr ptr ptr) kernel32.GetUserPreferredUILanguages
@ stdcall GetVersion() kernel32.GetVersion
@ stdcall GetVersionExA(ptr) kernel32.GetVersionExA
@ stdcall GetVersionExW(ptr) kernel32.GetVersionExW
@ stdcall GetVolumeInformationA(str ptr long ptr ptr ptr ptr long) kernel32.GetVolumeInformationA
@ stdcall GetVolumeInformationByHandleW(ptr ptr long ptr ptr ptr ptr long) kernel32.GetVolumeInformationByHandleW
@ stdcall GetVolumeInformationW(wstr ptr long ptr ptr ptr ptr long) kernel32.GetVolumeInformationW
@ stdcall GetVolumeNameForVolumeMountPointW(wstr ptr long) kernel32.GetVolumeNameForVolumeMountPointW
@ stdcall GetVolumePathNameW(wstr ptr long) kernel32.GetVolumePathNameW
@ stdcall GetVolumePathNamesForVolumeNameW(wstr ptr long ptr) kernel32.GetVolumePathNamesForVolumeNameW
@ stdcall GetWindowsAccountDomainSid(ptr ptr ptr) advapi32.GetWindowsAccountDomainSid
@ stdcall GetWindowsDirectoryA(ptr long) kernel32.GetWindowsDirectoryA
@ stdcall GetWindowsDirectoryW(ptr long) kernel32.GetWindowsDirectoryW
@ stdcall GetWriteWatch(long ptr long ptr ptr ptr) kernel32.GetWriteWatch
# @ stub GetWsChanges
# @ stub GetWsChangesEx
# @ stub GetXStateFeaturesMask
@ stdcall GlobalAlloc(long long) kernel32.GlobalAlloc
@ stdcall GlobalFree(long) kernel32.GlobalFree
@ stdcall GlobalMemoryStatusEx(ptr) kernel32.GlobalMemoryStatusEx
# @ stub GuardCheckLongJumpTarget
# @ stub HasPolicyForegroundProcessingCompletedInternal
@ stdcall HashData(ptr long ptr long)
@ stdcall HeapAlloc(long long long) kernel32.HeapAlloc
@ stdcall HeapCompact(long long) kernel32.HeapCompact
@ stdcall HeapCreate(long long long) kernel32.HeapCreate
@ stdcall HeapDestroy(long) kernel32.HeapDestroy
@ stdcall HeapFree(long long ptr) kernel32.HeapFree
@ stdcall HeapLock(long) kernel32.HeapLock
@ stdcall HeapQueryInformation(long long ptr long ptr) kernel32.HeapQueryInformation
@ stdcall HeapReAlloc(long long ptr long) kernel32.HeapReAlloc
@ stdcall HeapSetInformation(ptr long ptr long) kernel32.HeapSetInformation
@ stdcall HeapSize(long long ptr) kernel32.HeapSize
@ stub HeapSummary
@ stdcall HeapUnlock(long) kernel32.HeapUnlock
@ stdcall HeapValidate(long long ptr) kernel32.HeapValidate
@ stdcall HeapWalk(long ptr) kernel32.HeapWalk
@ stdcall IdnToAscii(long wstr long ptr long) kernel32.IdnToAscii
@ stdcall IdnToNameprepUnicode(long wstr long ptr long) kernel32.IdnToNameprepUnicode
@ stdcall IdnToUnicode(long wstr long ptr long) kernel32.IdnToUnicode
@ stdcall ImpersonateAnonymousToken(long) advapi32.ImpersonateAnonymousToken
@ stdcall ImpersonateLoggedOnUser(long) advapi32.ImpersonateLoggedOnUser
@ stdcall ImpersonateNamedPipeClient(long) advapi32.ImpersonateNamedPipeClient
@ stdcall ImpersonateSelf(long) advapi32.ImpersonateSelf
# @ stub IncrementPackageStatusVersion
@ stdcall InitOnceBeginInitialize(ptr long ptr ptr) kernel32.InitOnceBeginInitialize
@ stdcall InitOnceComplete(ptr long ptr) kernel32.InitOnceComplete
@ stdcall InitOnceExecuteOnce(ptr ptr ptr ptr) kernel32.InitOnceExecuteOnce
@ stdcall InitOnceInitialize(ptr) kernel32.InitOnceInitialize
@ stdcall InitializeAcl(ptr long long) advapi32.InitializeAcl
@ stdcall InitializeConditionVariable(ptr) kernel32.InitializeConditionVariable
# @ stub InitializeContext
@ stdcall InitializeCriticalSection(ptr) kernel32.InitializeCriticalSection
@ stdcall InitializeCriticalSectionAndSpinCount(ptr long) kernel32.InitializeCriticalSectionAndSpinCount
@ stdcall InitializeCriticalSectionEx(ptr long long) kernel32.InitializeCriticalSectionEx
# @ stub InitializeEnclave
@ stdcall InitializeProcThreadAttributeList(ptr long long ptr) kernel32.InitializeProcThreadAttributeList
# @ stub InitializeProcessForWsWatch
@ stdcall InitializeSListHead(ptr) kernel32.InitializeSListHead
@ stdcall InitializeSRWLock(ptr) kernel32.InitializeSRWLock
@ stdcall InitializeSecurityDescriptor(ptr long) advapi32.InitializeSecurityDescriptor
@ stdcall InitializeSid(ptr ptr long) advapi32.InitializeSid
# @ stub InitializeSynchronizationBarrier
# @ stub InstallELAMCertificateInfo
@ stdcall -arch=i386 InterlockedCompareExchange(ptr long long) kernel32.InterlockedCompareExchange
@ stdcall -arch=i386 -ret64 InterlockedCompareExchange64(ptr int64 int64) kernel32.InterlockedCompareExchange64
@ stdcall -arch=i386 InterlockedDecrement(ptr) kernel32.InterlockedDecrement
@ stdcall -arch=i386 InterlockedExchange(ptr long) kernel32.InterlockedExchange
@ stdcall -arch=i386 InterlockedExchangeAdd(ptr long ) kernel32.InterlockedExchangeAdd
@ stdcall InterlockedFlushSList(ptr) kernel32.InterlockedFlushSList
@ stdcall -arch=i386 InterlockedIncrement(ptr) kernel32.InterlockedIncrement
@ stdcall InterlockedPopEntrySList(ptr) kernel32.InterlockedPopEntrySList
@ stdcall InterlockedPushEntrySList(ptr ptr) kernel32.InterlockedPushEntrySList
@ stdcall -fastcall InterlockedPushListSList(ptr ptr ptr long) kernel32.InterlockedPushListSList
@ stdcall InterlockedPushListSListEx(ptr ptr ptr long) kernel32.InterlockedPushListSListEx
@ stub InternalLcidToName
@ stub Internal_EnumCalendarInfo
@ stub Internal_EnumDateFormats
@ stub Internal_EnumLanguageGroupLocales
@ stub Internal_EnumSystemCodePages
@ stub Internal_EnumSystemLanguageGroups
@ stub Internal_EnumSystemLocales
@ stub Internal_EnumTimeFormats
@ stub Internal_EnumUILanguages
# @ stub InternetTimeFromSystemTimeA
# @ stub InternetTimeFromSystemTimeW
# @ stub InternetTimeToSystemTimeA
# @ stub InternetTimeToSystemTimeW
# @ stub InvalidateAppModelVersionCache
@ stub InvalidateTzSpecificCache
@ stdcall IsCharAlphaA(long)
@ stdcall IsCharAlphaNumericA(long)
@ stdcall IsCharAlphaNumericW(long)
@ stdcall IsCharAlphaW(long)
@ stdcall IsCharBlankW(long)
@ stdcall IsCharCntrlW(long)
@ stdcall IsCharDigitW(long)
@ stdcall IsCharLowerA(long)
@ stdcall IsCharLowerW(long)
@ stdcall IsCharPunctW(long)
@ stdcall IsCharSpaceA(long)
@ stdcall IsCharSpaceW(long)
@ stdcall IsCharUpperA(long)
@ stdcall IsCharUpperW(long)
@ stdcall IsCharXDigitW(long)
@ stdcall IsDBCSLeadByte(long) kernel32.IsDBCSLeadByte
@ stdcall IsDBCSLeadByteEx(long long) kernel32.IsDBCSLeadByteEx
@ stdcall IsDebuggerPresent() kernel32.IsDebuggerPresent
# @ stub IsDeveloperModeEnabled
# @ stub IsDeveloperModePolicyApplied
# @ stub IsEnclaveTypeSupported
# @ stub IsGlobalizationUserSettingsKeyRedirected
@ stdcall IsInternetESCEnabled()
@ stub IsNLSDefinedString
@ stdcall IsNormalizedString(long wstr long) kernel32.IsNormalizedString
# @ stub IsProcessCritical
@ stdcall IsProcessInJob(long long ptr) kernel32.IsProcessInJob
@ stdcall IsProcessorFeaturePresent(long) kernel32.IsProcessorFeaturePresent
# @ stub IsSideloadingEnabled
# @ stub IsSideloadingPolicyApplied
# @ stub IsSyncForegroundPolicyRefresh
@ stdcall IsThreadAFiber() kernel32.IsThreadAFiber
@ stdcall IsThreadpoolTimerSet(ptr) kernel32.IsThreadpoolTimerSet
# @ stub IsTimeZoneRedirectionEnabled
@ stdcall IsTokenRestricted(long) advapi32.IsTokenRestricted
@ stdcall IsValidAcl(ptr) advapi32.IsValidAcl
@ stdcall IsValidCodePage(long) kernel32.IsValidCodePage
@ stdcall IsValidLanguageGroup(long long) kernel32.IsValidLanguageGroup
@ stdcall IsValidLocale(long long) kernel32.IsValidLocale
@ stdcall IsValidLocaleName(wstr) kernel32.IsValidLocaleName
# @ stub IsValidNLSVersion
@ stub IsValidRelativeSecurityDescriptor
@ stdcall IsValidSecurityDescriptor(ptr) advapi32.IsValidSecurityDescriptor
@ stdcall IsValidSid(ptr) advapi32.IsValidSid
@ stdcall IsWellKnownSid(ptr long) advapi32.IsWellKnownSid
@ stdcall IsWow64Process(ptr ptr) kernel32.IsWow64Process
# @ stub IsWow64Process2
@ stdcall K32EmptyWorkingSet(long) kernel32.K32EmptyWorkingSet
@ stdcall K32EnumDeviceDrivers(ptr long ptr) kernel32.K32EnumDeviceDrivers
@ stdcall K32EnumPageFilesA(ptr ptr) kernel32.K32EnumPageFilesA
@ stdcall K32EnumPageFilesW(ptr ptr) kernel32.K32EnumPageFilesW
@ stdcall K32EnumProcessModules(long ptr long ptr) kernel32.K32EnumProcessModules
@ stdcall K32EnumProcessModulesEx(long ptr long ptr long) kernel32.K32EnumProcessModulesEx
@ stdcall K32EnumProcesses(ptr long ptr) kernel32.K32EnumProcesses
@ stdcall K32GetDeviceDriverBaseNameA(ptr ptr long) kernel32.K32GetDeviceDriverBaseNameA
@ stdcall K32GetDeviceDriverBaseNameW(ptr ptr long) kernel32.K32GetDeviceDriverBaseNameW
@ stdcall K32GetDeviceDriverFileNameA(ptr ptr long) kernel32.K32GetDeviceDriverFileNameA
@ stdcall K32GetDeviceDriverFileNameW(ptr ptr long) kernel32.K32GetDeviceDriverFileNameW
@ stdcall K32GetMappedFileNameA(long ptr ptr long) kernel32.K32GetMappedFileNameA
@ stdcall K32GetMappedFileNameW(long ptr ptr long) kernel32.K32GetMappedFileNameW
@ stdcall K32GetModuleBaseNameA(long long ptr long) kernel32.K32GetModuleBaseNameA
@ stdcall K32GetModuleBaseNameW(long long ptr long) kernel32.K32GetModuleBaseNameW
@ stdcall K32GetModuleFileNameExA(long long ptr long) kernel32.K32GetModuleFileNameExA
@ stdcall K32GetModuleFileNameExW(long long ptr long) kernel32.K32GetModuleFileNameExW
@ stdcall K32GetModuleInformation(long long ptr long) kernel32.K32GetModuleInformation
@ stdcall K32GetPerformanceInfo(ptr long) kernel32.K32GetPerformanceInfo
@ stdcall K32GetProcessImageFileNameA(long ptr long) kernel32.K32GetProcessImageFileNameA
@ stdcall K32GetProcessImageFileNameW(long ptr long) kernel32.K32GetProcessImageFileNameW
@ stdcall K32GetProcessMemoryInfo(long ptr long) kernel32.K32GetProcessMemoryInfo
@ stdcall K32GetWsChanges(long ptr long) kernel32.K32GetWsChanges
@ stdcall K32GetWsChangesEx(long ptr ptr) kernel32.K32GetWsChangesEx
@ stdcall K32InitializeProcessForWsWatch(long) kernel32.K32InitializeProcessForWsWatch
@ stdcall K32QueryWorkingSet(long ptr long) kernel32.K32QueryWorkingSet
@ stdcall K32QueryWorkingSetEx(long ptr long) kernel32.K32QueryWorkingSetEx
@ stub KernelBaseGetGlobalData
@ stdcall LCIDToLocaleName(long ptr long long) kernel32.LCIDToLocaleName
@ stdcall LCMapStringA(long long str long ptr long) kernel32.LCMapStringA
@ stdcall LCMapStringEx(wstr long wstr long ptr long ptr ptr long) kernel32.LCMapStringEx
@ stdcall LCMapStringW(long long wstr long ptr long) kernel32.LCMapStringW
# @ stub LeaveCriticalPolicySectionInternal
@ stdcall LeaveCriticalSection(ptr) kernel32.LeaveCriticalSection
@ stdcall LeaveCriticalSectionWhenCallbackReturns(ptr ptr) kernel32.LeaveCriticalSectionWhenCallbackReturns
# @ stub LoadAppInitDlls
# @ stub LoadEnclaveData
@ stdcall LoadLibraryA(str) kernel32.LoadLibraryA
@ stdcall LoadLibraryExA( str long long) kernel32.LoadLibraryExA
@ stdcall LoadLibraryExW(wstr long long) kernel32.LoadLibraryExW
@ stdcall LoadLibraryW(wstr) kernel32.LoadLibraryW
# @ stub LoadPackagedLibrary
@ stdcall LoadResource(long long) kernel32.LoadResource
@ stdcall LoadStringA(long long ptr long)
@ stub LoadStringBaseExW
@ stub LoadStringByReference
@ stdcall LoadStringW(long long ptr long)
@ stdcall LocalAlloc(long long) kernel32.LocalAlloc
@ stdcall LocalFileTimeToFileTime(ptr ptr) kernel32.LocalFileTimeToFileTime
@ stdcall LocalFree(long) kernel32.LocalFree
@ stdcall LocalLock(long) kernel32.LocalLock
@ stdcall LocalReAlloc(long long long) kernel32.LocalReAlloc
@ stdcall LocalUnlock(long) kernel32.LocalUnlock
@ stdcall LocaleNameToLCID(wstr long) kernel32.LocaleNameToLCID
# @ stub LocateXStateFeature
@ stdcall LockFile(long long long long long) kernel32.LockFile
@ stdcall LockFileEx(long long long long long ptr) kernel32.LockFileEx
@ stdcall LockResource(long) kernel32.LockResource
@ stdcall MakeAbsoluteSD(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) advapi32.MakeAbsoluteSD
@ stub MakeAbsoluteSD2
@ stdcall MakeSelfRelativeSD(ptr ptr ptr) advapi32.MakeSelfRelativeSD
@ stdcall MapGenericMask(ptr ptr) advapi32.MapGenericMask
# @ stub MapPredefinedHandleInternal
@ stdcall MapUserPhysicalPages(ptr long ptr) kernel32.MapUserPhysicalPages
@ stdcall MapViewOfFile(long long long long long) kernel32.MapViewOfFile
@ stdcall MapViewOfFileEx(long long long long long ptr) kernel32.MapViewOfFileEx
@ stub MapViewOfFileExNuma
# @ stub MapViewOfFileFromApp
@ stdcall MoveFileExW(wstr wstr long) kernel32.MoveFileExW
# @ stub MoveFileWithProgressTransactedW
@ stdcall MoveFileWithProgressW(wstr wstr ptr ptr long) kernel32.MoveFileWithProgressW
@ stdcall MulDiv(long long long) kernel32.MulDiv
@ stdcall MultiByteToWideChar(long long str long ptr long) kernel32.MultiByteToWideChar
# @ stub NamedPipeEventEnum
# @ stub NamedPipeEventSelect
@ stdcall NeedCurrentDirectoryForExePathA(str) kernel32.NeedCurrentDirectoryForExePathA
@ stdcall NeedCurrentDirectoryForExePathW(wstr) kernel32.NeedCurrentDirectoryForExePathW
@ stub NlsCheckPolicy
@ stub NlsDispatchAnsiEnumProc
@ stub NlsEventDataDescCreate
@ stub NlsGetACPFromLocale
@ stub NlsGetCacheUpdateCount
@ stub NlsIsUserDefaultLocale
@ stub NlsUpdateLocale
@ stub NlsUpdateSystemLocale
@ stub NlsValidateLocale
@ stub NlsWriteEtwEvent
@ stdcall NormalizeString(long wstr long ptr long) kernel32.NormalizeString
@ stub NotifyMountMgr
@ stub NotifyRedirectedStringChange
@ stdcall ObjectCloseAuditAlarmW(wstr ptr long) advapi32.ObjectCloseAuditAlarmW
@ stdcall ObjectDeleteAuditAlarmW(wstr ptr long) advapi32.ObjectDeleteAuditAlarmW
@ stdcall ObjectOpenAuditAlarmW(wstr ptr wstr wstr ptr long long long ptr long long ptr) advapi32.ObjectOpenAuditAlarmW
@ stdcall ObjectPrivilegeAuditAlarmW(wstr ptr long long ptr long) advapi32.ObjectPrivilegeAuditAlarmW
# @ stub OfferVirtualMemory
@ stdcall OpenEventA(long long str) kernel32.OpenEventA
@ stdcall OpenEventW(long long wstr) kernel32.OpenEventW
@ stdcall OpenFileById(long ptr long long ptr long) kernel32.OpenFileById
# @ stub OpenFileMappingFromApp
@ stdcall OpenFileMappingW(long long wstr) kernel32.OpenFileMappingW
# @ stub OpenGlobalizationUserSettingsKey
@ stdcall OpenMutexW(long long wstr) kernel32.OpenMutexW
# @ stub OpenPackageInfoByFullName
# @ stub OpenPackageInfoByFullNameForUser
# @ stub OpenPrivateNamespaceW
@ stdcall OpenProcess(long long long) kernel32.OpenProcess
@ stdcall OpenProcessToken(long long ptr) advapi32.OpenProcessToken
@ stub OpenRegKey
@ stdcall OpenSemaphoreW(long long wstr) kernel32.OpenSemaphoreW
# @ stub OpenState
# @ stub OpenStateAtom
# @ stub OpenStateExplicit
# @ stub OpenStateExplicitForUserSid
# @ stub OpenStateExplicitForUserSidString
@ stdcall OpenThread(long long long) kernel32.OpenThread
@ stdcall OpenThreadToken(long long long ptr) advapi32.OpenThreadToken
@ stdcall OpenWaitableTimerW(long long wstr) kernel32.OpenWaitableTimerW
@ stdcall OutputDebugStringA(str) kernel32.OutputDebugStringA
@ stdcall OutputDebugStringW(wstr) kernel32.OutputDebugStringW
# @ stub OverrideRoamingDataModificationTimesInRange
# @ stub PackageFamilyNameFromFullName
# @ stub PackageFamilyNameFromId
# @ stub PackageFamilyNameFromProductId
# @ stub PackageFullNameFromId
# @ stub PackageFullNameFromProductId
# @ stub PackageIdFromFullName
# @ stub PackageIdFromProductId
# @ stub PackageNameAndPublisherIdFromFamilyName
# @ stub PackageRelativeApplicationIdFromProductId
# @ stub PackageSidFromFamilyName
# @ stub PackageSidFromProductId
# @ stub ParseApplicationUserModelId
@ stdcall ParseURLA(str ptr)
@ stdcall ParseURLW(wstr ptr)
@ stdcall PathAddBackslashA(str)
@ stdcall PathAddBackslashW(wstr)
@ stdcall PathAddExtensionA(str str)
@ stdcall PathAddExtensionW(wstr wstr)
@ stdcall PathAllocCanonicalize(wstr long ptr)
@ stdcall PathAllocCombine(wstr wstr long ptr)
@ stdcall PathAppendA(str str)
@ stdcall PathAppendW(wstr wstr)
@ stdcall PathCanonicalizeA(ptr str)
@ stdcall PathCanonicalizeW(ptr wstr)
@ stdcall PathCchAddBackslash(wstr long)
@ stdcall PathCchAddBackslashEx(wstr long ptr ptr)
@ stdcall PathCchAddExtension(wstr long wstr)
@ stdcall PathCchAppend(wstr long wstr)
@ stdcall PathCchAppendEx(wstr long wstr long)
@ stdcall PathCchCanonicalize(ptr long wstr)
@ stdcall PathCchCanonicalizeEx(ptr long wstr long)
@ stdcall PathCchCombine(ptr long wstr wstr)
@ stdcall PathCchCombineEx(ptr long wstr wstr long)
@ stdcall PathCchFindExtension(wstr long ptr)
@ stdcall PathCchIsRoot(wstr)
@ stdcall PathCchRemoveBackslash(wstr long)
@ stdcall PathCchRemoveBackslashEx(wstr long ptr ptr)
@ stdcall PathCchRemoveExtension(wstr long)
@ stdcall PathCchRemoveFileSpec(wstr long)
@ stdcall PathCchRenameExtension(wstr long wstr)
@ stdcall PathCchSkipRoot(wstr ptr)
@ stdcall PathCchStripPrefix(wstr long)
@ stdcall PathCchStripToRoot(wstr long)
@ stdcall PathCombineA(ptr str str)
@ stdcall PathCombineW(ptr wstr wstr)
@ stdcall PathCommonPrefixA(str str ptr)
@ stdcall PathCommonPrefixW(wstr wstr ptr)
@ stdcall PathCreateFromUrlA(str ptr ptr long)
@ stdcall PathCreateFromUrlAlloc(wstr ptr long)
@ stdcall PathCreateFromUrlW(wstr ptr ptr long)
@ stdcall PathFileExistsA(str)
@ stdcall PathFileExistsW(wstr)
@ stdcall PathFindExtensionA(str)
@ stdcall PathFindExtensionW(wstr)
@ stdcall PathFindFileNameA(str)
@ stdcall PathFindFileNameW(wstr)
@ stdcall PathFindNextComponentA(str)
@ stdcall PathFindNextComponentW(wstr)
@ stdcall PathGetArgsA(str)
@ stdcall PathGetArgsW(wstr)
@ stdcall PathGetCharTypeA(long)
@ stdcall PathGetCharTypeW(long)
@ stdcall PathGetDriveNumberA(str)
@ stdcall PathGetDriveNumberW(wstr)
@ stdcall PathIsFileSpecA(str)
@ stdcall PathIsFileSpecW(wstr)
@ stdcall PathIsLFNFileSpecA(str)
@ stdcall PathIsLFNFileSpecW(wstr)
@ stdcall PathIsPrefixA(str str)
@ stdcall PathIsPrefixW(wstr wstr)
@ stdcall PathIsRelativeA(str)
@ stdcall PathIsRelativeW(wstr)
@ stdcall PathIsRootA(str)
@ stdcall PathIsRootW(wstr)
@ stdcall PathIsSameRootA(str str)
@ stdcall PathIsSameRootW(wstr wstr)
@ stdcall PathIsUNCA(str)
@ stdcall PathIsUNCEx(wstr ptr)
@ stdcall PathIsUNCServerA(str)
@ stdcall PathIsUNCServerShareA(str)
@ stdcall PathIsUNCServerShareW(wstr)
@ stdcall PathIsUNCServerW(wstr)
@ stdcall PathIsUNCW(wstr)
@ stdcall PathIsURLA(str)
@ stdcall PathIsURLW(wstr)
@ stdcall PathIsValidCharA(long long)
@ stdcall PathIsValidCharW(long long)
@ stdcall PathMatchSpecA(str str)
# @ stub PathMatchSpecExA
# @ stub PathMatchSpecExW
@ stdcall PathMatchSpecW(wstr wstr)
@ stdcall PathParseIconLocationA(str)
@ stdcall PathParseIconLocationW(wstr)
@ stdcall PathQuoteSpacesA(str)
@ stdcall PathQuoteSpacesW(wstr)
@ stdcall PathRelativePathToA(ptr str long str long)
@ stdcall PathRelativePathToW(ptr wstr long wstr long)
@ stdcall PathRemoveBackslashA(str)
@ stdcall PathRemoveBackslashW(wstr)
@ stdcall PathRemoveBlanksA(str)
@ stdcall PathRemoveBlanksW(wstr)
@ stdcall PathRemoveExtensionA(str)
@ stdcall PathRemoveExtensionW(wstr)
@ stdcall PathRemoveFileSpecA(str)
@ stdcall PathRemoveFileSpecW(wstr)
@ stdcall PathRenameExtensionA(str str)
@ stdcall PathRenameExtensionW(wstr wstr)
@ stdcall PathSearchAndQualifyA(str ptr long)
@ stdcall PathSearchAndQualifyW(wstr ptr long)
@ stdcall PathSkipRootA(str)
@ stdcall PathSkipRootW(wstr)
@ stdcall PathStripPathA(str)
@ stdcall PathStripPathW(wstr)
@ stdcall PathStripToRootA(str)
@ stdcall PathStripToRootW(wstr)
@ stdcall PathUnExpandEnvStringsA(str ptr long)
@ stdcall PathUnExpandEnvStringsW(wstr ptr long)
@ stdcall PathUnquoteSpacesA(str)
@ stdcall PathUnquoteSpacesW(wstr)
# @ stub PcwAddQueryItem
# @ stub PcwClearCounterSetSecurity
# @ stub PcwCollectData
# @ stub PcwCompleteNotification
# @ stub PcwCreateNotifier
# @ stub PcwCreateQuery
# @ stub PcwDisconnectCounterSet
# @ stub PcwEnumerateInstances
# @ stub PcwIsNotifierAlive
# @ stub PcwQueryCounterSetSecurity
# @ stub PcwReadNotificationData
# @ stub PcwRegisterCounterSet
# @ stub PcwRemoveQueryItem
# @ stub PcwSendNotification
# @ stub PcwSendStatelessNotification
# @ stub PcwSetCounterSetSecurity
# @ stub PcwSetQueryItemUserData
@ stdcall PeekConsoleInputA(ptr ptr long ptr) kernel32.PeekConsoleInputA
@ stdcall PeekConsoleInputW(ptr ptr long ptr) kernel32.PeekConsoleInputW
@ stdcall PeekNamedPipe(long ptr long ptr ptr ptr) kernel32.PeekNamedPipe
@ stdcall PerfCreateInstance(long ptr wstr long) advapi32.PerfCreateInstance
# @ stub PerfDecrementULongCounterValue
# @ stub PerfDecrementULongLongCounterValue
@ stdcall PerfDeleteInstance(long ptr) advapi32.PerfDeleteInstance
# @ stub PerfIncrementULongCounterValue
# @ stub PerfIncrementULongLongCounterValue
# @ stub PerfQueryInstance
@ stdcall PerfSetCounterRefValue(long ptr long ptr) advapi32.PerfSetCounterRefValue
@ stdcall PerfSetCounterSetInfo(long ptr long) advapi32.PerfSetCounterSetInfo
# @ stub PerfSetULongCounterValue
# @ stub PerfSetULongLongCounterValue
@ stdcall PerfStartProvider(ptr ptr ptr) advapi32.PerfStartProvider
@ stdcall PerfStartProviderEx(ptr ptr ptr) advapi32.PerfStartProviderEx
@ stdcall PerfStopProvider(long) advapi32.PerfStopProvider
# @ stub PoolPerAppKeyStateInternal
@ stdcall PostQueuedCompletionStatus(long long ptr ptr) kernel32.PostQueuedCompletionStatus
# @ stub PrefetchVirtualMemory
@ stub PrivCopyFileExW
@ stdcall PrivilegeCheck(ptr ptr ptr) advapi32.PrivilegeCheck
@ stdcall PrivilegedServiceAuditAlarmW(wstr wstr long ptr long) advapi32.PrivilegedServiceAuditAlarmW
@ stdcall ProcessIdToSessionId(long ptr) kernel32.ProcessIdToSessionId
# @ stub ProductIdFromPackageFamilyName
# @ stub PsmCreateKey
# @ stub PsmCreateKeyWithDynamicId
# @ stub PsmEqualApplication
# @ stub PsmEqualPackage
# @ stub PsmGetApplicationNameFromKey
# @ stub PsmGetKeyFromProcess
# @ stub PsmGetKeyFromToken
# @ stub PsmGetPackageFullNameFromKey
# @ stub PsmIsChildKey
# @ stub PsmIsDynamicKey
# @ stub PsmIsValidKey
# @ stub PssCaptureSnapshot
# @ stub PssDuplicateSnapshot
# @ stub PssFreeSnapshot
# @ stub PssQuerySnapshot
# @ stub PssWalkMarkerCreate
# @ stub PssWalkMarkerFree
# @ stub PssWalkMarkerGetPosition
# @ stub PssWalkMarkerSeekToBeginning
# @ stub PssWalkMarkerSetPosition
# @ stub PssWalkSnapshot
# @ stub PublishStateChangeNotification
@ stdcall PulseEvent(long) kernel32.PulseEvent
@ stdcall PurgeComm(long long) kernel32.PurgeComm
@ stdcall QISearch(ptr ptr ptr ptr)
@ stdcall QueryActCtxSettingsW(long ptr wstr wstr ptr long ptr) kernel32.QueryActCtxSettingsW
@ stdcall QueryActCtxW(long ptr ptr long ptr long ptr) kernel32.QueryActCtxW
@ stdcall QueryDepthSList(ptr) kernel32.QueryDepthSList
@ stdcall QueryDosDeviceW(wstr ptr long) kernel32.QueryDosDeviceW
@ stdcall QueryFullProcessImageNameA(ptr long ptr ptr) kernel32.QueryFullProcessImageNameA
@ stdcall QueryFullProcessImageNameW(ptr long ptr ptr) kernel32.QueryFullProcessImageNameW
# @ stub QueryIdleProcessorCycleTime
# @ stub QueryIdleProcessorCycleTimeEx
# @ stub QueryInterruptTime
# @ stub QueryInterruptTimePrecise
@ stdcall QueryMemoryResourceNotification(ptr ptr) kernel32.QueryMemoryResourceNotification
# @ stub QueryOptionalDelayLoadedAPI
@ stdcall QueryPerformanceCounter(ptr) kernel32.QueryPerformanceCounter
@ stdcall QueryPerformanceFrequency(ptr) kernel32.QueryPerformanceFrequency
@ stub QueryProcessAffinityUpdateMode
@ stdcall QueryProcessCycleTime(long ptr) kernel32.QueryProcessCycleTime
# @ stub QueryProtectedPolicy
@ stub QuerySecurityAccessMask
# @ stub QueryStateAtomValueInfo
# @ stub QueryStateContainerCreatedNew
# @ stub QueryStateContainerItemInfo
@ stdcall QueryThreadCycleTime(long ptr) kernel32.QueryThreadCycleTime
@ stub QueryThreadpoolStackInformation
@ stdcall QueryUnbiasedInterruptTime(ptr) kernel32.QueryUnbiasedInterruptTime
# @ stub QueryUnbiasedInterruptTimePrecise
# @ stub QueryVirtualMemoryInformation
# @ stub QueryWorkingSet
# @ stub QueryWorkingSetEx
@ stdcall QueueUserAPC(ptr long long) kernel32.QueueUserAPC
@ stdcall QueueUserWorkItem(ptr ptr long) kernel32.QueueUserWorkItem
# @ stub QuirkGetData
# @ stub QuirkGetData2
@ stdcall QuirkIsEnabled(ptr)
# @ stub QuirkIsEnabled2
@ stdcall QuirkIsEnabled3(ptr ptr)
# @ stub QuirkIsEnabledForPackage
# @ stub QuirkIsEnabledForPackage2
# @ stub QuirkIsEnabledForPackage3
# @ stub QuirkIsEnabledForPackage4
# @ stub QuirkIsEnabledForProcess
@ stdcall RaiseException(long long long ptr) kernel32.RaiseException
# @ stub RaiseFailFastException
@ stdcall ReOpenFile(ptr long long long) kernel32.ReOpenFile
@ stdcall ReadConsoleA(long ptr long ptr ptr) kernel32.ReadConsoleA
@ stdcall ReadConsoleInputA(long ptr long ptr) kernel32.ReadConsoleInputA
@ stub ReadConsoleInputExA
@ stub ReadConsoleInputExW
@ stdcall ReadConsoleInputW(long ptr long ptr) kernel32.ReadConsoleInputW
@ stdcall ReadConsoleOutputA(long ptr long long ptr) kernel32.ReadConsoleOutputA
@ stdcall ReadConsoleOutputAttribute(long ptr long long ptr) kernel32.ReadConsoleOutputAttribute
@ stdcall ReadConsoleOutputCharacterA(long ptr long long ptr) kernel32.ReadConsoleOutputCharacterA
@ stdcall ReadConsoleOutputCharacterW(long ptr long long ptr) kernel32.ReadConsoleOutputCharacterW
@ stdcall ReadConsoleOutputW(long ptr long long ptr) kernel32.ReadConsoleOutputW
@ stdcall ReadConsoleW(long ptr long ptr ptr) kernel32.ReadConsoleW
@ stdcall ReadDirectoryChangesW(long ptr long long long ptr ptr ptr) kernel32.ReadDirectoryChangesW
@ stdcall ReadFile(long ptr long ptr ptr) kernel32.ReadFile
@ stdcall ReadFileEx(long ptr long ptr ptr) kernel32.ReadFileEx
@ stdcall ReadFileScatter(long ptr long ptr ptr) kernel32.ReadFileScatter
@ stdcall ReadProcessMemory(long ptr ptr long ptr) kernel32.ReadProcessMemory
# @ stub ReadStateAtomValue
# @ stub ReadStateContainerValue
# @ stub ReclaimVirtualMemory
# @ stub RefreshPolicyExInternal
# @ stub RefreshPolicyInternal
@ stdcall -private RegCloseKey(long) kernel32.RegCloseKey
@ stdcall RegCopyTreeW(long wstr long) advapi32.RegCopyTreeW
@ stdcall -private RegCreateKeyExA(long str long ptr long long ptr ptr ptr) kernel32.RegCreateKeyExA
# @ stub RegCreateKeyExInternalA
# @ stub RegCreateKeyExInternalW
@ stdcall -private RegCreateKeyExW(long wstr long ptr long long ptr ptr ptr) kernel32.RegCreateKeyExW
@ stdcall -private RegDeleteKeyExA(long str long long) kernel32.RegDeleteKeyExA
# @ stub RegDeleteKeyExInternalA
# @ stub RegDeleteKeyExInternalW
@ stdcall -private RegDeleteKeyExW(long wstr long long) kernel32.RegDeleteKeyExW
@ stdcall RegDeleteKeyValueA(long str str) advapi32.RegDeleteKeyValueA
@ stdcall RegDeleteKeyValueW(long wstr wstr) advapi32.RegDeleteKeyValueW
@ stdcall -private RegDeleteTreeA(long str) kernel32.RegDeleteTreeA
@ stdcall -private RegDeleteTreeW(long wstr) kernel32.RegDeleteTreeW
@ stdcall -private RegDeleteValueA(long str) kernel32.RegDeleteValueA
@ stdcall -private RegDeleteValueW(long wstr) kernel32.RegDeleteValueW
# @ stub RegDisablePredefinedCacheEx
@ stdcall -private RegEnumKeyExA(long long ptr ptr ptr ptr ptr ptr) kernel32.RegEnumKeyExA
@ stdcall -private RegEnumKeyExW(long long ptr ptr ptr ptr ptr ptr) kernel32.RegEnumKeyExW
@ stdcall -private RegEnumValueA(long long ptr ptr ptr ptr ptr ptr) kernel32.RegEnumValueA
@ stdcall -private RegEnumValueW(long long ptr ptr ptr ptr ptr ptr) kernel32.RegEnumValueW
@ stdcall -private RegFlushKey(long) kernel32.RegFlushKey
@ stdcall -private RegGetKeySecurity(long long ptr ptr) kernel32.RegGetKeySecurity
@ stdcall -private RegGetValueA(long str str long ptr ptr ptr) kernel32.RegGetValueA
@ stdcall -private RegGetValueW(long wstr wstr long ptr ptr ptr) kernel32.RegGetValueW
# @ stub RegKrnGetAppKeyEventAddressInternal
# @ stub RegKrnGetAppKeyLoaded
# @ stub RegKrnGetClassesEnumTableAddressInternal
# @ stub RegKrnGetHKEY_ClassesRootAddress
# @ stub RegKrnGetTermsrvRegistryExtensionFlags
# @ stub RegKrnResetAppKeyLoaded
# @ stub RegKrnSetDllHasThreadStateGlobal
# @ stub RegKrnSetTermsrvRegistryExtensionFlags
@ stdcall RegLoadAppKeyA(str ptr long long long) advapi32.RegLoadAppKeyA
@ stdcall RegLoadAppKeyW(wstr ptr long long long) advapi32.RegLoadAppKeyW
@ stdcall -private RegLoadKeyA(long str str) kernel32.RegLoadKeyA
@ stdcall -private RegLoadKeyW(long wstr wstr) kernel32.RegLoadKeyW
@ stdcall -private RegLoadMUIStringA(long str str long ptr long str) kernel32.RegLoadMUIStringA
@ stdcall -private RegLoadMUIStringW(long wstr wstr long ptr long wstr) kernel32.RegLoadMUIStringW
@ stdcall -private RegNotifyChangeKeyValue(long long long long long) kernel32.RegNotifyChangeKeyValue
@ stdcall -private RegOpenCurrentUser(long ptr) kernel32.RegOpenCurrentUser
@ stdcall -private RegOpenKeyExA(long str long long ptr) kernel32.RegOpenKeyExA
# @ stub RegOpenKeyExInternalA
# @ stub RegOpenKeyExInternalW
@ stdcall -private RegOpenKeyExW(long wstr long long ptr) kernel32.RegOpenKeyExW
@ stdcall -private RegOpenUserClassesRoot(ptr long long ptr) kernel32.RegOpenUserClassesRoot
@ stdcall -private RegQueryInfoKeyA(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) kernel32.RegQueryInfoKeyA
@ stdcall -private RegQueryInfoKeyW(long ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) kernel32.RegQueryInfoKeyW
@ stdcall -private RegQueryValueExA(long str ptr ptr ptr ptr) kernel32.RegQueryValueExA
@ stdcall -private RegQueryValueExW(long wstr ptr ptr ptr ptr) kernel32.RegQueryValueExW
@ stdcall -private RegRestoreKeyA(long str long) kernel32.RegRestoreKeyA
@ stdcall -private RegRestoreKeyW(long wstr long) kernel32.RegRestoreKeyW
@ stdcall RegSaveKeyExA(long str ptr long) advapi32.RegSaveKeyExA
@ stdcall RegSaveKeyExW(long wstr ptr long) advapi32.RegSaveKeyExW
@ stdcall -private RegSetKeySecurity(long long ptr) kernel32.RegSetKeySecurity
@ stdcall RegSetKeyValueA(long str str long ptr long) advapi32.RegSetKeyValueA
@ stdcall RegSetKeyValueW(long wstr wstr long ptr long) advapi32.RegSetKeyValueW
@ stdcall -private RegSetValueExA(long str long long ptr long) kernel32.RegSetValueExA
@ stdcall -private RegSetValueExW(long wstr long long ptr long) kernel32.RegSetValueExW
@ stdcall -private RegUnLoadKeyA(long str) kernel32.RegUnLoadKeyA
@ stdcall -private RegUnLoadKeyW(long wstr) kernel32.RegUnLoadKeyW
# @ stub RegisterBadMemoryNotification
# @ stub RegisterGPNotificationInternal
# @ stub RegisterStateChangeNotification
# @ stub RegisterStateLock
@ stdcall RegisterTraceGuidsW(ptr ptr ptr long ptr wstr wstr ptr) advapi32.RegisterTraceGuidsW
@ stdcall RegisterWaitForSingleObjectEx(long ptr ptr long long) kernel32.RegisterWaitForSingleObjectEx
@ stdcall ReleaseActCtx(ptr) kernel32.ReleaseActCtx
@ stdcall ReleaseMutex(long) kernel32.ReleaseMutex
@ stdcall ReleaseMutexWhenCallbackReturns(ptr long) kernel32.ReleaseMutexWhenCallbackReturns
@ stdcall ReleaseSRWLockExclusive(ptr) kernel32.ReleaseSRWLockExclusive
@ stdcall ReleaseSRWLockShared(ptr) kernel32.ReleaseSRWLockShared
@ stdcall ReleaseSemaphore(long long ptr) kernel32.ReleaseSemaphore
@ stdcall ReleaseSemaphoreWhenCallbackReturns(ptr long long) kernel32.ReleaseSemaphoreWhenCallbackReturns
# @ stub ReleaseStateLock
# @ stub RemapPredefinedHandleInternal
@ stdcall RemoveDirectoryA(str) kernel32.RemoveDirectoryA
@ stdcall RemoveDirectoryW(wstr) kernel32.RemoveDirectoryW
@ stdcall RemoveDllDirectory(ptr) kernel32.RemoveDllDirectory
# @ stub RemovePackageStatus
# @ stub RemovePackageStatusForUser
@ stdcall RemoveVectoredContinueHandler(ptr) kernel32.RemoveVectoredContinueHandler
@ stdcall RemoveVectoredExceptionHandler(ptr) kernel32.RemoveVectoredExceptionHandler
# @ stub ReplaceFileExInternal
@ stdcall ReplaceFileW(wstr wstr wstr long ptr ptr) kernel32.ReplaceFileW
@ stdcall ResetEvent(long) kernel32.ResetEvent
# @ stub ResetState
@ stdcall ResetWriteWatch(ptr long) kernel32.ResetWriteWatch
@ stdcall ResolveDelayLoadedAPI(ptr ptr ptr ptr ptr long) kernel32.ResolveDelayLoadedAPI
# @ stub ResolveDelayLoadsFromDll
@ stdcall ResolveLocaleName(wstr ptr long) kernel32.ResolveLocaleName
@ stdcall RestoreLastError(long) kernel32.RestoreLastError
@ stdcall ResumeThread(long) kernel32.ResumeThread
@ stdcall RevertToSelf() advapi32.RevertToSelf
# @ stub RsopLoggingEnabledInternal
# @ stub SHCoCreateInstance
@ stdcall SHExpandEnvironmentStringsA(str ptr long) kernel32.ExpandEnvironmentStringsA
@ stdcall SHExpandEnvironmentStringsW(wstr ptr long) kernel32.ExpandEnvironmentStringsW
@ stdcall SHLoadIndirectString(wstr ptr long ptr)
# @ stub SHLoadIndirectStringInternal
@ stdcall SHRegCloseUSKey(ptr)
@ stdcall SHRegCreateUSKeyA(str long long ptr long)
@ stdcall SHRegCreateUSKeyW(wstr long long ptr long)
@ stdcall SHRegDeleteEmptyUSKeyA(long str long)
@ stdcall SHRegDeleteEmptyUSKeyW(long wstr long)
@ stdcall SHRegDeleteUSValueA(long str long)
@ stdcall SHRegDeleteUSValueW(long wstr long)
@ stdcall SHRegEnumUSKeyA(long long str ptr long)
@ stdcall SHRegEnumUSKeyW(long long wstr ptr long)
@ stdcall SHRegEnumUSValueA(long long ptr ptr ptr ptr ptr long)
@ stdcall SHRegEnumUSValueW(long long ptr ptr ptr ptr ptr long)
@ stdcall SHRegGetBoolUSValueA(str str long long)
@ stdcall SHRegGetBoolUSValueW(wstr wstr long long)
@ stdcall SHRegGetUSValueA(str str ptr ptr ptr long ptr long)
@ stdcall SHRegGetUSValueW(wstr wstr ptr ptr ptr long ptr long)
@ stdcall SHRegOpenUSKeyA(str long long ptr long)
@ stdcall SHRegOpenUSKeyW(wstr long long ptr long)
@ stdcall SHRegQueryInfoUSKeyA(long ptr ptr ptr ptr long)
@ stdcall SHRegQueryInfoUSKeyW(long ptr ptr ptr ptr long)
@ stdcall SHRegQueryUSValueA(long str ptr ptr ptr long ptr long)
@ stdcall SHRegQueryUSValueW(long wstr ptr ptr ptr long ptr long)
@ stdcall SHRegSetUSValueA(str str long ptr long long)
@ stdcall SHRegSetUSValueW(wstr wstr long ptr long long)
@ stdcall SHRegWriteUSValueA(long str long ptr long long)
@ stdcall SHRegWriteUSValueW(long wstr long ptr long long)
@ stdcall SHTruncateString(str long)
# @ stub SaveAlternatePackageRootPath
# @ stub SaveStateRootFolderPath
@ stdcall ScrollConsoleScreenBufferA(long ptr ptr ptr ptr) kernel32.ScrollConsoleScreenBufferA
@ stdcall ScrollConsoleScreenBufferW(long ptr ptr ptr ptr) kernel32.ScrollConsoleScreenBufferW
@ stdcall SearchPathA(str str str long ptr ptr) kernel32.SearchPathA
@ stdcall SearchPathW(wstr wstr wstr long ptr ptr) kernel32.SearchPathW
@ stdcall SetAclInformation(ptr ptr long long) advapi32.SetAclInformation
# @ stub SetCachedSigningLevel
@ stdcall SetCalendarInfoW(long long long wstr) kernel32.SetCalendarInfoW
# @ stub SetClientDynamicTimeZoneInformation
# @ stub SetClientTimeZoneInformation
@ stdcall SetCommBreak(long) kernel32.SetCommBreak
@ stdcall SetCommConfig(long ptr long) kernel32.SetCommConfig
@ stdcall SetCommMask(long long) kernel32.SetCommMask
@ stdcall SetCommState(long ptr) kernel32.SetCommState
@ stdcall SetCommTimeouts(long ptr) kernel32.SetCommTimeouts
@ stdcall SetComputerNameA(str) kernel32.SetComputerNameA
# @ stub SetComputerNameEx2W
@ stdcall SetComputerNameExA(long str) kernel32.SetComputerNameExA
@ stdcall SetComputerNameExW(long wstr) kernel32.SetComputerNameExW
@ stdcall SetComputerNameW(wstr) kernel32.SetComputerNameW
@ stdcall SetConsoleActiveScreenBuffer(long) kernel32.SetConsoleActiveScreenBuffer
@ stdcall SetConsoleCP(long) kernel32.SetConsoleCP
@ stdcall SetConsoleCtrlHandler(ptr long) kernel32.SetConsoleCtrlHandler
@ stdcall SetConsoleCursorInfo(long ptr) kernel32.SetConsoleCursorInfo
@ stdcall SetConsoleCursorPosition(long long) kernel32.SetConsoleCursorPosition
@ stdcall SetConsoleInputExeNameA(str) kernel32.SetConsoleInputExeNameA
@ stdcall SetConsoleInputExeNameW(wstr) kernel32.SetConsoleInputExeNameW
@ stdcall SetConsoleMode(long long) kernel32.SetConsoleMode
@ stdcall SetConsoleOutputCP(long) kernel32.SetConsoleOutputCP
@ stdcall SetConsoleScreenBufferInfoEx(long ptr) kernel32.SetConsoleScreenBufferInfoEx
@ stdcall SetConsoleScreenBufferSize(long long) kernel32.SetConsoleScreenBufferSize
@ stdcall SetConsoleTextAttribute(long long) kernel32.SetConsoleTextAttribute
@ stdcall SetConsoleTitleW(wstr) kernel32.SetConsoleTitleW
@ stdcall SetConsoleWindowInfo(long long ptr) kernel32.SetConsoleWindowInfo
@ stdcall SetCriticalSectionSpinCount(ptr long) kernel32.SetCriticalSectionSpinCount
@ stdcall SetCurrentDirectoryA(str) kernel32.SetCurrentDirectoryA
@ stdcall SetCurrentDirectoryW(wstr) kernel32.SetCurrentDirectoryW
@ stdcall SetDefaultDllDirectories(long) kernel32.SetDefaultDllDirectories
# @ stub SetDynamicTimeZoneInformation
@ stdcall SetEndOfFile(long) kernel32.SetEndOfFile
@ stub SetEnvironmentStringsW
@ stdcall SetEnvironmentVariableA(str str) kernel32.SetEnvironmentVariableA
@ stdcall SetEnvironmentVariableW(wstr wstr) kernel32.SetEnvironmentVariableW
@ stdcall SetErrorMode(long) kernel32.SetErrorMode
@ stdcall SetEvent(long) kernel32.SetEvent
@ stdcall SetEventWhenCallbackReturns(ptr long) kernel32.SetEventWhenCallbackReturns
@ stdcall SetFileApisToANSI() kernel32.SetFileApisToANSI
@ stdcall SetFileApisToOEM() kernel32.SetFileApisToOEM
@ stdcall SetFileAttributesA(str long) kernel32.SetFileAttributesA
@ stdcall SetFileAttributesW(wstr long) kernel32.SetFileAttributesW
@ stdcall SetFileInformationByHandle(long long ptr long) kernel32.SetFileInformationByHandle
# @ stub SetFileIoOverlappedRange
@ stdcall SetFilePointer(long long ptr long) kernel32.SetFilePointer
@ stdcall SetFilePointerEx(long int64 ptr long) kernel32.SetFilePointerEx
@ stdcall SetFileSecurityW(wstr long ptr) advapi32.SetFileSecurityW
@ stdcall SetFileTime(long ptr ptr ptr) kernel32.SetFileTime
@ stdcall SetFileValidData(ptr int64) kernel32.SetFileValidData
@ stdcall SetHandleCount(long) kernel32.SetHandleCount
@ stdcall SetHandleInformation(long long long) kernel32.SetHandleInformation
# @ stub SetIsDeveloperModeEnabled
# @ stub SetIsSideloadingEnabled
@ stdcall SetKernelObjectSecurity(long long ptr) advapi32.SetKernelObjectSecurity
@ stub SetLastConsoleEventActive
@ stdcall SetLastError(long) kernel32.SetLastError
@ stdcall SetLocalTime(ptr) kernel32.SetLocalTime
@ stdcall SetLocaleInfoW(long long wstr) kernel32.SetLocaleInfoW
@ stdcall SetNamedPipeHandleState(long ptr ptr ptr) kernel32.SetNamedPipeHandleState
@ stdcall SetPriorityClass(long long) kernel32.SetPriorityClass
@ stdcall SetPrivateObjectSecurity(long ptr ptr ptr long) advapi32.SetPrivateObjectSecurity
@ stub SetPrivateObjectSecurityEx
@ stdcall SetProcessAffinityUpdateMode(long long) kernel32.SetProcessAffinityUpdateMode
# @ stub SetProcessDefaultCpuSets
# @ stub SetProcessGroupAffinity
# @ stub SetProcessInformation
@ stdcall SetProcessMitigationPolicy(long ptr long) kernel32.SetProcessMitigationPolicy
@ stdcall SetProcessPreferredUILanguages(long ptr ptr) kernel32.SetProcessPreferredUILanguages
@ stdcall SetProcessPriorityBoost(long long) kernel32.SetProcessPriorityBoost
@ stdcall SetProcessShutdownParameters(long long) kernel32.SetProcessShutdownParameters
# @ stub SetProcessValidCallTargets
@ stdcall SetProcessWorkingSetSizeEx(long long long long) kernel32.SetProcessWorkingSetSizeEx
# @ stub SetProtectedPolicy
# @ stub SetRoamingLastObservedChangeTime
@ stub SetSecurityAccessMask
@ stdcall SetSecurityDescriptorControl(ptr long long) advapi32.SetSecurityDescriptorControl
@ stdcall SetSecurityDescriptorDacl(ptr long ptr long) advapi32.SetSecurityDescriptorDacl
@ stdcall SetSecurityDescriptorGroup(ptr ptr long) advapi32.SetSecurityDescriptorGroup
@ stdcall SetSecurityDescriptorOwner(ptr ptr long) advapi32.SetSecurityDescriptorOwner
@ stub SetSecurityDescriptorRMControl
@ stdcall SetSecurityDescriptorSacl(ptr long ptr long) advapi32.SetSecurityDescriptorSacl
# @ stub SetStateVersion
@ stdcall SetStdHandle(long long) kernel32.SetStdHandle
@ stub SetStdHandleEx
@ stdcall SetSystemFileCacheSize(long long long) kernel32.SetSystemFileCacheSize
@ stdcall SetSystemTime(ptr) kernel32.SetSystemTime
@ stdcall SetSystemTimeAdjustment(long long) kernel32.SetSystemTimeAdjustment
@ stdcall SetThreadContext(long ptr) kernel32.SetThreadContext
# @ stub SetThreadDescription
@ stdcall SetThreadErrorMode(long ptr) kernel32.SetThreadErrorMode
@ stdcall SetThreadGroupAffinity(long ptr ptr) kernel32.SetThreadGroupAffinity
@ stdcall SetThreadIdealProcessor(long long) kernel32.SetThreadIdealProcessor
@ stdcall SetThreadIdealProcessorEx(long ptr ptr) kernel32.SetThreadIdealProcessorEx
# @ stub SetThreadInformation
@ stdcall SetThreadLocale(long) kernel32.SetThreadLocale
@ stdcall SetThreadPreferredUILanguages(long ptr ptr) kernel32.SetThreadPreferredUILanguages
@ stdcall SetThreadPriority(long long) kernel32.SetThreadPriority
@ stdcall SetThreadPriorityBoost(long long) kernel32.SetThreadPriorityBoost
# @ stub SetThreadSelectedCpuSets
@ stdcall SetThreadStackGuarantee(ptr) kernel32.SetThreadStackGuarantee
@ stdcall SetThreadToken(ptr ptr) advapi32.SetThreadToken
@ stdcall SetThreadUILanguage(long) kernel32.SetThreadUILanguage
@ stub SetThreadpoolStackInformation
@ stdcall SetThreadpoolThreadMaximum(ptr long) kernel32.SetThreadpoolThreadMaximum
@ stdcall SetThreadpoolThreadMinimum(ptr long) kernel32.SetThreadpoolThreadMinimum
@ stdcall SetThreadpoolTimer(ptr ptr long long) kernel32.SetThreadpoolTimer
# @ stub SetThreadpoolTimerEx
@ stdcall SetThreadpoolWait(ptr long ptr) kernel32.SetThreadpoolWait
# @ stub SetThreadpoolWaitEx
@ stdcall SetTimeZoneInformation(ptr) kernel32.SetTimeZoneInformation
@ stdcall SetTokenInformation(long long ptr long) advapi32.SetTokenInformation
@ stdcall SetUnhandledExceptionFilter(ptr) kernel32.SetUnhandledExceptionFilter
@ stdcall SetUserGeoID(long) kernel32.SetUserGeoID
@ stdcall SetWaitableTimer(long ptr long ptr ptr long) kernel32.SetWaitableTimer
@ stdcall SetWaitableTimerEx(long ptr long ptr ptr ptr long) kernel32.SetWaitableTimerEx
# @ stub SetXStateFeaturesMask
@ stdcall SetupComm(long long long) kernel32.SetupComm
# @ stub SharedLocalIsEnabled
@ stdcall SignalObjectAndWait(long long long long) kernel32.SignalObjectAndWait
@ stdcall SizeofResource(long long) kernel32.SizeofResource
@ stdcall Sleep(long) kernel32.Sleep
@ stdcall SleepConditionVariableCS(ptr ptr long) kernel32.SleepConditionVariableCS
@ stdcall SleepConditionVariableSRW(ptr ptr long long) kernel32.SleepConditionVariableSRW
@ stdcall SleepEx(long long) kernel32.SleepEx
@ stub SpecialMBToWC
@ stub StartThreadpoolIo
# @ stub StmAlignSize
# @ stub StmAllocateFlat
# @ stub StmCoalesceChunks
# @ stub StmDeinitialize
# @ stub StmInitialize
# @ stub StmReduceSize
# @ stub StmReserve
# @ stub StmWrite
@ stdcall StrCSpnA(str str)
@ stdcall StrCSpnIA(str str)
@ stdcall StrCSpnIW(wstr wstr)
@ stdcall StrCSpnW(wstr wstr)
@ stdcall StrCatBuffA(str str long)
@ stdcall StrCatBuffW(wstr wstr long)
@ stdcall StrCatChainW(ptr long long wstr)
@ stdcall StrChrA(str long)
# @ stub StrChrA_MB
@ stdcall StrChrIA(str long)
@ stdcall StrChrIW(wstr long)
# @ stub StrChrNIW
@ stdcall StrChrNW(wstr long long)
@ stdcall StrChrW(wstr long)
@ stdcall StrCmpCA(str str)
@ stdcall StrCmpCW(wstr wstr)
@ stdcall StrCmpICA(str str)
@ stdcall StrCmpICW(wstr wstr)
@ stdcall StrCmpIW(wstr wstr)
@ stdcall StrCmpLogicalW(wstr wstr)
@ stdcall StrCmpNA(str str long)
@ stdcall StrCmpNCA(str str long)
@ stdcall StrCmpNCW(wstr wstr long)
@ stdcall StrCmpNIA(str str long)
@ stdcall StrCmpNICA(str str long)
@ stdcall StrCmpNICW(wstr wstr long)
@ stdcall StrCmpNIW(wstr wstr long)
@ stdcall StrCmpNW(wstr wstr long)
@ stdcall StrCmpW(wstr wstr)
@ stdcall StrCpyNW(ptr wstr long)
@ stdcall StrCpyNXA(ptr str long)
@ stdcall StrCpyNXW(ptr wstr long)
@ stdcall StrDupA(str)
@ stdcall StrDupW(wstr)
@ stdcall StrIsIntlEqualA(long str str long)
@ stdcall StrIsIntlEqualW(long wstr wstr long)
@ stdcall StrPBrkA(str str)
@ stdcall StrPBrkW(wstr wstr)
@ stdcall StrRChrA(str str long)
@ stdcall StrRChrIA(str str long)
@ stdcall StrRChrIW(wstr wstr long)
@ stdcall StrRChrW(wstr wstr long)
@ stdcall StrRStrIA(str str str)
@ stdcall StrRStrIW(wstr wstr wstr)
@ stdcall StrSpnA(str str)
@ stdcall StrSpnW(wstr wstr)
@ stdcall StrStrA(str str)
@ stdcall StrStrIA(str str)
@ stdcall StrStrIW(wstr wstr)
@ stdcall StrStrNIW(wstr wstr long)
@ stdcall StrStrNW(wstr wstr long)
@ stdcall StrStrW(wstr wstr)
@ stdcall StrToInt64ExA(str long ptr)
@ stdcall StrToInt64ExW(wstr long ptr)
@ stdcall StrToIntA(str)
@ stdcall StrToIntExA(str long ptr)
@ stdcall StrToIntExW(wstr long ptr)
@ stdcall StrToIntW(wstr)
@ stdcall StrTrimA(str str)
@ stdcall StrTrimW(wstr wstr)
@ stdcall SubmitThreadpoolWork(ptr) kernel32.SubmitThreadpoolWork
# @ stub SubscribeEdpEnabledStateChange
# @ stub SubscribeStateChangeNotification
@ stdcall SuspendThread(long) kernel32.SuspendThread
@ stdcall SwitchToFiber(ptr) kernel32.SwitchToFiber
@ stdcall SwitchToThread() kernel32.SwitchToThread
@ stdcall SystemTimeToFileTime(ptr ptr) kernel32.SystemTimeToFileTime
@ stdcall SystemTimeToTzSpecificLocalTime(ptr ptr ptr) kernel32.SystemTimeToTzSpecificLocalTime
@ stub SystemTimeToTzSpecificLocalTimeEx
@ stdcall TerminateProcess(long long) kernel32.TerminateProcess
# @ stub TerminateProcessOnMemoryExhaustion
@ stdcall TerminateThread(long long) kernel32.TerminateThread
@ stdcall TlsAlloc() kernel32.TlsAlloc
@ stdcall TlsFree(long) kernel32.TlsFree
@ stdcall TlsGetValue(long) kernel32.TlsGetValue
@ stdcall TlsSetValue(long ptr) kernel32.TlsSetValue
@ stdcall TraceEvent(int64 ptr) advapi32.TraceEvent
@ varargs TraceMessage(int64 long ptr long) advapi32.TraceMessage
@ stdcall TraceMessageVa(int64 long ptr long ptr) advapi32.TraceMessageVa
@ stdcall TransactNamedPipe(long ptr long ptr long ptr ptr) kernel32.TransactNamedPipe
@ stdcall TransmitCommChar(long long) kernel32.TransmitCommChar
@ stdcall TryAcquireSRWLockExclusive(ptr) kernel32.TryAcquireSRWLockExclusive
@ stdcall TryAcquireSRWLockShared(ptr) kernel32.TryAcquireSRWLockShared
@ stdcall TryEnterCriticalSection(ptr) kernel32.TryEnterCriticalSection
@ stdcall TrySubmitThreadpoolCallback(ptr ptr ptr) kernel32.TrySubmitThreadpoolCallback
@ stdcall TzSpecificLocalTimeToSystemTime(ptr ptr ptr) kernel32.TzSpecificLocalTimeToSystemTime
@ stub TzSpecificLocalTimeToSystemTimeEx
@ stdcall UnhandledExceptionFilter(ptr) kernel32.UnhandledExceptionFilter
@ stdcall UnlockFile(long long long long long) kernel32.UnlockFile
@ stdcall UnlockFileEx(long long long long ptr) kernel32.UnlockFileEx
@ stdcall UnmapViewOfFile(ptr) kernel32.UnmapViewOfFile
# @ stub UnmapViewOfFileEx
# @ stub UnregisterBadMemoryNotification
# @ stub UnregisterGPNotificationInternal
# @ stub UnregisterStateChangeNotification
# @ stub UnregisterStateLock
@ stdcall UnregisterTraceGuids(int64) advapi32.UnregisterTraceGuids
@ stdcall UnregisterWaitEx(long long) kernel32.UnregisterWaitEx
# @ stub UnsubscribeEdpEnabledStateChange
# @ stub UnsubscribeStateChangeNotification
# @ stub UpdatePackageStatus
# @ stub UpdatePackageStatusForUser
@ stdcall UpdateProcThreadAttribute(ptr long long ptr long ptr ptr) kernel32.UpdateProcThreadAttribute
@ stdcall UrlApplySchemeA(str ptr ptr long)
@ stdcall UrlApplySchemeW(wstr ptr ptr long)
@ stdcall UrlCanonicalizeA(str ptr ptr long)
@ stdcall UrlCanonicalizeW(wstr ptr ptr long)
@ stdcall UrlCombineA(str str ptr ptr long)
@ stdcall UrlCombineW(wstr wstr ptr ptr long)
@ stdcall UrlCompareA(str str long)
@ stdcall UrlCompareW(wstr wstr long)
@ stdcall UrlCreateFromPathA(str ptr ptr long)
@ stdcall UrlCreateFromPathW(wstr ptr ptr long)
@ stdcall UrlEscapeA(str ptr ptr long)
@ stdcall UrlEscapeW(wstr ptr ptr long)
@ stdcall UrlFixupW(wstr wstr long)
@ stdcall UrlGetLocationA(str)
@ stdcall UrlGetLocationW(wstr)
@ stdcall UrlGetPartA(str ptr ptr long long)
@ stdcall UrlGetPartW(wstr ptr ptr long long)
@ stdcall UrlHashA(str ptr long)
@ stdcall UrlHashW(wstr ptr long)
@ stdcall UrlIsA(str long)
@ stdcall UrlIsNoHistoryA(str)
@ stdcall UrlIsNoHistoryW(wstr)
@ stdcall UrlIsOpaqueA(str)
@ stdcall UrlIsOpaqueW(wstr)
@ stdcall UrlIsW(wstr long)
@ stdcall UrlUnescapeA(str ptr ptr long)
@ stdcall UrlUnescapeW(wstr ptr ptr long)
@ stdcall VerFindFileA(long str str str ptr ptr ptr ptr)
@ stdcall VerFindFileW(long wstr wstr wstr ptr ptr ptr ptr)
@ stdcall VerLanguageNameA(long str long) kernel32.VerLanguageNameA
@ stdcall VerLanguageNameW(long wstr long) kernel32.VerLanguageNameW
@ stdcall VerQueryValueA(ptr str ptr ptr)
@ stdcall VerQueryValueW(ptr wstr ptr ptr)
@ stdcall -ret64 VerSetConditionMask(long long long long) kernel32.VerSetConditionMask
# @ stub VerifyApplicationUserModelId
# @ stub VerifyPackageFamilyName
# @ stub VerifyPackageFullName
# @ stub VerifyPackageId
# @ stub VerifyPackageRelativeApplicationId
# @ stub VerifyScripts
@ stdcall VirtualAlloc(ptr long long long) kernel32.VirtualAlloc
@ stdcall VirtualAllocEx(long ptr long long long) kernel32.VirtualAllocEx
@ stdcall VirtualAllocExNuma(long ptr long long long long) kernel32.VirtualAllocExNuma
# @ stub VirtualAllocFromApp
@ stdcall VirtualFree(ptr long long) kernel32.VirtualFree
@ stdcall VirtualFreeEx(long ptr long long) kernel32.VirtualFreeEx
@ stdcall VirtualLock(ptr long) kernel32.VirtualLock
@ stdcall VirtualProtect(ptr long long ptr) kernel32.VirtualProtect
@ stdcall VirtualProtectEx(long ptr long long ptr) kernel32.VirtualProtectEx
# @ stub VirtualProtectFromApp
@ stdcall VirtualQuery(ptr ptr long) kernel32.VirtualQuery
@ stdcall VirtualQueryEx(long ptr ptr long) kernel32.VirtualQueryEx
@ stdcall VirtualUnlock(ptr long) kernel32.VirtualUnlock
# @ stub WTSGetServiceSessionId
# @ stub WTSIsServerContainer
@ stdcall WaitCommEvent(long ptr ptr) kernel32.WaitCommEvent
@ stdcall WaitForDebugEvent(ptr long) kernel32.WaitForDebugEvent
# @ stub WaitForDebugEventEx
# @ stub WaitForMachinePolicyForegroundProcessingInternal
@ stdcall WaitForMultipleObjects(long ptr long long) kernel32.WaitForMultipleObjects
@ stdcall WaitForMultipleObjectsEx(long ptr long long long) kernel32.WaitForMultipleObjectsEx
@ stdcall WaitForSingleObject(long long) kernel32.WaitForSingleObject
@ stdcall WaitForSingleObjectEx(long long long) kernel32.WaitForSingleObjectEx
@ stub WaitForThreadpoolIoCallbacks
@ stdcall WaitForThreadpoolTimerCallbacks(ptr long) kernel32.WaitForThreadpoolTimerCallbacks
@ stdcall WaitForThreadpoolWaitCallbacks(ptr long) kernel32.WaitForThreadpoolWaitCallbacks
@ stdcall WaitForThreadpoolWorkCallbacks(ptr long) kernel32.WaitForThreadpoolWorkCallbacks
# @ stub WaitForUserPolicyForegroundProcessingInternal
@ stdcall WaitNamedPipeW(wstr long) kernel32.WaitNamedPipeW
@ stdcall WaitOnAddress(ptr ptr long long)
@ stdcall WakeAllConditionVariable(ptr) kernel32.WakeAllConditionVariable
@ stdcall WakeByAddressAll(ptr) ntdll.RtlWakeAddressAll
@ stdcall WakeByAddressSingle(ptr) ntdll.RtlWakeAddressSingle
@ stdcall WakeConditionVariable(ptr) kernel32.WakeConditionVariable
# @ stub WerGetFlags
@ stdcall WerRegisterFile(wstr long long) kernel32.WerRegisterFile
@ stdcall WerRegisterMemoryBlock(ptr long) kernel32.WerRegisterMemoryBlock
@ stdcall WerRegisterRuntimeExceptionModule(wstr ptr) kernel32.WerRegisterRuntimeExceptionModule
@ stdcall WerSetFlags(long) kernel32.WerSetFlags
# @ stub WerUnregisterFile
@ stdcall WerUnregisterMemoryBlock(ptr) kernel32.WerUnregisterMemoryBlock
@ stdcall WerUnregisterRuntimeExceptionModule(wstr ptr) kernel32.WerUnregisterRuntimeExceptionModule
# @ stub WerpNotifyLoadStringResource
# @ stub WerpNotifyUseStringResource
@ stdcall WideCharToMultiByte(long long wstr long ptr long ptr ptr) kernel32.WideCharToMultiByte
@ stdcall Wow64DisableWow64FsRedirection(ptr) kernel32.Wow64DisableWow64FsRedirection
@ stdcall Wow64RevertWow64FsRedirection(ptr) kernel32.Wow64RevertWow64FsRedirection
# @ stub Wow64SetThreadDefaultGuestMachine
# @ stub -arch=i386 Wow64Transition
@ stdcall WriteConsoleA(long ptr long ptr ptr) kernel32.WriteConsoleA
@ stdcall WriteConsoleInputA(long ptr long ptr) kernel32.WriteConsoleInputA
@ stdcall WriteConsoleInputW(long ptr long ptr) kernel32.WriteConsoleInputW
@ stdcall WriteConsoleOutputA(long ptr long long ptr) kernel32.WriteConsoleOutputA
@ stdcall WriteConsoleOutputAttribute(long ptr long long ptr) kernel32.WriteConsoleOutputAttribute
@ stdcall WriteConsoleOutputCharacterA(long ptr long long ptr) kernel32.WriteConsoleOutputCharacterA
@ stdcall WriteConsoleOutputCharacterW(long ptr long long ptr) kernel32.WriteConsoleOutputCharacterW
@ stdcall WriteConsoleOutputW(long ptr long long ptr) kernel32.WriteConsoleOutputW
@ stdcall WriteConsoleW(long ptr long ptr ptr) kernel32.WriteConsoleW
@ stdcall WriteFile(long ptr long ptr ptr) kernel32.WriteFile
@ stdcall WriteFileEx(long ptr long ptr ptr) kernel32.WriteFileEx
@ stdcall WriteFileGather(long ptr long ptr ptr) kernel32.WriteFileGather
@ stdcall WriteProcessMemory(long ptr ptr long ptr) kernel32.WriteProcessMemory
# @ stub WriteStateAtomValue
# @ stub WriteStateContainerValue
@ stdcall ZombifyActCtx(ptr) kernel32.ZombifyActCtx
# @ stub _AddMUIStringToCache
# @ stub _GetMUIStringFromCache
# @ stub _OpenMuiStringCache
@ stdcall -arch=x86_64 -private __C_specific_handler(ptr long ptr ptr) kernel32.__C_specific_handler
@ stdcall -arch=arm,x86_64 -private -norelay __chkstk() kernel32.__chkstk
# @ stub __dllonexit3
@ stub __misaligned_access
# @ stub __wgetmainargs
# @ stub _amsg_exit
# @ stub _c_exit
# @ stub _cexit
# @ stub _exit
# @ stub _initterm
# @ stub _initterm_e
# @ stub _invalid_parameter
@ stdcall -arch=x86_64 -private _local_unwind(ptr ptr) kernel32._local_unwind
# @ stub _onexit
# @ stub _purecall
# @ stub _time64
# @ stub atexit
# @ stub exit
# @ stub hgets
# @ stub hwprintf
@ stdcall lstrcmp(str str) kernel32.lstrcmp
@ stdcall lstrcmpA(str str) kernel32.lstrcmpA
@ stdcall lstrcmpW(wstr wstr) kernel32.lstrcmpW
@ stdcall lstrcmpi(str str) kernel32.lstrcmpi
@ stdcall lstrcmpiA(str str) kernel32.lstrcmpiA
@ stdcall lstrcmpiW(wstr wstr) kernel32.lstrcmpiW
@ stdcall lstrcpyn(ptr str long) kernel32.lstrcpyn
@ stdcall lstrcpynA(ptr str long) kernel32.lstrcpynA
@ stdcall lstrcpynW(ptr wstr long) kernel32.lstrcpynW
@ stdcall lstrlen(str) kernel32.lstrlen
@ stdcall lstrlenA(str) kernel32.lstrlenA
@ stdcall lstrlenW(wstr) kernel32.lstrlenW
# @ stub time
# @ stub wprintf
