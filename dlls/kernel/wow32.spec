name	wow32
type	win32

# ordinal exports
1 stdcall WOWGetDescriptor(long long) WOWGetDescriptor

@ stdcall WOWCallback16(long long) WOWCallback16
@ stdcall WOWCallback16Ex(ptr long long ptr ptr) WOWCallback16Ex
@ stdcall WOWDirectedYield16(long) WOWDirectedYield16
@ stdcall WOWGetVDMPointer(long long long) WOWGetVDMPointer
@ stdcall WOWGetVDMPointerFix(long long long) WOWGetVDMPointerFix
@ stdcall WOWGetVDMPointerUnfix(long) WOWGetVDMPointerUnfix
@ stdcall WOWGlobalAlloc16(long long) WOWGlobalAlloc16
@ stdcall WOWGlobalAllocLock16(long long ptr) WOWGlobalAllocLock16
@ stdcall WOWGlobalFree16(long) WOWGlobalFree16
@ stdcall WOWGlobalLock16(long) WOWGlobalLock16
@ stdcall WOWGlobalLockSize16(long ptr) WOWGlobalLockSize16
@ stdcall WOWGlobalUnlock16(long) WOWGlobalUnlock16
@ stdcall WOWGlobalUnlockFree16(long) WOWGlobalUnlockFree16
@ stdcall WOWHandle16(long long) WOWHandle16
@ stdcall WOWHandle32(long long) WOWHandle32
@ stdcall WOWYield16() WOWYield16
