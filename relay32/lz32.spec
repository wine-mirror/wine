name	lz32
type	win32

0 stdcall LZCopy(long long) LZCopy32
1 stdcall LZOpenFileA(str ptr long) LZOpenFile32A
2 stdcall LZInit(long) LZInit32
3 stdcall LZSeek(long long long) LZSeek32
4 stdcall LZRead(long ptr long) LZRead32
5 stdcall LZClose(long) LZClose32
6 stdcall LZStart() LZStart32
7 stdcall CopyLZFile(long long) CopyLZFile32
8 stdcall LZDone() LZDone
9 stdcall GetExpandedNameA(str ptr) GetExpandedName32A
10 stdcall LZOpenFileW(wstr ptr long) LZOpenFile32W
11 stdcall GetExpandedNameW(wstr ptr) GetExpandedName32W
