@ stdcall CreateFileMappingW(long ptr long long long wstr) kernelbase.CreateFileMappingW
@ stdcall FlushViewOfFile(ptr long) kernelbase.FlushViewOfFile
@ stdcall MapViewOfFile(long long long long long) kernelbase.MapViewOfFile
@ stdcall MapViewOfFileEx(long long long long long ptr) kernelbase.MapViewOfFileEx
@ stdcall OpenFileMappingW(long long wstr) kernelbase.OpenFileMappingW
@ stdcall ReadProcessMemory(long ptr ptr long ptr) kernelbase.ReadProcessMemory
@ stdcall UnmapViewOfFile(ptr) kernelbase.UnmapViewOfFile
@ stdcall VirtualAlloc(ptr long long long) kernelbase.VirtualAlloc
@ stdcall VirtualAllocEx(long ptr long long long) kernelbase.VirtualAllocEx
@ stdcall VirtualFree(ptr long long) kernelbase.VirtualFree
@ stdcall VirtualFreeEx(long ptr long long) kernelbase.VirtualFreeEx
@ stdcall VirtualProtect(ptr long long ptr) kernelbase.VirtualProtect
@ stdcall VirtualProtectEx(long ptr long long ptr) kernelbase.VirtualProtectEx
@ stdcall VirtualQuery(ptr ptr long) kernelbase.VirtualQuery
@ stdcall VirtualQueryEx(long ptr ptr long) kernelbase.VirtualQueryEx
@ stdcall WriteProcessMemory(long ptr ptr long ptr) kernelbase.WriteProcessMemory
