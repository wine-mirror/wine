/*
 * SHLWAPI registry functions
 *
 * Copyright 1998 Juergen Schmied
 * Copyright 2001 Guy Albertelli
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "wine/debug.h"
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/* Key/Value names for MIME content types */
static const char lpszContentTypeA[] = "Content Type";
static const WCHAR lpszContentTypeW[] = { 'C','o','n','t','e','n','t',' ','T','y','p','e','\0'};

static const char szMimeDbContentA[] = "MIME\\Database\\Content Type\\";
static const WCHAR szMimeDbContentW[] = { 'M', 'I', 'M','E','\\',
  'D','a','t','a','b','a','s','e','\\','C','o','n','t','e','n','t',
  ' ','T','y','p','e','\\', 0 };
static const DWORD dwLenMimeDbContent = 27; /* strlen of szMimeDbContentA/W */

static const char szExtensionA[] = "Extension";
static const WCHAR szExtensionW[] = { 'E', 'x', 't','e','n','s','i','o','n','\0' };

INT     WINAPI SHStringFromGUIDW(REFGUID,LPWSTR,INT);
HRESULT WINAPI SHRegGetCLSIDKeyW(REFGUID,LPCWSTR,BOOL,BOOL,PHKEY);


/*************************************************************************
 * SHRegGetPathA   [SHLWAPI.@]
 *
 * Get a path from the registry.
 *
 * PARAMS
 *   hKey       [I] Handle to registry key
 *   lpszSubKey [I] Name of sub key containing path to get
 *   lpszValue  [I] Name of value containing path to get
 *   lpszPath   [O] Buffer for returned path
 *   dwFlags    [I] Reserved
 *
 * RETURNS
 *   Success: ERROR_SUCCESS. lpszPath contains the path.
 *   Failure: An error code from RegOpenKeyExA() or SHQueryValueExA().
 */
DWORD WINAPI SHRegGetPathA(HKEY hKey, LPCSTR lpszSubKey, LPCSTR lpszValue,
                           LPSTR lpszPath, DWORD dwFlags)
{
  DWORD dwSize = MAX_PATH;

  TRACE("(hkey=%p,%s,%s,%p,%d)\n", hKey, debugstr_a(lpszSubKey),
        debugstr_a(lpszValue), lpszPath, dwFlags);

  return SHGetValueA(hKey, lpszSubKey, lpszValue, 0, lpszPath, &dwSize);
}

/*************************************************************************
 * SHRegGetPathW   [SHLWAPI.@]
 *
 * See SHRegGetPathA.
 */
DWORD WINAPI SHRegGetPathW(HKEY hKey, LPCWSTR lpszSubKey, LPCWSTR lpszValue,
                           LPWSTR lpszPath, DWORD dwFlags)
{
  DWORD dwSize = MAX_PATH;

  TRACE("(hkey=%p,%s,%s,%p,%d)\n", hKey, debugstr_w(lpszSubKey),
        debugstr_w(lpszValue), lpszPath, dwFlags);

  return SHGetValueW(hKey, lpszSubKey, lpszValue, 0, lpszPath, &dwSize);
}


/*************************************************************************
 * SHRegSetPathA   [SHLWAPI.@]
 *
 * Write a path to the registry.
 *
 * PARAMS
 *   hKey       [I] Handle to registry key
 *   lpszSubKey [I] Name of sub key containing path to set
 *   lpszValue  [I] Name of value containing path to set
 *   lpszPath   [O] Path to write
 *   dwFlags    [I] Reserved, must be 0.
 *
 * RETURNS
 *   Success: ERROR_SUCCESS.
 *   Failure: An error code from SHSetValueA().
 */
DWORD WINAPI SHRegSetPathA(HKEY hKey, LPCSTR lpszSubKey, LPCSTR lpszValue,
                           LPCSTR lpszPath, DWORD dwFlags)
{
  char szBuff[MAX_PATH];

  FIXME("(hkey=%p,%s,%s,%p,%d) - semi-stub\n",hKey, debugstr_a(lpszSubKey),
        debugstr_a(lpszValue), lpszPath, dwFlags);

  lstrcpyA(szBuff, lpszPath);

  /* FIXME: PathUnExpandEnvStringsA(szBuff); */

  return SHSetValueA(hKey,lpszSubKey, lpszValue, REG_SZ, szBuff,
                     lstrlenA(szBuff));
}

/*************************************************************************
 * SHRegSetPathW   [SHLWAPI.@]
 *
 * See SHRegSetPathA.
 */
DWORD WINAPI SHRegSetPathW(HKEY hKey, LPCWSTR lpszSubKey, LPCWSTR lpszValue,
                           LPCWSTR lpszPath, DWORD dwFlags)
{
  WCHAR szBuff[MAX_PATH];

  FIXME("(hkey=%p,%s,%s,%p,%d) - semi-stub\n",hKey, debugstr_w(lpszSubKey),
        debugstr_w(lpszValue), lpszPath, dwFlags);

  lstrcpyW(szBuff, lpszPath);

  /* FIXME: PathUnExpandEnvStringsW(szBuff); */

  return SHSetValueW(hKey,lpszSubKey, lpszValue, REG_SZ, szBuff,
                     lstrlenW(szBuff));
}

/*************************************************************************
 * SHGetValueA   [SHLWAPI.@]
 *
 * Get a value from the registry.
 *
 * PARAMS
 *   hKey       [I] Handle to registry key
 *   lpszSubKey [I] Name of sub key containing value to get
 *   lpszValue  [I] Name of value to get
 *   pwType     [O] Pointer to the values type
 *   pvData     [O] Pointer to the values data
 *   pcbData    [O] Pointer to the values size
 *
 * RETURNS
 *   Success: ERROR_SUCCESS. Output parameters contain the details read.
 *   Failure: An error code from RegOpenKeyExA() or SHQueryValueExA().
 */
DWORD WINAPI SHGetValueA(HKEY hKey, LPCSTR lpszSubKey, LPCSTR lpszValue,
                         LPDWORD pwType, LPVOID pvData, LPDWORD pcbData)
{
  DWORD dwRet = 0;
  HKEY hSubKey = 0;

  TRACE("(hkey=%p,%s,%s,%p,%p,%p)\n", hKey, debugstr_a(lpszSubKey),
        debugstr_a(lpszValue), pwType, pvData, pcbData);

  /*   lpszSubKey can be 0. In this case the value is taken from the
   *   current key.
   */
  if(lpszSubKey)
    dwRet = RegOpenKeyExA(hKey, lpszSubKey, 0, KEY_QUERY_VALUE, &hSubKey);

  if (! dwRet)
  {
    /* SHQueryValueEx expands Environment strings */
    dwRet = SHQueryValueExA(hSubKey ? hSubKey : hKey, lpszValue, 0, pwType, pvData, pcbData);
    if (hSubKey) RegCloseKey(hSubKey);
  }
  return dwRet;
}

/*************************************************************************
 * SHGetValueW   [SHLWAPI.@]
 *
 * See SHGetValueA.
 */
DWORD WINAPI SHGetValueW(HKEY hKey, LPCWSTR lpszSubKey, LPCWSTR lpszValue,
                         LPDWORD pwType, LPVOID pvData, LPDWORD pcbData)
{
  DWORD dwRet = 0;
  HKEY hSubKey = 0;

  TRACE("(hkey=%p,%s,%s,%p,%p,%p)\n", hKey, debugstr_w(lpszSubKey),
        debugstr_w(lpszValue), pwType, pvData, pcbData);

  if(lpszSubKey)
    dwRet = RegOpenKeyExW(hKey, lpszSubKey, 0, KEY_QUERY_VALUE, &hSubKey);

  if (! dwRet)
  {
    dwRet = SHQueryValueExW(hSubKey ? hSubKey : hKey, lpszValue, 0, pwType, pvData, pcbData);
    if (hSubKey) RegCloseKey(hSubKey);
  }
  return dwRet;
}

/*************************************************************************
 * SHSetValueA   [SHLWAPI.@]
 *
 * Set a value in the registry.
 *
 * PARAMS
 *   hKey       [I] Handle to registry key
 *   lpszSubKey [I] Name of sub key under hKey
 *   lpszValue  [I] Name of value to set
 *   dwType     [I] Type of the value
 *   pvData     [I] Data of the value
 *   cbData     [I] Size of the value
 *
 * RETURNS
 *   Success: ERROR_SUCCESS. The value is set with the data given.
 *   Failure: An error code from RegCreateKeyExA() or RegSetValueExA()
 *
 * NOTES
 *   If lpszSubKey does not exist, it is created before the value is set. If
 *   lpszSubKey is NULL or an empty string, then the value is added directly
 *   to hKey instead.
 */
DWORD WINAPI SHSetValueA(HKEY hKey, LPCSTR lpszSubKey, LPCSTR lpszValue,
                         DWORD dwType, LPCVOID pvData, DWORD cbData)
{
  DWORD dwRet = ERROR_SUCCESS, dwDummy;
  HKEY  hSubKey;

  TRACE("(hkey=%p,%s,%s,%d,%p,%d)\n", hKey, debugstr_a(lpszSubKey),
          debugstr_a(lpszValue), dwType, pvData, cbData);

  if (lpszSubKey && *lpszSubKey)
    dwRet = RegCreateKeyExA(hKey, lpszSubKey, 0, NULL,
                            0, KEY_SET_VALUE, NULL, &hSubKey, &dwDummy);
  else
    hSubKey = hKey;
  if (!dwRet)
  {
    dwRet = RegSetValueExA(hSubKey, lpszValue, 0, dwType, pvData, cbData);
    if (hSubKey != hKey)
      RegCloseKey(hSubKey);
  }
  return dwRet;
}

/*************************************************************************
 * SHSetValueW   [SHLWAPI.@]
 *
 * See SHSetValueA.
 */
DWORD WINAPI SHSetValueW(HKEY hKey, LPCWSTR lpszSubKey, LPCWSTR lpszValue,
                         DWORD dwType, LPCVOID pvData, DWORD cbData)
{
  DWORD dwRet = ERROR_SUCCESS, dwDummy;
  HKEY  hSubKey;

  TRACE("(hkey=%p,%s,%s,%d,%p,%d)\n", hKey, debugstr_w(lpszSubKey),
        debugstr_w(lpszValue), dwType, pvData, cbData);

  if (lpszSubKey && *lpszSubKey)
    dwRet = RegCreateKeyExW(hKey, lpszSubKey, 0, NULL,
                            0, KEY_SET_VALUE, NULL, &hSubKey, &dwDummy);
  else
    hSubKey = hKey;
  if (!dwRet)
  {
    dwRet = RegSetValueExW(hSubKey, lpszValue, 0, dwType, pvData, cbData);
    if (hSubKey != hKey)
      RegCloseKey(hSubKey);
  }
  return dwRet;
}

/*************************************************************************
 * SHQueryInfoKeyA   [SHLWAPI.@]
 *
 * Get information about a registry key. See RegQueryInfoKeyA().
 *
 * RETURNS
 *  The result of calling RegQueryInfoKeyA().
 */
LONG WINAPI SHQueryInfoKeyA(HKEY hKey, LPDWORD pwSubKeys, LPDWORD pwSubKeyMax,
                            LPDWORD pwValues, LPDWORD pwValueMax)
{
  TRACE("(hkey=%p,%p,%p,%p,%p)\n", hKey, pwSubKeys, pwSubKeyMax,
        pwValues, pwValueMax);
  return RegQueryInfoKeyA(hKey, NULL, NULL, NULL, pwSubKeys, pwSubKeyMax,
                          NULL, pwValues, pwValueMax, NULL, NULL, NULL);
}

/*************************************************************************
 * SHQueryInfoKeyW   [SHLWAPI.@]
 *
 * See SHQueryInfoKeyA.
 */
LONG WINAPI SHQueryInfoKeyW(HKEY hKey, LPDWORD pwSubKeys, LPDWORD pwSubKeyMax,
                            LPDWORD pwValues, LPDWORD pwValueMax)
{
  TRACE("(hkey=%p,%p,%p,%p,%p)\n", hKey, pwSubKeys, pwSubKeyMax,
        pwValues, pwValueMax);
  return RegQueryInfoKeyW(hKey, NULL, NULL, NULL, pwSubKeys, pwSubKeyMax,
                          NULL, pwValues, pwValueMax, NULL, NULL, NULL);
}

/*************************************************************************
 * SHQueryValueExA   [SHLWAPI.@]
 *
 * Get a value from the registry, expanding environment variable strings.
 *
 * PARAMS
 *   hKey       [I] Handle to registry key
 *   lpszValue  [I] Name of value to query
 *   lpReserved [O] Reserved for future use; must be NULL
 *   pwType     [O] Optional pointer updated with the values type
 *   pvData     [O] Optional pointer updated with the values data
 *   pcbData    [O] Optional pointer updated with the values size
 *
 * RETURNS
 *   Success: ERROR_SUCCESS. Any non NULL output parameters are updated with
 *            information about the value.
 *   Failure: ERROR_OUTOFMEMORY if memory allocation fails, or the type of the
 *            data is REG_EXPAND_SZ and pcbData is NULL. Otherwise an error
 *            code from RegQueryValueExA() or ExpandEnvironmentStringsA().
 *
 * NOTES
 *   Either pwType, pvData or pcbData may be NULL if the caller doesn't want
 *   the type, data or size information for the value.
 *
 *   If the type of the data is REG_EXPAND_SZ, it is expanded to REG_SZ. The
 *   value returned will be truncated if it is of type REG_SZ and bigger than
 *   the buffer given to store it.
 *
 *   REG_EXPAND_SZ:
 *     case-1: the unexpanded string is smaller than the expanded one
 *       subcase-1: the buffer is too small to hold the unexpanded string:
 *          function fails and returns the size of the unexpanded string.
 *
 *       subcase-2: buffer is too small to hold the expanded string:
 *          the function return success (!!) and the result is truncated
 *	    *** This is clearly an error in the native implementation. ***
 *
 *     case-2: the unexpanded string is bigger than the expanded one
 *       The buffer must have enough space to hold the unexpanded
 *       string even if the result is smaller.
 *
 */
DWORD WINAPI SHQueryValueExA( HKEY hKey, LPCSTR lpszValue,
                              LPDWORD lpReserved, LPDWORD pwType,
                              LPVOID pvData, LPDWORD pcbData)
{
  DWORD dwRet, dwType, dwUnExpDataLen = 0, dwExpDataLen;

  TRACE("(hkey=%p,%s,%p,%p,%p,%p=%d)\n", hKey, debugstr_a(lpszValue),
        lpReserved, pwType, pvData, pcbData, pcbData ? *pcbData : 0);

  if (pcbData) dwUnExpDataLen = *pcbData;

  dwRet = RegQueryValueExA(hKey, lpszValue, lpReserved, &dwType, pvData, &dwUnExpDataLen);

  if (pcbData && (dwType == REG_EXPAND_SZ))
  {
    DWORD nBytesToAlloc;

    /* Expand type REG_EXPAND_SZ into REG_SZ */
    LPSTR szData;

    /* If the caller didn't supply a buffer or the buffer is too small we have
     * to allocate our own
     */
    if ((!pvData) || (dwRet == ERROR_MORE_DATA) )
    {
      char cNull = '\0';
      nBytesToAlloc = dwUnExpDataLen;

      szData = LocalAlloc(LMEM_ZEROINIT, nBytesToAlloc);
      RegQueryValueExA (hKey, lpszValue, lpReserved, NULL, (LPBYTE)szData, &nBytesToAlloc);
      dwExpDataLen = ExpandEnvironmentStringsA(szData, &cNull, 1);
      dwUnExpDataLen = max(nBytesToAlloc, dwExpDataLen);
      LocalFree(szData);
    }
    else
    {
      nBytesToAlloc = (lstrlenA(pvData)+1) * sizeof (CHAR);
      szData = LocalAlloc(LMEM_ZEROINIT, nBytesToAlloc);
      lstrcpyA(szData, pvData);
      dwExpDataLen = ExpandEnvironmentStringsA(szData, pvData, *pcbData / sizeof(CHAR));
      if (dwExpDataLen > *pcbData) dwRet = ERROR_MORE_DATA;
      dwUnExpDataLen = max(nBytesToAlloc, dwExpDataLen);
      LocalFree(szData);
    }
  }

  /* Update the type and data size if the caller wanted them */
  if ( dwType == REG_EXPAND_SZ ) dwType = REG_SZ;
  if ( pwType ) *pwType = dwType;
  if ( pcbData ) *pcbData = dwUnExpDataLen;
  return dwRet;
}


/*************************************************************************
 * SHQueryValueExW   [SHLWAPI.@]
 *
 * See SHQueryValueExA.
 */
DWORD WINAPI SHQueryValueExW(HKEY hKey, LPCWSTR lpszValue,
                             LPDWORD lpReserved, LPDWORD pwType,
                             LPVOID pvData, LPDWORD pcbData)
{
  DWORD dwRet, dwType, dwUnExpDataLen = 0, dwExpDataLen;

  TRACE("(hkey=%p,%s,%p,%p,%p,%p=%d)\n", hKey, debugstr_w(lpszValue),
        lpReserved, pwType, pvData, pcbData, pcbData ? *pcbData : 0);

  if (pcbData) dwUnExpDataLen = *pcbData;

  dwRet = RegQueryValueExW(hKey, lpszValue, lpReserved, &dwType, pvData, &dwUnExpDataLen);
  if (dwRet!=ERROR_SUCCESS && dwRet!=ERROR_MORE_DATA)
      return dwRet;

  if (pcbData && (dwType == REG_EXPAND_SZ))
  {
    DWORD nBytesToAlloc;

    /* Expand type REG_EXPAND_SZ into REG_SZ */
    LPWSTR szData;

    /* If the caller didn't supply a buffer or the buffer is too small we have
     * to allocate our own
     */
    if ((!pvData) || (dwRet == ERROR_MORE_DATA) )
    {
      WCHAR cNull = '\0';
      nBytesToAlloc = dwUnExpDataLen;

      szData = LocalAlloc(LMEM_ZEROINIT, nBytesToAlloc);
      RegQueryValueExW (hKey, lpszValue, lpReserved, NULL, (LPBYTE)szData, &nBytesToAlloc);
      dwExpDataLen = ExpandEnvironmentStringsW(szData, &cNull, 1);
      dwUnExpDataLen = max(nBytesToAlloc, dwExpDataLen);
      LocalFree(szData);
    }
    else
    {
      nBytesToAlloc = (lstrlenW(pvData) + 1) * sizeof(WCHAR);
      szData = LocalAlloc(LMEM_ZEROINIT, nBytesToAlloc);
      lstrcpyW(szData, pvData);
      dwExpDataLen = ExpandEnvironmentStringsW(szData, pvData, *pcbData/sizeof(WCHAR) );
      if (dwExpDataLen > *pcbData) dwRet = ERROR_MORE_DATA;
      dwUnExpDataLen = max(nBytesToAlloc, dwExpDataLen);
      LocalFree(szData);
    }
  }

  /* Update the type and data size if the caller wanted them */
  if ( dwType == REG_EXPAND_SZ ) dwType = REG_SZ;
  if ( pwType ) *pwType = dwType;
  if ( pcbData ) *pcbData = dwUnExpDataLen;
  return dwRet;
}

/*************************************************************************
 * SHDeleteOrphanKeyA   [SHLWAPI.@]
 *
 * Delete a registry key with no sub keys or values.
 *
 * PARAMS
 *   hKey       [I] Handle to registry key
 *   lpszSubKey [I] Name of sub key to possibly delete
 *
 * RETURNS
 *   Success: ERROR_SUCCESS. The key has been deleted if it was an orphan.
 *   Failure: An error from RegOpenKeyExA(), RegQueryValueExA(), or RegDeleteKeyA().
 */
DWORD WINAPI SHDeleteOrphanKeyA(HKEY hKey, LPCSTR lpszSubKey)
{
  HKEY hSubKey;
  DWORD dwKeyCount = 0, dwValueCount = 0, dwRet;

  TRACE("(hkey=%p,%s)\n", hKey, debugstr_a(lpszSubKey));

  dwRet = RegOpenKeyExA(hKey, lpszSubKey, 0, KEY_READ, &hSubKey);

  if(!dwRet)
  {
    /* Get subkey and value count */
    dwRet = RegQueryInfoKeyA(hSubKey, NULL, NULL, NULL, &dwKeyCount,
                             NULL, NULL, &dwValueCount, NULL, NULL, NULL, NULL);

    if(!dwRet && !dwKeyCount && !dwValueCount)
    {
      dwRet = RegDeleteKeyA(hKey, lpszSubKey);
    }
    RegCloseKey(hSubKey);
  }
  return dwRet;
}

/*************************************************************************
 * SHDeleteOrphanKeyW   [SHLWAPI.@]
 *
 * See SHDeleteOrphanKeyA.
 */
DWORD WINAPI SHDeleteOrphanKeyW(HKEY hKey, LPCWSTR lpszSubKey)
{
  HKEY hSubKey;
  DWORD dwKeyCount = 0, dwValueCount = 0, dwRet;

  TRACE("(hkey=%p,%s)\n", hKey, debugstr_w(lpszSubKey));

  dwRet = RegOpenKeyExW(hKey, lpszSubKey, 0, KEY_READ, &hSubKey);

  if(!dwRet)
  {
    /* Get subkey and value count */
    dwRet = RegQueryInfoKeyW(hSubKey, NULL, NULL, NULL, &dwKeyCount,
                             NULL, NULL, &dwValueCount, NULL, NULL, NULL, NULL);

    if(!dwRet && !dwKeyCount && !dwValueCount)
    {
      dwRet = RegDeleteKeyW(hKey, lpszSubKey);
    }
    RegCloseKey(hSubKey);
  }
  return dwRet;
}

/*************************************************************************
 * SHDeleteValueA   [SHLWAPI.@]
 *
 * Delete a value from the registry.
 *
 * PARAMS
 *   hKey       [I] Handle to registry key
 *   lpszSubKey [I] Name of sub key containing value to delete
 *   lpszValue  [I] Name of value to delete
 *
 * RETURNS
 *   Success: ERROR_SUCCESS. The value is deleted.
 *   Failure: An error code from RegOpenKeyExA() or RegDeleteValueA().
 */
DWORD WINAPI SHDeleteValueA(HKEY hKey, LPCSTR lpszSubKey, LPCSTR lpszValue)
{
  DWORD dwRet;
  HKEY hSubKey;

  TRACE("(hkey=%p,%s,%s)\n", hKey, debugstr_a(lpszSubKey), debugstr_a(lpszValue));

  dwRet = RegOpenKeyExA(hKey, lpszSubKey, 0, KEY_SET_VALUE, &hSubKey);
  if (!dwRet)
  {
    dwRet = RegDeleteValueA(hSubKey, lpszValue);
    RegCloseKey(hSubKey);
  }
  return dwRet;
}

/*************************************************************************
 * SHDeleteValueW   [SHLWAPI.@]
 *
 * See SHDeleteValueA.
 */
DWORD WINAPI SHDeleteValueW(HKEY hKey, LPCWSTR lpszSubKey, LPCWSTR lpszValue)
{
  DWORD dwRet;
  HKEY hSubKey;

  TRACE("(hkey=%p,%s,%s)\n", hKey, debugstr_w(lpszSubKey), debugstr_w(lpszValue));

  dwRet = RegOpenKeyExW(hKey, lpszSubKey, 0, KEY_SET_VALUE, &hSubKey);
  if (!dwRet)
  {
    dwRet = RegDeleteValueW(hSubKey, lpszValue);
    RegCloseKey(hSubKey);
  }
  return dwRet;
}

/*************************************************************************
 * @   [SHLWAPI.205]
 *
 * Get a value from the registry.
 *
 * PARAMS
 *   hKey    [I] Handle to registry key
 *   pSubKey [I] Name of sub key containing value to get
 *   pValue  [I] Name of value to get
 *   pwType  [O] Destination for the values type
 *   pvData  [O] Destination for the values data
 *   pbData  [O] Destination for the values size
 *
 * RETURNS
 *   Success: ERROR_SUCCESS. Output parameters contain the details read.
 *   Failure: An error code from RegOpenKeyExA() or SHQueryValueExA(),
 *            or ERROR_INVALID_FUNCTION in the machine is in safe mode.
 */
DWORD WINAPI SHGetValueGoodBootA(HKEY hkey, LPCSTR pSubKey, LPCSTR pValue,
                         LPDWORD pwType, LPVOID pvData, LPDWORD pbData)
{
  if (GetSystemMetrics(SM_CLEANBOOT))
    return ERROR_INVALID_FUNCTION;
  return SHGetValueA(hkey, pSubKey, pValue, pwType, pvData, pbData);
}

/*************************************************************************
 * @   [SHLWAPI.206]
 *
 * Unicode version of SHGetValueGoodBootW.
 */
DWORD WINAPI SHGetValueGoodBootW(HKEY hkey, LPCWSTR pSubKey, LPCWSTR pValue,
                         LPDWORD pwType, LPVOID pvData, LPDWORD pbData)
{
  if (GetSystemMetrics(SM_CLEANBOOT))
    return ERROR_INVALID_FUNCTION;
  return SHGetValueW(hkey, pSubKey, pValue, pwType, pvData, pbData);
}

/*************************************************************************
 * @   [SHLWAPI.320]
 *
 * Set a MIME content type in the registry.
 *
 * PARAMS
 *   lpszSubKey [I] Name of key under HKEY_CLASSES_ROOT.
 *   lpszValue  [I] Value to set
 *
 * RETURNS
 *   Success: TRUE
 *   Failure: FALSE
 */
BOOL WINAPI RegisterMIMETypeForExtensionA(LPCSTR lpszSubKey, LPCSTR lpszValue)
{
  if (!lpszValue)
  {
    WARN("Invalid lpszValue would crash under Win32!\n");
    return FALSE;
  }

  return !SHSetValueA(HKEY_CLASSES_ROOT, lpszSubKey, lpszContentTypeA,
                      REG_SZ, lpszValue, strlen(lpszValue));
}

/*************************************************************************
 * @   [SHLWAPI.321]
 *
 * Unicode version of RegisterMIMETypeForExtensionA.
 */
BOOL WINAPI RegisterMIMETypeForExtensionW(LPCWSTR lpszSubKey, LPCWSTR lpszValue)
{
  if (!lpszValue)
  {
    WARN("Invalid lpszValue would crash under Win32!\n");
    return FALSE;
  }

  return !SHSetValueW(HKEY_CLASSES_ROOT, lpszSubKey, lpszContentTypeW,
                      REG_SZ, lpszValue, lstrlenW(lpszValue));
}

/*************************************************************************
 * @   [SHLWAPI.322]
 *
 * Delete a MIME content type from the registry.
 *
 * PARAMS
 *   lpszSubKey [I] Name of sub key
 *
 * RETURNS
 *   Success: TRUE
 *   Failure: FALSE
 */
BOOL WINAPI UnregisterMIMETypeForExtensionA(LPCSTR lpszSubKey)
{
  return !SHDeleteValueA(HKEY_CLASSES_ROOT, lpszSubKey, lpszContentTypeA);
}

/*************************************************************************
 * @   [SHLWAPI.323]
 *
 * Unicode version of UnregisterMIMETypeForExtensionA.
 */
BOOL WINAPI UnregisterMIMETypeForExtensionW(LPCWSTR lpszSubKey)
{
  return !SHDeleteValueW(HKEY_CLASSES_ROOT, lpszSubKey, lpszContentTypeW);
}

/*************************************************************************
 * @   [SHLWAPI.328]
 *
 * Get the registry path to a MIME content key.
 *
 * PARAMS
 *   lpszType   [I] Content type to get the path for
 *   lpszBuffer [O] Destination for path
 *   dwLen      [I] Length of lpszBuffer
 *
 * RETURNS
 *   Success: TRUE. lpszBuffer contains the full path.
 *   Failure: FALSE.
 *
 * NOTES
 *   The base path for the key is "MIME\Database\Content Type\"
 */
BOOL WINAPI GetMIMETypeSubKeyA(LPCSTR lpszType, LPSTR lpszBuffer, DWORD dwLen)
{
  TRACE("(%s,%p,%d)\n", debugstr_a(lpszType), lpszBuffer, dwLen);

  if (dwLen > dwLenMimeDbContent && lpszType && lpszBuffer)
  {
    size_t dwStrLen = strlen(lpszType);

    if (dwStrLen < dwLen - dwLenMimeDbContent)
    {
      memcpy(lpszBuffer, szMimeDbContentA, dwLenMimeDbContent);
      memcpy(lpszBuffer + dwLenMimeDbContent, lpszType, dwStrLen + 1);
      return TRUE;
    }
  }
  return FALSE;
}

/*************************************************************************
 * @   [SHLWAPI.329]
 *
 * Unicode version of GetMIMETypeSubKeyA.
 */
BOOL WINAPI GetMIMETypeSubKeyW(LPCWSTR lpszType, LPWSTR lpszBuffer, DWORD dwLen)
{
  TRACE("(%s,%p,%d)\n", debugstr_w(lpszType), lpszBuffer, dwLen);

  if (dwLen > dwLenMimeDbContent && lpszType && lpszBuffer)
  {
    DWORD dwStrLen = lstrlenW(lpszType);

    if (dwStrLen < dwLen - dwLenMimeDbContent)
    {
      memcpy(lpszBuffer, szMimeDbContentW, dwLenMimeDbContent * sizeof(WCHAR));
      memcpy(lpszBuffer + dwLenMimeDbContent, lpszType, (dwStrLen + 1) * sizeof(WCHAR));
      return TRUE;
    }
  }
  return FALSE;
}

/*************************************************************************
 * @   [SHLWAPI.330]
 *
 * Get the file extension for a given Mime type.
 *
 * PARAMS
 *  lpszType [I] Mime type to get the file extension for
 *  lpExt    [O] Destination for the resulting extension
 *  iLen     [I] Length of lpExt in characters
 *
 * RETURNS
 *  Success: TRUE. lpExt contains the file extension.
 *  Failure: FALSE, if any parameter is invalid or the extension cannot be
 *           retrieved. If iLen > 0, lpExt is set to an empty string.
 *
 * NOTES
 *  - The extension returned in lpExt always has a leading '.' character, even
 *  if the registry Mime database entry does not.
 *  - iLen must be long enough for the file extension for this function to succeed.
 */
BOOL WINAPI MIME_GetExtensionA(LPCSTR lpszType, LPSTR lpExt, INT iLen)
{
  char szSubKey[MAX_PATH];
  DWORD dwlen = iLen - 1, dwType;
  BOOL bRet = FALSE;

  if (iLen > 0 && lpExt)
    *lpExt = '\0';

  if (lpszType && lpExt && iLen > 2 &&
      GetMIMETypeSubKeyA(lpszType, szSubKey, MAX_PATH) &&
      !SHGetValueA(HKEY_CLASSES_ROOT, szSubKey, szExtensionA, &dwType, lpExt + 1, &dwlen) &&
      lpExt[1])
  {
    if (lpExt[1] == '.')
      memmove(lpExt, lpExt + 1, strlen(lpExt + 1) + 1);
    else
      *lpExt = '.'; /* Supply a '.' */
    bRet = TRUE;
  }
  return bRet;
}

/*************************************************************************
 * @   [SHLWAPI.331]
 *
 * Unicode version of MIME_GetExtensionA.
 */
BOOL WINAPI MIME_GetExtensionW(LPCWSTR lpszType, LPWSTR lpExt, INT iLen)
{
  WCHAR szSubKey[MAX_PATH];
  DWORD dwlen = iLen - 1, dwType;
  BOOL bRet = FALSE;

  if (iLen > 0 && lpExt)
    *lpExt = '\0';

  if (lpszType && lpExt && iLen > 2 &&
      GetMIMETypeSubKeyW(lpszType, szSubKey, MAX_PATH) &&
      !SHGetValueW(HKEY_CLASSES_ROOT, szSubKey, szExtensionW, &dwType, lpExt + 1, &dwlen) &&
      lpExt[1])
  {
    if (lpExt[1] == '.')
      memmove(lpExt, lpExt + 1, (lstrlenW(lpExt + 1) + 1) * sizeof(WCHAR));
    else
      *lpExt = '.'; /* Supply a '.' */
    bRet = TRUE;
  }
  return bRet;
}

/*************************************************************************
 * @   [SHLWAPI.324]
 *
 * Set the file extension for a MIME content key.
 *
 * PARAMS
 *   lpszExt  [I] File extension to set
 *   lpszType [I] Content type to set the extension for
 *
 * RETURNS
 *   Success: TRUE. The file extension is set in the registry.
 *   Failure: FALSE.
 */
BOOL WINAPI RegisterExtensionForMIMETypeA(LPCSTR lpszExt, LPCSTR lpszType)
{
  DWORD dwLen;
  char szKey[MAX_PATH];

  TRACE("(%s,%s)\n", debugstr_a(lpszExt), debugstr_a(lpszType));

  if (!GetMIMETypeSubKeyA(lpszType, szKey, MAX_PATH)) /* Get full path to the key */
    return FALSE;

  dwLen = strlen(lpszExt) + 1;

  if (SHSetValueA(HKEY_CLASSES_ROOT, szKey, szExtensionA, REG_SZ, lpszExt, dwLen))
    return FALSE;
  return TRUE;
}

/*************************************************************************
 * @   [SHLWAPI.325]
 *
 * Unicode version of RegisterExtensionForMIMETypeA.
 */
BOOL WINAPI RegisterExtensionForMIMETypeW(LPCWSTR lpszExt, LPCWSTR lpszType)
{
  DWORD dwLen;
  WCHAR szKey[MAX_PATH];

  TRACE("(%s,%s)\n", debugstr_w(lpszExt), debugstr_w(lpszType));

  /* Get the full path to the key */
  if (!GetMIMETypeSubKeyW(lpszType, szKey, MAX_PATH)) /* Get full path to the key */
    return FALSE;

  dwLen = (lstrlenW(lpszExt) + 1) * sizeof(WCHAR);

  if (SHSetValueW(HKEY_CLASSES_ROOT, szKey, szExtensionW, REG_SZ, lpszExt, dwLen))
    return FALSE;
  return TRUE;
}

/*************************************************************************
 * @   [SHLWAPI.326]
 *
 * Delete a file extension from a MIME content type.
 *
 * PARAMS
 *   lpszType [I] Content type to delete the extension for
 *
 * RETURNS
 *   Success: TRUE. The file extension is deleted from the registry.
 *   Failure: FALSE. The extension may have been removed but the key remains.
 *
 * NOTES
 *   If deleting the extension leaves an orphan key, the key is removed also.
 */
BOOL WINAPI UnregisterExtensionForMIMETypeA(LPCSTR lpszType)
{
  char szKey[MAX_PATH];

  TRACE("(%s)\n", debugstr_a(lpszType));

  if (!GetMIMETypeSubKeyA(lpszType, szKey, MAX_PATH)) /* Get full path to the key */
    return FALSE;

  if (!SHDeleteValueA(HKEY_CLASSES_ROOT, szKey, szExtensionA))
    return FALSE;

  if (!SHDeleteOrphanKeyA(HKEY_CLASSES_ROOT, szKey))
    return FALSE;
  return TRUE;
}

/*************************************************************************
 * @   [SHLWAPI.327]
 *
 * Unicode version of UnregisterExtensionForMIMETypeA.
 */
BOOL WINAPI UnregisterExtensionForMIMETypeW(LPCWSTR lpszType)
{
  WCHAR szKey[MAX_PATH];

  TRACE("(%s)\n", debugstr_w(lpszType));

  if (!GetMIMETypeSubKeyW(lpszType, szKey, MAX_PATH)) /* Get full path to the key */
    return FALSE;

  if (!SHDeleteValueW(HKEY_CLASSES_ROOT, szKey, szExtensionW))
    return FALSE;

  if (!SHDeleteOrphanKeyW(HKEY_CLASSES_ROOT, szKey))
    return FALSE;
  return TRUE;
}

/*************************************************************************
 * SHRegDuplicateHKey   [SHLWAPI.@]
 *
 * Create a duplicate of a registry handle.
 *
 * PARAMS
 *  hKey [I] key to duplicate.
 *
 * RETURNS
 *  A new handle pointing to the same key as hKey.
 */
HKEY WINAPI SHRegDuplicateHKey(HKEY hKey)
{
    HKEY newKey = 0;

    RegOpenKeyExA(hKey, 0, 0, MAXIMUM_ALLOWED, &newKey);
    TRACE("new key is %p\n", newKey);
    return newKey;
}

/*
 * The following functions are ORDINAL ONLY:
 */

/*************************************************************************
 *      @	[SHLWAPI.343]
 *
 * Create or open an explorer ClassId Key.
 *
 * PARAMS
 *  guid      [I] Explorer ClassId key to open
 *  lpszValue [I] Value name under the ClassId Key
 *  bUseHKCU  [I] TRUE=Use HKEY_CURRENT_USER, FALSE=Use HKEY_CLASSES_ROOT
 *  bCreate   [I] TRUE=Create the key if it doesn't exist, FALSE=Don't
 *  phKey     [O] Destination for the resulting key handle
 *
 * RETURNS
 *  Success: S_OK. phKey contains the resulting registry handle.
 *  Failure: An HRESULT error code indicating the problem.
 */
HRESULT WINAPI SHRegGetCLSIDKeyA(REFGUID guid, LPCSTR lpszValue, BOOL bUseHKCU, BOOL bCreate, PHKEY phKey)
{
  WCHAR szValue[MAX_PATH];

  if (lpszValue)
    MultiByteToWideChar(CP_ACP, 0, lpszValue, -1, szValue, ARRAY_SIZE(szValue));

  return SHRegGetCLSIDKeyW(guid, lpszValue ? szValue : NULL, bUseHKCU, bCreate, phKey);
}

/*************************************************************************
 *      @	[SHLWAPI.344]
 *
 * Unicode version of SHRegGetCLSIDKeyA.
 */
HRESULT WINAPI SHRegGetCLSIDKeyW(REFGUID guid, LPCWSTR lpszValue, BOOL bUseHKCU,
                           BOOL bCreate, PHKEY phKey)
{
  static const WCHAR szClassIdKey[] = { 'S','o','f','t','w','a','r','e','\\',
    'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\',
    'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
    'E','x','p','l','o','r','e','r','\\','C','L','S','I','D','\\' };
  WCHAR szKey[MAX_PATH];
  DWORD dwRet;
  HKEY hkey;

  /* Create the key string */
  memcpy(szKey, szClassIdKey, sizeof(szClassIdKey));
  SHStringFromGUIDW(guid, szKey + ARRAY_SIZE(szClassIdKey), 39); /* Append guid */

  if(lpszValue)
  {
    szKey[ARRAY_SIZE(szClassIdKey) + 39] = '\\';
    lstrcpyW(szKey + ARRAY_SIZE(szClassIdKey) + 40, lpszValue); /* Append value name */
  }

  hkey = bUseHKCU ? HKEY_CURRENT_USER : HKEY_CLASSES_ROOT;

  if(bCreate)
    dwRet = RegCreateKeyW(hkey, szKey, phKey);
  else
    dwRet = RegOpenKeyExW(hkey, szKey, 0, KEY_READ, phKey);

  return dwRet ? HRESULT_FROM_WIN32(dwRet) : S_OK;
}

/*************************************************************************
 * SHRegisterValidateTemplate	[SHLWAPI.@]
 *
 * observed from the ie 5.5 installer:
 *  - allocates a buffer with the size of the given file
 *  - read the file content into the buffer
 *  - creates the key szTemplateKey
 *  - sets "205523652929647911071668590831910975402"=dword:00002e37 at
 *    the key
 *
 * PARAMS
 *  filename [I] An existing file its content is read into an allocated
 *               buffer
 *  unknown  [I]
 *
 * RETURNS
 *  Success: ERROR_SUCCESS.
 */
HRESULT WINAPI SHRegisterValidateTemplate(LPCWSTR filename, BOOL unknown)
{
/* static const WCHAR szTemplateKey[] = { 'S','o','f','t','w','a','r','e','\\',
 *  'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\',
 *  'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
 *  'E','x','p','l','o','r','e','r','\\',
 *  'T','e','m','p','l','a','t','e','R','e','g','i','s','t','r','y',0 };
 */
  FIXME("stub: %s, %08x\n", debugstr_w(filename), unknown);

  return S_OK;
}
