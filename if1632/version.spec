name version
type win32

0 stdcall GetFileVersionInfoA(ptr long long ptr) GetFileVersionInfo32A
1 stdcall GetFileVersionInfoSizeA(ptr ptr) GetFileVersionInfoSize32A
2 stdcall GetFileVersionInfoSizeW(ptr ptr) GetFileVersionInfoSize32W
3 stdcall GetFileVersionInfoW(ptr long long ptr) GetFileVersionInfo32W
#4 stub VerFThk_ThunkData32
5 stdcall VerFindFileA(word ptr ptr ptr ptr ptr ptr ptr) VerFindFile32A
6 stdcall VerFindFileW(word ptr ptr ptr ptr ptr ptr ptr) VerFindFile32W
7 stdcall VerInstallFileA(word ptr ptr ptr ptr ptr ptr ptr) VerInstallFile32A
8 stdcall VerInstallFileW(word ptr ptr ptr ptr ptr ptr ptr) VerInstallFile32W
9 stdcall VerLanguageNameA(word ptr word) VerLanguageName32A
10 stdcall VerLanguageNameW(word ptr word) VerLanguageName32W
11 stdcall VerQueryValueA(segptr ptr ptr ptr) VerQueryValue32A
12 stdcall VerQueryValueW(segptr ptr ptr ptr) VerQueryValue32W
#13 stub VerThkSL_ThunkData32
