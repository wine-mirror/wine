@ stdcall -import EmptyWorkingSet(long) K32EmptyWorkingSet
@ stdcall -import EnumDeviceDrivers(ptr long ptr) K32EnumDeviceDrivers
@ stdcall -import EnumPageFilesA(ptr ptr) K32EnumPageFilesA
@ stdcall -import EnumPageFilesW(ptr ptr) K32EnumPageFilesW
@ stdcall -import EnumProcessModules(long ptr long ptr) K32EnumProcessModules
@ stdcall -import EnumProcessModulesEx(long ptr long ptr long) K32EnumProcessModulesEx
@ stdcall -import EnumProcesses(ptr long ptr) K32EnumProcesses
@ stdcall -import GetDeviceDriverBaseNameA(ptr ptr long) K32GetDeviceDriverBaseNameA
@ stdcall -import GetDeviceDriverBaseNameW(ptr ptr long) K32GetDeviceDriverBaseNameW
@ stdcall -import GetDeviceDriverFileNameA(ptr ptr long) K32GetDeviceDriverFileNameA
@ stdcall -import GetDeviceDriverFileNameW(ptr ptr long) K32GetDeviceDriverFileNameW
@ stdcall -import GetMappedFileNameA(long ptr ptr long) K32GetMappedFileNameA
@ stdcall -import GetMappedFileNameW(long ptr ptr long) K32GetMappedFileNameW
@ stdcall -import GetModuleBaseNameA(long long ptr long) K32GetModuleBaseNameA
@ stdcall -import GetModuleBaseNameW(long long ptr long) K32GetModuleBaseNameW
@ stdcall -import GetModuleFileNameExA(long long ptr long) K32GetModuleFileNameExA
@ stdcall -import GetModuleFileNameExW(long long ptr long) K32GetModuleFileNameExW
@ stdcall -import GetModuleInformation(long long ptr long) K32GetModuleInformation
@ stdcall -import GetPerformanceInfo(ptr long) K32GetPerformanceInfo
@ stdcall -import GetProcessImageFileNameA(long ptr long) K32GetProcessImageFileNameA
@ stdcall -import GetProcessImageFileNameW(long ptr long) K32GetProcessImageFileNameW
@ stdcall -import GetProcessMemoryInfo(long ptr long) K32GetProcessMemoryInfo
@ stdcall -import GetWsChanges(long ptr long) K32GetWsChanges
@ stdcall -import GetWsChangesEx(long ptr ptr) K32GetWsChangesEx
@ stdcall -import InitializeProcessForWsWatch(long) K32InitializeProcessForWsWatch
@ stdcall -import QueryWorkingSet(long ptr long) K32QueryWorkingSet
@ stdcall -import QueryWorkingSetEx(long ptr long) K32QueryWorkingSetEx
