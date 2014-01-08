@ stdcall CheckRemoteDebuggerPresent(long ptr) kernel32.CheckRemoteDebuggerPresent
@ stdcall ContinueDebugEvent(long long long) kernel32.ContinueDebugEvent
@ stdcall DebugActiveProcess(long) kernel32.DebugActiveProcess
@ stdcall DebugActiveProcessStop(long) kernel32.DebugActiveProcessStop
@ stdcall DebugBreak() kernel32.DebugBreak
@ stdcall IsDebuggerPresent() kernel32.IsDebuggerPresent
@ stdcall OutputDebugStringA(str) kernel32.OutputDebugStringA
@ stdcall OutputDebugStringW(wstr) kernel32.OutputDebugStringW
@ stdcall WaitForDebugEvent(ptr long) kernel32.WaitForDebugEvent
