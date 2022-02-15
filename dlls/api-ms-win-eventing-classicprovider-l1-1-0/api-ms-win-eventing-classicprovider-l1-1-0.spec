@ stdcall GetTraceEnableFlags(int64) kernelbase.GetTraceEnableFlags
@ stdcall GetTraceEnableLevel(int64) kernelbase.GetTraceEnableLevel
@ stdcall -ret64 GetTraceLoggerHandle(ptr) kernelbase.GetTraceLoggerHandle
@ stdcall RegisterTraceGuidsW(ptr ptr ptr long ptr wstr wstr ptr) kernelbase.RegisterTraceGuidsW
@ stdcall TraceEvent(int64 ptr) kernelbase.TraceEvent
@ varargs TraceMessage(int64 long ptr long) kernelbase.TraceMessage
@ stdcall TraceMessageVa(int64 long ptr long ptr) kernelbase.TraceMessageVa
@ stdcall UnregisterTraceGuids(int64) kernelbase.UnregisterTraceGuids
