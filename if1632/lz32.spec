name	lz32
type	win32

0 stdcall LZCopy(long long) LZCopy
1 stdcall LZOpenFileA(ptr ptr long) LZOpenFile32A
2 stdcall LZInit(long) LZInit
3 stdcall LZSeek(long long long) LZSeek
4 stdcall LZRead(long ptr long) LZRead32
5 stdcall LZClose(long) LZClose
6 stdcall LZStart() LZStart
7 stdcall CopyLZFile(long long) CopyLZFile
8 stdcall LZDone() LZDone
9 stdcall GetExpandedNameA(ptr ptr) GetExpandedName32A
10 stdcall LZOpenFileW(ptr ptr long) LZOpenFile32W
11 stdcall GetExpandedNameW(ptr ptr) GetExpandedName32W
