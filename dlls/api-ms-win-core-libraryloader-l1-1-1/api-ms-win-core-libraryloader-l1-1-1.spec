@ stdcall AddDllDirectory(wstr) kernelbase.AddDllDirectory
@ stdcall DisableThreadLibraryCalls(long) kernelbase.DisableThreadLibraryCalls
@ stdcall EnumResourceLanguagesExA(long str str ptr long long long) kernelbase.EnumResourceLanguagesExA
@ stdcall EnumResourceLanguagesExW(long wstr wstr ptr long long long) kernelbase.EnumResourceLanguagesExW
@ stdcall EnumResourceNamesExA(long str ptr long long long) kernelbase.EnumResourceNamesExA
@ stdcall EnumResourceNamesExW(long wstr ptr long long long) kernelbase.EnumResourceNamesExW
@ stdcall EnumResourceTypesExA(long ptr long long long) kernelbase.EnumResourceTypesExA
@ stdcall EnumResourceTypesExW(long ptr long long long) kernelbase.EnumResourceTypesExW
@ stdcall FindResourceExW(long wstr wstr long) kernelbase.FindResourceExW
@ stdcall FindStringOrdinal(long wstr long wstr long long) kernelbase.FindStringOrdinal
@ stdcall FreeLibrary(long) kernelbase.FreeLibrary
@ stdcall FreeLibraryAndExitThread(long long) kernelbase.FreeLibraryAndExitThread
@ stdcall FreeResource(long) kernelbase.FreeResource
@ stdcall GetModuleFileNameA(long ptr long) kernelbase.GetModuleFileNameA
@ stdcall GetModuleFileNameW(long ptr long) kernelbase.GetModuleFileNameW
@ stdcall GetModuleHandleA(str) kernelbase.GetModuleHandleA
@ stdcall GetModuleHandleExA(long ptr ptr) kernelbase.GetModuleHandleExA
@ stdcall GetModuleHandleExW(long ptr ptr) kernelbase.GetModuleHandleExW
@ stdcall GetModuleHandleW(wstr) kernelbase.GetModuleHandleW
@ stdcall GetProcAddress(long str) kernelbase.GetProcAddress
@ stdcall LoadLibraryExA( str long long) kernelbase.LoadLibraryExA
@ stdcall LoadLibraryExW(wstr long long) kernelbase.LoadLibraryExW
@ stdcall LoadResource(long long) kernelbase.LoadResource
@ stdcall LoadStringA(long long ptr long) kernelbase.LoadStringA
@ stdcall LoadStringW(long long ptr long) kernelbase.LoadStringW
@ stdcall LockResource(long) kernelbase.LockResource
@ stub QueryOptionalDelayLoadedAPI
@ stdcall RemoveDllDirectory(ptr) kernelbase.RemoveDllDirectory
@ stdcall SetDefaultDllDirectories(long) kernelbase.SetDefaultDllDirectories
@ stdcall SizeofResource(long long) kernelbase.SizeofResource
