@ stdcall GetComputerNameExA(long ptr ptr) kernelbase.GetComputerNameExA
@ stdcall GetComputerNameExW(long ptr ptr) kernelbase.GetComputerNameExW
@ stdcall GetDynamicTimeZoneInformation(ptr) kernelbase.GetDynamicTimeZoneInformation
@ stdcall GetLocalTime(ptr) kernelbase.GetLocalTime
@ stdcall GetLogicalProcessorInformation(ptr ptr) kernelbase.GetLogicalProcessorInformation
@ stdcall GetLogicalProcessorInformationEx(long ptr ptr) kernelbase.GetLogicalProcessorInformationEx
@ stdcall GetSystemDirectoryA(ptr long) kernelbase.GetSystemDirectoryA
@ stdcall GetSystemDirectoryW(ptr long) kernelbase.GetSystemDirectoryW
@ stdcall GetSystemInfo(ptr) kernelbase.GetSystemInfo
@ stdcall GetSystemTime(ptr) kernelbase.GetSystemTime
@ stdcall GetSystemTimeAdjustment(ptr ptr ptr) kernelbase.GetSystemTimeAdjustment
@ stdcall GetSystemTimeAsFileTime(ptr) kernelbase.GetSystemTimeAsFileTime
@ stdcall GetSystemWindowsDirectoryA(ptr long) kernelbase.GetSystemWindowsDirectoryA
@ stdcall GetSystemWindowsDirectoryW(ptr long) kernelbase.GetSystemWindowsDirectoryW
@ stdcall -ret64 GetTickCount64() kernelbase.GetTickCount64
@ stdcall GetTickCount() kernelbase.GetTickCount
@ stdcall GetTimeZoneInformation(ptr) kernelbase.GetTimeZoneInformation
@ stdcall GetTimeZoneInformationForYear(long ptr ptr) kernelbase.GetTimeZoneInformationForYear
@ stdcall GetVersion() kernelbase.GetVersion
@ stdcall GetVersionExA(ptr) kernelbase.GetVersionExA
@ stdcall GetVersionExW(ptr) kernelbase.GetVersionExW
@ stdcall GetWindowsDirectoryA(ptr long) kernelbase.GetWindowsDirectoryA
@ stdcall GetWindowsDirectoryW(ptr long) kernelbase.GetWindowsDirectoryW
@ stdcall GlobalMemoryStatusEx(ptr) kernelbase.GlobalMemoryStatusEx
@ stdcall SetLocalTime(ptr) kernelbase.SetLocalTime
@ stdcall SystemTimeToFileTime(ptr ptr) kernelbase.SystemTimeToFileTime
@ stdcall SystemTimeToTzSpecificLocalTime(ptr ptr ptr) kernelbase.SystemTimeToTzSpecificLocalTime
@ stdcall TzSpecificLocalTimeToSystemTime(ptr ptr ptr) kernelbase.TzSpecificLocalTimeToSystemTime
