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
 * SHRegQueryValueExA   [SHELL32.509]
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
 * SHRegQueryValueExW	[NT4.0:SHELL32.511]
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

/*************************************************************************
 * SHRegDeleteKeyA   [SHELL32]
 */
HRESULT WINAPI SHRegDeleteKeyA(
	HKEY hkey,
	LPCSTR pszSubKey)
{
	FIXME("hkey=0x%08x, %s\n", hkey, debugstr_a(pszSubKey));
	return 0;
}

/*************************************************************************
 * SHRegDeleteKeyW   [SHELL32]
 */
HRESULT WINAPI SHRegDeleteKeyW(
	HKEY hkey,
	LPCWSTR pszSubKey)
{
	FIXME("hkey=0x%08x, %s\n", hkey, debugstr_w(pszSubKey));
	return 0;
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
