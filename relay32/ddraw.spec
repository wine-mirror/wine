name ddraw
type win32

   1  stub DDHAL32_VidMemAlloc
   2  stub DDHAL32_VidMemFree
   3  stub DDInternalLock
   4  stub DDInternalUnlock
   5  stdcall DSoundHelp(long long long) DSoundHelp
   6  stdcall DirectDrawCreate(ptr ptr ptr) DirectDrawCreate
   7  stdcall DirectDrawCreateClipper(long ptr ptr) DirectDrawCreateClipper
   8  stdcall DirectDrawEnumerateA(ptr ptr) DirectDrawEnumerateA
   9  stdcall DirectDrawEnumerateExA(ptr ptr long) DirectDrawEnumerateExA
  10  stdcall DirectDrawEnumerateExW(ptr ptr long) DirectDrawEnumerateExW
  11  stdcall DirectDrawEnumerateW(ptr ptr) DirectDrawEnumerateW
  12  stdcall DllCanUnloadNow() DDRAW_DllCanUnloadNow
  13  stdcall DllGetClassObject(ptr ptr ptr) DDRAW_DllGetClassObject
  14  stub GetNextMipMap
  15  stub GetSurfaceFromDC
  16  stub HeapVidMemAllocAligned
  17  stub InternalLock
  18  stub InternalUnlock
  19  stub LateAllocateSurfaceMem
  20  stub VidMemAlloc
  21  stub VidMemAmountFree
  22  stub VidMemFini
  23  stub VidMemFree
  24  stub VidMemInit
  25  stub VidMemLargestFree
  26  stub thk1632_ThunkData32
  27  stub thk3216_ThunkData32
