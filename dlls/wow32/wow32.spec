name	wow32
type	win32

import kernel32.dll

# ordinal exports
1 forward WOWGetDescriptor kernel32.K32WOWGetDescriptor

@ forward WOWCallback16 kernel32.K32WOWCallback16
@ forward WOWCallback16Ex kernel32.K32WOWCallback16Ex
@ forward WOWDirectedYield16 kernel32.K32WOWDirectedYield16
@ forward WOWGetVDMPointer kernel32.K32WOWGetVDMPointer
@ forward WOWGetVDMPointerFix kernel32.K32WOWGetVDMPointerFix
@ forward WOWGetVDMPointerUnfix kernel32.K32WOWGetVDMPointerUnfix
@ forward WOWGlobalAlloc16 kernel32.K32WOWGlobalAlloc16
@ forward WOWGlobalAllocLock16 kernel32.K32WOWGlobalAllocLock16
@ forward WOWGlobalFree16 kernel32.K32WOWGlobalFree16
@ forward WOWGlobalLock16 kernel32.K32WOWGlobalLock16
@ forward WOWGlobalLockSize16 kernel32.K32WOWGlobalLockSize16
@ forward WOWGlobalUnlock16 kernel32.K32WOWGlobalUnlock16
@ forward WOWGlobalUnlockFree16 kernel32.K32WOWGlobalUnlockFree16
@ forward WOWHandle16 kernel32.K32WOWHandle16
@ forward WOWHandle32 kernel32.K32WOWHandle32
@ forward WOWYield16 kernel32.K32WOWYield16
