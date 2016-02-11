@ stdcall InitializeSListHead(ptr) kernel32.InitializeSListHead
@ stdcall -arch=i386 -ret64 InterlockedCompareExchange64(ptr int64 int64) kernel32.InterlockedCompareExchange64
@ stdcall -arch=i386 InterlockedCompareExchange(ptr long long) kernel32.InterlockedCompareExchange
@ stdcall -arch=i386 InterlockedDecrement(ptr) kernel32.InterlockedDecrement
@ stdcall -arch=i386 InterlockedExchange(ptr long) kernel32.InterlockedExchange
@ stdcall -arch=i386 InterlockedExchangeAdd(ptr long ) kernel32.InterlockedExchangeAdd
@ stdcall InterlockedFlushSList(ptr) kernel32.InterlockedFlushSList
@ stdcall -arch=i386 InterlockedIncrement(ptr) kernel32.InterlockedIncrement
@ stdcall InterlockedPopEntrySList(ptr) kernel32.InterlockedPopEntrySList
@ stdcall InterlockedPushEntrySList(ptr ptr) kernel32.InterlockedPushEntrySList
@ stdcall InterlockedPushListSListEx(ptr ptr ptr long) kernel32.InterlockedPushListSListEx
@ stdcall QueryDepthSList(ptr) kernel32.QueryDepthSList
