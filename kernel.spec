# $Id: kernel.spec,v 1.2 1993/06/30 14:24:33 root Exp root $
#
name	kernel
id	1
length	410

3   return GetVersion 0 0x301
5   pascal LocalAlloc(word word) HEAP_LocalAlloc(1 2)
23  pascal LockSegment(s_word) KERNEL_LockSegment(1)
24  pascal UnlockSegment(s_word) KERNEL_UnlockSegment(1)
30  pascal WaitEvent(word) KERNEL_WaitEvent(1)
49  pascal GetModuleFileName(word ptr s_word) KERNEL_GetModuleFileName(1 2 3)
91  pascal InitTask() KERNEL_InitTask()
102 register DOS3Call(word word word word word
		      word word word word word) 
	     KERNEL_DOS3Call(1 2 3 4 5 6 7 8 9 10)
131 pascal GetDOSEnvironment() GetDOSEnvironment()
178 equate __WINFLAGS 0x413
