/*
 *	IMAGEHLP library
 *
 *	Copyright 1998	Patrik Stridvall
 */

#include "winerror.h"
#include "winbase.h"
#include "windef.h"
#include "debug.h"
#include "imagehlp.h"

/***********************************************************************
 *           FindDebugInfoFile32 (IMAGEHLP.5)
 */
HANDLE WINAPI FindDebugInfoFile(
  LPSTR FileName, LPSTR SymbolPath, LPSTR DebugFilePath)
{
  FIXME(imagehlp, "(%s, %s, %s): stub\n",
    debugstr_a(FileName), debugstr_a(SymbolPath),
    debugstr_a(DebugFilePath)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return (HANDLE) NULL;
}

/***********************************************************************
 *           FindExecutableImage32 (IMAGEHLP.6)
 */
HANDLE WINAPI FindExecutableImage(
  LPSTR FileName, LPSTR SymbolPath, LPSTR ImageFilePath)
{
  FIXME(imagehlp, "(%s, %s, %s): stub\n",
    debugstr_a(FileName), debugstr_a(SymbolPath),
    debugstr_a(ImageFilePath)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return (HANDLE) NULL;
}

/***********************************************************************
 *           MapDebugInformation32 (IMAGEHLP.26)
 */
PIMAGE_DEBUG_INFORMATION WINAPI MapDebugInformation(
  HANDLE FileHandle, LPSTR FileName,
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
BOOL WINAPI StackWalk(
  DWORD MachineType, HANDLE hProcess, HANDLE hThread,
  PSTACKFRAME StackFrame, PVOID ContextRecord,
  PREAD_PROCESS_MEMORY_ROUTINE ReadMemoryRoutine,
  PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
  PGET_MODULE_BASE_ROUTINE GetModuleBaseRoutine,
  PTRANSLATE_ADDRESS_ROUTINE TranslateAddress)
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
DWORD WINAPI UnDecorateSymbolName(
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

BOOL WINAPI UnmapDebugInformation(
  PIMAGE_DEBUG_INFORMATION DebugInfo)
{
  FIXME(imagehlp, "(%p): stub\n", DebugInfo);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}
