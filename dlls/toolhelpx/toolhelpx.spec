@ stdcall -private DllMain(long long ptr)
@ stdcall GetSourceLineFromAddress(long long ptr ptr ptr)
@ stdcall GetSymbolFromAddress(long long ptr ptr ptr)
@ stdcall QuerySystemHardwareInfo()
@ stdcall SetThreadName(long wstr)
@ stdcall GetThreadName(long ptr long ptr)
@ stdcall CloseTrace(int64) advapi32.CloseTrace
@ stdcall ContinueDebugEvent(long long long) kernel32.ContinueDebugEvent
@ stdcall ControlTraceW(int64 wstr ptr long) advapi32.ControlTraceW
@ stdcall DebugActiveProcess(long) kernel32.DebugActiveProcess
@ stdcall DebugActiveProcessStop(long) kernel32.DebugActiveProcessStop
@ stdcall EnableTrace(long long long ptr int64) advapi32.EnableTrace
@ stdcall EnableTraceEx(ptr ptr int64 long long int64 long ptr) advapi32.EnableTraceEx
@ stdcall EnableTraceEx2(int64 ptr long long int64 long ptr) advapi32.EnableTraceEx2
@ stdcall K32EnumProcesses(ptr long ptr) kernel32.K32EnumProcesses
@ stdcall K32EnumProcessModules(long ptr long ptr) kernel32.K32EnumProcessModules
@ stdcall K32GetProcessMemoryInfo(long ptr long) kernel32.K32GetProcessMemoryInfo
@ stdcall K32GetModuleBaseNameW(long long ptr long) kernel32.K32GetModuleBaseNameW
@ stdcall K32GetModuleFileNameExW(long long ptr long) kernel32.K32GetModuleFileNameExW
@ stdcall K32GetModuleInformation(long long ptr long) kernel32.K32GetModuleInformation
@ stdcall MiniDumpWriteDump(long long long long ptr ptr ptr) dbghelp.MiniDumpWriteDump
@ stdcall QueryFullProcessImageNameW(long long ptr ptr) kernel32.QueryFullProcessImageNameW
@ stdcall StartTraceW(ptr wstr ptr) advapi32.StartTraceW
@ stdcall StopTraceW(int64 wstr ptr) advapi32.StopTraceW
@ stdcall WaitForDebugEvent(ptr long) kernel32.WaitForDebugEvent
