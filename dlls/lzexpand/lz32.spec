name	lz32
type	win32

import	kernel32.dll

@ stdcall CopyLZFile(long long) CopyLZFile
@ stdcall GetExpandedNameA(str ptr) GetExpandedNameA
@ stdcall GetExpandedNameW(wstr ptr) GetExpandedNameW
@ stdcall LZClose(long) LZClose
@ stdcall LZCopy(long long) LZCopy
@ stdcall LZDone() LZDone
@ stdcall LZInit(long) LZInit
@ stdcall LZOpenFileA(str ptr long) LZOpenFileA
@ stdcall LZOpenFileW(wstr ptr long) LZOpenFileW
@ stdcall LZRead(long ptr long) LZRead
@ stdcall LZSeek(long long long) LZSeek
@ stdcall LZStart() LZStart
