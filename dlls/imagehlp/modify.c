/*
 *	IMAGEHLP library
 *
 *	Copyright 1998	Patrik Stridvall
 */

#include "windows.h"
#include "winerror.h"
#include "wintypes.h"
#include "debug.h"
#include "imagehlp.h"

/***********************************************************************
 *           BindImage32 (IMAGEHLP.1)
 */
BOOL32 WINAPI BindImage32(
  LPSTR ImageName, LPSTR DllPath, LPSTR SymbolPath)
{
  return BindImageEx32(0, ImageName, DllPath, SymbolPath, NULL);
}

/***********************************************************************
 *           BindImageEx32 (IMAGEHLP.2)
 */
BOOL32 WINAPI BindImageEx32(
  DWORD Flags, LPSTR ImageName, LPSTR DllPath, LPSTR SymbolPath,
  PIMAGEHLP_STATUS_ROUTINE32 StatusRoutine)
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
PIMAGE_NT_HEADERS32 WINAPI CheckSumMappedFile32(
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
DWORD WINAPI MapFileAndCheckSum32A(
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
DWORD WINAPI MapFileAndCheckSum32W(
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
BOOL32 WINAPI ReBaseImage32(
  LPSTR CurrentImageName, LPSTR SymbolPath, BOOL32 fReBase,
  BOOL32 fRebaseSysfileOk, BOOL32 fGoingDown, ULONG CheckImageSize,
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
BOOL32 WINAPI RemovePrivateCvSymbolic32(
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
VOID WINAPI RemoveRelocations32(PCHAR ImageName)
{
  FIXME(imagehlp, "(%p): stub\n", ImageName);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/***********************************************************************
 *           SplitSymbols32 (IMAGEHLP.35)
 */
BOOL32 WINAPI SplitSymbols32(
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
BOOL32 WINAPI UpdateDebugInfoFile32(
  LPSTR ImageFileName, LPSTR SymbolPath,
  LPSTR DebugFilePath, PIMAGE_NT_HEADERS32 NtHeaders)
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
BOOL32 WINAPI UpdateDebugInfoFileEx32(
  LPSTR ImageFileName, LPSTR SymbolPath, LPSTR DebugFilePath,
  PIMAGE_NT_HEADERS32 NtHeaders, DWORD OldChecksum)
{
  FIXME(imagehlp, "(%s, %s, %s, %p, %ld): stub\n",
    debugstr_a(ImageFileName), debugstr_a(SymbolPath),
    debugstr_a(DebugFilePath), NtHeaders, OldChecksum
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}
