@ stdcall EnumSystemFirmwareTables(long ptr long) kernelbase.EnumSystemFirmwareTables
@ stdcall GetComputerNameExA(long ptr ptr) kernelbase.GetComputerNameExA
@ stdcall GetComputerNameExW(long ptr ptr) kernelbase.GetComputerNameExW
@ stdcall GetLocalTime(ptr) kernelbase.GetLocalTime
@ stdcall GetLogicalProcessorInformation(ptr ptr) kernelbase.GetLogicalProcessorInformation
@ stdcall GetLogicalProcessorInformationEx(long ptr ptr) kernelbase.GetLogicalProcessorInformationEx
@ stdcall GetNativeSystemInfo(ptr) kernelbase.GetNativeSystemInfo
@ stub GetOsSafeBootMode
@ stdcall GetProductInfo(long long long long ptr) kernelbase.GetProductInfo
@ stdcall GetSystemDirectoryA(ptr long) kernelbase.GetSystemDirectoryA
@ stdcall GetSystemDirectoryW(ptr long) kernelbase.GetSystemDirectoryW
@ stdcall GetSystemFirmwareTable(long long ptr long) kernelbase.GetSystemFirmwareTable
@ stdcall GetSystemInfo(ptr) kernelbase.GetSystemInfo
@ stdcall GetSystemTime(ptr) kernelbase.GetSystemTime
@ stdcall GetSystemTimeAdjustment(ptr ptr ptr) kernelbase.GetSystemTimeAdjustment
@ stdcall GetSystemTimeAsFileTime(ptr) kernelbase.GetSystemTimeAsFileTime
@ stdcall GetSystemTimePreciseAsFileTime(ptr) kernelbase.GetSystemTimePreciseAsFileTime
@ stdcall GetSystemWindowsDirectoryA(ptr long) kernelbase.GetSystemWindowsDirectoryA
@ stdcall GetSystemWindowsDirectoryW(ptr long) kernelbase.GetSystemWindowsDirectoryW
@ stdcall GetTickCount() kernelbase.GetTickCount
@ stdcall -ret64 GetTickCount64() kernelbase.GetTickCount64
@ stdcall GetVersion() kernelbase.GetVersion
@ stdcall GetVersionExA(ptr) kernelbase.GetVersionExA
@ stdcall GetVersionExW(ptr) kernelbase.GetVersionExW
@ stdcall GetWindowsDirectoryA(ptr long) kernelbase.GetWindowsDirectoryA
@ stdcall GetWindowsDirectoryW(ptr long) kernelbase.GetWindowsDirectoryW
@ stdcall GlobalMemoryStatusEx(ptr) kernelbase.GlobalMemoryStatusEx
@ stdcall SetComputerNameExW(long wstr) kernelbase.SetComputerNameExW
@ stdcall SetLocalTime(ptr) kernelbase.SetLocalTime
@ stdcall SetSystemTime(ptr) kernelbase.SetSystemTime
@ stdcall -ret64 VerSetConditionMask(long long long long) kernelbase.VerSetConditionMask
