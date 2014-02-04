@ stdcall -norelay RtlCaptureContext(ptr) ntdll.RtlCaptureContext
@ stdcall RtlCaptureStackBackTrace(long long ptr ptr) ntdll.RtlCaptureStackBackTrace
@ stdcall RtlCompareMemory(ptr ptr long) ntdll.RtlCompareMemory
@ stdcall RtlPcToFileHeader(ptr ptr) ntdll.RtlPcToFileHeader
@ stdcall -register RtlRaiseException(ptr) ntdll.RtlRaiseException
@ stdcall -register RtlUnwind(ptr ptr ptr ptr) ntdll.RtlUnwind
