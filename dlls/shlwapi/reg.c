/*
 * SHLWAPI registry functions
 */

#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "winreg.h"
#include "debugtools.h"
#include "shlwapi.h"
#include "wine/unicode.h"

DEFAULT_DEBUG_CHANNEL(shell);

static const char *lpszContentTypeA = "Content Type";
static const WCHAR lpszContentTypeW[] = { 'C','o','n','t','e','n','t',' ','T','y','p','e','\0'};

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
	HKEY hUSKey,             /* [in] FIXME: HUSKEY */
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
HRESULT WINAPI SHSetValueA(
	HKEY hkey,
	LPCSTR pszSubKey,
	LPCSTR pszValue,
	DWORD dwType,
	LPCVOID pvData,
	DWORD cbData)
{
    HKEY	subkey;
    HRESULT	hres;

    hres = RegCreateKeyA(hkey,pszSubKey,&subkey);
    if (!hres)
	return hres;
    hres = RegSetValueExA(subkey,pszValue,0,dwType,pvData,cbData);
    RegCloseKey(subkey);
    return hres;
}

/*************************************************************************
 *      SHSetValueW   [SHLWAPI.@]
 */
HRESULT WINAPI SHSetValueW(
	HKEY hkey,
	LPCWSTR pszSubKey,
	LPCWSTR pszValue,
	DWORD dwType,
	LPCVOID pvData,
	DWORD cbData)
{
    HKEY	subkey;
    HRESULT	hres;

    hres = RegCreateKeyW(hkey,pszSubKey,&subkey);
    if (!hres)
	return hres;
    hres = RegSetValueExW(subkey,pszValue,0,dwType,pvData,cbData);
    RegCloseKey(subkey);
    return hres;
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
 *
 * It appears this function is made available to account for the differences
 * between the Win9x and WinNT/2k RegDeleteKeyA functions.
 *
 * According to docs, Win9x RegDeleteKeyA will delete all subkeys, whereas
 * WinNt/2k will only delete the key if empty.
 */
HRESULT WINAPI SHDeleteKeyA(
	HKEY hKey,
	LPCSTR lpszSubKey)
{
    DWORD r, dwKeyCount, dwSize, i, dwMaxSubkeyLen;
    HKEY hSubKey;
    LPSTR lpszName;

    TRACE("hkey=0x%08x, %s\n", hKey, debugstr_a(lpszSubKey));

    hSubKey = 0;
    r = RegOpenKeyExA(hKey, lpszSubKey, 0, KEY_READ, &hSubKey);
    if(r != ERROR_SUCCESS)
        return r;

    /* find how many subkeys there are */
    dwKeyCount = 0;
    dwMaxSubkeyLen = 0;
    r = RegQueryInfoKeyA(hSubKey, NULL, NULL, NULL, &dwKeyCount,
                    &dwMaxSubkeyLen, NULL, NULL, NULL, NULL, NULL, NULL);
    if(r != ERROR_SUCCESS)
    {
        RegCloseKey(hSubKey);
        return r;
    }

    /* alloc memory for the longest string terminating 0 */
    dwMaxSubkeyLen++;
    lpszName = HeapAlloc(GetProcessHeap(), 0, dwMaxSubkeyLen*sizeof(CHAR));
    if(!lpszName)
    {
        RegCloseKey(hSubKey);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    /* recursively delete all the subkeys */
    for(i=0; i<dwKeyCount; i++)
    {
        dwSize = dwMaxSubkeyLen;
        r = RegEnumKeyExA(hSubKey, i, lpszName, &dwSize, NULL, NULL, NULL, NULL);
        if(r != ERROR_SUCCESS)
            break;
        r = SHDeleteKeyA(hSubKey, lpszName);
        if(r != ERROR_SUCCESS)
            break;
    }

    HeapFree(GetProcessHeap(), 0, lpszName);

    RegCloseKey(hSubKey);

    if(r == ERROR_SUCCESS)
        r = RegDeleteKeyA(hKey, lpszSubKey);

    return r;
}

/*************************************************************************
 * SHDeleteKeyW   [SHLWAPI.@]
 *
 * It appears this function is made available to account for the differences
 * between the Win9x and WinNT/2k RegDeleteKeyA functions.
 *
 * According to docs, Win9x RegDeleteKeyA will delete all subkeys, whereas
 * WinNt/2k will only delete the key if empty.
 */
HRESULT WINAPI SHDeleteKeyW(
	HKEY hkey,
	LPCWSTR pszSubKey)
{
	FIXME("hkey=0x%08x, %s\n", hkey, debugstr_w(pszSubKey));
	return 0;
}

/*************************************************************************
 * SHDeleteValueA   [SHLWAPI.@]
 *
 * Function opens the key, get/set/delete the value, then close the key.
 */
HRESULT WINAPI SHDeleteValueA(HKEY hkey, LPCSTR pszSubKey, LPCSTR pszValue) {
    HKEY	subkey;
    HRESULT	hres;

    hres = RegOpenKeyA(hkey,pszSubKey,&subkey);
    if (hres)
	return hres;
    hres = RegDeleteValueA(subkey,pszValue);
    RegCloseKey(subkey);
    return hres;
}

/*************************************************************************
 * SHDeleteValueW   [SHLWAPI.@]
 *
 * Function opens the key, get/set/delete the value, then close the key.
 */
HRESULT WINAPI SHDeleteValueW(HKEY hkey, LPCWSTR pszSubKey, LPCWSTR pszValue) {
    HKEY	subkey;
    HRESULT	hres;

    hres = RegOpenKeyW(hkey,pszSubKey,&subkey);
    if (hres)
	return hres;
    hres = RegDeleteValueW(subkey,pszValue);
    RegCloseKey(subkey);
    return hres;
}

/*************************************************************************
 * SHDeleteEmptyKeyA   [SHLWAPI.@]
 *
 * It appears this function is made available to account for the differences
 * between the Win9x and WinNT/2k RegDeleteKeyA functions.
 *
 * According to docs, Win9x RegDeleteKeyA will delete all subkeys, whereas
 * WinNt/2k will only delete the key if empty.
 */
DWORD WINAPI SHDeleteEmptyKeyA(HKEY hKey, LPCSTR lpszSubKey)
{
    DWORD r, dwKeyCount;
    HKEY hSubKey;

    TRACE("hkey=0x%08x, %s\n", hKey, debugstr_a(lpszSubKey));

    hSubKey = 0;
    r = RegOpenKeyExA(hKey, lpszSubKey, 0, KEY_READ, &hSubKey);
    if(r != ERROR_SUCCESS)
        return r;

    dwKeyCount = 0;
    r = RegQueryInfoKeyA(hSubKey, NULL, NULL, NULL, &dwKeyCount,
                    NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    if(r != ERROR_SUCCESS)
        return r;

    RegCloseKey(hSubKey);

    if(dwKeyCount)
        return ERROR_KEY_HAS_CHILDREN;

    r = RegDeleteKeyA(hKey, lpszSubKey);

    return r;
}

/*************************************************************************
 * SHDeleteEmptyKeyW   [SHLWAPI.@]
 *
 * It appears this function is made available to account for the differences
 * between the Win9x and WinNT/2k RegDeleteKeyA functions.
 *
 * According to docs, Win9x RegDeleteKeyA will delete all subkeys, whereas
 * WinNt/2k will only delete the key if empty.
 */
DWORD WINAPI SHDeleteEmptyKeyW(HKEY hKey, LPCWSTR lpszSubKey)
{
    FIXME("hkey=0x%08x, %s\n", hKey, debugstr_w(lpszSubKey));
    return 0;
}

/*************************************************************************
 * SHLWAPI_205   [SHLWAPI.205]
 *
 * Wrapper for SHGetValueA in case machine is in safe mode.
 */
DWORD WINAPI SHLWAPI_205(HKEY hkey, LPCSTR pSubKey, LPCSTR pValue,
                         LPDWORD pwType, LPVOID pvData, LPDWORD pbData)
{
  if (GetSystemMetrics(SM_CLEANBOOT))
    return ERROR_INVALID_FUNCTION;
  return SHGetValueA(hkey, pSubKey, pValue, pwType, pvData, pbData);
}

/*************************************************************************
 * SHLWAPI_206   [SHLWAPI.206]
 *
 * Unicode version of SHLWAPI_205.
 */
DWORD WINAPI SHLWAPI_206(HKEY hkey, LPCWSTR pSubKey, LPCWSTR pValue,
                         LPDWORD pwType, LPVOID pvData, LPDWORD pbData)
{
  if (GetSystemMetrics(SM_CLEANBOOT))
    return ERROR_INVALID_FUNCTION;
  return SHGetValueW(hkey, pSubKey, pValue, pwType, pvData, pbData);
}

/*************************************************************************
 * SHLWAPI_320   [SHLWAPI.320]
 *
 */
BOOL WINAPI SHLWAPI_320(LPCSTR lpszSubKey, LPCSTR lpszValue)
{
  DWORD dwLen = strlen(lpszValue);
  HRESULT ret = SHSetValueA(HKEY_CLASSES_ROOT, lpszSubKey, lpszContentTypeA,
                            REG_SZ, lpszValue, dwLen);
  return ret ? FALSE : TRUE;
}

/*************************************************************************
 * SHLWAPI_321   [SHLWAPI.321]
 *
 * Unicode version of SHLWAPI_320.
 */
BOOL WINAPI SHLWAPI_321(LPCWSTR lpszSubKey, LPCWSTR lpszValue)
{
  DWORD dwLen = strlenW(lpszValue);
  HRESULT ret = SHSetValueW(HKEY_CLASSES_ROOT, lpszSubKey, lpszContentTypeW,
                            REG_SZ, lpszValue, dwLen);
  return ret ? FALSE : TRUE;
}

/*************************************************************************
 * SHLWAPI_322   [SHLWAPI.322]
 *
 */
BOOL WINAPI SHLWAPI_322(LPCSTR lpszSubKey)
{
  HRESULT ret = SHDeleteValueA(HKEY_CLASSES_ROOT, lpszSubKey, lpszContentTypeA);
  return ret ? FALSE : TRUE;
}

/*************************************************************************
 * SHLWAPI_323   [SHLWAPI.323]
 *
 * Unicode version of SHLWAPI_322.
 */
BOOL WINAPI SHLWAPI_323(LPCWSTR lpszSubKey)
{
  HRESULT ret = SHDeleteValueW(HKEY_CLASSES_ROOT, lpszSubKey, lpszContentTypeW);
  return ret ? FALSE : TRUE;
}

