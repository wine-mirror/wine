name    psapi
type    win32

  1 stdcall EmptyWorkingSet(long) EmptyWorkingSet
  2 stdcall EnumDeviceDrivers(ptr long ptr) EnumDeviceDrivers
  3 stdcall EnumProcessModules(long ptr long ptr) EnumProcessModules
  4 stdcall EnumProcesses(ptr long ptr) EnumProcesses
  5 stdcall GetDeviceDriverBaseNameA(ptr str long) GetDeviceDriverBaseNameA
  6 stdcall GetDeviceDriverBaseNameW(ptr wstr long) GetDeviceDriverBaseNameW
  7 stdcall GetDeviceDriverFileNameA(ptr str long) GetDeviceDriverFileNameA
  8 stdcall GetDeviceDriverFileNameW(ptr wstr long) GetDeviceDriverFileNameW
  9 stdcall GetMappedFileNameA(long ptr str long) GetMappedFileNameA
 10 stdcall GetMappedFileNameW(long ptr wstr long) GetMappedFileNameW
 11 stdcall GetModuleBaseNameA(long long str long) GetModuleBaseNameA
 12 stdcall GetModuleBaseNameW(long long wstr long) GetModuleBaseNameW
 13 stdcall GetModuleFileNameExA(long long str long) GetModuleFileNameExA 
 14 stdcall GetModuleFileNameExW(long long wstr long) GetModuleFileNameExW
 15 stdcall GetModuleInformation(long long ptr long) GetModuleInformation
 16 stdcall GetProcessMemoryInfo(long ptr long) GetProcessMemoryInfo
 17 stdcall GetWsChanges(long ptr long) GetWsChanges
 18 stdcall InitializeProcessForWsWatch(long) InitializeProcessForWsWatch
