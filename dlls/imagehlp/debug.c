/*
 *	IMAGEHLP library
 *
 *	Copyright 1998	Patrik Stridvall
 */

#include "winerror.h"
#include "winbase.h"
#include "windef.h"
#include "debugtools.h"
#include "imagehlp.h"

DEFAULT_DEBUG_CHANNEL(imagehlp);

/***********************************************************************
 *		FindDebugInfoFile (IMAGEHLP.@)
 */
HANDLE WINAPI FindDebugInfoFile(
  LPSTR FileName, LPSTR SymbolPath, LPSTR DebugFilePath)
{
  FIXME("(%s, %s, %s): stub\n",
    debugstr_a(FileName), debugstr_a(SymbolPath),
    debugstr_a(DebugFilePath)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return (HANDLE) NULL;
}

/***********************************************************************
 *		FindExecutableImage (IMAGEHLP.@)
 */
HANDLE WINAPI FindExecutableImage(
  LPSTR FileName, LPSTR SymbolPath, LPSTR ImageFilePath)
{
  FIXME("(%s, %s, %s): stub\n",
    debugstr_a(FileName), debugstr_a(SymbolPath),
    debugstr_a(ImageFilePath)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return (HANDLE) NULL;
}

/***********************************************************************
 *		MapDebugInformation (IMAGEHLP.@)
 */
PIMAGE_DEBUG_INFORMATION WINAPI MapDebugInformation(
  HANDLE FileHandle, LPSTR FileName,
  LPSTR SymbolPath, DWORD ImageBase)
{
  FIXME("(0x%08x, %s, %s, 0x%08lx): stub\n",
    FileHandle, FileName, SymbolPath, ImageBase
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return NULL;
}

/***********************************************************************
 *		StackWalk (IMAGEHLP.@)
 */
BOOL WINAPI StackWalk(
  DWORD MachineType, HANDLE hProcess, HANDLE hThread,
  PSTACKFRAME StackFrame, PVOID ContextRecord,
  PREAD_PROCESS_MEMORY_ROUTINE ReadMemoryRoutine,
  PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
  PGET_MODULE_BASE_ROUTINE GetModuleBaseRoutine,
  PTRANSLATE_ADDRESS_ROUTINE TranslateAddress)
{
  FIXME(
    "(%ld, 0x%08x, 0x%08x, %p, %p, %p, %p, %p, %p): stub\n",
      MachineType, hProcess, hThread, StackFrame, ContextRecord,
      ReadMemoryRoutine, FunctionTableAccessRoutine,
      GetModuleBaseRoutine, TranslateAddress
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		UnDecorateSymbolName (IMAGEHLP.@)
 */
DWORD WINAPI UnDecorateSymbolName(
  LPCSTR DecoratedName, LPSTR UnDecoratedName,
  DWORD UndecoratedLength, DWORD Flags)
{
  FIXME("(%s, %s, %ld, 0x%08lx): stub\n",
    debugstr_a(DecoratedName), debugstr_a(UnDecoratedName),
    UndecoratedLength, Flags
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		UnmapDebugInformation (IMAGEHLP.@)
 */
BOOL WINAPI UnmapDebugInformation(
  PIMAGE_DEBUG_INFORMATION DebugInfo)
{
  FIXME("(%p): stub\n", DebugInfo);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}
