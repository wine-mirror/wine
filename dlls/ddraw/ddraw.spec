name ddraw
type win32

import kernel32.dll
import user32.dll
import ole32.dll
import x11drv.dll

@ stub DDHAL32_VidMemAlloc
@ stub DDHAL32_VidMemFree
@ stub DDInternalLock
@ stub DDInternalUnlock
@ stub DSoundHelp
@ stdcall DirectDrawCreate(ptr ptr ptr) DirectDrawCreate
@ stdcall DirectDrawCreateClipper(long ptr ptr) DirectDrawCreateClipper
@ stdcall DirectDrawEnumerateA(ptr ptr) DirectDrawEnumerateA
@ stdcall DirectDrawEnumerateExA(ptr ptr long) DirectDrawEnumerateExA
@ stdcall DirectDrawEnumerateExW(ptr ptr long) DirectDrawEnumerateExW
@ stdcall DirectDrawEnumerateW(ptr ptr) DirectDrawEnumerateW
@ stdcall DllCanUnloadNow() DDRAW_DllCanUnloadNow
@ stdcall DllGetClassObject(ptr ptr ptr) DDRAW_DllGetClassObject
@ stub GetNextMipMap
@ stub GetSurfaceFromDC
@ stub HeapVidMemAllocAligned
@ stub InternalLock
@ stub InternalUnlock
@ stub LateAllocateSurfaceMem
@ stub VidMemAlloc
@ stub VidMemAmountFree
@ stub VidMemFini
@ stub VidMemFree
@ stub VidMemInit
@ stub VidMemLargestFree
@ stub thk1632_ThunkData32
@ stub thk3216_ThunkData32
