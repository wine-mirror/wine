# $Id: kernel.spec,v 1.3 1993/07/04 04:04:21 root Exp root $
#
name	kernel
id	1
length	410

3   return GetVersion 0 0x301
5   pascal LocalAlloc(word word) HEAP_LocalAlloc(1 2)
15  pascal GlobalAlloc(word long) GlobalAlloc(1 2)
16  pascal GlobalReAlloc(word long word) GlobalReAlloc(1 2 3)
17  pascal GlobalFree(word) GlobalFree(1)
18  pascal GlobalLock(word) GlobalLock(1)
19  pascal GlobalUnlock(word) GlobalUnlock(1)
20  pascal GlobalSize(word) GlobalSize(1)
21  pascal GlobalHandle(word) GlobalHandle(1)
22  pascal GlobalFlags(word) GlobalFlags(1)
23  pascal LockSegment(s_word) KERNEL_LockSegment(1)
24  pascal UnlockSegment(s_word) KERNEL_UnlockSegment(1)
25  pascal GlobalCompact(long) GlobalCompact(1)
30  pascal WaitEvent(word) KERNEL_WaitEvent(1)
49  pascal GetModuleFileName(word ptr s_word) KERNEL_GetModuleFileName(1 2 3)
51  pascal MakeProcInstance(ptr word) CALLBACK_MakeProcInstance(1 2)
91  pascal InitTask() KERNEL_InitTask()
102 register DOS3Call(word word word word word
		      word word word word word) 
	     KERNEL_DOS3Call(1 2 3 4 5 6 7 8 9 10)
111 pascal GlobalWire(word) GlobalLock(1)
112 pascal GlobalUnWire(word) GlobalUnlock(1)
131 pascal GetDOSEnvironment() GetDOSEnvironment()
132 return GetWinFlags 0 0x413
154 return GlobalNotify 4 0
163 pascal GlobalLRUOldest(word) ReturnArg(1)
164 pascal GlobalLRUNewest(word) ReturnArg(1)
178 equate __WINFLAGS 0x413
184 return GlobalDOSAlloc 4 0
185 return GlobalDOSFree 2 0
191 pascal GlobalPageLock(word) GlobalLock(1)
192 pascal GlobalPageUnlock(word) GlobalUnlock(1)
197 pascal GlobalFix(word) GlobalLock(1)
198 pascal GlobalUnfix(word) GlobalUnlock(1)
