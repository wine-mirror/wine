@ stdcall AllocateUserPhysicalPages(long ptr ptr) kernelbase.AllocateUserPhysicalPages
@ stdcall AllocateUserPhysicalPagesNuma(long ptr ptr long) kernelbase.AllocateUserPhysicalPagesNuma
@ stub CreateFileMappingFromApp
@ stdcall CreateFileMappingNumaW(long ptr long long long wstr long) kernelbase.CreateFileMappingNumaW
@ stdcall CreateFileMappingW(long ptr long long long wstr) kernelbase.CreateFileMappingW
@ stdcall CreateMemoryResourceNotification(long) kernelbase.CreateMemoryResourceNotification
@ stdcall FlushViewOfFile(ptr long) kernelbase.FlushViewOfFile
@ stdcall FreeUserPhysicalPages(long ptr ptr) kernelbase.FreeUserPhysicalPages
@ stdcall GetLargePageMinimum() kernelbase.GetLargePageMinimum
@ stub GetMemoryErrorHandlingCapabilities
@ stdcall GetProcessWorkingSetSizeEx(long ptr ptr ptr) kernelbase.GetProcessWorkingSetSizeEx
@ stdcall GetSystemFileCacheSize(ptr ptr ptr) kernelbase.GetSystemFileCacheSize
@ stdcall GetWriteWatch(long ptr long ptr ptr ptr) kernelbase.GetWriteWatch
@ stdcall MapUserPhysicalPages(ptr long ptr) kernelbase.MapUserPhysicalPages
@ stdcall MapViewOfFile(long long long long long) kernelbase.MapViewOfFile
@ stdcall MapViewOfFileEx(long long long long long ptr) kernelbase.MapViewOfFileEx
@ stub MapViewOfFileFromApp
@ stdcall OpenFileMappingW(long long wstr) kernelbase.OpenFileMappingW
@ stdcall PrefetchVirtualMemory(ptr ptr ptr long) kernelbase.PrefetchVirtualMemory
@ stdcall QueryMemoryResourceNotification(ptr ptr) kernelbase.QueryMemoryResourceNotification
@ stdcall ReadProcessMemory(long ptr ptr long ptr) kernelbase.ReadProcessMemory
@ stub RegisterBadMemoryNotification
@ stdcall ResetWriteWatch(ptr long) kernelbase.ResetWriteWatch
@ stdcall SetProcessWorkingSetSizeEx(long long long long) kernelbase.SetProcessWorkingSetSizeEx
@ stdcall SetSystemFileCacheSize(long long long) kernelbase.SetSystemFileCacheSize
@ stdcall UnmapViewOfFile(ptr) kernelbase.UnmapViewOfFile
@ stub UnmapViewOfFileEx
@ stub UnregisterBadMemoryNotification
@ stdcall VirtualAlloc(ptr long long long) kernelbase.VirtualAlloc
@ stdcall VirtualAllocEx(long ptr long long long) kernelbase.VirtualAllocEx
@ stdcall VirtualAllocExNuma(long ptr long long long long) kernelbase.VirtualAllocExNuma
@ stdcall VirtualFree(ptr long long) kernelbase.VirtualFree
@ stdcall VirtualFreeEx(long ptr long long) kernelbase.VirtualFreeEx
@ stdcall VirtualLock(ptr long) kernelbase.VirtualLock
@ stdcall VirtualProtect(ptr long long ptr) kernelbase.VirtualProtect
@ stdcall VirtualProtectEx(long ptr long long ptr) kernelbase.VirtualProtectEx
@ stdcall VirtualQuery(ptr ptr long) kernelbase.VirtualQuery
@ stdcall VirtualUnlock(ptr long) kernelbase.VirtualUnlock
@ stdcall WriteProcessMemory(long ptr ptr long ptr) kernelbase.WriteProcessMemory
