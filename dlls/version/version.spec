name version
type win32

import kernel32.dll
import lz32.dll

@ stdcall GetFileVersionInfoA(str long long ptr) GetFileVersionInfoA
@ stdcall GetFileVersionInfoSizeA(str ptr) GetFileVersionInfoSizeA
@ stdcall GetFileVersionInfoSizeW(wstr ptr) GetFileVersionInfoSizeW
@ stdcall GetFileVersionInfoW(wstr long long ptr) GetFileVersionInfoW
@ stub VerFThk_ThunkData32
@ stdcall VerFindFileA(long str str str ptr ptr ptr ptr) VerFindFileA
@ stdcall VerFindFileW(long wstr wstr wstr ptr ptr ptr ptr) VerFindFileW
@ stdcall VerInstallFileA(long str str str str str ptr ptr) VerInstallFileA
@ stdcall VerInstallFileW(long wstr wstr wstr wstr wstr ptr ptr) VerInstallFileW
@ forward VerLanguageNameA KERNEL32.VerLanguageNameA
@ forward VerLanguageNameW KERNEL32.VerLanguageNameW
@ stdcall VerQueryValueA(ptr str ptr ptr) VerQueryValueA
@ stdcall VerQueryValueW(ptr wstr ptr ptr) VerQueryValueW
@ stub VerThkSL_ThunkData32
