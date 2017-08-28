@ stdcall ControlTraceA(int64 str ptr long) advapi32.ControlTraceA
@ stdcall EnableTrace(long long long ptr int64) advapi32.EnableTrace
@ stdcall EnableTraceEx(ptr ptr int64 long long int64 int64 long ptr) advapi32.EnableTraceEx
@ stdcall EnumerateTraceGuids(ptr long ptr) advapi32.EnumerateTraceGuids
@ stdcall FlushTraceA(int64 str ptr) advapi32.FlushTraceA
@ stdcall FlushTraceW(int64 wstr ptr) advapi32.FlushTraceW
@ stdcall -ret64 OpenTraceA(ptr) advapi32.OpenTraceA
@ stdcall QueryAllTracesA(ptr long ptr) advapi32.QueryAllTracesA
@ stub QueryTraceA
@ stdcall QueryTraceW(int64 wstr ptr) advapi32.QueryTraceW
@ stdcall StartTraceA(ptr str ptr) advapi32.StartTraceA
@ stdcall StopTraceA(int64 str ptr) advapi32.StopTraceA
@ stub UpdateTraceA
@ stub UpdateTraceW
