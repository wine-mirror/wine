/*
 *	IMAGEHLP library
 *
 *	Copyright 1998	Patrik Stridvall
 */

#include "winbase.h"
#include "winerror.h"
#include "windef.h"
#include "debug.h"
#include "imagehlp.h"

DEFAULT_DEBUG_CHANNEL(imagehlp)

/***********************************************************************
 *           BindImage32 (IMAGEHLP.1)
 */
BOOL WINAPI BindImage(
  LPSTR ImageName, LPSTR DllPath, LPSTR SymbolPath)
{
  return BindImageEx(0, ImageName, DllPath, SymbolPath, NULL);
}

/***********************************************************************
 *           BindImageEx32 (IMAGEHLP.2)
 */
BOOL WINAPI BindImageEx(
  DWORD Flags, LPSTR ImageName, LPSTR DllPath, LPSTR SymbolPath,
  PIMAGEHLP_STATUS_ROUTINE StatusRoutine)
{
  FIXME(imagehlp, "(%ld, %s, %s, %s, %p): stub\n", 
    Flags, debugstr_a(ImageName), debugstr_a(DllPath),
    debugstr_a(SymbolPath), StatusRoutine
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           CheckSumMappedFile32 (IMAGEHLP.3)
 */
PIMAGE_NT_HEADERS WINAPI CheckSumMappedFile(
  LPVOID BaseAddress, DWORD FileLength, 
  LPDWORD HeaderSum, LPDWORD CheckSum)
{
  FIXME(imagehlp, "(%p, %ld, %p, %p): stub\n",
    BaseAddress, FileLength, HeaderSum, CheckSum
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return NULL;
}

/***********************************************************************
 *           MapFileAndCheckSum32A (IMAGEHLP.27)
 */
DWORD WINAPI MapFileAndCheckSumA(
  LPSTR Filename, LPDWORD HeaderSum, LPDWORD CheckSum)
{
  FIXME(imagehlp, "(%s, %p, %p): stub\n",
    debugstr_a(Filename), HeaderSum, CheckSum
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return CHECKSUM_OPEN_FAILURE;
}

/***********************************************************************
 *           MapFileAndCheckSum32W (IMAGEHLP.28)
 */
DWORD WINAPI MapFileAndCheckSumW(
  LPWSTR Filename, LPDWORD HeaderSum, LPDWORD CheckSum)
{
  FIXME(imagehlp, "(%s, %p, %p): stub\n",
    debugstr_w(Filename), HeaderSum, CheckSum
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return CHECKSUM_OPEN_FAILURE;
}

/***********************************************************************
 *           ReBaseImage32 (IMAGEHLP.30)
 */
BOOL WINAPI ReBaseImage(
  LPSTR CurrentImageName, LPSTR SymbolPath, BOOL fReBase,
  BOOL fRebaseSysfileOk, BOOL fGoingDown, ULONG CheckImageSize,
  ULONG *OldImageSize, ULONG *OldImageBase, ULONG *NewImageSize,
  ULONG *NewImageBase, ULONG TimeStamp)
{
  FIXME(imagehlp,
    "(%s, %s, %d, %d, %d, %ld, %p, %p, %p, %p, %ld): stub\n",
      debugstr_a(CurrentImageName),debugstr_a(SymbolPath), fReBase, 
      fRebaseSysfileOk, fGoingDown, CheckImageSize, OldImageSize, 
      OldImageBase, NewImageSize, NewImageBase, TimeStamp
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           RemovePrivateCvSymbolic32 (IMAGEHLP.31)
 */
BOOL WINAPI RemovePrivateCvSymbolic(
  PCHAR DebugData, PCHAR *NewDebugData, ULONG *NewDebugSize)
{
  FIXME(imagehlp, "(%p, %p, %p): stub\n",
    DebugData, NewDebugData, NewDebugSize
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           RemoveRelocations32 (IMAGEHLP.32)
 */
VOID WINAPI RemoveRelocations(PCHAR ImageName)
{
  FIXME(imagehlp, "(%p): stub\n", ImageName);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/***********************************************************************
 *           SplitSymbols32 (IMAGEHLP.35)
 */
BOOL WINAPI SplitSymbols(
  LPSTR ImageName, LPSTR SymbolsPath, 
  LPSTR SymbolFilePath, DWORD Flags)
{
  FIXME(imagehlp, "(%s, %s, %s, %ld): stub\n",
    debugstr_a(ImageName), debugstr_a(SymbolsPath),
    debugstr_a(SymbolFilePath), Flags
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           UpdateDebugInfoFile32 (IMAGEHLP.60)
 */
BOOL WINAPI UpdateDebugInfoFile(
  LPSTR ImageFileName, LPSTR SymbolPath,
  LPSTR DebugFilePath, PIMAGE_NT_HEADERS NtHeaders)
{
  FIXME(imagehlp, "(%s, %s, %s, %p): stub\n",
    debugstr_a(ImageFileName), debugstr_a(SymbolPath),
    debugstr_a(DebugFilePath), NtHeaders
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           UpdateDebugInfoFileEx32 (IMAGEHLP.?)
 * FIXME
 *   Function has no ordinal.
 */
BOOL WINAPI UpdateDebugInfoFileEx(
  LPSTR ImageFileName, LPSTR SymbolPath, LPSTR DebugFilePath,
  PIMAGE_NT_HEADERS NtHeaders, DWORD OldChecksum)
{
  FIXME(imagehlp, "(%s, %s, %s, %p, %ld): stub\n",
    debugstr_a(ImageFileName), debugstr_a(SymbolPath),
    debugstr_a(DebugFilePath), NtHeaders, OldChecksum
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}
