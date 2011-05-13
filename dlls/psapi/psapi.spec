@ stdcall EmptyWorkingSet(long) kernel32.K32EmptyWorkingSet
@ stdcall EnumDeviceDrivers(ptr long ptr)
@ stdcall EnumPageFilesA(ptr ptr)
@ stdcall EnumPageFilesW(ptr ptr)
@ stdcall EnumProcessModules(long ptr long ptr) kernel32.K32EnumProcessModules
@ stdcall EnumProcesses(ptr long ptr) kernel32.K32EnumProcesses
@ stdcall GetDeviceDriverBaseNameA(ptr ptr long)
@ stdcall GetDeviceDriverBaseNameW(ptr ptr long)
@ stdcall GetDeviceDriverFileNameA(ptr ptr long)
@ stdcall GetDeviceDriverFileNameW(ptr ptr long)
@ stdcall GetMappedFileNameA(long ptr ptr long) kernel32.K32GetMappedFileNameA
@ stdcall GetMappedFileNameW(long ptr ptr long) kernel32.K32GetMappedFileNameW
@ stdcall GetModuleBaseNameA(long long ptr long) kernel32.K32GetModuleBaseNameA
@ stdcall GetModuleBaseNameW(long long ptr long) kernel32.K32GetModuleBaseNameW
@ stdcall GetModuleFileNameExA(long long ptr long) kernel32.K32GetModuleFileNameExA
@ stdcall GetModuleFileNameExW(long long ptr long) kernel32.K32GetModuleFileNameExW
@ stdcall GetModuleInformation(long long ptr long) kernel32.K32GetModuleInformation
@ stdcall GetPerformanceInfo(ptr long)
@ stdcall GetProcessImageFileNameA(long ptr long) kernel32.K32GetProcessImageFileNameA
@ stdcall GetProcessImageFileNameW(long ptr long) kernel32.K32GetProcessImageFileNameW
@ stdcall GetProcessMemoryInfo(long ptr long) kernel32.K32GetProcessMemoryInfo
@ stdcall GetWsChanges(long ptr long)
@ stdcall InitializeProcessForWsWatch(long)
@ stdcall QueryWorkingSet(long ptr long) kernel32.K32QueryWorkingSet
@ stdcall QueryWorkingSetEx(long ptr long) kernel32.K32QueryWorkingSetEx
