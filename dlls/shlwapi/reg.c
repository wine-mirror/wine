/*
 * SHLWAPI registry functions
 */

#include "windef.h"
#include "winerror.h"
#include "winreg.h"
#include "wine/undocshell.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(shell);

/*************************************************************************
 * SHRegGetUSValueA	[SHLWAPI.@]
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
 * SHRegGetUSValueW	[SHLWAPI.@]
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
 * SHRegGetBoolUSValueA   [SHLWAPI.@]
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
 * SHRegGetBoolUSValueW	  [SHLWAPI.@]
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
 * SHRegGetPathA   [SHLWAPI.@]
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
 * SHRegGetPathW   [SHLWAPI.@]
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
 * SHGetValueA   [SHLWAPI.@]
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
 * SHGetValueW   [SHLWAPI.@]
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

/*************************************************************************
 *      SHSetValueA   [SHLWAPI.@]
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
 * SHQueryValueExA		[SHLWAPI.@]
 *
 */
HRESULT WINAPI SHQueryValueExA(
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
 * SHQueryValueExW   [SHLWAPI.@]
 *
 * FIXME 
 *  if the datatype REG_EXPAND_SZ then expand the string and change
 *  *pdwType to REG_SZ. 
 */
HRESULT WINAPI SHQueryValueExW (
	HKEY hkey,
	LPWSTR pszValue,
	LPDWORD pdwReserved,
	LPDWORD pdwType,
	LPVOID pvData,
	LPDWORD pcbData)
{
	WARN("0x%04x %s %p %p %p %p semi-stub\n",
             hkey, debugstr_w(pszValue), pdwReserved, pdwType, pvData, pcbData);
	return RegQueryValueExW ( hkey, pszValue, pdwReserved, pdwType, pvData, pcbData);
}

/*************************************************************************
 * SHDeleteKeyA   [SHLWAPI.@]
 */
HRESULT WINAPI SHDeleteKeyA(
	HKEY hkey,
	LPCSTR pszSubKey)
{
	FIXME("hkey=0x%08x, %s\n", hkey, debugstr_a(pszSubKey));
	return 0;
}

/*************************************************************************
 * SHDeleteKeyW   [SHLWAPI.@]
 */
HRESULT WINAPI SHDeleteKeyW(
	HKEY hkey,
	LPCWSTR pszSubKey)
{
	FIXME("hkey=0x%08x, %s\n", hkey, debugstr_w(pszSubKey));
	return 0;
}
