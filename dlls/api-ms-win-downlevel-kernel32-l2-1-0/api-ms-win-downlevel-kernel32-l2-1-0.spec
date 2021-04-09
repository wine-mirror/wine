@ stdcall AddAtomA(str) kernel32.AddAtomA
@ stdcall AddAtomW(wstr) kernel32.AddAtomW
@ stdcall BackupRead(ptr ptr long ptr long long ptr) kernel32.BackupRead
@ stdcall BackupWrite(ptr ptr long ptr long long ptr) kernel32.BackupWrite
@ stdcall BindIoCompletionCallback(long ptr long) kernel32.BindIoCompletionCallback
@ stdcall ConvertFiberToThread() kernel32.ConvertFiberToThread
@ stdcall ConvertThreadToFiber(ptr) kernel32.ConvertThreadToFiber
@ stdcall CopyFileA(str str long) kernel32.CopyFileA
@ stdcall CopyFileW(wstr wstr long) kernel32.CopyFileW
@ stdcall CreateFiber(long ptr ptr) kernel32.CreateFiber
@ stdcall CreateFileMappingA(long ptr long long long str) kernel32.CreateFileMappingA
@ stub CreateFileTransactedW
@ stdcall CreateMailslotA(str long long ptr) kernel32.CreateMailslotA
@ stdcall CreateNamedPipeA(str long long long long long long ptr) kernel32.CreateNamedPipeA
@ stdcall CreateSemaphoreW(ptr long long wstr) kernel32.CreateSemaphoreW
@ stdcall DeleteAtom(long) kernel32.DeleteAtom
@ stdcall DeleteFiber(ptr) kernel32.DeleteFiber
@ stdcall DnsHostnameToComputerNameW(wstr ptr ptr) kernel32.DnsHostnameToComputerNameW
@ stdcall DosDateTimeToFileTime(long long ptr) kernel32.DosDateTimeToFileTime
@ stdcall FatalAppExitA(long str) kernel32.FatalAppExitA
@ stdcall FatalAppExitW(long wstr) kernel32.FatalAppExitW
@ stdcall FileTimeToDosDateTime(ptr ptr ptr) kernel32.FileTimeToDosDateTime
@ stdcall FindAtomA(str) kernel32.FindAtomA
@ stdcall FindAtomW(wstr) kernel32.FindAtomW
@ stdcall FindResourceA(long str str) kernel32.FindResourceA
@ stdcall FindResourceExA(long str str long) kernel32.FindResourceExA
@ stdcall FindResourceW(long wstr wstr) kernel32.FindResourceW
@ stdcall GetActiveProcessorCount(long) kernel32.GetActiveProcessorCount
@ stdcall GetAtomNameA(long ptr long) kernel32.GetAtomNameA
@ stdcall GetAtomNameW(long ptr long) kernel32.GetAtomNameW
@ stdcall GetComputerNameA(ptr ptr) kernel32.GetComputerNameA
@ stdcall GetComputerNameW(ptr ptr) kernel32.GetComputerNameW
@ stdcall GetConsoleWindow() kernel32.GetConsoleWindow
@ stub GetDurationFormatEx
@ stdcall GetFirmwareEnvironmentVariableW(wstr wstr ptr long) kernel32.GetFirmwareEnvironmentVariableW
@ stdcall GetMaximumProcessorGroupCount() kernel32.GetMaximumProcessorGroupCount
@ stdcall GetNamedPipeClientProcessId(long ptr) kernel32.GetNamedPipeClientProcessId
@ stdcall GetNamedPipeServerProcessId(long ptr) kernel32.GetNamedPipeServerProcessId
@ stdcall GetPrivateProfileIntA(str str long str) kernel32.GetPrivateProfileIntA
@ stdcall GetPrivateProfileIntW(wstr wstr long wstr) kernel32.GetPrivateProfileIntW
@ stdcall GetPrivateProfileSectionW(wstr ptr long wstr) kernel32.GetPrivateProfileSectionW
@ stdcall GetPrivateProfileStringA(str str str ptr long str) kernel32.GetPrivateProfileStringA
@ stdcall GetPrivateProfileStringW(wstr wstr wstr ptr long wstr) kernel32.GetPrivateProfileStringW
@ stdcall GetProcessAffinityMask(long ptr ptr) kernel32.GetProcessAffinityMask
@ stdcall GetProcessIoCounters(long ptr) kernel32.GetProcessIoCounters
@ stdcall GetProfileIntA(str str long) kernel32.GetProfileIntA
@ stdcall GetProfileIntW(wstr wstr long) kernel32.GetProfileIntW
@ stdcall GetProfileSectionA(str ptr long) kernel32.GetProfileSectionA
@ stdcall GetProfileSectionW(wstr ptr long) kernel32.GetProfileSectionW
@ stdcall GetProfileStringA(str str str ptr long) kernel32.GetProfileStringA
@ stdcall GetProfileStringW(wstr wstr wstr ptr long) kernel32.GetProfileStringW
@ stdcall GetShortPathNameA(str ptr long) kernel32.GetShortPathNameA
@ stdcall GetStartupInfoA(ptr) kernel32.GetStartupInfoA
@ stdcall GetStringTypeExA(long long str long ptr) kernel32.GetStringTypeExA
@ stdcall GetSystemPowerStatus(ptr) kernel32.GetSystemPowerStatus
@ stdcall GetSystemWow64DirectoryA(ptr long) kernel32.GetSystemWow64DirectoryA
@ stdcall GetSystemWow64DirectoryW(ptr long) kernel32.GetSystemWow64DirectoryW
@ stdcall GetTapeParameters(ptr long ptr ptr) kernel32.GetTapeParameters
@ stdcall GetTempPathA(long ptr) kernel32.GetTempPathA
@ stdcall GetThreadSelectorEntry(long long ptr) kernel32.GetThreadSelectorEntry
@ stdcall GlobalAddAtomA(str) kernel32.GlobalAddAtomA
@ stdcall GlobalAddAtomW(wstr) kernel32.GlobalAddAtomW
@ stdcall GlobalAlloc(long long) kernel32.GlobalAlloc
@ stdcall GlobalDeleteAtom(long) kernel32.GlobalDeleteAtom
@ stdcall GlobalFindAtomA(str) kernel32.GlobalFindAtomA
@ stdcall GlobalFindAtomW(wstr) kernel32.GlobalFindAtomW
@ stdcall GlobalFlags(long) kernel32.GlobalFlags
@ stdcall GlobalFree(long) kernel32.GlobalFree
@ stdcall GlobalGetAtomNameA(long ptr long) kernel32.GlobalGetAtomNameA
@ stdcall GlobalGetAtomNameW(long ptr long) kernel32.GlobalGetAtomNameW
@ stdcall GlobalHandle(ptr) kernel32.GlobalHandle
@ stdcall GlobalLock(long) kernel32.GlobalLock
@ stdcall GlobalMemoryStatus(ptr) kernel32.GlobalMemoryStatus
@ stdcall GlobalReAlloc(long long long) kernel32.GlobalReAlloc
@ stdcall GlobalSize(long) kernel32.GlobalSize
@ stdcall GlobalUnlock(long) kernel32.GlobalUnlock
@ stdcall InitAtomTable(long) kernel32.InitAtomTable
@ stdcall LoadLibraryA(str) kernel32.LoadLibraryA
@ stdcall LoadLibraryW(wstr) kernel32.LoadLibraryW
@ stdcall LocalAlloc(long long) kernel32.LocalAlloc
@ stdcall LocalFlags(long) kernel32.LocalFlags
@ stdcall LocalFree(long) kernel32.LocalFree
@ stdcall LocalLock(long) kernel32.LocalLock
@ stdcall LocalReAlloc(long long long) kernel32.LocalReAlloc
@ stdcall LocalSize(long) kernel32.LocalSize
@ stdcall LocalUnlock(long) kernel32.LocalUnlock
@ stdcall MoveFileA(str str) kernel32.MoveFileA
@ stdcall MoveFileExA(str str long) kernel32.MoveFileExA
@ stdcall MoveFileW(wstr wstr) kernel32.MoveFileW
@ stdcall MulDiv(long long long) kernel32.MulDiv
@ stdcall OpenFile(str ptr long) kernel32.OpenFile
@ stdcall PulseEvent(long) kernel32.PulseEvent
@ stub RaiseFailFastException
@ stdcall RegisterWaitForSingleObject(ptr long ptr ptr long long) kernel32.RegisterWaitForSingleObject
@ stdcall SetConsoleTitleA(str) kernel32.SetConsoleTitleA
@ stdcall SetFileCompletionNotificationModes(long long) kernel32.SetFileCompletionNotificationModes
@ stdcall SetFirmwareEnvironmentVariableW(wstr wstr ptr long) kernel32.SetFirmwareEnvironmentVariableW
@ stdcall SetHandleCount(long) kernel32.SetHandleCount
@ stdcall SetMailslotInfo(long long) kernel32.SetMailslotInfo
@ stdcall SetProcessAffinityMask(long long) kernel32.SetProcessAffinityMask
@ stdcall SetThreadAffinityMask(long long) kernel32.SetThreadAffinityMask
@ stdcall SetThreadIdealProcessor(long long) kernel32.SetThreadIdealProcessor
@ stdcall SetVolumeLabelW(wstr wstr) kernel32.SetVolumeLabelW
@ stdcall SwitchToFiber(ptr) kernel32.SwitchToFiber
@ stdcall UnregisterWait(long) kernel32.UnregisterWait
@ stdcall WTSGetActiveConsoleSessionId() kernel32.WTSGetActiveConsoleSessionId
@ stdcall WaitForMultipleObjects(long ptr long long) kernel32.WaitForMultipleObjects
@ stdcall WritePrivateProfileSectionA(str str str) kernel32.WritePrivateProfileSectionA
@ stdcall WritePrivateProfileSectionW(wstr wstr wstr) kernel32.WritePrivateProfileSectionW
@ stdcall WritePrivateProfileStringA(str str str str) kernel32.WritePrivateProfileStringA
@ stdcall WritePrivateProfileStringW(wstr wstr wstr wstr) kernel32.WritePrivateProfileStringW
@ stdcall lstrcatW(wstr wstr) kernel32.lstrcatW
@ stdcall lstrcmpA(str str) kernel32.lstrcmpA
@ stdcall lstrcmpW(wstr wstr) kernel32.lstrcmpW
@ stdcall lstrcmpiA(str str) kernel32.lstrcmpiA
@ stdcall lstrcmpiW(wstr wstr) kernel32.lstrcmpiW
@ stdcall lstrcpyW(ptr wstr) kernel32.lstrcpyW
@ stdcall lstrcpynA(ptr str long) kernel32.lstrcpynA
@ stdcall lstrcpynW(ptr wstr long) kernel32.lstrcpynW
@ stdcall lstrlenA(str) kernel32.lstrlenA
@ stdcall lstrlenW(wstr) kernel32.lstrlenW
