/*
 *	IMAGEHLP library
 *
 *	Copyright 1998	Patrik Stridvall
 */

#include "winerror.h"
#include "winbase.h"
#include "wintypes.h"
#include "debug.h"
#include "imagehlp.h"

/***********************************************************************
 *           FindDebugInfoFile32 (IMAGEHLP.5)
 */
HANDLE32 WINAPI FindDebugInfoFile32(
  LPSTR FileName, LPSTR SymbolPath, LPSTR DebugFilePath)
{
  FIXME(imagehlp, "(%s, %s, %s): stub\n",
    debugstr_a(FileName), debugstr_a(SymbolPath),
    debugstr_a(DebugFilePath)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return (HANDLE32) NULL;
}

/***********************************************************************
 *           FindExecutableImage32 (IMAGEHLP.6)
 */
HANDLE32 WINAPI FindExecutableImage32(
  LPSTR FileName, LPSTR SymbolPath, LPSTR ImageFilePath)
{
  FIXME(imagehlp, "(%s, %s, %s): stub\n",
    debugstr_a(FileName), debugstr_a(SymbolPath),
    debugstr_a(ImageFilePath)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return (HANDLE32) NULL;
}

/***********************************************************************
 *           MapDebugInformation32 (IMAGEHLP.26)
 */
PIMAGE_DEBUG_INFORMATION32 WINAPI MapDebugInformation32(
  HANDLE32 FileHandle, LPSTR FileName,
  LPSTR SymbolPath, DWORD ImageBase)
{
  FIXME(imagehlp, "(0x%08x, %s, %s, 0x%08lx): stub\n",
    FileHandle, FileName, SymbolPath, ImageBase
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return NULL;
}

/***********************************************************************
 *           StackWalk32 (IMAGEHLP.36)
 */
BOOL32 WINAPI StackWalk32(
  DWORD MachineType, HANDLE32 hProcess, HANDLE32 hThread,
  PSTACKFRAME32 StackFrame, PVOID ContextRecord,
  PREAD_PROCESS_MEMORY_ROUTINE32 ReadMemoryRoutine,
  PFUNCTION_TABLE_ACCESS_ROUTINE32 FunctionTableAccessRoutine,
  PGET_MODULE_BASE_ROUTINE32 GetModuleBaseRoutine,
  PTRANSLATE_ADDRESS_ROUTINE32 TranslateAddress)
{
  FIXME(imagehlp,
    "(%ld, 0x%08x, 0x%08x, %p, %p, %p, %p, %p, %p): stub\n",
      MachineType, hProcess, hThread, StackFrame, ContextRecord,
      ReadMemoryRoutine, FunctionTableAccessRoutine,
      GetModuleBaseRoutine, TranslateAddress
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           UnDecorateSymbolName32 (IMAGEHLP.57)
 */
DWORD WINAPI UnDecorateSymbolName32(
  LPCSTR DecoratedName, LPSTR UnDecoratedName,
  DWORD UndecoratedLength, DWORD Flags)
{
  FIXME(imagehlp, "(%s, %s, %ld, 0x%08lx): stub\n",
    debugstr_a(DecoratedName), debugstr_a(UnDecoratedName),
    UndecoratedLength, Flags
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           UnmapDebugInformation32 (IMAGEHLP.59)
 */

BOOL32 WINAPI UnmapDebugInformation32(
  PIMAGE_DEBUG_INFORMATION32 DebugInfo)
{
  FIXME(imagehlp, "(%p): stub\n", DebugInfo);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}
