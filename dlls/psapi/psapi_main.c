/*
 *      PSAPI library
 *
 *      Copyright 1998  Patrik Stridvall
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "winbase.h"
#include "windef.h"
#include "winerror.h"
#include "wine/server.h"
#include "wine/debug.h"
#include "tlhelp32.h"
#include "psapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(psapi);

#include <string.h>


/***********************************************************************
 *	get pid from hProcess (internal)
 */
static DWORD get_pid_from_process_handle(HANDLE hProcess)
{
	DWORD ret = 0;

	SERVER_START_REQ( get_process_info )
	{
		req->handle = hProcess;
		if ( !wine_server_call_err( req ) )
			ret = (DWORD)reply->pid;
	}
	SERVER_END_REQ;

	return ret;
}

/***********************************************************************
 *           EmptyWorkingSet (PSAPI.@)
 */
BOOL WINAPI EmptyWorkingSet(HANDLE hProcess)
{
  return SetProcessWorkingSetSize(hProcess, 0xFFFFFFFF, 0xFFFFFFFF);
}

/***********************************************************************
 *           EnumDeviceDrivers (PSAPI.@)
 */
BOOL WINAPI EnumDeviceDrivers(
  LPVOID *lpImageBase, DWORD cb, LPDWORD lpcbNeeded)
{
  FIXME("(%p, %ld, %p): stub\n", lpImageBase, cb, lpcbNeeded);

  if(lpcbNeeded)
    *lpcbNeeded = 0;

  return TRUE;
}


/***********************************************************************
 *           EnumProcesses (PSAPI.@)
 */
BOOL WINAPI EnumProcesses(DWORD *lpidProcess, DWORD cb, DWORD *lpcbNeeded)
{
	PROCESSENTRY32	pe;
	HANDLE	hSnapshot;
	BOOL	res;
	DWORD	count;
	DWORD	countMax;

	FIXME("(%p, %ld, %p)\n", lpidProcess,cb, lpcbNeeded);

	if ( lpidProcess == NULL )
		cb = 0;
	if ( lpcbNeeded != NULL )
		*lpcbNeeded = 0;

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
	if ( hSnapshot == INVALID_HANDLE_VALUE )
	{
		FIXME("cannot create snapshot\n");
		return FALSE;
	}
	count = 0;
	countMax = cb / sizeof(DWORD);
	while (1)
	{
		ZeroMemory( &pe, sizeof(PROCESSENTRY32) );
		pe.dwSize = sizeof(PROCESSENTRY32);
		res = (count == 0) ? Process32First( hSnapshot, &pe ) : Process32Next( hSnapshot, &pe );
		if ( !res )
			break;
		TRACE("process 0x%08lx\n",(long)pe.th32ProcessID);
		if ( count < countMax )
			lpidProcess[count] = pe.th32ProcessID;
		count ++;
	}
	CloseHandle( hSnapshot );

	if ( lpcbNeeded != NULL )
		*lpcbNeeded = sizeof(DWORD) * count;

	TRACE("return %lu processes\n",count);

	return TRUE;
}

/***********************************************************************
 *           EnumProcessModules (PSAPI.@)
 */
BOOL WINAPI EnumProcessModules(
  HANDLE hProcess, HMODULE *lphModule, DWORD cb, LPDWORD lpcbNeeded)
{
	MODULEENTRY32	me;
	HANDLE	hSnapshot;
	BOOL	res;
	DWORD	pid;
	DWORD	count;
	DWORD	countMax;

	FIXME("(hProcess=%p, %p, %ld, %p)\n",
		hProcess, lphModule, cb, lpcbNeeded );

	if ( lphModule == NULL )
		cb = 0;
	if ( lpcbNeeded != NULL )
		*lpcbNeeded = 0;

	pid = get_pid_from_process_handle(hProcess);
	if ( pid == 0 )
	{
		FIXME("no pid for hProcess %p\n",hProcess);
		return FALSE;
	}

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,pid);
	if ( hSnapshot == INVALID_HANDLE_VALUE )
	{
		FIXME("cannot create snapshot\n");
		return FALSE;
	}
	count = 0;
	countMax = cb / sizeof(HMODULE);
	while (1)
	{
		ZeroMemory( &me, sizeof(MODULEENTRY32) );
		me.dwSize = sizeof(MODULEENTRY32);
		res = (count == 0) ? Module32First( hSnapshot, &me ) : Module32Next( hSnapshot, &me );
		if ( !res )
			break;
		TRACE("module 0x%08lx\n",(long)me.hModule);
		if ( count < countMax )
			lphModule[count] = me.hModule;
		count ++;
	}
	CloseHandle( hSnapshot );

	if ( lpcbNeeded != NULL )
		*lpcbNeeded = sizeof(HMODULE) * count;

	TRACE("return %lu modules\n",count);

	return TRUE;
}

/***********************************************************************
 *          GetDeviceDriverBaseNameA (PSAPI.@)
 */
DWORD WINAPI GetDeviceDriverBaseNameA(
  LPVOID ImageBase, LPSTR lpBaseName, DWORD nSize)
{
  FIXME("(%p, %s, %ld): stub\n",
    ImageBase, debugstr_a(lpBaseName), nSize
  );

  if(lpBaseName && nSize)
    lpBaseName[0] = '\0';

  return 0;
}

/***********************************************************************
 *           GetDeviceDriverBaseNameW (PSAPI.@)
 */
DWORD WINAPI GetDeviceDriverBaseNameW(
  LPVOID ImageBase, LPWSTR lpBaseName, DWORD nSize)
{
  FIXME("(%p, %s, %ld): stub\n",
    ImageBase, debugstr_w(lpBaseName), nSize
  );

  if(lpBaseName && nSize)
    lpBaseName[0] = '\0';

  return 0;
}

/***********************************************************************
 *           GetDeviceDriverFileNameA (PSAPI.@)
 */
DWORD WINAPI GetDeviceDriverFileNameA(
  LPVOID ImageBase, LPSTR lpFilename, DWORD nSize)
{
  FIXME("(%p, %s, %ld): stub\n",
    ImageBase, debugstr_a(lpFilename), nSize
  );

  if(lpFilename && nSize)
    lpFilename[0] = '\0';

  return 0;
}

/***********************************************************************
 *           GetDeviceDriverFileNameW (PSAPI.@)
 */
DWORD WINAPI GetDeviceDriverFileNameW(
  LPVOID ImageBase, LPWSTR lpFilename, DWORD nSize)
{
  FIXME("(%p, %s, %ld): stub\n",
    ImageBase, debugstr_w(lpFilename), nSize
  );

  if(lpFilename && nSize)
    lpFilename[0] = '\0';

  return 0;
}

/***********************************************************************
 *           GetMappedFileNameA (PSAPI.@)
 */
DWORD WINAPI GetMappedFileNameA(
  HANDLE hProcess, LPVOID lpv, LPSTR lpFilename, DWORD nSize)
{
  FIXME("(hProcess=%p, %p, %s, %ld): stub\n",
    hProcess, lpv, debugstr_a(lpFilename), nSize
  );

  if(lpFilename && nSize)
    lpFilename[0] = '\0';

  return 0;
}

/***********************************************************************
 *           GetMappedFileNameW (PSAPI.@)
 */
DWORD WINAPI GetMappedFileNameW(
  HANDLE hProcess, LPVOID lpv, LPWSTR lpFilename, DWORD nSize)
{
  FIXME("(hProcess=%p, %p, %s, %ld): stub\n",
    hProcess, lpv, debugstr_w(lpFilename), nSize
  );

  if(lpFilename && nSize)
    lpFilename[0] = '\0';

  return 0;
}

/***********************************************************************
 *           GetModuleBaseNameA (PSAPI.@)
 */
DWORD WINAPI GetModuleBaseNameA(
  HANDLE hProcess, HMODULE hModule, LPSTR lpBaseName, DWORD nSize)
{
  FIXME("(hProcess=%p, hModule=%p, %s, %ld): stub\n",
    hProcess, hModule, debugstr_a(lpBaseName), nSize
  );

  if(lpBaseName && nSize)
    lpBaseName[0] = '\0';

  return 0;
}

/***********************************************************************
 *           GetModuleBaseNameW (PSAPI.@)
 */
DWORD WINAPI GetModuleBaseNameW(
  HANDLE hProcess, HMODULE hModule, LPWSTR lpBaseName, DWORD nSize)
{
  FIXME("(hProcess=%p, hModule=%p, %s, %ld): stub\n",
    hProcess, hModule, debugstr_w(lpBaseName), nSize);

  if(lpBaseName && nSize)
    lpBaseName[0] = '\0';

  return 0;
}

/***********************************************************************
 *           GetModuleFileNameExA (PSAPI.@)
 */
DWORD WINAPI GetModuleFileNameExA(
  HANDLE hProcess, HMODULE hModule, LPSTR lpFilename, DWORD nSize)
{
  FIXME("(hProcess=%p,hModule=%p, %s, %ld): stub\n",
    hProcess, hModule, debugstr_a(lpFilename), nSize
  );

	if ( get_pid_from_process_handle(hProcess) == GetCurrentProcessId() )
		return GetModuleFileNameA( hModule, lpFilename, nSize );

  if(lpFilename&&nSize)
    lpFilename[0]='\0';

  return 0;
}

/***********************************************************************
 *           GetModuleFileNameExW (PSAPI.@)
 */
DWORD WINAPI GetModuleFileNameExW(
  HANDLE hProcess, HMODULE hModule, LPWSTR lpFilename, DWORD nSize)
{
  FIXME("(hProcess=%p,hModule=%p, %s, %ld): stub\n",
    hProcess, hModule, debugstr_w(lpFilename), nSize
  );

	if ( get_pid_from_process_handle(hProcess) == GetCurrentProcessId() )
		return GetModuleFileNameW( hModule, lpFilename, nSize );

  if(lpFilename && nSize)
    lpFilename[0] = '\0';

  return 0;
}

/***********************************************************************
 *           GetModuleInformation (PSAPI.@)
 */
BOOL WINAPI GetModuleInformation(
  HANDLE hProcess, HMODULE hModule, LPMODULEINFO lpmodinfo, DWORD cb)
{
  FIXME("(hProcess=%p, hModule=%p, %p, %ld): stub\n",
    hProcess, hModule, lpmodinfo, cb
  );

  memset(lpmodinfo, 0, cb);

  return TRUE;
}

/***********************************************************************
 *           GetProcessMemoryInfo (PSAPI.@)
 */
BOOL WINAPI GetProcessMemoryInfo(
  HANDLE Process, PPROCESS_MEMORY_COUNTERS ppsmemCounters, DWORD cb)
{
  FIXME("(hProcess=%p, %p, %ld): stub\n",
    Process, ppsmemCounters, cb
  );

  memset(ppsmemCounters, 0, cb);

  return TRUE;
}

/***********************************************************************
 *           GetWsChanges (PSAPI.@)
 */
BOOL WINAPI GetWsChanges(
  HANDLE hProcess, PPSAPI_WS_WATCH_INFORMATION lpWatchInfo, DWORD cb)
{
  FIXME("(hProcess=%p, %p, %ld): stub\n",
    hProcess, lpWatchInfo, cb
  );

  memset(lpWatchInfo, 0, cb);

  return TRUE;
}

/***********************************************************************
 *           InitializeProcessForWsWatch (PSAPI.@)
 */
BOOL WINAPI InitializeProcessForWsWatch(HANDLE hProcess)
{
  FIXME("(hProcess=%p): stub\n", hProcess);

  return TRUE;
}

/***********************************************************************
 *           QueryWorkingSet (PSAPI.?)
 * FIXME
 *     I haven't been able to find the ordinal for this function,
 *     This means it can't be called from outside the DLL.
 */
BOOL WINAPI QueryWorkingSet(HANDLE hProcess, LPVOID pv, DWORD cb)
{
  FIXME("(hProcess=%p, %p, %ld)\n", hProcess, pv, cb);

  if(pv && cb)
    ((DWORD *) pv)[0] = 0; /* Empty WorkingSet */

  return TRUE;
}
