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

DEFAULT_DEBUG_CHANNEL(imagehlp);

/***********************************************************************
 *		SymCleanup (IMAGEHLP.@)
 */
BOOL WINAPI SymCleanup(HANDLE hProcess)
{
  FIXME("(0x%08x): stub\n", hProcess);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		SymEnumerateModules (IMAGEHLP.@)
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
 *		SymEnumerateSymbols (IMAGEHLP.@)
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
 *		SymFunctionTableAccess (IMAGEHLP.@)
 */
PVOID WINAPI SymFunctionTableAccess(HANDLE hProcess, DWORD AddrBase)
{
  FIXME("(0x%08x, 0x%08lx): stub\n", hProcess, AddrBase);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		SymGetModuleBase (IMAGEHLP.@)
 */
DWORD WINAPI SymGetModuleBase(HANDLE hProcess, DWORD dwAddr)
{
  FIXME("(0x%08x, 0x%08lx): stub\n", hProcess, dwAddr);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		SymGetModuleInfo (IMAGEHLP.@)
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
 *		SymGetOptions (IMAGEHLP.@)
 */
DWORD WINAPI SymGetOptions()
{
  FIXME("(): stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		SymGetSearchPath (IMAGEHLP.@)
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
 *		SymGetSymFromAddr (IMAGEHLP.@)
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
 *		SymGetSymFromName (IMAGEHLP.@)
 */
BOOL WINAPI SymGetSymFromName(
  HANDLE hProcess, LPSTR Name, PIMAGEHLP_SYMBOL Symbol)
{
  FIXME("(0x%08x, %s, %p): stub\n", hProcess, Name, Symbol);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		SymGetSymNext (IMAGEHLP.@)
 */
BOOL WINAPI SymGetSymNext(
  HANDLE hProcess, PIMAGEHLP_SYMBOL Symbol)
{
  FIXME("(0x%08x, %p): stub\n", hProcess, Symbol);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		SymGetSymPrev (IMAGEHLP.@)
 */

BOOL WINAPI SymGetSymPrev(
  HANDLE hProcess, PIMAGEHLP_SYMBOL Symbol)
{
  FIXME("(0x%08x, %p): stub\n", hProcess, Symbol);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		SymInitialize (IMAGEHLP.@)
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
 *		SymLoadModule (IMAGEHLP.@)
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
 *		SymRegisterCallback (IMAGEHLP.@)
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
 *		SymSetOptions (IMAGEHLP.@)
 */
DWORD WINAPI SymSetOptions(DWORD SymOptions)
{
  FIXME("(%lx): stub\n", SymOptions);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		SymSetSearchPath (IMAGEHLP.@)
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
 *		SymUnDName (IMAGEHLP.@)
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
 *		SymUnloadModule (IMAGEHLP.@)
 */
BOOL WINAPI SymUnloadModule(
  HANDLE hProcess, DWORD BaseOfDll)
{
  FIXME("(0x%08x, 0x%08lx): stub\n", hProcess, BaseOfDll);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}
