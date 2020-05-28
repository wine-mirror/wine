@ cdecl -arch=arm,arm64,x86_64 RtlAddFunctionTable(ptr long long) ntdll.RtlAddFunctionTable
@ stdcall -norelay RtlCaptureContext(ptr) ntdll.RtlCaptureContext
@ stdcall RtlCaptureStackBackTrace(long long ptr ptr) ntdll.RtlCaptureStackBackTrace
@ stdcall RtlCompareMemory(ptr ptr long) ntdll.RtlCompareMemory
@ stdcall -arch=x86_64 RtlCopyMemory(ptr ptr long) ntdll.RtlCopyMemory
@ cdecl -arch=arm,arm64,x86_64 RtlDeleteFunctionTable(ptr) ntdll.RtlDeleteFunctionTable
@ stdcall RtlFillMemory(ptr long long) ntdll.RtlFillMemory
@ cdecl -arch=arm,arm64,x86_64 RtlInstallFunctionTableCallback(long long long ptr ptr wstr) ntdll.RtlInstallFunctionTableCallback
@ stdcall -arch=arm,arm64,x86_64 RtlLookupFunctionEntry(long ptr ptr) ntdll.RtlLookupFunctionEntry
@ stdcall RtlPcToFileHeader(ptr ptr) ntdll.RtlPcToFileHeader
@ stdcall -norelay RtlRaiseException(ptr) ntdll.RtlRaiseException
@ cdecl -arch=arm64,x86_64 RtlRestoreContext(ptr ptr) ntdll.RtlRestoreContext
@ stdcall -norelay RtlUnwind(ptr ptr ptr ptr) ntdll.RtlUnwind
@ stdcall -arch=arm64,x86_64 RtlUnwindEx(ptr ptr ptr ptr ptr ptr) ntdll.RtlUnwindEx
@ stdcall -arch=arm64,x86_64 RtlVirtualUnwind(long long long ptr ptr ptr ptr ptr) ntdll.RtlVirtualUnwind
