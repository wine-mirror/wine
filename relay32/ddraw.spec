name ddraw
type win32

   1  stub DDHAL32_VidMemAlloc
   2  stub DDHAL32_VidMemFree
   3  stub DDInternalLock
   4  stub DDInternalUnlock
   5  stdcall DSoundHelp(long long long) DSoundHelp
   6  stdcall DirectDrawCreate(ptr ptr ptr) DirectDrawCreate
   7  stub DirectDrawCreateClipper
   8  stdcall DirectDrawEnumerateA(ptr ptr) DirectDrawEnumerate32A
   9  stub DirectDrawEnumerateW
  10  stub DllCanUnloadNow
  11  stub DllGetClassObject
  12  stub GetNextMipMap
  13  stub GetSurfaceFromDC
  14  stub HeapVidMemAllocAligned
  15  stub InternalLock
  16  stub InternalUnlock
  17  stub LateAllocateSurfaceMem
  18  stub VidMemAlloc
  19  stub VidMemAmountFree
  20  stub VidMemFini
  21  stub VidMemFree
  22  stub VidMemInit
  23  stub VidMemLargestFree
  24  stub thk1632_ThunkData32
  25  stub thk3216_ThunkData32
