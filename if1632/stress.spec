# summary: resource modification dll
#
name	stress
id	10
length	15

2   pascal allocmem(long)		AllocMem(1)
3   pascal freeallmem()			FreeAllMem()
6   pascal allocfilehandles(word)	AllocFileHandles(1)
7   pascal unallocfilehandles()		UnAllocFileHandles()
8   pascal getfreefilehandles()		GetFreeFileHandles()
10  pascal allocdiskspace(long word)	AllocDiskSpace(1 2)
11  pascal unallocdiskspace(word)	UnAllocDiskSpace(1)
12  pascal allocusermem(word)		AllocUserMem(1)
13  pascal freeallusermem()		FreeAllUserMem()
14  pascal allocgdimem(word)		AllocGDIMem(1)
15  pascal freeallgdimem()		FreeAllGDIMem()

