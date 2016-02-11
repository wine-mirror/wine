@ stdcall GetTraceEnableFlags(int64) advapi32.GetTraceEnableFlags
@ stdcall GetTraceEnableLevel(int64) advapi32.GetTraceEnableLevel
@ stdcall -ret64 GetTraceLoggerHandle(ptr) advapi32.GetTraceLoggerHandle
@ stdcall RegisterTraceGuidsW(ptr ptr ptr long ptr wstr wstr ptr) advapi32.RegisterTraceGuidsW
@ stdcall TraceEvent(int64 ptr) advapi32.TraceEvent
@ varargs TraceMessage(int64 long ptr long) advapi32.TraceMessage
@ stdcall TraceMessageVa(int64 long ptr long ptr) advapi32.TraceMessageVa
@ stdcall UnregisterTraceGuids(int64) advapi32.UnregisterTraceGuids
