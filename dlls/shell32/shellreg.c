/*
	Shell Registry Access
*/
#include <string.h>
#include <stdio.h>
#include "winerror.h"
#include "winreg.h"
#include "debugtools.h"
#include "winnls.h"
#include "winversion.h"
#include "heap.h"
#include "crtdll.h"

#include "shellapi.h"
#include "shlobj.h"
#include "shell32_main.h"
#include "wine/undocshell.h"

DEFAULT_DEBUG_CHANNEL(shell);

/*************************************************************************
 * SHRegOpenKeyA				[SHELL32.506]
 *
 */
HRESULT WINAPI SHRegOpenKeyA(
	HKEY hKey,
	LPSTR lpSubKey,
	LPHKEY phkResult)
{
	TRACE("(0x%08x, %s, %p)\n", hKey, debugstr_a(lpSubKey), phkResult);
	return RegOpenKeyA(hKey, lpSubKey, phkResult);
}

/*************************************************************************
 * SHRegOpenKeyW				[NT4.0:SHELL32.507]
 *
 */
HRESULT WINAPI SHRegOpenKeyW (
	HKEY hkey,
	LPCWSTR lpszSubKey,
	LPHKEY retkey)
{
	WARN("0x%04x %s %p\n",hkey,debugstr_w(lpszSubKey),retkey);
	return RegOpenKeyW( hkey, lpszSubKey, retkey );
}

/*************************************************************************
 * SHRegQueryValueExA SHQueryValueExA		[SHELL32.509][SHLWAPI.@]
 *
 */
HRESULT WINAPI SHRegQueryValueExA(
	HKEY hkey,
	LPSTR lpValueName,
	LPDWORD lpReserved,
	LPDWORD lpType,
	LPBYTE lpData,
	LPDWORD lpcbData)
{
	TRACE("0x%04x %s %p %p %p %p\n", hkey, lpValueName, lpReserved, lpType, lpData, lpcbData);
	return RegQueryValueExA (hkey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

/*************************************************************************
 * SHRegQueryValueW				[NT4.0:SHELL32.510]
 *
 */
HRESULT WINAPI SHRegQueryValueW(
	HKEY hkey,
	LPWSTR lpszSubKey,
	LPWSTR lpszData,
	LPDWORD lpcbData )
{
	WARN("0x%04x %s %p %p semi-stub\n",
		hkey, debugstr_w(lpszSubKey), lpszData, lpcbData);
	return RegQueryValueW( hkey, lpszSubKey, lpszData, lpcbData );
}

/*************************************************************************
 * SHRegQueryValueExW, SHQueryValueExW	[NT4.0:SHELL32.511][SHLWAPI.@]
 *
 * FIXME 
 *  if the datatype REG_EXPAND_SZ then expand the string and change
 *  *pdwType to REG_SZ. 
 */
HRESULT WINAPI SHRegQueryValueExW (
	HKEY hkey,
	LPWSTR pszValue,
	LPDWORD pdwReserved,
	LPDWORD pdwType,
	LPVOID pvData,
	LPDWORD pcbData)
{
	DWORD ret;
	WARN("0x%04x %s %p %p %p %p semi-stub\n",
		hkey, debugstr_w(pszValue), pdwReserved, pdwType, pvData, pcbData);
	ret = RegQueryValueExW ( hkey, pszValue, pdwReserved, pdwType, pvData, pcbData);
	return ret;
}

 /* SHGetValue: Gets a value from the registry */

/*************************************************************************
 * SHGetValueA
 *
 * Gets a value from the registry
 */
DWORD WINAPI SHGetValueA(
	HKEY     hkey,
	LPCSTR   pSubKey,
	LPCSTR   pValue,
	LPDWORD  pwType,
	LPVOID   pvData,
	LPDWORD  pbData)
{
	HKEY hSubKey;
	DWORD res;
	
	TRACE("(%s %s)\n", pSubKey, pValue);
	
	if((res = RegOpenKeyA(hkey, pSubKey, &hSubKey))) return res;
	res = RegQueryValueExA(hSubKey, pValue, 0, pwType, pvData, pbData);
	RegCloseKey( hSubKey );
	
	return res;
}

/*************************************************************************
 * SHGetValueW
 *
 * Gets a value from the registry
 */
DWORD WINAPI SHGetValueW(
	HKEY     hkey,
	LPCWSTR  pSubKey,
	LPCWSTR  pValue,
	LPDWORD  pwType,
	LPVOID   pvData,
	LPDWORD  pbData)
{
	HKEY hSubKey;
	DWORD res;
	
	TRACE("(%s %s)\n", debugstr_w(pSubKey), debugstr_w(pValue));
	
	if((res = RegOpenKeyW(hkey, pSubKey, &hSubKey))) return res;
	res = RegQueryValueExW(hSubKey, pValue, 0, pwType, pvData, pbData);
	RegCloseKey( hSubKey );
	
	return res;
}

/* gets a user-specific registry value. */

/*************************************************************************
 * SHRegGetUSValueA
 *
 * Gets a user-specific registry value
 */
LONG WINAPI SHRegGetUSValueA(
	LPCSTR   pSubKey,
	LPCSTR   pValue,
	LPDWORD  pwType,
	LPVOID   pvData,
	LPDWORD  pbData,
	BOOL     fIgnoreHKCU,
	LPVOID   pDefaultData,
	DWORD    wDefaultDataSize)
{
	FIXME("(%p),stub!\n", pSubKey);
	return ERROR_SUCCESS;  /* return success */
}

/*************************************************************************
 * SHRegGetUSValueW
 *
 * Gets a user-specific registry value
 */
LONG WINAPI SHRegGetUSValueW(
	LPCWSTR  pSubKey,
	LPCWSTR  pValue,
	LPDWORD  pwType,
	LPVOID   pvData,
	LPDWORD  pbData,
	BOOL     flagIgnoreHKCU,
	LPVOID   pDefaultData,
	DWORD    wDefaultDataSize)
{
	FIXME("(%p),stub!\n", pSubKey);
	return ERROR_SUCCESS;  /* return success */
}

/*************************************************************************
 * SHRegGetBoolUSValueA
 */
BOOL WINAPI SHRegGetBoolUSValueA(
	LPCSTR pszSubKey,
	LPCSTR pszValue,
	BOOL fIgnoreHKCU,
	BOOL fDefault)
{
	FIXME("%s %s\n", pszSubKey,pszValue);
	return fDefault;
}

/*************************************************************************
 * SHRegGetBoolUSValueW
 */
BOOL WINAPI SHRegGetBoolUSValueW(
	LPCWSTR pszSubKey,
	LPCWSTR pszValue,
	BOOL fIgnoreHKCU,
	BOOL fDefault)
{
	FIXME("%s %s\n", debugstr_w(pszSubKey),debugstr_w(pszValue));
	return fDefault;
}

/*************************************************************************
 *      SHRegQueryUSValueA	[SHLWAPI]
 */
LONG WINAPI SHRegQueryUSValueA(
	HKEY/*HUSKEY*/ hUSKey,
	LPCSTR pszValue,
	LPDWORD pdwType,
	void *pvData,
	LPDWORD pcbData,
	BOOL fIgnoreHKCU,
	void *pvDefaultData,
	DWORD dwDefaultDataSize)
{
	FIXME("%s stub\n",pszValue);
	return 1;
}

/*************************************************************************
 * SHRegGetPathA
 */
DWORD WINAPI SHRegGetPathA(
	HKEY hKey,
	LPCSTR pcszSubKey,
	LPCSTR pcszValue,
	LPSTR pszPath,
	DWORD dwFlags)
{
	FIXME("%s %s\n", pcszSubKey, pcszValue);
	return 0;
}

/*************************************************************************
 * SHRegGetPathW
 */
DWORD WINAPI SHRegGetPathW(
	HKEY hKey,
	LPCWSTR pcszSubKey,
	LPCWSTR pcszValue,
	LPWSTR pszPath,
	DWORD dwFlags)
{
	FIXME("%s %s\n", debugstr_w(pcszSubKey), debugstr_w(pcszValue));
	return 0;
}
/*************************************************************************
 * SHRegDeleteKeyA and SHDeleteKeyA
 */
HRESULT WINAPI SHRegDeleteKeyA(
	HKEY hkey,
	LPCSTR pszSubKey)
{
	FIXME("hkey=0x%08x, %s\n", hkey, debugstr_a(pszSubKey));
	return 0;
}

/*************************************************************************
 * SHRegDeleteKeyW and SHDeleteKeyA
 */
HRESULT WINAPI SHRegDeleteKeyW(
	HKEY hkey,
	LPCWSTR pszSubKey)
{
	FIXME("hkey=0x%08x, %s\n", hkey, debugstr_w(pszSubKey));
	return 0;
}

/*************************************************************************
 *      SHSetValueA	[SHLWAPI]
 */
DWORD WINAPI SHSetValueA(
	HKEY hkey,
	LPCSTR pszSubKey,
	LPCSTR pszValue,
	DWORD dwType,
	LPCVOID pvData,
	DWORD cbData)
{
	FIXME("(%s %s)stub\n",pszSubKey, pszValue);
	return 1;
}

/*************************************************************************
 * SHRegCloseKey			[NT4.0:SHELL32.505]
 *
 */
HRESULT WINAPI SHRegCloseKey (HKEY hkey)
{
	TRACE("0x%04x\n",hkey);
	return RegCloseKey( hkey );
}
