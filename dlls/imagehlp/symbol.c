/*
 *	IMAGEHLP library
 *
 *	Copyright 1998	Patrik Stridvall
 */

#include "winbase.h"
#include "winerror.h"
#include "windef.h"
#include "debugtools.h"
#include "imagehlp.h"

DEFAULT_DEBUG_CHANNEL(imagehlp)

/***********************************************************************
 *           SymCleanup32 (IMAGEHLP.37)
 */
BOOL WINAPI SymCleanup(HANDLE hProcess)
{
  FIXME("(0x%08x): stub\n", hProcess);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           SymEnumerateModules32 (IMAGEHLP.38)
 */

BOOL WINAPI SymEnumerateModules(
  HANDLE hProcess, PSYM_ENUMMODULES_CALLBACK EnumModulesCallback,
  PVOID UserContext)
{
  FIXME("(0x%08x, %p, %p): stub\n",
    hProcess, EnumModulesCallback, UserContext
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           SymEnumerateSymbols32 (IMAGEHLP.39)
 */
BOOL WINAPI SymEnumerateSymbols(
  HANDLE hProcess, DWORD BaseOfDll,
  PSYM_ENUMSYMBOLS_CALLBACK EnumSymbolsCallback, PVOID UserContext)
{
  FIXME("(0x%08x, %p, %p): stub\n",
    hProcess, EnumSymbolsCallback, UserContext
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           SymFunctionTableAccess32 (IMAGEHLP.40)
 */
PVOID WINAPI SymFunctionTableAccess(HANDLE hProcess, DWORD AddrBase)
{
  FIXME("(0x%08x, 0x%08lx): stub\n", hProcess, AddrBase);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           SymGetModuleBase32 (IMAGEHLP.41)
 */
DWORD WINAPI SymGetModuleBase(HANDLE hProcess, DWORD dwAddr)
{
  FIXME("(0x%08x, 0x%08lx): stub\n", hProcess, dwAddr);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           SymGetModuleInfo32 (IMAGEHLP.42)
 */
BOOL WINAPI SymGetModuleInfo(
  HANDLE hProcess, DWORD dwAddr,
  PIMAGEHLP_MODULE ModuleInfo)
{
  FIXME("(0x%08x, 0x%08lx, %p): stub\n",
    hProcess, dwAddr, ModuleInfo
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           SymGetOptions32 (IMAGEHLP.43)
 */
DWORD WINAPI SymGetOptions()
{
  FIXME("(): stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           SymGetSearchPath32 (IMAGEHLP.44)
 */
BOOL WINAPI SymGetSearchPath(
  HANDLE hProcess, LPSTR szSearchPath, DWORD SearchPathLength)
{
  FIXME("(0x%08x, %s, %ld): stub\n",
    hProcess, debugstr_an(szSearchPath,SearchPathLength), SearchPathLength
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           SymGetSymFromAddr32 (IMAGEHLP.45)
 */
BOOL WINAPI SymGetSymFromAddr(
  HANDLE hProcess, DWORD dwAddr, 
  PDWORD pdwDisplacement, PIMAGEHLP_SYMBOL Symbol)
{
  FIXME("(0x%08x, 0x%08lx, %p, %p): stub\n",
    hProcess, dwAddr, pdwDisplacement, Symbol
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           SymGetSymFromName32 (IMAGEHLP.46)
 */
BOOL WINAPI SymGetSymFromName(
  HANDLE hProcess, LPSTR Name, PIMAGEHLP_SYMBOL Symbol)
{
  FIXME("(0x%08x, %s, %p): stub\n", hProcess, Name, Symbol);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           SymGetSymNext32 (IMAGEHLP.47)
 */
BOOL WINAPI SymGetSymNext(
  HANDLE hProcess, PIMAGEHLP_SYMBOL Symbol)
{
  FIXME("(0x%08x, %p): stub\n", hProcess, Symbol);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           SymGetSymPrev32 (IMAGEHLP.48)
 */

BOOL WINAPI SymGetSymPrev(
  HANDLE hProcess, PIMAGEHLP_SYMBOL Symbol)
{
  FIXME("(0x%08x, %p): stub\n", hProcess, Symbol);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           SymInitialize32 (IMAGEHLP.49)
 */
BOOL WINAPI SymInitialize(
  HANDLE hProcess, LPSTR UserSearchPath, BOOL fInvadeProcess)
{
  FIXME("(0x%08x, %s, %d): stub\n",
    hProcess, debugstr_a(UserSearchPath), fInvadeProcess
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           SymLoadModule32 (IMAGEHLP.50)
 */

BOOL WINAPI SymLoadModule(
  HANDLE hProcess, HANDLE hFile, LPSTR ImageName, LPSTR ModuleName,
  DWORD BaseOfDll, DWORD SizeOfDll)
{
  FIXME("(0x%08x, 0x%08x, %s, %s, %ld, %ld): stub\n",
    hProcess, hFile, debugstr_a(ImageName), debugstr_a(ModuleName),
    BaseOfDll, SizeOfDll
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           SymRegisterCallback32 (IMAGEHLP.51)
 */
BOOL WINAPI SymRegisterCallback(
  HANDLE hProcess, PSYMBOL_REGISTERED_CALLBACK CallbackFunction,
  PVOID UserContext)
{
  FIXME("(0x%08x, %p, %p): stub\n",
    hProcess, CallbackFunction, UserContext
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           SymSetOptions32 (IMAGEHLP.52)
 */
DWORD WINAPI SymSetOptions(DWORD SymOptions)
{
  FIXME("(%lx): stub\n", SymOptions);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           SymSetSearchPath32 (IMAGEHLP.53)
 */
BOOL WINAPI SymSetSearchPath(HANDLE hProcess, LPSTR szSearchPath)
{
  FIXME("(0x%08x, %s): stub\n",
    hProcess, debugstr_a(szSearchPath)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           SymUnDName32 (IMAGEHLP.54)
 */
BOOL WINAPI SymUnDName(
  PIMAGEHLP_SYMBOL sym, LPSTR UnDecName, DWORD UnDecNameLength)
{
  FIXME("(%p, %s, %ld): stub\n",
    sym, UnDecName, UnDecNameLength
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           SymUnloadModule32 (IMAGEHLP.55)
 */
BOOL WINAPI SymUnloadModule(
  HANDLE hProcess, DWORD BaseOfDll)
{
  FIXME("(0x%08x, 0x%08lx): stub\n", hProcess, BaseOfDll);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}
