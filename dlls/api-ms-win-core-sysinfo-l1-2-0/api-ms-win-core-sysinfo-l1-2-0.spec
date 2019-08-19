@ stdcall EnumSystemFirmwareTables(long ptr long) kernel32.EnumSystemFirmwareTables
@ stdcall GetComputerNameExA(long ptr ptr) kernel32.GetComputerNameExA
@ stdcall GetComputerNameExW(long ptr ptr) kernel32.GetComputerNameExW
@ stdcall GetLocalTime(ptr) kernel32.GetLocalTime
@ stdcall GetLogicalProcessorInformation(ptr ptr) kernel32.GetLogicalProcessorInformation
@ stdcall GetLogicalProcessorInformationEx(long ptr ptr) kernel32.GetLogicalProcessorInformationEx
@ stdcall GetNativeSystemInfo(ptr) kernel32.GetNativeSystemInfo
@ stub GetOsSafeBootMode
@ stdcall GetProductInfo(long long long long ptr) kernel32.GetProductInfo
@ stdcall GetSystemDirectoryA(ptr long) kernel32.GetSystemDirectoryA
@ stdcall GetSystemDirectoryW(ptr long) kernel32.GetSystemDirectoryW
@ stdcall GetSystemFirmwareTable(long long ptr long) kernel32.GetSystemFirmwareTable
@ stdcall GetSystemInfo(ptr) kernel32.GetSystemInfo
@ stdcall GetSystemTime(ptr) kernel32.GetSystemTime
@ stdcall GetSystemTimeAdjustment(ptr ptr ptr) kernel32.GetSystemTimeAdjustment
@ stdcall GetSystemTimeAsFileTime(ptr) kernel32.GetSystemTimeAsFileTime
@ stdcall GetSystemTimePreciseAsFileTime(ptr) kernel32.GetSystemTimePreciseAsFileTime
@ stdcall GetSystemWindowsDirectoryA(ptr long) kernel32.GetSystemWindowsDirectoryA
@ stdcall GetSystemWindowsDirectoryW(ptr long) kernel32.GetSystemWindowsDirectoryW
@ stdcall GetTickCount() kernel32.GetTickCount
@ stdcall -ret64 GetTickCount64() kernel32.GetTickCount64
@ stdcall GetVersion() kernel32.GetVersion
@ stdcall GetVersionExA(ptr) kernel32.GetVersionExA
@ stdcall GetVersionExW(ptr) kernel32.GetVersionExW
@ stdcall GetWindowsDirectoryA(ptr long) kernel32.GetWindowsDirectoryA
@ stdcall GetWindowsDirectoryW(ptr long) kernel32.GetWindowsDirectoryW
@ stdcall GlobalMemoryStatusEx(ptr) kernel32.GlobalMemoryStatusEx
@ stdcall SetComputerNameExW(long wstr) kernel32.SetComputerNameExW
@ stdcall SetLocalTime(ptr) kernel32.SetLocalTime
@ stdcall SetSystemTime(ptr) kernel32.SetSystemTime
@ stdcall -ret64 VerSetConditionMask(long long long long) kernel32.VerSetConditionMask
