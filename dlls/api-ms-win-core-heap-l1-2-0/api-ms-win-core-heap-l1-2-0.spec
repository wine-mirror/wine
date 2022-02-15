@ stdcall -norelay GetProcessHeap() kernelbase.GetProcessHeap
@ stdcall GetProcessHeaps(long ptr) kernelbase.GetProcessHeaps
@ stdcall HeapAlloc(long long long) kernelbase.HeapAlloc
@ stdcall HeapCompact(long long) kernelbase.HeapCompact
@ stdcall HeapCreate(long long long) kernelbase.HeapCreate
@ stdcall HeapDestroy(long) kernelbase.HeapDestroy
@ stdcall HeapFree(long long ptr) kernelbase.HeapFree
@ stdcall HeapLock(long) kernelbase.HeapLock
@ stdcall HeapQueryInformation(long long ptr long ptr) kernelbase.HeapQueryInformation
@ stdcall HeapReAlloc(long long ptr long) kernelbase.HeapReAlloc
@ stdcall HeapSetInformation(ptr long ptr long) kernelbase.HeapSetInformation
@ stdcall HeapSize(long long ptr) kernelbase.HeapSize
@ stdcall HeapUnlock(long) kernelbase.HeapUnlock
@ stdcall HeapValidate(long long ptr) kernelbase.HeapValidate
@ stdcall HeapWalk(long ptr) kernelbase.HeapWalk
