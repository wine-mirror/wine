# summary: resource modification dll
#
name	stress
type	win16
owner	kernel32

2   pascal allocmem(long)		AllocMem
3   pascal freeallmem()			FreeAllMem
6   pascal allocfilehandles(word)	AllocFileHandles
7   pascal unallocfilehandles()		UnAllocFileHandles
8   pascal getfreefilehandles()		GetFreeFileHandles
10  pascal allocdiskspace(long word)	AllocDiskSpace
11  pascal unallocdiskspace(word)	UnAllocDiskSpace
12  pascal allocusermem(word)		AllocUserMem
13  pascal freeallusermem()		FreeAllUserMem
14  pascal allocgdimem(word)		AllocGDIMem
15  pascal freeallgdimem()		FreeAllGDIMem
16  stub   GETFREEHEAP32SPACE
17  stub   ALLOCHEAP32MEM
18  stub   FREEALLHEAP32MEM
