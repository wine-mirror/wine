name	lz32
type	win32

0 stdcall LZCopy(word word) LZCopy
1 stdcall LZOpenFileA(ptr ptr word) LZOpenFile32A
2 stdcall LZInit(word) LZInit
3 stdcall LZSeek(word long word) LZSeek
4 stdcall LZRead(word ptr long) LZRead32
5 stdcall LZClose(word) LZClose
6 stdcall LZStart() LZStart
7 stdcall CopyLZFile(word word) CopyLZFile
8 stdcall LZDone() LZDone
9 stdcall GetExpandedNameA(ptr ptr) GetExpandedName32A
10 stdcall LZOpenFileW(ptr ptr word) LZOpenFile32W
11 stdcall GetExpandedNameW(ptr ptr) GetExpandedName32W
