@ stdcall CheckRemoteDebuggerPresent(long ptr) kernelbase.CheckRemoteDebuggerPresent
@ stdcall ContinueDebugEvent(long long long) kernelbase.ContinueDebugEvent
@ stdcall DebugActiveProcess(long) kernelbase.DebugActiveProcess
@ stdcall DebugActiveProcessStop(long) kernelbase.DebugActiveProcessStop
@ stdcall DebugBreak() kernelbase.DebugBreak
@ stdcall IsDebuggerPresent() kernelbase.IsDebuggerPresent
@ stdcall OutputDebugStringA(str) kernelbase.OutputDebugStringA
@ stdcall OutputDebugStringW(wstr) kernelbase.OutputDebugStringW
@ stdcall WaitForDebugEvent(ptr long) kernelbase.WaitForDebugEvent
