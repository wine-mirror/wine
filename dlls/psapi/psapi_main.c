/*
 *      PSAPI library
 *
 *      Copyright 1998  Patrik Stridvall
 */

#include "winbase.h"
#include "wintypes.h"
#include "winerror.h"
#include "debug.h"
#include "psapi.h"

#include <string.h>

/***********************************************************************
 *           EmptyWorkingSet (PSAPI.1)
 */
BOOL32 WINAPI EmptyWorkingSet32(HANDLE32 hProcess)
{
  return SetProcessWorkingSetSize(hProcess, 0xFFFFFFFF, 0xFFFFFFFF);
}

/***********************************************************************
 *           EnumDeviceDrivers (PSAPI.2)
 */
BOOL32 WINAPI EnumDeviceDrivers(
  LPVOID *lpImageBase, DWORD cb, LPDWORD lpcbNeeded)
{
  FIXME(psapi, "(%p, %ld, %p): stub\n", lpImageBase, cb, lpcbNeeded);

  if(lpcbNeeded)
    *lpcbNeeded = 0;

  return TRUE;
}    


/***********************************************************************
 *           EnumProcesses (PSAPI.3)
 */
BOOL32 WINAPI EnumProcesses(DWORD *lpidProcess, DWORD cb, DWORD *lpcbNeeded)
{
  FIXME(psapi, "(%p, %ld, %p): stub\n", lpidProcess,cb, lpcbNeeded);

  if(lpcbNeeded)
    *lpcbNeeded = 0;

  return TRUE;
}

/***********************************************************************
 *           EnumProcessModules (PSAPI.4)
 */
BOOL32 WINAPI EnumProcessModules(
  HANDLE32 hProcess, HMODULE32 *lphModule, DWORD cb, LPDWORD lpcbNeeded)
{
  FIXME(psapi, "(hProcess=0x%08x, %p, %ld, %p): stub\n",
    hProcess, lphModule, cb, lpcbNeeded
  );

  if(lpcbNeeded)
    *lpcbNeeded = 0;

  return TRUE;
}

/***********************************************************************
 *          GetDeviceDriverBaseName32A (PSAPI.5)
 */
DWORD WINAPI GetDeviceDriverBaseName32A(
  LPVOID ImageBase, LPSTR lpBaseName, DWORD nSize)
{
  FIXME(psapi, "(%p, %s, %ld): stub\n",
    ImageBase, debugstr_a(lpBaseName), nSize
  );

  if(lpBaseName && nSize)
    lpBaseName[0] = '\0';

  return 0;
}

/***********************************************************************
 *           GetDeviceDriverBaseName32W (PSAPI.6)
 */
DWORD WINAPI GetDeviceDriverBaseName32W(
  LPVOID ImageBase, LPWSTR lpBaseName, DWORD nSize)
{
  FIXME(psapi, "(%p, %s, %ld): stub\n",
    ImageBase, debugstr_w(lpBaseName), nSize
  );

  if(lpBaseName && nSize)
    lpBaseName[0] = '\0';

  return 0;
}

/***********************************************************************
 *           GetDeviceDriverFileName32A (PSAPI.7)
 */
DWORD WINAPI GetDeviceDriverFileName32A(
  LPVOID ImageBase, LPSTR lpFilename, DWORD nSize)
{
  FIXME(psapi, "(%p, %s, %ld): stub\n",
    ImageBase, debugstr_a(lpFilename), nSize
  );

  if(lpFilename && nSize)
    lpFilename[0] = '\0';

  return 0;
}

/***********************************************************************
 *           GetDeviceDriverFileName32W (PSAPI.8)
 */
DWORD WINAPI GetDeviceDriverFileName32W(
  LPVOID ImageBase, LPWSTR lpFilename, DWORD nSize)
{
  FIXME(psapi, "(%p, %s, %ld): stub\n",
    ImageBase, debugstr_w(lpFilename), nSize
  );

  if(lpFilename && nSize)
    lpFilename[0] = '\0';

  return 0;
}

/***********************************************************************
 *           GetMappedFileName32A (PSAPI.9)
 */
DWORD WINAPI GetMappedFileName32A(
  HANDLE32 hProcess, LPVOID lpv, LPSTR lpFilename, DWORD nSize)
{
  FIXME(psapi, "(hProcess=0x%08x, %p, %s, %ld): stub\n",
    hProcess, lpv, debugstr_a(lpFilename), nSize
  );

  if(lpFilename && nSize)
    lpFilename[0] = '\0';

  return 0;
}

/***********************************************************************
 *           GetMappedFileName32W (PSAPI.10)
 */
DWORD WINAPI GetMappedFileName32W(
  HANDLE32 hProcess, LPVOID lpv, LPWSTR lpFilename, DWORD nSize)
{
  FIXME(psapi, "(hProcess=0x%08x, %p, %s, %ld): stub\n",
    hProcess, lpv, debugstr_w(lpFilename), nSize
  );

  if(lpFilename && nSize)
    lpFilename[0] = '\0';

  return 0;
}

/***********************************************************************
 *           GetModuleBaseName32A (PSAPI.11)
 */
DWORD WINAPI GetModuleBaseName32A(
  HANDLE32 hProcess, HMODULE32 hModule, LPSTR lpBaseName, DWORD nSize)
{
  FIXME(psapi, "(hProcess=0x%08x, hModule=0x%08x, %s, %ld): stub\n",
    hProcess, hModule, debugstr_a(lpBaseName), nSize
  );

  if(lpBaseName && nSize)
    lpBaseName[0] = '\0';

  return 0;
}

/***********************************************************************
 *           GetModuleBaseName32W (PSAPI.12)
 */
DWORD WINAPI GetModuleBaseName32W(
  HANDLE32 hProcess, HMODULE32 hModule, LPWSTR lpBaseName, DWORD nSize)
{
  FIXME(psapi, "(hProcess=0x%08x, hModule=0x%08x, %s, %ld): stub\n",
    hProcess, hModule, debugstr_w(lpBaseName), nSize);

  if(lpBaseName && nSize)
    lpBaseName[0] = '\0';

  return 0;
}

/***********************************************************************
 *           GetModuleFileNameEx32A (PSAPI.13)
 */
DWORD WINAPI GetModuleFileNameEx32A(
  HANDLE32 hProcess, HMODULE32 hModule, LPSTR lpFilename, DWORD nSize)
{
  FIXME(psapi, "(hProcess=0x%08x,hModule=0x%08x, %s, %ld): stub\n",
    hProcess, hModule, debugstr_a(lpFilename), nSize
  );

  if(lpFilename&&nSize)
    lpFilename[0]='\0';

  return 0;
}

/***********************************************************************
 *           GetModuleFileNameEx32W (PSAPI.14)
 */
DWORD WINAPI GetModuleFileNameEx32W(
  HANDLE32 hProcess, HMODULE32 hModule, LPWSTR lpFilename, DWORD nSize)
{
  FIXME(psapi, "(hProcess=0x%08x,hModule=0x%08x, %s, %ld): stub\n",
    hProcess, hModule, debugstr_w(lpFilename), nSize
  );

  if(lpFilename && nSize)
    lpFilename[0] = '\0';

  return 0;
}

/***********************************************************************
 *           GetModuleInformation32 (PSAPI.15)
 */
BOOL32 WINAPI GetModuleInformation32(
  HANDLE32 hProcess, HMODULE32 hModule, LPMODULEINFO32 lpmodinfo, DWORD cb)
{
  FIXME(psapi, "(hProcess=0x%08x, hModule=0x%08x, %p, %ld): stub\n",
    hProcess, hModule, lpmodinfo, cb
  );

  memset(lpmodinfo, 0, cb);

  return TRUE;
}

/***********************************************************************
 *           GetProcessMemoryInfo32 (PSAPI.16)
 */
BOOL32 WINAPI GetProcessMemoryInfo32(
  HANDLE32 Process, PPROCESS_MEMORY_COUNTERS32 ppsmemCounters, DWORD cb)
{
  FIXME(psapi, "(hProcess=0x%08x, %p, %ld): stub\n",
    Process, ppsmemCounters, cb
  );

  memset(ppsmemCounters, 0, cb);

  return TRUE;
}

/***********************************************************************
 *           GetWsChanges32 (PSAPI.17)
 */
BOOL32 WINAPI GetWsChanges32(
  HANDLE32 hProcess, PPSAPI_WS_WATCH_INFORMATION32 lpWatchInfo, DWORD cb)
{
  FIXME(psapi, "(hProcess=0x%08x, %p, %ld): stub\n",
    hProcess, lpWatchInfo, cb
  );

  memset(lpWatchInfo, 0, cb);

  return TRUE;
}

/***********************************************************************
 *           InitializeProcessForWsWatch32 (PSAPI.18)
 */
BOOL32 WINAPI InitializeProcessForWsWatch32(HANDLE32 hProcess)
{
  FIXME(psapi, "(hProcess=0x%08x): stub\n", hProcess);

  return TRUE;
}

/***********************************************************************
 *           QueryWorkingSet32 (PSAPI.?)
 * FIXME
 *     I haven't been able to find the ordinal for this function,
 *     This means it can't be called from outside the DLL.
 */
BOOL32 WINAPI QueryWorkingSet32(HANDLE32 hProcess, LPVOID pv, DWORD cb)
{
  FIXME(psapi, "(hProcess=0x%08x, %p, %ld)", hProcess, pv, cb);

  if(pv && cb)
    ((DWORD *) pv)[0] = 0; /* Empty WorkingSet */

  return TRUE;
}





