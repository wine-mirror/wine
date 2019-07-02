@ stdcall AddDllDirectory(wstr) kernel32.AddDllDirectory
@ stdcall DisableThreadLibraryCalls(long) kernel32.DisableThreadLibraryCalls
@ stdcall EnumResourceLanguagesExA(long str str ptr long long long) kernel32.EnumResourceLanguagesExA
@ stdcall EnumResourceLanguagesExW(long wstr wstr ptr long long long) kernel32.EnumResourceLanguagesExW
@ stdcall EnumResourceNamesExA(long str ptr long long long) kernel32.EnumResourceNamesExA
@ stdcall EnumResourceNamesExW(long wstr ptr long long long) kernel32.EnumResourceNamesExW
@ stdcall EnumResourceTypesExA(long ptr long long long) kernel32.EnumResourceTypesExA
@ stdcall EnumResourceTypesExW(long ptr long long long) kernel32.EnumResourceTypesExW
@ stdcall FindResourceExW(long wstr wstr long) kernel32.FindResourceExW
@ stdcall FindStringOrdinal(long wstr long wstr long long) kernel32.FindStringOrdinal
@ stdcall FreeLibrary(long) kernel32.FreeLibrary
@ stdcall FreeLibraryAndExitThread(long long) kernel32.FreeLibraryAndExitThread
@ stdcall FreeResource(long) kernel32.FreeResource
@ stdcall GetModuleFileNameA(long ptr long) kernel32.GetModuleFileNameA
@ stdcall GetModuleFileNameW(long ptr long) kernel32.GetModuleFileNameW
@ stdcall GetModuleHandleA(str) kernel32.GetModuleHandleA
@ stdcall GetModuleHandleExA(long ptr ptr) kernel32.GetModuleHandleExA
@ stdcall GetModuleHandleExW(long ptr ptr) kernel32.GetModuleHandleExW
@ stdcall GetModuleHandleW(wstr) kernel32.GetModuleHandleW
@ stdcall GetProcAddress(long str) kernel32.GetProcAddress
@ stdcall LoadLibraryExA( str long long) kernel32.LoadLibraryExA
@ stdcall LoadLibraryExW(wstr long long) kernel32.LoadLibraryExW
@ stdcall LoadResource(long long) kernel32.LoadResource
@ stdcall LoadStringA(long long ptr long) user32.LoadStringA
@ stdcall LoadStringW(long long ptr long) user32.LoadStringW
@ stdcall LockResource(long) kernel32.LockResource
@ stub QueryOptionalDelayLoadedAPI
@ stdcall RemoveDllDirectory(ptr) kernel32.RemoveDllDirectory
@ stdcall SetDefaultDllDirectories(long) kernel32.SetDefaultDllDirectories
@ stdcall SizeofResource(long long) kernel32.SizeofResource
