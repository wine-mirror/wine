name	wow32
type	win32

  1 stdcall WOWGetDescriptor(long long) WOWGetDescriptor
  2 stdcall WOWCallback16(long long) WOWCallback16
  3 stdcall WOWCallback16Ex(ptr long long ptr ptr) WOWCallback16Ex
  4 stdcall WOWDirectedYield16(long) WOWDirectedYield16
  5 stdcall WOWGetVDMPointer(long long long) WOWGetVDMPointer
  6 stdcall WOWGetVDMPointerFix(long long long) WOWGetVDMPointerFix
  7 stdcall WOWGetVDMPointerUnfix(long) WOWGetVDMPointerUnfix
  8 stdcall WOWGlobalAlloc16(long long) WOWGlobalAlloc16
  9 stdcall WOWGlobalAllocLock16(long long ptr) WOWGlobalAllocLock16
 10 stdcall WOWGlobalFree16(long) WOWGlobalFree16
 11 stdcall WOWGlobalLock16(long) WOWGlobalLock16
 12 stdcall WOWGlobalLockSize16(long ptr) WOWGlobalLockSize16
 13 stdcall WOWGlobalUnlock16(long) WOWGlobalUnlock16
 14 stdcall WOWGlobalUnlockFree16(long) WOWGlobalUnlockFree16
 15 stdcall WOWHandle16(long long) WOWHandle16
 16 stdcall WOWHandle32(long long) WOWHandle32
 17 stdcall WOWYield16() WOWYield16
