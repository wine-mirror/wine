@ stub AddLocalAlternateComputerNameW
@ stdcall BackupRead(ptr ptr long ptr long long ptr) kernel32.BackupRead
@ stdcall BackupWrite(ptr ptr long ptr long long ptr) kernel32.BackupWrite
@ stdcall BindIoCompletionCallback(long ptr long) kernel32.BindIoCompletionCallback
@ stdcall CopyFileA(str str long) kernel32.CopyFileA
@ stdcall CopyFileW(wstr wstr long) kernel32.CopyFileW
@ stdcall CreateFileMappingA(long ptr long long long str) kernel32.CreateFileMappingA
@ stub CreateFileTransactedW
@ stdcall CreateMailslotA(str long long ptr) kernel32.CreateMailslotA
@ stdcall CreateNamedPipeA(str long long long long long long ptr) kernel32.CreateNamedPipeA
@ stdcall CreateSemaphoreW(ptr long long wstr) kernel32.CreateSemaphoreW
@ stdcall DnsHostnameToComputerNameW(wstr ptr ptr) kernel32.DnsHostnameToComputerNameW
@ stdcall DosDateTimeToFileTime(long long ptr) kernel32.DosDateTimeToFileTime
@ stdcall FatalAppExitA(long str) kernel32.FatalAppExitA
@ stdcall FatalAppExitW(long wstr) kernel32.FatalAppExitW
@ stdcall FileTimeToDosDateTime(ptr ptr ptr) kernel32.FileTimeToDosDateTime
@ stdcall FindFirstVolumeMountPointW(wstr ptr long) kernel32.FindFirstVolumeMountPointW
@ stub FindNextVolumeMountPointW
@ stdcall FindResourceA(long str str) kernel32.FindResourceA
@ stdcall FindResourceExA(long str str long) kernel32.FindResourceExA
@ stdcall FindResourceW(long wstr wstr) kernel32.FindResourceW
@ stdcall FindVolumeMountPointClose(ptr) kernel32.FindVolumeMountPointClose
@ stdcall GetComputerNameA(ptr ptr) kernel32.GetComputerNameA
@ stdcall GetComputerNameW(ptr ptr) kernel32.GetComputerNameW
@ stdcall GetConsoleWindow() kernel32.GetConsoleWindow
@ stub GetDurationFormatEx
@ stub GetFileAttributesTransactedW
@ stub GetFirmwareType
@ stdcall GetMaximumProcessorGroupCount() kernel32.GetMaximumProcessorGroupCount
@ stdcall GetNamedPipeClientProcessId(long ptr) kernel32.GetNamedPipeClientProcessId
@ stdcall GetNamedPipeServerProcessId(long ptr) kernel32.GetNamedPipeServerProcessId
@ stdcall GetNumaAvailableMemoryNodeEx(long ptr) kernel32.GetNumaAvailableMemoryNodeEx
@ stdcall GetNumaNodeProcessorMask(long ptr) kernel32.GetNumaNodeProcessorMask
@ stdcall GetNumaProcessorNodeEx(ptr ptr) kernel32.GetNumaProcessorNodeEx
@ stdcall GetShortPathNameA(str ptr long) kernel32.GetShortPathNameA
@ stdcall GetStartupInfoA(ptr) kernel32.GetStartupInfoA
@ stdcall GetStringTypeExA(long long str long ptr) kernel32.GetStringTypeExA
@ stdcall GetSystemPowerStatus(ptr) kernel32.GetSystemPowerStatus
@ stdcall GetSystemWow64DirectoryA(ptr long) kernel32.GetSystemWow64DirectoryA
@ stdcall GetSystemWow64DirectoryW(ptr long) kernel32.GetSystemWow64DirectoryW
@ stdcall GetTapeParameters(ptr long ptr ptr) kernel32.GetTapeParameters
@ stdcall GetTempPathA(long ptr) kernel32.GetTempPathA
@ stdcall GetThreadSelectorEntry(long long ptr) kernel32.GetThreadSelectorEntry
@ stdcall GlobalMemoryStatus(ptr) kernel32.GlobalMemoryStatus
@ stdcall LoadLibraryA(str) kernel32.LoadLibraryA
@ stdcall LoadLibraryW(wstr) kernel32.LoadLibraryW
@ stdcall MoveFileA(str str) kernel32.MoveFileA
@ stdcall MoveFileExA(str str long) kernel32.MoveFileExA
@ stdcall MoveFileW(wstr wstr) kernel32.MoveFileW
@ stdcall MulDiv(long long long) kernel32.MulDiv
@ stdcall OpenFile(str ptr long) kernel32.OpenFile
@ stdcall PowerClearRequest(long long) kernel32.PowerClearRequest
@ stdcall PowerCreateRequest(ptr) kernel32.PowerCreateRequest
@ stdcall PowerSetRequest(long long) kernel32.PowerSetRequest
@ stdcall PulseEvent(long) kernel32.PulseEvent
@ stub RaiseFailFastException
@ stdcall RegisterWaitForSingleObject(ptr long ptr ptr long long) kernel32.RegisterWaitForSingleObject
@ stdcall SetConsoleTitleA(str) kernel32.SetConsoleTitleA
@ stdcall SetDllDirectoryW(wstr) kernel32.SetDllDirectoryW
@ stdcall SetFileCompletionNotificationModes(long long) kernel32.SetFileCompletionNotificationModes
@ stdcall SetHandleCount(long) kernel32.SetHandleCount
@ stdcall SetMailslotInfo(long long) kernel32.SetMailslotInfo
@ stdcall SetThreadIdealProcessor(long long) kernel32.SetThreadIdealProcessor
@ stdcall SetVolumeLabelW(wstr wstr) kernel32.SetVolumeLabelW
@ stdcall SetVolumeMountPointW(wstr wstr) kernel32.SetVolumeMountPointW
@ stdcall UnregisterWait(long) kernel32.UnregisterWait
@ stdcall VerifyVersionInfoW(ptr long int64) kernel32.VerifyVersionInfoW
@ stdcall WaitForMultipleObjects(long ptr long long) kernel32.WaitForMultipleObjects
@ stdcall WTSGetActiveConsoleSessionId() kernel32.WTSGetActiveConsoleSessionId
