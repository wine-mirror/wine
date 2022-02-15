@ stub CreateFileMappingFromApp
@ stdcall CreateFileMappingW(long ptr long long long wstr) kernelbase.CreateFileMappingW
@ stub DiscardVirtualMemory
@ stdcall FlushViewOfFile(ptr long) kernelbase.FlushViewOfFile
@ stdcall GetLargePageMinimum() kernelbase.GetLargePageMinimum
@ stdcall GetProcessWorkingSetSizeEx(long ptr ptr ptr) kernelbase.GetProcessWorkingSetSizeEx
@ stdcall GetWriteWatch(long ptr long ptr ptr ptr) kernelbase.GetWriteWatch
@ stdcall MapViewOfFile(long long long long long) kernelbase.MapViewOfFile
@ stdcall MapViewOfFileEx(long long long long long ptr) kernelbase.MapViewOfFileEx
@ stub MapViewOfFileFromApp
@ stub OfferVirtualMemory
@ stub OpenFileMappingFromApp
@ stdcall OpenFileMappingW(long long wstr) kernelbase.OpenFileMappingW
@ stdcall ReadProcessMemory(long ptr ptr long ptr) kernelbase.ReadProcessMemory
@ stub ReclaimVirtualMemory
@ stdcall ResetWriteWatch(ptr long) kernelbase.ResetWriteWatch
@ stub SetProcessValidCallTargets
@ stdcall SetProcessWorkingSetSizeEx(long long long long) kernelbase.SetProcessWorkingSetSizeEx
@ stdcall UnmapViewOfFile(ptr) kernelbase.UnmapViewOfFile
@ stub UnmapViewOfFileEx
@ stdcall VirtualAlloc(ptr long long long) kernelbase.VirtualAlloc
@ stdcall VirtualAllocFromApp(ptr long long long) kernelbase.VirtualAllocFromApp
@ stdcall VirtualFree(ptr long long) kernelbase.VirtualFree
@ stdcall VirtualFreeEx(long ptr long long) kernelbase.VirtualFreeEx
@ stdcall VirtualLock(ptr long) kernelbase.VirtualLock
@ stdcall VirtualProtect(ptr long long ptr) kernelbase.VirtualProtect
@ stub VirtualProtectFromApp
@ stdcall VirtualQuery(ptr ptr long) kernelbase.VirtualQuery
@ stdcall VirtualQueryEx(long ptr ptr long) kernelbase.VirtualQueryEx
@ stdcall VirtualUnlock(ptr long) kernelbase.VirtualUnlock
@ stdcall WriteProcessMemory(long ptr ptr long ptr) kernelbase.WriteProcessMemory
