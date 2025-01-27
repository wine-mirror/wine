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

static const char szMimeDbContentA[] = "MIME\\Database\\Content Type\\";
static const WCHAR szMimeDbContentW[] = L"MIME\\Database\\Content Type\\";
static const DWORD dwLenMimeDbContent = 27; /* strlen of szMimeDbContentA/W */

INT     WINAPI SHStringFromGUIDW(REFGUID,LPWSTR,INT);
HRESULT WINAPI SHRegGetCLSIDKeyW(REFGUID,LPCWSTR,BOOL,BOOL,PHKEY);

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
 * @   [SHLWAPI.205]
 *
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

  return !SHSetValueA(HKEY_CLASSES_ROOT, lpszSubKey, "Content Type",
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

  return !SHSetValueW(HKEY_CLASSES_ROOT, lpszSubKey, L"Content Type",
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
  return !SHDeleteValueA(HKEY_CLASSES_ROOT, lpszSubKey, "Content Type");
}

/*************************************************************************
 * @   [SHLWAPI.323]
 *
 * Unicode version of UnregisterMIMETypeForExtensionA.
 */
BOOL WINAPI UnregisterMIMETypeForExtensionW(LPCWSTR lpszSubKey)
{
  return !SHDeleteValueW(HKEY_CLASSES_ROOT, lpszSubKey, L"Content Type");
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
  TRACE("(%s,%p,%ld)\n", debugstr_a(lpszType), lpszBuffer, dwLen);

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
  TRACE("(%s,%p,%ld)\n", debugstr_w(lpszType), lpszBuffer, dwLen);

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
      !SHGetValueA(HKEY_CLASSES_ROOT, szSubKey, "Extension", &dwType, lpExt + 1, &dwlen) &&
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
      !SHGetValueW(HKEY_CLASSES_ROOT, szSubKey, L"Extension", &dwType, lpExt + 1, &dwlen) &&
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

  if (SHSetValueA(HKEY_CLASSES_ROOT, szKey, "Extension", REG_SZ, lpszExt, dwLen))
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

  if (SHSetValueW(HKEY_CLASSES_ROOT, szKey, L"Extension", REG_SZ, lpszExt, dwLen))
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

  if (!SHDeleteValueA(HKEY_CLASSES_ROOT, szKey, "Extension"))
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

  if (!SHDeleteValueW(HKEY_CLASSES_ROOT, szKey, L"Extension"))
    return FALSE;

  if (!SHDeleteOrphanKeyW(HKEY_CLASSES_ROOT, szKey))
    return FALSE;
  return TRUE;
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
