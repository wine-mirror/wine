/*
 * SHLWAPI registry functions
 */

#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "winnls.h"
#include "winreg.h"
#include "debugtools.h"
#include "shlwapi.h"
#include "wine/unicode.h"

DEFAULT_DEBUG_CHANNEL(shell);

static const char *lpszContentTypeA = "Content Type";
static const WCHAR lpszContentTypeW[] = { 'C','o','n','t','e','n','t',' ','T','y','p','e','\0'};

/* internal structure of what the HUSKEY points to */
typedef struct {
    HKEY     realkey;                  /* HKEY of opened key */
    HKEY     start;                    /* HKEY of where to start */
    WCHAR    key_string[MAX_PATH];     /* additional path from 'start' */
} Internal_HUSKEY, *LPInternal_HUSKEY;


/*************************************************************************
 * SHRegOpenUSKeyA	[SHLWAPI.@]
 *
 * Opens a user-specific registry key
 */
LONG WINAPI SHRegOpenUSKeyA(
        LPCSTR Path,
        REGSAM AccessType,
        HUSKEY hRelativeUSKey,
        PHUSKEY phNewUSKey,
        BOOL fIgnoreHKCU)
{
    HKEY startpoint, openkey;
    LONG ret = ~ERROR_SUCCESS;
    LPInternal_HUSKEY ihky;

    TRACE("(%s, 0x%lx, 0x%lx, %p, %s)\n", debugstr_a(Path), 
	  (LONG)AccessType, (LONG)hRelativeUSKey, phNewUSKey, 
	  (fIgnoreHKCU) ? "Ignoring HKCU" : "Process HKCU then HKLM");
    if (hRelativeUSKey)
	startpoint = ((LPInternal_HUSKEY)hRelativeUSKey)->realkey;
    else {
	startpoint = HKEY_LOCAL_MACHINE;
	if (!fIgnoreHKCU) {
	    ret = RegOpenKeyExA(HKEY_CURRENT_USER, Path, 
				0, AccessType, &openkey);
	    /* if successful, then save real starting point */
	    if (ret == ERROR_SUCCESS)
		startpoint = HKEY_CURRENT_USER;
	}
    }

    /* if current_user didn't have it, or have relative start point,
     * try it.
     */
    if (ret != ERROR_SUCCESS)
	ret = RegOpenKeyExA(startpoint, Path, 0, AccessType, &openkey);

    /* if all attempts have failed then bail */
    if (ret != ERROR_SUCCESS) {
	TRACE("failed %ld\n", ret);
	return ret;
    }

    /* now create the internal version of HUSKEY */
    ihky = (LPInternal_HUSKEY)HeapAlloc(GetProcessHeap(), 0 , 
					sizeof(Internal_HUSKEY));
    ihky->realkey = openkey;
    ihky->start = startpoint;
    MultiByteToWideChar(0, 0, Path, -1, ihky->key_string,
			sizeof(ihky->key_string)-1);
    TRACE("HUSKEY=0x%08lx\n", (LONG)ihky);
    if (phNewUSKey)
	*phNewUSKey = (HUSKEY)ihky;
    return ERROR_SUCCESS;
}

/*************************************************************************
 * SHRegOpenUSKeyW	[SHLWAPI.@]
 *
 * Opens a user-specific registry key
 */
LONG WINAPI SHRegOpenUSKeyW(
        LPCWSTR Path,
        REGSAM AccessType,
        HUSKEY hRelativeUSKey,
        PHUSKEY phNewUSKey,
        BOOL fIgnoreHKCU)
{
    HKEY startpoint, openkey;
    LONG ret = ~ERROR_SUCCESS;
    LPInternal_HUSKEY ihky;

    TRACE("(%s, 0x%lx, 0x%lx, %p, %s)\n", debugstr_w(Path), 
	  (LONG)AccessType, (LONG)hRelativeUSKey, phNewUSKey, 
	  (fIgnoreHKCU) ? "Ignoring HKCU" : "Process HKCU then HKLM");
    if (hRelativeUSKey)
	startpoint = ((LPInternal_HUSKEY)hRelativeUSKey)->realkey;
    else {
	startpoint = HKEY_LOCAL_MACHINE;
	if (!fIgnoreHKCU) {
	    ret = RegOpenKeyExW(HKEY_CURRENT_USER, Path, 
				0, AccessType, &openkey);
	    /* if successful, then save real starting point */
	    if (ret == ERROR_SUCCESS)
		startpoint = HKEY_CURRENT_USER;
	}
    }

    /* if current_user didn't have it, or have relative start point,
     * try it.
     */
    if (ret != ERROR_SUCCESS)
	ret = RegOpenKeyExW(startpoint, Path, 0, AccessType, &openkey);

    /* if all attempts have failed then bail */
    if (ret != ERROR_SUCCESS) {
	TRACE("failed %ld\n", ret);
	return ret;
    }

    /* now create the internal version of HUSKEY */
    ihky = (LPInternal_HUSKEY)HeapAlloc(GetProcessHeap(), 0 , 
					sizeof(Internal_HUSKEY));
    ihky->realkey = openkey;
    ihky->start = startpoint;
    lstrcpynW(ihky->key_string, Path, sizeof(ihky->key_string));
    TRACE("HUSKEY=0x%08lx\n", (LONG)ihky);
    if (phNewUSKey)
	*phNewUSKey = (HUSKEY)ihky;
    return ERROR_SUCCESS;
}

/*************************************************************************
 * SHRegCloseUSKey	[SHLWAPI.@]
 *
 * Closes a user-specific registry key
 */
LONG WINAPI SHRegCloseUSKey(
        HUSKEY hUSKey)
{
    LPInternal_HUSKEY mihk = (LPInternal_HUSKEY)hUSKey;
    LONG ret;

    ret = RegCloseKey(mihk->realkey);
    HeapFree(GetProcessHeap(), 0, mihk);
    return ret;
}

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
	LPDWORD  pcbData,
	BOOL     flagIgnoreHKCU,
	LPVOID   pDefaultData,
	DWORD    wDefaultDataSize)
{
	HUSKEY myhuskey;
	LPInternal_HUSKEY mihk;
	LONG ret, maxmove, i;
	CHAR *src, *dst;

	if (!pvData || !pcbData) return ERROR_INVALID_FUNCTION; /* FIXME:wrong*/
	TRACE("key '%s', value '%s', datalen %ld,  %s\n",
	      debugstr_a(pSubKey), debugstr_a(pValue), *pcbData,
	      (flagIgnoreHKCU) ? "Ignoring HKCU" : "Trys HKCU then HKLM");

	ret = SHRegOpenUSKeyA(pSubKey, 0x1, 0, &myhuskey, flagIgnoreHKCU);
	if (ret == ERROR_SUCCESS) {
	    mihk = (LPInternal_HUSKEY) myhuskey;
	    ret = RegQueryValueExA(mihk->realkey, pValue, 0, pwType,
				   (LPBYTE)pvData, pcbData);
	    SHRegCloseUSKey(myhuskey);
	}
	if (ret != ERROR_SUCCESS) {
	    if (pDefaultData && (wDefaultDataSize != 0)) {
		maxmove = (wDefaultDataSize >= *pcbData) ? *pcbData : wDefaultDataSize;
		src = (CHAR*)pDefaultData;
		dst = (CHAR*)pvData;
		for(i=0; i<maxmove; i++) *dst++ = *src++;
		*pcbData = maxmove;
		TRACE("setting default data\n");
		ret = ERROR_SUCCESS;
	    }
	}
	return ret;
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
	LPDWORD  pcbData,
	BOOL     flagIgnoreHKCU,
	LPVOID   pDefaultData,
	DWORD    wDefaultDataSize)
{
	HUSKEY myhuskey;
	LPInternal_HUSKEY mihk;
	LONG ret, maxmove, i;
	CHAR *src, *dst;

	if (!pvData || !pcbData) return ERROR_INVALID_FUNCTION; /* FIXME:wrong*/
	TRACE("key '%s', value '%s', datalen %ld,  %s\n",
	      debugstr_w(pSubKey), debugstr_w(pValue), *pcbData,
	      (flagIgnoreHKCU) ? "Ignoring HKCU" : "Trys HKCU then HKLM");

	ret = SHRegOpenUSKeyW(pSubKey, 0x1, 0, &myhuskey, flagIgnoreHKCU);
	if (ret == ERROR_SUCCESS) {
	    mihk = (LPInternal_HUSKEY) myhuskey;
	    ret = RegQueryValueExW(mihk->realkey, pValue, 0, pwType,
				   (LPBYTE)pvData, pcbData);
	    SHRegCloseUSKey(myhuskey);
	}
	if (ret != ERROR_SUCCESS) {
	    if (pDefaultData && (wDefaultDataSize != 0)) {
		maxmove = (wDefaultDataSize >= *pcbData) ? *pcbData : wDefaultDataSize;
		src = (CHAR*)pDefaultData;
		dst = (CHAR*)pvData;
		for(i=0; i<maxmove; i++) *dst++ = *src++;
		*pcbData = maxmove;
		TRACE("setting default data\n");
		ret = ERROR_SUCCESS;
	    }
	}
	return ret;
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
 *      SHRegQueryUSValueA	[SHLWAPI.@]
 */
LONG WINAPI SHRegQueryUSValueA(
	HUSKEY hUSKey,             /* [in]  */
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
 *      SHRegQueryUSValueW	[SHLWAPI.@]
 */
LONG WINAPI SHRegQueryUSValueW(
	HUSKEY hUSKey,             /* [in]  */
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
 *      SHRegQueryInfoUSKeyA	[SHLWAPI.@]
 */
DWORD WINAPI SHRegQueryInfoUSKeyA(
	HUSKEY hUSKey,             /* [in] FIXME: HUSKEY */
	LPDWORD pcSubKeys,
	LPDWORD pcchMaxSubKeyLen,
	LPDWORD pcValues,
	LPDWORD pcchMaxValueNameLen,
	SHREGENUM_FLAGS enumRegFlags)
{
	TRACE("(0x%lx,%p,%p,%p,%p,%d)\n",
	      (LONG)hUSKey, pcSubKeys, pcchMaxSubKeyLen, pcValues,
	      pcchMaxValueNameLen, enumRegFlags);
	return RegQueryInfoKeyA(((LPInternal_HUSKEY)hUSKey)->realkey, 0, 0, 0, 
				pcSubKeys, pcchMaxSubKeyLen, 0,
				pcValues, pcchMaxValueNameLen, 0, 0, 0);
}

/*************************************************************************
 *      SHRegQueryInfoUSKeyW	[SHLWAPI.@]
 */
DWORD WINAPI SHRegQueryInfoUSKeyW(
	HUSKEY hUSKey,             /* [in] FIXME: HUSKEY */
	LPDWORD pcSubKeys,
	LPDWORD pcchMaxSubKeyLen,
	LPDWORD pcValues,
	LPDWORD pcchMaxValueNameLen,
	SHREGENUM_FLAGS enumRegFlags)
{
	TRACE("(0x%lx,%p,%p,%p,%p,%d)\n",
	      (LONG)hUSKey,pcSubKeys,pcchMaxSubKeyLen,pcValues,
	      pcchMaxValueNameLen,enumRegFlags);
	return RegQueryInfoKeyW(((LPInternal_HUSKEY)hUSKey)->realkey, 0, 0, 0, 
				pcSubKeys, pcchMaxSubKeyLen, 0,
				pcValues, pcchMaxValueNameLen, 0, 0, 0);
}

/*************************************************************************
 *      SHRegEnumUSKeyA   	[SHLWAPI.@]
 */
LONG WINAPI SHRegEnumUSKeyA(
	HUSKEY hUSKey,                 /* [in]  */
	DWORD dwIndex,
	LPSTR pszName,
	LPDWORD pcchValueNameLen,
	SHREGENUM_FLAGS enumRegFlags)  /* [in]  */
{
	FIXME("%s stub\n",debugstr_a(pszName));
	return ERROR_NO_MORE_ITEMS;
}

/*************************************************************************
 *      SHRegEnumUSKeyW   	[SHLWAPI.@]
 */
LONG WINAPI SHRegEnumUSKeyW(
	HUSKEY hUSKey,                 /* [in]  */
	DWORD dwIndex,
	LPWSTR pszName,
	LPDWORD pcchValueNameLen,
	SHREGENUM_FLAGS enumRegFlags)  /* [in]  */
{
	FIXME("%s stub\n",debugstr_w(pszName));
	return ERROR_NO_MORE_ITEMS;
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
 * @   [SHLWAPI.205]
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
 * @   [SHLWAPI.206]
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
 * @   [SHLWAPI.320]
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
 * @   [SHLWAPI.321]
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
 * @   [SHLWAPI.322]
 *
 */
BOOL WINAPI SHLWAPI_322(LPCSTR lpszSubKey)
{
  HRESULT ret = SHDeleteValueA(HKEY_CLASSES_ROOT, lpszSubKey, lpszContentTypeA);
  return ret ? FALSE : TRUE;
}

/*************************************************************************
 * @   [SHLWAPI.323]
 *
 * Unicode version of SHLWAPI_322.
 */
BOOL WINAPI SHLWAPI_323(LPCWSTR lpszSubKey)
{
  HRESULT ret = SHDeleteValueW(HKEY_CLASSES_ROOT, lpszSubKey, lpszContentTypeW);
  return ret ? FALSE : TRUE;
}

