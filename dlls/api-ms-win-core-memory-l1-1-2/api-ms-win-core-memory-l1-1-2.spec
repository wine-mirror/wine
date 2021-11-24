@ stdcall AllocateUserPhysicalPages(long ptr ptr) kernel32.AllocateUserPhysicalPages
@ stdcall AllocateUserPhysicalPagesNuma(long ptr ptr long) kernel32.AllocateUserPhysicalPagesNuma
@ stub CreateFileMappingFromApp
@ stdcall CreateFileMappingNumaW(long ptr long long long wstr long) kernel32.CreateFileMappingNumaW
@ stdcall CreateFileMappingW(long ptr long long long wstr) kernel32.CreateFileMappingW
@ stdcall CreateMemoryResourceNotification(long) kernel32.CreateMemoryResourceNotification
@ stdcall FlushViewOfFile(ptr long) kernel32.FlushViewOfFile
@ stdcall FreeUserPhysicalPages(long ptr ptr) kernel32.FreeUserPhysicalPages
@ stdcall GetLargePageMinimum() kernel32.GetLargePageMinimum
@ stub GetMemoryErrorHandlingCapabilities
@ stdcall GetProcessWorkingSetSizeEx(long ptr ptr ptr) kernel32.GetProcessWorkingSetSizeEx
@ stdcall GetSystemFileCacheSize(ptr ptr ptr) kernel32.GetSystemFileCacheSize
@ stdcall GetWriteWatch(long ptr long ptr ptr ptr) kernel32.GetWriteWatch
@ stdcall MapUserPhysicalPages(ptr long ptr) kernel32.MapUserPhysicalPages
@ stdcall MapViewOfFile(long long long long long) kernel32.MapViewOfFile
@ stdcall MapViewOfFileEx(long long long long long ptr) kernel32.MapViewOfFileEx
@ stub MapViewOfFileFromApp
@ stdcall OpenFileMappingW(long long wstr) kernel32.OpenFileMappingW
@ stdcall PrefetchVirtualMemory(ptr ptr ptr long) kernel32.PrefetchVirtualMemory
@ stdcall QueryMemoryResourceNotification(ptr ptr) kernel32.QueryMemoryResourceNotification
@ stdcall ReadProcessMemory(long ptr ptr long ptr) kernel32.ReadProcessMemory
@ stub RegisterBadMemoryNotification
@ stdcall ResetWriteWatch(ptr long) kernel32.ResetWriteWatch
@ stdcall SetProcessWorkingSetSizeEx(long long long long) kernel32.SetProcessWorkingSetSizeEx
@ stdcall SetSystemFileCacheSize(long long long) kernel32.SetSystemFileCacheSize
@ stdcall UnmapViewOfFile(ptr) kernel32.UnmapViewOfFile
@ stub UnmapViewOfFileEx
@ stub UnregisterBadMemoryNotification
@ stdcall VirtualAlloc(ptr long long long) kernel32.VirtualAlloc
@ stdcall VirtualAllocEx(long ptr long long long) kernel32.VirtualAllocEx
@ stdcall VirtualAllocExNuma(long ptr long long long long) kernel32.VirtualAllocExNuma
@ stdcall VirtualFree(ptr long long) kernel32.VirtualFree
@ stdcall VirtualFreeEx(long ptr long long) kernel32.VirtualFreeEx
@ stdcall VirtualLock(ptr long) kernel32.VirtualLock
@ stdcall VirtualProtect(ptr long long ptr) kernel32.VirtualProtect
@ stdcall VirtualProtectEx(long ptr long long ptr) kernel32.VirtualProtectEx
@ stdcall VirtualQuery(ptr ptr long) kernel32.VirtualQuery
@ stdcall VirtualUnlock(ptr long) kernel32.VirtualUnlock
@ stdcall WriteProcessMemory(long ptr ptr long ptr) kernel32.WriteProcessMemory
