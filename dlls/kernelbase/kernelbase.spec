@ stdcall AccessCheck(ptr long long ptr ptr ptr ptr ptr) advapi32.AccessCheck
@ stdcall AccessCheckAndAuditAlarmW(wstr ptr wstr wstr ptr long ptr long ptr ptr ptr) advapi32.AccessCheckAndAuditAlarmW
@ stdcall AccessCheckByType(ptr ptr long long ptr long ptr ptr ptr ptr ptr) advapi32.AccessCheckByType
@ stub AccessCheckByTypeAndAuditAlarmW
@ stub AccessCheckByTypeResultList
@ stub AccessCheckByTypeResultListAndAuditAlarmByHandleW
@ stub AccessCheckByTypeResultListAndAuditAlarmW
@ stdcall AcquireSRWLockExclusive(ptr) kernel32.AcquireSRWLockExclusive
@ stdcall AcquireSRWLockShared(ptr) kernel32.AcquireSRWLockShared
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
@ stub AddDllDirectory
@ stdcall AddMandatoryAce(ptr long long long ptr) advapi32.AddMandatoryAce
@ stdcall AdjustTokenGroups(long long ptr long ptr ptr) advapi32.AdjustTokenGroups
@ stdcall AdjustTokenPrivileges(long long ptr long ptr ptr) advapi32.AdjustTokenPrivileges
@ stdcall AllocateAndInitializeSid(ptr long long long long long long long long long ptr) advapi32.AllocateAndInitializeSid
@ stdcall AllocateLocallyUniqueId(ptr) advapi32.AllocateLocallyUniqueId
@ stdcall AreAllAccessesGranted(long long) advapi32.AreAllAccessesGranted
@ stdcall AreAnyAccessesGranted(long long) advapi32.AreAnyAccessesGranted
@ stdcall AreFileApisANSI() kernel32.AreFileApisANSI
@ stub BaseDllFreeResourceId
@ stub BaseDllMapResourceIdW
@ stub BaseGetProcessDllPath
@ stub BaseGetProcessExePath
@ stub BaseInvalidateDllSearchPathCache
@ stub BaseInvalidateProcessSearchPathCache
@ stub BaseReleaseProcessDllPath
@ stub BaseReleaseProcessExePath
@ stdcall Beep(long long) kernel32.Beep
@ stub BemCopyReference
@ stub BemCreateContractFrom
@ stub BemCreateReference
@ stub BemFreeContract
@ stub BemFreeReference
@ stdcall CallbackMayRunLong(ptr) kernel32.CallbackMayRunLong
@ stdcall CancelIoEx(long ptr) kernel32.CancelIoEx
@ stub CancelThreadpoolIo
@ stdcall CancelWaitableTimer(long) kernel32.CancelWaitableTimer
@ stdcall ChangeTimerQueueTimer(ptr ptr long long) kernel32.ChangeTimerQueueTimer
@ stub CheckGroupPolicyEnabled
@ stdcall CheckTokenMembership(long ptr ptr) advapi32.CheckTokenMembership
@ stdcall CloseHandle(long) kernel32.CloseHandle
@ stdcall CloseThreadpool(ptr) kernel32.CloseThreadpool
@ stdcall CloseThreadpoolCleanupGroup(ptr) kernel32.CloseThreadpoolCleanupGroup
@ stdcall CloseThreadpoolCleanupGroupMembers(ptr long ptr) kernel32.CloseThreadpoolCleanupGroupMembers
@ stub CloseThreadpoolIo
@ stdcall CloseThreadpoolTimer(ptr) kernel32.CloseThreadpoolTimer
@ stdcall CloseThreadpoolWait(ptr) kernel32.CloseThreadpoolWait
@ stdcall CloseThreadpoolWork(ptr) kernel32.CloseThreadpoolWork
@ stdcall CompareFileTime(ptr ptr) kernel32.CompareFileTime
@ stdcall CompareStringA(long long str long str long) kernel32.CompareStringA
@ stdcall CompareStringEx(wstr long wstr long wstr long ptr ptr long) kernel32.CompareStringEx
@ stdcall CompareStringOrdinal(wstr long wstr long long) kernel32.CompareStringOrdinal
@ stdcall CompareStringW(long long wstr long wstr long) kernel32.CompareStringW
@ stdcall ConnectNamedPipe(long ptr) kernel32.ConnectNamedPipe
@ stdcall ConvertDefaultLocale(long) kernel32.ConvertDefaultLocale
@ stdcall ConvertToAutoInheritPrivateObjectSecurity(ptr ptr ptr ptr long ptr) advapi32.ConvertToAutoInheritPrivateObjectSecurity
@ stdcall CopySid(long ptr ptr) advapi32.CopySid
@ stdcall CreateDirectoryA(str ptr) kernel32.CreateDirectoryA
@ stdcall CreateDirectoryW(wstr ptr) kernel32.CreateDirectoryW
@ stdcall CreateEventA(ptr long long str) kernel32.CreateEventA
@ stdcall CreateEventExA(ptr str long long) kernel32.CreateEventExA
@ stdcall CreateEventExW(ptr wstr long long) kernel32.CreateEventExW
@ stdcall CreateEventW(ptr long long wstr) kernel32.CreateEventW
@ stdcall CreateFileA(str long long ptr long long long) kernel32.CreateFileA
@ stub CreateFileMappingNumaW
@ stdcall CreateFileMappingW(long ptr long long long wstr) kernel32.CreateFileMappingW
@ stdcall CreateFileW(wstr long long ptr long long long) kernel32.CreateFileW
@ stdcall CreateIoCompletionPort(long long long long) kernel32.CreateIoCompletionPort
@ stdcall CreateMutexA(ptr long str) kernel32.CreateMutexA
@ stdcall CreateMutexExA(ptr str long long) kernel32.CreateMutexExA
@ stdcall CreateMutexExW(ptr wstr long long) kernel32.CreateMutexExW
@ stdcall CreateMutexW(ptr long wstr) kernel32.CreateMutexW
@ stdcall CreateNamedPipeW(wstr long long long long long long ptr) kernel32.CreateNamedPipeW
@ stdcall CreatePipe(ptr ptr ptr long) kernel32.CreatePipe
@ stdcall CreatePrivateObjectSecurity(ptr ptr ptr long long ptr) advapi32.CreatePrivateObjectSecurity
@ stdcall CreatePrivateObjectSecurityEx(ptr ptr ptr ptr long long long ptr) advapi32.CreatePrivateObjectSecurityEx
@ stdcall CreatePrivateObjectSecurityWithMultipleInheritance(ptr ptr ptr ptr long long long long ptr) advapi32.CreatePrivateObjectSecurityWithMultipleInheritance
@ stdcall CreateRemoteThread(long ptr long ptr long long ptr) kernel32.CreateRemoteThread
@ stub CreateRemoteThreadEx
@ stdcall CreateRestrictedToken(long long long ptr long ptr long ptr ptr) advapi32.CreateRestrictedToken
@ stdcall CreateSemaphoreExW(ptr long long wstr long long) kernel32.CreateSemaphoreExW
@ stdcall CreateThread(ptr long ptr long long ptr) kernel32.CreateThread
@ stdcall CreateThreadpool(ptr) kernel32.CreateThreadpool
@ stdcall CreateThreadpoolCleanupGroup() kernel32.CreateThreadpoolCleanupGroup
@ stub CreateThreadpoolIo
@ stdcall CreateThreadpoolTimer(ptr ptr ptr) kernel32.CreateThreadpoolTimer
@ stdcall CreateThreadpoolWait(ptr ptr ptr) kernel32.CreateThreadpoolWait
@ stdcall CreateThreadpoolWork(ptr ptr ptr) kernel32.CreateThreadpoolWork
@ stdcall CreateTimerQueue() kernel32.CreateTimerQueue
@ stdcall CreateTimerQueueTimer(ptr long ptr ptr long long long) kernel32.CreateTimerQueueTimer
@ stdcall CreateWaitableTimerExW(ptr wstr long long) kernel32.CreateWaitableTimerExW
@ stdcall CreateWellKnownSid(long ptr ptr ptr) advapi32.CreateWellKnownSid
@ stdcall DebugBreak() kernel32.DebugBreak
@ stdcall DecodePointer(ptr) kernel32.DecodePointer
@ stdcall DecodeSystemPointer(ptr) kernel32.DecodeSystemPointer
@ stdcall DefineDosDeviceW(long wstr wstr) kernel32.DefineDosDeviceW
@ stdcall DeleteAce(ptr long) advapi32.DeleteAce
@ stdcall DeleteCriticalSection(ptr) kernel32.DeleteCriticalSection
@ stdcall DeleteFileA(str) kernel32.DeleteFileA
@ stdcall DeleteFileW(wstr) kernel32.DeleteFileW
@ stub DeleteProcThreadAttributeList
@ stdcall DeleteTimerQueueEx(long long) kernel32.DeleteTimerQueueEx
@ stdcall DeleteTimerQueueTimer(long long long) kernel32.DeleteTimerQueueTimer
@ stdcall DeleteVolumeMountPointW(wstr) kernel32.DeleteVolumeMountPointW
@ stdcall DestroyPrivateObjectSecurity(ptr) advapi32.DestroyPrivateObjectSecurity
@ stdcall DeviceIoControl(long long ptr long ptr long ptr ptr) kernel32.DeviceIoControl
@ stdcall DisableThreadLibraryCalls(long) kernel32.DisableThreadLibraryCalls
@ stdcall DisassociateCurrentThreadFromCallback(ptr) kernel32.DisassociateCurrentThreadFromCallback
@ stdcall DisconnectNamedPipe(long) kernel32.DisconnectNamedPipe
@ stdcall DuplicateHandle(long long long ptr long long long) kernel32.DuplicateHandle
@ stdcall DuplicateToken(long long ptr) advapi32.DuplicateToken
@ stdcall DuplicateTokenEx(long long ptr long long ptr) advapi32.DuplicateTokenEx
@ stdcall EncodePointer(ptr) kernel32.EncodePointer
@ stdcall EncodeSystemPointer(ptr) kernel32.EncodeSystemPointer
@ stdcall EnterCriticalSection(ptr) kernel32.EnterCriticalSection
@ stdcall EnumCalendarInfoExEx(ptr wstr long wstr long long) kernel32.EnumCalendarInfoExEx
@ stdcall EnumCalendarInfoExW(ptr long long long) kernel32.EnumCalendarInfoExW
@ stdcall EnumCalendarInfoW(ptr long long long) kernel32.EnumCalendarInfoW
@ stdcall EnumDateFormatsExEx(ptr wstr long long) kernel32.EnumDateFormatsExEx
@ stdcall EnumDateFormatsExW(ptr long long) kernel32.EnumDateFormatsExW
@ stdcall EnumDateFormatsW(ptr long long) kernel32.EnumDateFormatsW
@ stdcall EnumLanguageGroupLocalesW(ptr long long ptr) kernel32.EnumLanguageGroupLocalesW
@ stdcall EnumSystemCodePagesW(ptr long) kernel32.EnumSystemCodePagesW
@ stdcall EnumSystemLanguageGroupsW(ptr long ptr) kernel32.EnumSystemLanguageGroupsW
@ stdcall EnumSystemLocalesA(ptr long) kernel32.EnumSystemLocalesA
@ stdcall EnumSystemLocalesEx(ptr long long ptr) kernel32.EnumSystemLocalesEx
@ stdcall EnumSystemLocalesW(ptr long) kernel32.EnumSystemLocalesW
@ stdcall EnumTimeFormatsEx(ptr wstr long long) kernel32.EnumTimeFormatsEx
@ stdcall EnumTimeFormatsW(ptr long long) kernel32.EnumTimeFormatsW
@ stdcall EnumUILanguagesW(ptr long long) kernel32.EnumUILanguagesW
@ stub EqualDomainSid
@ stdcall EqualPrefixSid(ptr ptr) advapi32.EqualPrefixSid
@ stdcall EqualSid(ptr ptr) advapi32.EqualSid
@ stdcall ExitProcess(long) kernel32.ExitProcess
@ stdcall ExitThread(long) kernel32.ExitThread
@ stdcall ExpandEnvironmentStringsA(str ptr long) kernel32.ExpandEnvironmentStringsA
@ stdcall ExpandEnvironmentStringsW(wstr ptr long) kernel32.ExpandEnvironmentStringsW
@ stdcall FatalAppExitA(long str) kernel32.FatalAppExitA
@ stdcall FatalAppExitW(long wstr) kernel32.FatalAppExitW
@ stdcall FileTimeToLocalFileTime(ptr ptr) kernel32.FileTimeToLocalFileTime
@ stdcall FileTimeToSystemTime(ptr ptr) kernel32.FileTimeToSystemTime
@ stdcall FindClose(long) kernel32.FindClose
@ stdcall FindCloseChangeNotification(long) kernel32.FindCloseChangeNotification
@ stdcall FindFirstChangeNotificationA(str long long) kernel32.FindFirstChangeNotificationA
@ stdcall FindFirstChangeNotificationW(wstr long long) kernel32.FindFirstChangeNotificationW
@ stdcall FindFirstFileA(str ptr) kernel32.FindFirstFileA
@ stdcall FindFirstFileExA(str long ptr long ptr long) kernel32.FindFirstFileExA
@ stdcall FindFirstFileExW(wstr long ptr long ptr long) kernel32.FindFirstFileExW
@ stdcall FindFirstFileW(wstr ptr) kernel32.FindFirstFileW
@ stdcall FindFirstFreeAce(ptr ptr) advapi32.FindFirstFreeAce
@ stdcall FindFirstVolumeW(ptr long) kernel32.FindFirstVolumeW
@ stub FindNLSString
@ stub FindNLSStringEx
@ stdcall FindNextChangeNotification(long) kernel32.FindNextChangeNotification
@ stdcall FindNextFileA(long ptr) kernel32.FindNextFileA
@ stdcall FindNextFileW(long ptr) kernel32.FindNextFileW
@ stdcall FindNextVolumeW(long ptr long) kernel32.FindNextVolumeW
@ stdcall FindResourceExW(long wstr wstr long) kernel32.FindResourceExW
@ stub FindStringOrdinal
@ stdcall FindVolumeClose(ptr) kernel32.FindVolumeClose
@ stdcall FlsAlloc(ptr) kernel32.FlsAlloc
@ stdcall FlsFree(long) kernel32.FlsFree
@ stdcall FlsGetValue(long) kernel32.FlsGetValue
@ stdcall FlsSetValue(long ptr) kernel32.FlsSetValue
@ stdcall FlushFileBuffers(long) kernel32.FlushFileBuffers
@ stdcall FlushProcessWriteBuffers() kernel32.FlushProcessWriteBuffers
@ stdcall FlushViewOfFile(ptr long) kernel32.FlushViewOfFile
@ stdcall FoldStringW(long wstr long ptr long) kernel32.FoldStringW
@ stdcall FormatMessageA(long ptr long long ptr long ptr) kernel32.FormatMessageA
@ stdcall FormatMessageW(long ptr long long ptr long ptr) kernel32.FormatMessageW
@ stdcall FreeEnvironmentStringsA(ptr) kernel32.FreeEnvironmentStringsA
@ stdcall FreeEnvironmentStringsW(ptr) kernel32.FreeEnvironmentStringsW
@ stdcall FreeLibrary(long) kernel32.FreeLibrary
@ stdcall FreeLibraryAndExitThread(long long) kernel32.FreeLibraryAndExitThread
@ stdcall FreeLibraryWhenCallbackReturns(ptr ptr) kernel32.FreeLibraryWhenCallbackReturns
@ stdcall FreeResource(long) kernel32.FreeResource
@ stdcall FreeSid(ptr) advapi32.FreeSid
@ stdcall GetACP() kernel32.GetACP
@ stdcall GetAce(ptr long ptr) advapi32.GetAce
@ stdcall GetAclInformation(ptr ptr long long) advapi32.GetAclInformation
@ stub GetCPFileNameFromRegistry
@ stub GetCPHashNode
@ stdcall GetCPInfo(long ptr) kernel32.GetCPInfo
@ stdcall GetCPInfoExW(long long ptr) kernel32.GetCPInfoExW
@ stub GetCalendar
@ stdcall GetCalendarInfoEx(wstr long ptr long ptr long ptr) kernel32.GetCalendarInfoEx
@ stdcall GetCalendarInfoW(long long long ptr long ptr) kernel32.GetCalendarInfoW
@ stdcall GetCommandLineA() kernel32.GetCommandLineA
@ stdcall GetCommandLineW() kernel32.GetCommandLineW
@ stdcall GetComputerNameExA(long ptr ptr) kernel32.GetComputerNameExA
@ stdcall GetComputerNameExW(long ptr ptr) kernel32.GetComputerNameExW
@ stub GetCurrencyFormatEx
@ stdcall GetCurrencyFormatW(long long str ptr str long) kernel32.GetCurrencyFormatW
@ stdcall GetCurrentDirectoryA(long ptr) kernel32.GetCurrentDirectoryA
@ stdcall GetCurrentDirectoryW(long ptr) kernel32.GetCurrentDirectoryW
@ stdcall -norelay GetCurrentProcess() kernel32.GetCurrentProcess
@ stdcall -norelay GetCurrentProcessId() kernel32.GetCurrentProcessId
@ stdcall -norelay GetCurrentThread() kernel32.GetCurrentThread
@ stdcall -norelay GetCurrentThreadId() kernel32.GetCurrentThreadId
@ stdcall GetDiskFreeSpaceA(str ptr ptr ptr ptr) kernel32.GetDiskFreeSpaceA
@ stdcall GetDiskFreeSpaceExA(str ptr ptr ptr) kernel32.GetDiskFreeSpaceExA
@ stdcall GetDiskFreeSpaceExW(wstr ptr ptr ptr) kernel32.GetDiskFreeSpaceExW
@ stdcall GetDiskFreeSpaceW(wstr ptr ptr ptr ptr) kernel32.GetDiskFreeSpaceW
@ stdcall GetDriveTypeA(str) kernel32.GetDriveTypeA
@ stdcall GetDriveTypeW(wstr) kernel32.GetDriveTypeW
@ stdcall GetDynamicTimeZoneInformation(ptr) kernel32.GetDynamicTimeZoneInformation
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
@ stdcall GetFileMUIInfo(long wstr ptr ptr) kernel32.GetFileMUIInfo
@ stdcall GetFileMUIPath(long wstr wstr ptr ptr ptr ptr) kernel32.GetFileMUIPath
@ stdcall GetFileSecurityW(wstr long ptr long ptr) advapi32.GetFileSecurityW
@ stdcall GetFileSize(long ptr) kernel32.GetFileSize
@ stdcall GetFileSizeEx(long ptr) kernel32.GetFileSizeEx
@ stdcall GetFileTime(long ptr ptr ptr) kernel32.GetFileTime
@ stdcall GetFileType(long) kernel32.GetFileType
@ stdcall GetFinalPathNameByHandleA(long ptr long long) kernel32.GetFinalPathNameByHandleA
@ stdcall GetFinalPathNameByHandleW(long ptr long long) kernel32.GetFinalPathNameByHandleW
@ stdcall GetFullPathNameA(str long ptr ptr) kernel32.GetFullPathNameA
@ stdcall GetFullPathNameW(wstr long ptr ptr) kernel32.GetFullPathNameW
@ stdcall GetHandleInformation(long ptr) kernel32.GetHandleInformation
@ stdcall GetKernelObjectSecurity(long long ptr long ptr) advapi32.GetKernelObjectSecurity
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
@ stdcall GetModuleFileNameA(long ptr long) kernel32.GetModuleFileNameA
@ stdcall GetModuleFileNameW(long ptr long) kernel32.GetModuleFileNameW
@ stdcall GetModuleHandleA(str) kernel32.GetModuleHandleA
@ stdcall GetModuleHandleExA(long ptr ptr) kernel32.GetModuleHandleExA
@ stdcall GetModuleHandleExW(long ptr ptr) kernel32.GetModuleHandleExW
@ stdcall GetModuleHandleW(wstr) kernel32.GetModuleHandleW
@ stub GetNLSVersion
@ stub GetNLSVersionEx
@ stub GetNamedLocaleHashNode
@ stub GetNamedPipeAttribute
@ stub GetNamedPipeClientComputerNameW
@ stdcall GetNumberFormatEx(wstr long wstr ptr ptr long) kernel32.GetNumberFormatEx
@ stdcall GetNumberFormatW(long long wstr ptr ptr long) kernel32.GetNumberFormatW
@ stdcall GetOEMCP() kernel32.GetOEMCP
@ stdcall GetOverlappedResult(long ptr ptr long) kernel32.GetOverlappedResult
@ stdcall GetPriorityClass(long) kernel32.GetPriorityClass
@ stdcall GetPrivateObjectSecurity(ptr long ptr long ptr) advapi32.GetPrivateObjectSecurity
@ stdcall GetProcAddress(long str) kernel32.GetProcAddress
@ stdcall -norelay GetProcessHeap() kernel32.GetProcessHeap
@ stdcall GetProcessHeaps(long ptr) kernel32.GetProcessHeaps
@ stdcall GetProcessId(long) kernel32.GetProcessId
@ stdcall GetProcessIdOfThread(long) kernel32.GetProcessIdOfThread
@ stub GetProcessPreferredUILanguages
@ stdcall GetProcessTimes(long ptr ptr ptr ptr) kernel32.GetProcessTimes
@ stdcall GetProcessVersion(long) kernel32.GetProcessVersion
@ stub GetPtrCalData
@ stub GetPtrCalDataArray
@ stdcall GetQueuedCompletionStatus(long ptr ptr ptr long) kernel32.GetQueuedCompletionStatus
@ stub GetQueuedCompletionStatusEx
@ stdcall GetSecurityDescriptorControl(ptr ptr ptr) advapi32.GetSecurityDescriptorControl
@ stdcall GetSecurityDescriptorDacl(ptr ptr ptr ptr) advapi32.GetSecurityDescriptorDacl
@ stdcall GetSecurityDescriptorGroup(ptr ptr ptr) advapi32.GetSecurityDescriptorGroup
@ stdcall GetSecurityDescriptorLength(ptr) advapi32.GetSecurityDescriptorLength
@ stdcall GetSecurityDescriptorOwner(ptr ptr ptr) advapi32.GetSecurityDescriptorOwner
@ stub GetSecurityDescriptorRMControl
@ stdcall GetSecurityDescriptorSacl(ptr ptr ptr ptr) advapi32.GetSecurityDescriptorSacl
@ stdcall GetShortPathNameW(wstr ptr long) kernel32.GetShortPathNameW
@ stdcall GetSidIdentifierAuthority(ptr) advapi32.GetSidIdentifierAuthority
@ stdcall GetSidLengthRequired(long) advapi32.GetSidLengthRequired
@ stdcall GetSidSubAuthority(ptr long) advapi32.GetSidSubAuthority
@ stdcall GetSidSubAuthorityCount(ptr) advapi32.GetSidSubAuthorityCount
@ stdcall GetStartupInfoW(ptr) kernel32.GetStartupInfoW
@ stdcall GetStdHandle(long) kernel32.GetStdHandle
@ stub GetStringTableEntry
@ stdcall GetStringTypeA(long long str long ptr) kernel32.GetStringTypeA
@ stdcall GetStringTypeExW(long long wstr long ptr) kernel32.GetStringTypeExW
@ stdcall GetStringTypeW(long wstr long ptr) kernel32.GetStringTypeW
@ stdcall GetSystemDefaultLCID() kernel32.GetSystemDefaultLCID
@ stdcall GetSystemDefaultLangID() kernel32.GetSystemDefaultLangID
@ stdcall GetSystemDefaultLocaleName(ptr long) kernel32.GetSystemDefaultLocaleName
@ stdcall GetSystemDefaultUILanguage() kernel32.GetSystemDefaultUILanguage
@ stdcall GetSystemDirectoryA(ptr long) kernel32.GetSystemDirectoryA
@ stdcall GetSystemDirectoryW(ptr long) kernel32.GetSystemDirectoryW
@ stdcall GetSystemInfo(ptr) kernel32.GetSystemInfo
@ stdcall GetSystemPreferredUILanguages(long ptr ptr ptr) kernel32.GetSystemPreferredUILanguages
@ stdcall GetSystemTime(ptr) kernel32.GetSystemTime
@ stdcall GetSystemTimeAdjustment(ptr ptr ptr) kernel32.GetSystemTimeAdjustment
@ stdcall GetSystemTimeAsFileTime(ptr) kernel32.GetSystemTimeAsFileTime
@ stdcall GetSystemWindowsDirectoryA(ptr long) kernel32.GetSystemWindowsDirectoryA
@ stdcall GetSystemWindowsDirectoryW(ptr long) kernel32.GetSystemWindowsDirectoryW
@ stdcall GetTempFileNameW(wstr wstr long ptr) kernel32.GetTempFileNameW
@ stdcall GetThreadId(ptr) kernel32.GetThreadId
@ stdcall GetThreadLocale() kernel32.GetThreadLocale
@ stdcall GetThreadPreferredUILanguages(long ptr ptr ptr) kernel32.GetThreadPreferredUILanguages
@ stdcall GetThreadPriority(long) kernel32.GetThreadPriority
@ stdcall GetThreadPriorityBoost(long ptr) kernel32.GetThreadPriorityBoost
@ stdcall GetThreadUILanguage() kernel32.GetThreadUILanguage
@ stdcall GetTickCount() kernel32.GetTickCount
@ stdcall -ret64 GetTickCount64() kernel32.GetTickCount64
@ stdcall GetTimeZoneInformation(ptr) kernel32.GetTimeZoneInformation
@ stdcall GetTimeZoneInformationForYear(long ptr ptr) kernel32.GetTimeZoneInformationForYear
@ stdcall GetTokenInformation(long long ptr long ptr) advapi32.GetTokenInformation
@ stub GetUILanguageInfo
@ stdcall GetUserDefaultLCID() kernel32.GetUserDefaultLCID
@ stdcall GetUserDefaultLangID() kernel32.GetUserDefaultLangID
@ stdcall GetUserDefaultLocaleName(ptr long) kernel32.GetUserDefaultLocaleName
@ stdcall GetUserDefaultUILanguage() kernel32.GetUserDefaultUILanguage
@ stub GetUserInfo
@ stub GetUserInfoWord
@ stdcall GetUserPreferredUILanguages(long ptr ptr ptr) kernel32.GetUserPreferredUILanguages
@ stdcall GetVersion() kernel32.GetVersion
@ stdcall GetVersionExA(ptr) kernel32.GetVersionExA
@ stdcall GetVersionExW(ptr) kernel32.GetVersionExW
@ stub GetVolumeInformationByHandleW
@ stdcall GetVolumeInformationW(wstr ptr long ptr ptr ptr ptr long) kernel32.GetVolumeInformationW
@ stdcall GetVolumePathNameW(wstr ptr long) kernel32.GetVolumePathNameW
@ stdcall GetWindowsAccountDomainSid(ptr ptr ptr) advapi32.GetWindowsAccountDomainSid
@ stdcall GetWindowsDirectoryA(ptr long) kernel32.GetWindowsDirectoryA
@ stdcall GetWindowsDirectoryW(ptr long) kernel32.GetWindowsDirectoryW
@ stdcall GlobalAlloc(long long) kernel32.GlobalAlloc
@ stdcall GlobalFree(long) kernel32.GlobalFree
@ stdcall GlobalMemoryStatusEx(ptr) kernel32.GlobalMemoryStatusEx
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
@ stdcall ImpersonateAnonymousToken(long) advapi32.ImpersonateAnonymousToken
@ stdcall ImpersonateLoggedOnUser(long) advapi32.ImpersonateLoggedOnUser
@ stdcall ImpersonateNamedPipeClient(long) advapi32.ImpersonateNamedPipeClient
@ stdcall ImpersonateSelf(long) advapi32.ImpersonateSelf
@ stdcall InitializeAcl(ptr long long) advapi32.InitializeAcl
@ stdcall InitializeCriticalSection(ptr) kernel32.InitializeCriticalSection
@ stdcall InitializeCriticalSectionAndSpinCount(ptr long) kernel32.InitializeCriticalSectionAndSpinCount
@ stdcall InitializeCriticalSectionEx(ptr long long) kernel32.InitializeCriticalSectionEx
@ stub InitializeProcThreadAttributeList
@ stdcall InitializeSListHead(ptr) kernel32.InitializeSListHead
@ stdcall InitializeSRWLock(ptr) kernel32.InitializeSRWLock
@ stdcall InitializeSecurityDescriptor(ptr long) advapi32.InitializeSecurityDescriptor
@ stdcall InitializeSid(ptr ptr long) advapi32.InitializeSid
@ stdcall InterlockedFlushSList(ptr) kernel32.InterlockedFlushSList
@ stdcall InterlockedPopEntrySList(ptr) kernel32.InterlockedPopEntrySList
@ stdcall InterlockedPushEntrySList(ptr ptr) kernel32.InterlockedPushEntrySList
@ stdcall -norelay InterlockedPushListSList(ptr ptr ptr long) kernel32.InterlockedPushListSList
@ stub InternalLcidToName
@ stub Internal_EnumCalendarInfo
@ stub Internal_EnumDateFormats
@ stub Internal_EnumLanguageGroupLocales
@ stub Internal_EnumSystemCodePages
@ stub Internal_EnumSystemLanguageGroups
@ stub Internal_EnumSystemLocales
@ stub Internal_EnumTimeFormats
@ stub Internal_EnumUILanguages
@ stub InvalidateTzSpecificCache
@ stdcall IsDBCSLeadByte(long) kernel32.IsDBCSLeadByte
@ stdcall IsDBCSLeadByteEx(long long) kernel32.IsDBCSLeadByteEx
@ stdcall IsDebuggerPresent() kernel32.IsDebuggerPresent
@ stub IsNLSDefinedString
@ stdcall IsProcessInJob(long long ptr) kernel32.IsProcessInJob
@ stdcall IsThreadpoolTimerSet(ptr) kernel32.IsThreadpoolTimerSet
@ stdcall IsTokenRestricted(long) advapi32.IsTokenRestricted
@ stdcall IsValidAcl(ptr) advapi32.IsValidAcl
@ stdcall IsValidCodePage(long) kernel32.IsValidCodePage
@ stdcall IsValidLanguageGroup(long long) kernel32.IsValidLanguageGroup
@ stdcall IsValidLocale(long long) kernel32.IsValidLocale
@ stdcall IsValidLocaleName(wstr) kernel32.IsValidLocaleName
@ stub IsValidRelativeSecurityDescriptor
@ stdcall IsValidSecurityDescriptor(ptr) advapi32.IsValidSecurityDescriptor
@ stdcall IsValidSid(ptr) advapi32.IsValidSid
@ stdcall IsWellKnownSid(ptr long) advapi32.IsWellKnownSid
@ stdcall IsWow64Process(ptr ptr) kernel32.IsWow64Process
@ stub KernelBaseGetGlobalData
@ stdcall LCIDToLocaleName(long ptr long long) kernel32.LCIDToLocaleName
@ stdcall LCMapStringA(long long str long ptr long) kernel32.LCMapStringA
@ stdcall LCMapStringEx(wstr long wstr long ptr long ptr ptr long) kernel32.LCMapStringEx
@ stdcall LCMapStringW(long long wstr long ptr long) kernel32.LCMapStringW
@ stdcall LeaveCriticalSection(ptr) kernel32.LeaveCriticalSection
@ stdcall LeaveCriticalSectionWhenCallbackReturns(ptr ptr) kernel32.LeaveCriticalSectionWhenCallbackReturns
@ stdcall LoadLibraryExA( str long long) kernel32.LoadLibraryExA
@ stdcall LoadLibraryExW(wstr long long) kernel32.LoadLibraryExW
@ stdcall LoadResource(long long) kernel32.LoadResource
@ stdcall LoadStringA(long long ptr long) user32.LoadStringA
@ stub LoadStringBaseExW
@ stub LoadStringByReference
@ stdcall LoadStringW(long long ptr long) user32.LoadStringW
@ stdcall LocalAlloc(long long) kernel32.LocalAlloc
@ stdcall LocalFileTimeToFileTime(ptr ptr) kernel32.LocalFileTimeToFileTime
@ stdcall LocalFree(long) kernel32.LocalFree
@ stdcall LocalLock(long) kernel32.LocalLock
@ stdcall LocalReAlloc(long long long) kernel32.LocalReAlloc
@ stdcall LocalUnlock(long) kernel32.LocalUnlock
@ stdcall LocaleNameToLCID(wstr long) kernel32.LocaleNameToLCID
@ stdcall LockFile(long long long long long) kernel32.LockFile
@ stdcall LockFileEx(long long long long long ptr) kernel32.LockFileEx
@ stdcall LockResource(long) kernel32.LockResource
@ stdcall MakeAbsoluteSD(ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr ptr) advapi32.MakeAbsoluteSD
@ stub MakeAbsoluteSD2
@ stdcall MakeSelfRelativeSD(ptr ptr ptr) advapi32.MakeSelfRelativeSD
@ stdcall MapGenericMask(ptr ptr) advapi32.MapGenericMask
@ stdcall MapViewOfFile(long long long long long) kernel32.MapViewOfFile
@ stdcall MapViewOfFileEx(long long long long long ptr) kernel32.MapViewOfFileEx
@ stub MapViewOfFileExNuma
@ stdcall MultiByteToWideChar(long long str long ptr long) kernel32.MultiByteToWideChar
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
@ stub NotifyMountMgr
@ stub NotifyRedirectedStringChange
@ stdcall ObjectCloseAuditAlarmW(wstr ptr long) advapi32.ObjectCloseAuditAlarmW
@ stdcall ObjectDeleteAuditAlarmW(wstr ptr long) advapi32.ObjectDeleteAuditAlarmW
@ stdcall ObjectOpenAuditAlarmW(wstr ptr wstr wstr ptr long long long ptr long long ptr) advapi32.ObjectOpenAuditAlarmW
@ stdcall ObjectPrivilegeAuditAlarmW(wstr ptr long long ptr long) advapi32.ObjectPrivilegeAuditAlarmW
@ stdcall OpenEventA(long long str) kernel32.OpenEventA
@ stdcall OpenEventW(long long wstr) kernel32.OpenEventW
@ stdcall OpenFileMappingW(long long wstr) kernel32.OpenFileMappingW
@ stdcall OpenMutexW(long long wstr) kernel32.OpenMutexW
@ stdcall OpenProcess(long long long) kernel32.OpenProcess
@ stdcall OpenProcessToken(long long ptr) advapi32.OpenProcessToken
@ stub OpenRegKey
@ stdcall OpenSemaphoreW(long long wstr) kernel32.OpenSemaphoreW
@ stdcall OpenThread(long long long) kernel32.OpenThread
@ stdcall OpenThreadToken(long long long ptr) advapi32.OpenThreadToken
@ stdcall OpenWaitableTimerW(long long wstr) kernel32.OpenWaitableTimerW
@ stdcall OutputDebugStringA(str) kernel32.OutputDebugStringA
@ stdcall OutputDebugStringW(wstr) kernel32.OutputDebugStringW
@ stdcall PeekNamedPipe(long ptr long ptr ptr ptr) kernel32.PeekNamedPipe
@ stdcall PostQueuedCompletionStatus(long long ptr ptr) kernel32.PostQueuedCompletionStatus
@ stdcall PrivilegeCheck(ptr ptr ptr) advapi32.PrivilegeCheck
@ stdcall PrivilegedServiceAuditAlarmW(wstr wstr long ptr long) advapi32.PrivilegedServiceAuditAlarmW
@ stdcall ProcessIdToSessionId(long ptr) kernel32.ProcessIdToSessionId
@ stdcall PulseEvent(long) kernel32.PulseEvent
@ stdcall QueryDepthSList(ptr) kernel32.QueryDepthSList
@ stdcall QueryDosDeviceW(wstr ptr long) kernel32.QueryDosDeviceW
@ stdcall QueryPerformanceCounter(ptr) kernel32.QueryPerformanceCounter
@ stdcall QueryPerformanceFrequency(ptr) kernel32.QueryPerformanceFrequency
@ stub QueryProcessAffinityUpdateMode
@ stub QuerySecurityAccessMask
@ stub QueryThreadpoolStackInformation
@ stdcall QueueUserAPC(ptr long long) kernel32.QueueUserAPC
@ stdcall RaiseException(long long long ptr) kernel32.RaiseException
@ stdcall ReadFile(long ptr long ptr ptr) kernel32.ReadFile
@ stdcall ReadFileEx(long ptr long ptr ptr) kernel32.ReadFileEx
@ stdcall ReadFileScatter(long ptr long ptr ptr) kernel32.ReadFileScatter
@ stdcall ReadProcessMemory(long ptr ptr long ptr) kernel32.ReadProcessMemory
@ stdcall RegisterWaitForSingleObjectEx(long ptr ptr long long) kernel32.RegisterWaitForSingleObjectEx
@ stdcall ReleaseMutex(long) kernel32.ReleaseMutex
@ stdcall ReleaseMutexWhenCallbackReturns(ptr long) kernel32.ReleaseMutexWhenCallbackReturns
@ stdcall ReleaseSRWLockExclusive(ptr) kernel32.ReleaseSRWLockExclusive
@ stdcall ReleaseSRWLockShared(ptr) kernel32.ReleaseSRWLockShared
@ stdcall ReleaseSemaphore(long long ptr) kernel32.ReleaseSemaphore
@ stdcall ReleaseSemaphoreWhenCallbackReturns(ptr long long) kernel32.ReleaseSemaphoreWhenCallbackReturns
@ stdcall RemoveDirectoryA(str) kernel32.RemoveDirectoryA
@ stdcall RemoveDirectoryW(wstr) kernel32.RemoveDirectoryW
@ stub RemoveDllDirectory
@ stdcall ResetEvent(long) kernel32.ResetEvent
@ stub ResolveLocaleName
@ stdcall ResumeThread(long) kernel32.ResumeThread
@ stdcall RevertToSelf() advapi32.RevertToSelf
@ stdcall SearchPathW(wstr wstr wstr long ptr ptr) kernel32.SearchPathW
@ stdcall SetAclInformation(ptr ptr long long) advapi32.SetAclInformation
@ stdcall SetCalendarInfoW(long long long wstr) kernel32.SetCalendarInfoW
@ stdcall SetCriticalSectionSpinCount(ptr long) kernel32.SetCriticalSectionSpinCount
@ stdcall SetCurrentDirectoryA(str) kernel32.SetCurrentDirectoryA
@ stdcall SetCurrentDirectoryW(wstr) kernel32.SetCurrentDirectoryW
@ stdcall SetDefaultDllDirectories(long) kernel32.SetDefaultDllDirectories
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
@ stdcall SetFilePointer(long long ptr long) kernel32.SetFilePointer
@ stdcall SetFilePointerEx(long int64 ptr long) kernel32.SetFilePointerEx
@ stdcall SetFileSecurityW(wstr long ptr) advapi32.SetFileSecurityW
@ stdcall SetFileTime(long ptr ptr ptr) kernel32.SetFileTime
@ stdcall SetFileValidData(ptr int64) kernel32.SetFileValidData
@ stdcall SetHandleCount(long) kernel32.SetHandleCount
@ stdcall SetHandleInformation(long long long) kernel32.SetHandleInformation
@ stdcall SetKernelObjectSecurity(long long ptr) advapi32.SetKernelObjectSecurity
@ stdcall SetLastError(long) kernel32.SetLastError
@ stdcall SetLocalTime(ptr) kernel32.SetLocalTime
@ stdcall SetLocaleInfoW(long long wstr) kernel32.SetLocaleInfoW
@ stdcall SetNamedPipeHandleState(long ptr ptr ptr) kernel32.SetNamedPipeHandleState
@ stdcall SetPriorityClass(long long) kernel32.SetPriorityClass
@ stdcall SetPrivateObjectSecurity(long ptr ptr ptr long) advapi32.SetPrivateObjectSecurity
@ stub SetPrivateObjectSecurityEx
@ stub SetProcessAffinityUpdateMode
@ stdcall SetProcessShutdownParameters(long long) kernel32.SetProcessShutdownParameters
@ stub SetSecurityAccessMask
@ stdcall SetSecurityDescriptorControl(ptr long long) advapi32.SetSecurityDescriptorControl
@ stdcall SetSecurityDescriptorDacl(ptr long ptr long) advapi32.SetSecurityDescriptorDacl
@ stdcall SetSecurityDescriptorGroup(ptr ptr long) advapi32.SetSecurityDescriptorGroup
@ stdcall SetSecurityDescriptorOwner(ptr ptr long) advapi32.SetSecurityDescriptorOwner
@ stub SetSecurityDescriptorRMControl
@ stdcall SetSecurityDescriptorSacl(ptr long ptr long) advapi32.SetSecurityDescriptorSacl
@ stdcall SetStdHandle(long long) kernel32.SetStdHandle
@ stub SetStdHandleEx
@ stdcall SetThreadLocale(long) kernel32.SetThreadLocale
@ stdcall SetThreadPriority(long long) kernel32.SetThreadPriority
@ stdcall SetThreadPriorityBoost(long long) kernel32.SetThreadPriorityBoost
@ stdcall SetThreadStackGuarantee(ptr) kernel32.SetThreadStackGuarantee
@ stdcall SetThreadToken(ptr ptr) advapi32.SetThreadToken
@ stub SetThreadpoolStackInformation
@ stdcall SetThreadpoolThreadMaximum(ptr long) kernel32.SetThreadpoolThreadMaximum
@ stdcall SetThreadpoolThreadMinimum(ptr long) kernel32.SetThreadpoolThreadMinimum
@ stdcall SetThreadpoolTimer(ptr ptr long long) kernel32.SetThreadpoolTimer
@ stdcall SetThreadpoolWait(ptr long ptr) kernel32.SetThreadpoolWait
@ stdcall SetTokenInformation(long long ptr long) advapi32.SetTokenInformation
@ stdcall SetWaitableTimer(long ptr long ptr ptr long) kernel32.SetWaitableTimer
@ stdcall SetWaitableTimerEx(long ptr long ptr ptr ptr long) kernel32.SetWaitableTimerEx
@ stdcall SizeofResource(long long) kernel32.SizeofResource
@ stdcall Sleep(long) kernel32.Sleep
@ stdcall SleepEx(long long) kernel32.SleepEx
@ stub SpecialMBToWC
@ stub StartThreadpoolIo
@ stdcall SubmitThreadpoolWork(ptr) kernel32.SubmitThreadpoolWork
@ stdcall SuspendThread(long) kernel32.SuspendThread
@ stdcall SwitchToThread() kernel32.SwitchToThread
@ stdcall SystemTimeToFileTime(ptr ptr) kernel32.SystemTimeToFileTime
@ stdcall SystemTimeToTzSpecificLocalTime(ptr ptr ptr) kernel32.SystemTimeToTzSpecificLocalTime
@ stub SystemTimeToTzSpecificLocalTimeEx
@ stdcall TerminateProcess(long long) kernel32.TerminateProcess
@ stdcall TerminateThread(long long) kernel32.TerminateThread
@ stdcall TlsAlloc() kernel32.TlsAlloc
@ stdcall TlsFree(long) kernel32.TlsFree
@ stdcall TlsGetValue(long) kernel32.TlsGetValue
@ stdcall TlsSetValue(long ptr) kernel32.TlsSetValue
@ stdcall TransactNamedPipe(long ptr long ptr long ptr ptr) kernel32.TransactNamedPipe
@ stdcall TryAcquireSRWLockExclusive(ptr) kernel32.TryAcquireSRWLockExclusive
@ stdcall TryAcquireSRWLockShared(ptr) kernel32.TryAcquireSRWLockShared
@ stdcall TryEnterCriticalSection(ptr) kernel32.TryEnterCriticalSection
@ stdcall TrySubmitThreadpoolCallback(ptr ptr ptr) kernel32.TrySubmitThreadpoolCallback
@ stdcall TzSpecificLocalTimeToSystemTime(ptr ptr ptr) kernel32.TzSpecificLocalTimeToSystemTime
@ stub TzSpecificLocalTimeToSystemTimeEx
@ stdcall UnlockFile(long long long long long) kernel32.UnlockFile
@ stdcall UnlockFileEx(long long long long ptr) kernel32.UnlockFileEx
@ stdcall UnmapViewOfFile(ptr) kernel32.UnmapViewOfFile
@ stdcall UnregisterWaitEx(long long) kernel32.UnregisterWaitEx
@ stub UpdateProcThreadAttribute
@ stdcall VerLanguageNameA(long str long) kernel32.VerLanguageNameA
@ stdcall VerLanguageNameW(long wstr long) kernel32.VerLanguageNameW
@ stdcall VirtualAlloc(ptr long long long) kernel32.VirtualAlloc
@ stdcall VirtualAllocEx(long ptr long long long) kernel32.VirtualAllocEx
@ stub VirtualAllocExNuma
@ stdcall VirtualFree(ptr long long) kernel32.VirtualFree
@ stdcall VirtualFreeEx(long ptr long long) kernel32.VirtualFreeEx
@ stdcall VirtualProtect(ptr long long ptr) kernel32.VirtualProtect
@ stdcall VirtualProtectEx(long ptr long long ptr) kernel32.VirtualProtectEx
@ stdcall VirtualQuery(ptr ptr long) kernel32.VirtualQuery
@ stdcall VirtualQueryEx(long ptr ptr long) kernel32.VirtualQueryEx
@ stdcall WaitForMultipleObjectsEx(long ptr long long long) kernel32.WaitForMultipleObjectsEx
@ stdcall WaitForSingleObject(long long) kernel32.WaitForSingleObject
@ stdcall WaitForSingleObjectEx(long long long) kernel32.WaitForSingleObjectEx
@ stub WaitForThreadpoolIoCallbacks
@ stdcall WaitForThreadpoolTimerCallbacks(ptr long) kernel32.WaitForThreadpoolTimerCallbacks
@ stdcall WaitForThreadpoolWaitCallbacks(ptr long) kernel32.WaitForThreadpoolWaitCallbacks
@ stdcall WaitForThreadpoolWorkCallbacks(ptr long) kernel32.WaitForThreadpoolWorkCallbacks
@ stdcall WaitNamedPipeW(wstr long) kernel32.WaitNamedPipeW
@ stdcall WideCharToMultiByte(long long wstr long ptr long ptr ptr) kernel32.WideCharToMultiByte
@ stdcall Wow64DisableWow64FsRedirection(ptr) kernel32.Wow64DisableWow64FsRedirection
@ stdcall Wow64RevertWow64FsRedirection(ptr) kernel32.Wow64RevertWow64FsRedirection
@ stdcall WriteFile(long ptr long ptr ptr) kernel32.WriteFile
@ stdcall WriteFileEx(long ptr long ptr ptr) kernel32.WriteFileEx
@ stdcall WriteFileGather(long ptr long ptr ptr) kernel32.WriteFileGather
@ stdcall WriteProcessMemory(long ptr ptr long ptr) kernel32.WriteProcessMemory
@ stdcall -arch=x86_64 -private __C_specific_handler(ptr long ptr ptr) kernel32.__C_specific_handler
@ stdcall -arch=arm,x86_64 -private -norelay __chkstk() kernel32.__chkstk
@ stub __misaligned_access
@ stdcall -arch=x86_64 -private _local_unwind(ptr ptr) kernel32._local_unwind
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
