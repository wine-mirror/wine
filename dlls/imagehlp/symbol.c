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
 *           SymCleanup32 (IMAGEHLP.37)
 */
BOOL32 WINAPI SymCleanup32(HANDLE32 hProcess)
{
  FIXME(imagehlp, "(0x%08x): stub\n", hProcess);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           SymEnumerateModules32 (IMAGEHLP.38)
 */

BOOL32 WINAPI SymEnumerateModules32(
  HANDLE32 hProcess, PSYM_ENUMMODULES_CALLBACK32 EnumModulesCallback,
  PVOID UserContext)
{
  FIXME(imagehlp, "(0x%08x, %p, %p): stub\n",
    hProcess, EnumModulesCallback, UserContext
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           SymEnumerateSymbols32 (IMAGEHLP.39)
 */
BOOL32 WINAPI SymEnumerateSymbols32(
  HANDLE32 hProcess, DWORD BaseOfDll,
  PSYM_ENUMSYMBOLS_CALLBACK32 EnumSymbolsCallback, PVOID UserContext)
{
  FIXME(imagehlp, "(0x%08x, %p, %p): stub\n",
    hProcess, EnumSymbolsCallback, UserContext
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           SymFunctionTableAccess32 (IMAGEHLP.40)
 */
PVOID WINAPI SymFunctionTableAccess32(HANDLE32 hProcess, DWORD AddrBase)
{
  FIXME(imagehlp, "(0x%08x, 0x%08lx): stub\n", hProcess, AddrBase);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           SymGetModuleBase32 (IMAGEHLP.41)
 */
DWORD WINAPI SymGetModuleBase32(HANDLE32 hProcess, DWORD dwAddr)
{
  FIXME(imagehlp, "(0x%08x, 0x%08lx): stub\n", hProcess, dwAddr);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           SymGetModuleInfo32 (IMAGEHLP.42)
 */
BOOL32 WINAPI SymGetModuleInfo32(
  HANDLE32 hProcess, DWORD dwAddr,
  PIMAGEHLP_MODULE32 ModuleInfo)
{
  FIXME(imagehlp, "(0x%08x, 0x%08lx, %p): stub\n",
    hProcess, dwAddr, ModuleInfo
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           SymGetOptions32 (IMAGEHLP.43)
 */
DWORD WINAPI SymGetOptions32()
{
  FIXME(imagehlp, "(): stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           SymGetSearchPath32 (IMAGEHLP.44)
 */
BOOL32 WINAPI SymGetSearchPath32(
  HANDLE32 hProcess, LPSTR szSearchPath, DWORD SearchPathLength)
{
  FIXME(imagehlp, "(0x%08x, %s, %ld): stub\n",
    hProcess, debugstr_an(szSearchPath,SearchPathLength), SearchPathLength
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           SymGetSymFromAddr32 (IMAGEHLP.45)
 */
BOOL32 WINAPI SymGetSymFromAddr32(
  HANDLE32 hProcess, DWORD dwAddr, 
  PDWORD pdwDisplacement, PIMAGEHLP_SYMBOL32 Symbol)
{
  FIXME(imagehlp, "(0x%08x, 0x%08lx, %p, %p): stub\n",
    hProcess, dwAddr, pdwDisplacement, Symbol
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           SymGetSymFromName32 (IMAGEHLP.46)
 */
BOOL32 WINAPI SymGetSymFromName32(
  HANDLE32 hProcess, LPSTR Name, PIMAGEHLP_SYMBOL32 Symbol)
{
  FIXME(imagehlp, "(0x%08x, %s, %p): stub\n", hProcess, Name, Symbol);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           SymGetSymNext32 (IMAGEHLP.47)
 */
BOOL32 WINAPI SymGetSymNext32(
  HANDLE32 hProcess, PIMAGEHLP_SYMBOL32 Symbol)
{
  FIXME(imagehlp, "(0x%08x, %p): stub\n", hProcess, Symbol);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           SymGetSymPrev32 (IMAGEHLP.48)
 */

BOOL32 WINAPI SymGetSymPrev32(
  HANDLE32 hProcess, PIMAGEHLP_SYMBOL32 Symbol)
{
  FIXME(imagehlp, "(0x%08x, %p): stub\n", hProcess, Symbol);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           SymInitialize32 (IMAGEHLP.49)
 */
BOOL32 WINAPI SymInitialize32(
  HANDLE32 hProcess, LPSTR UserSearchPath, BOOL32 fInvadeProcess)
{
  FIXME(imagehlp, "(0x%08x, %s, %d): stub\n",
    hProcess, debugstr_a(UserSearchPath), fInvadeProcess
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           SymLoadModule32 (IMAGEHLP.50)
 */

BOOL32 WINAPI SymLoadModule32(
  HANDLE32 hProcess, HANDLE32 hFile, LPSTR ImageName, LPSTR ModuleName,
  DWORD BaseOfDll, DWORD SizeOfDll)
{
  FIXME(imagehlp, "(0x%08x, 0x%08x, %s, %s, %ld, %ld): stub\n",
    hProcess, hFile, debugstr_a(ImageName), debugstr_a(ModuleName),
    BaseOfDll, SizeOfDll
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           SymRegisterCallback32 (IMAGEHLP.51)
 */
BOOL32 WINAPI SymRegisterCallback32(
  HANDLE32 hProcess, PSYMBOL_REGISTERED_CALLBACK32 CallbackFunction,
  PVOID UserContext)
{
  FIXME(imagehlp, "(0x%08x, %p, %p): stub\n",
    hProcess, CallbackFunction, UserContext
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           SymSetOptions32 (IMAGEHLP.52)
 */
DWORD WINAPI SymSetOptions32(DWORD SymOptions)
{
  FIXME(imagehlp, "(%lx): stub\n", SymOptions);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           SymSetSearchPath32 (IMAGEHLP.53)
 */
BOOL32 WINAPI SymSetSearchPath32(HANDLE32 hProcess, LPSTR szSearchPath)
{
  FIXME(imagehlp, "(0x%08x, %s): stub\n",
    hProcess, debugstr_a(szSearchPath)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           SymUnDName32 (IMAGEHLP.54)
 */
BOOL32 WINAPI SymUnDName32(
  PIMAGEHLP_SYMBOL32 sym, LPSTR UnDecName, DWORD UnDecNameLength)
{
  FIXME(imagehlp, "(%p, %s, %ld): stub\n",
    sym, UnDecName, UnDecNameLength
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           SymUnloadModule32 (IMAGEHLP.55)
 */
BOOL32 WINAPI SymUnloadModule32(
  HANDLE32 hProcess, DWORD BaseOfDll)
{
  FIXME(imagehlp, "(0x%08x, 0x%08lx): stub\n", hProcess, BaseOfDll);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}
