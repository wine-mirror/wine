name version
type win32

import kernel32.dll

0 stdcall GetFileVersionInfoA(str long long ptr) GetFileVersionInfoA
1 stdcall GetFileVersionInfoSizeA(str ptr) GetFileVersionInfoSizeA
2 stdcall GetFileVersionInfoSizeW(wstr ptr) GetFileVersionInfoSizeW
3 stdcall GetFileVersionInfoW(wstr long long ptr) GetFileVersionInfoW
#4 stub VerFThk_ThunkData32
5 stdcall VerFindFileA(long str str str ptr ptr ptr ptr) VerFindFileA
6 stdcall VerFindFileW(long wstr wstr wstr ptr ptr ptr ptr) VerFindFileW
7 stdcall VerInstallFileA(long str str str str str ptr ptr) VerInstallFileA
8 stdcall VerInstallFileW(long wstr wstr wstr wstr wstr ptr ptr) VerInstallFileW
9 forward VerLanguageNameA KERNEL32.VerLanguageNameA
10 forward VerLanguageNameW KERNEL32.VerLanguageNameW
11 stdcall VerQueryValueA(ptr str ptr ptr) VerQueryValueA
12 stdcall VerQueryValueW(ptr wstr ptr ptr) VerQueryValueW
#13 stub VerThkSL_ThunkData32
