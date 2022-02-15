@ stdcall InitializeSListHead(ptr) kernelbase.InitializeSListHead
@ stdcall -arch=i386 -ret64 InterlockedCompareExchange64(ptr int64 int64) kernelbase.InterlockedCompareExchange64
@ stdcall -arch=i386 InterlockedCompareExchange(ptr long long) kernelbase.InterlockedCompareExchange
@ stdcall -arch=i386 InterlockedDecrement(ptr) kernelbase.InterlockedDecrement
@ stdcall -arch=i386 InterlockedExchange(ptr long) kernelbase.InterlockedExchange
@ stdcall -arch=i386 InterlockedExchangeAdd(ptr long ) kernelbase.InterlockedExchangeAdd
@ stdcall InterlockedFlushSList(ptr) kernelbase.InterlockedFlushSList
@ stdcall -arch=i386 InterlockedIncrement(ptr) kernelbase.InterlockedIncrement
@ stdcall InterlockedPopEntrySList(ptr) kernelbase.InterlockedPopEntrySList
@ stdcall InterlockedPushEntrySList(ptr ptr) kernelbase.InterlockedPushEntrySList
@ stdcall InterlockedPushListSListEx(ptr ptr ptr long) kernelbase.InterlockedPushListSListEx
@ stdcall QueryDepthSList(ptr) kernelbase.QueryDepthSList
