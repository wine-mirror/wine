# summary: resource modification dll
#
2   pascal AllocMem(long) AllocMem
3   pascal FreeAllMem() FreeAllMem
6   pascal AllocFileHandles(word) AllocFileHandles
7   pascal UnAllocFileHandles() UnAllocFileHandles
8   pascal GetFreeFileHandles() GetFreeFileHandles
10  pascal AllocDiskSpace(long word) AllocDiskSpace
11  pascal UnAllocDiskSpace(word) UnAllocDiskSpace
12  pascal AllocUserMem(word) AllocUserMem
13  pascal FreeAllUserMem() FreeAllUserMem
14  pascal AllocGDIMem(word) AllocGDIMem
15  pascal FreeAllGDIMem() FreeAllGDIMem
16  stub   GETFREEHEAP32SPACE
17  stub   ALLOCHEAP32MEM
18  stub   FREEALLHEAP32MEM
