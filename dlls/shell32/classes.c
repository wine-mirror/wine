/*
 *	file type mapping
 *	(HKEY_CLASSES_ROOT - Stuff)
 *
 * Copyright 1998, 1999, 2000 Juergen Schmied
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

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "wine/debug.h"
#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"

#include "shlobj.h"
#include "shell32_main.h"
#include "shlguid.h"
#include "shresdef.h"
#include "shlwapi.h"
#include "pidl.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define MAX_EXTENSION_LENGTH 20

BOOL HCR_MapTypeToValueW(LPCWSTR szExtension, LPWSTR szFileType, LONG len, BOOL bPrependDot)
{	
	HKEY	hkey;
	WCHAR	szTemp[MAX_EXTENSION_LENGTH + 2];

	TRACE("%s %p\n", debugstr_w(szExtension), szFileType);

        /* added because we do not want to have double dots */
        if (szExtension[0] == '.')
          bPrependDot = FALSE;

	if (bPrependDot)
	  szTemp[0] = '.';

	lstrcpynW(szTemp + (bPrependDot?1:0), szExtension, MAX_EXTENSION_LENGTH);

	if (RegOpenKeyExW(HKEY_CLASSES_ROOT, szTemp, 0, KEY_READ, &hkey))
	{ 
	  return FALSE;
	}

	if (RegQueryValueW(hkey, NULL, szFileType, &len))
	{ 
	  RegCloseKey(hkey);
	  return FALSE;
	}

	RegCloseKey(hkey);

	TRACE("--UE;\n} %s\n", debugstr_w(szFileType));

	return TRUE;
}

BOOL HCR_MapTypeToValueA(LPCSTR szExtension, LPSTR szFileType, LONG len, BOOL bPrependDot)
{
	HKEY	hkey;
	char	szTemp[MAX_EXTENSION_LENGTH + 2];

	TRACE("%s %p\n", szExtension, szFileType);

        /* added because we do not want to have double dots */
        if (szExtension[0] == '.')
          bPrependDot = FALSE;

	if (bPrependDot)
	  szTemp[0] = '.';

	lstrcpynA(szTemp + (bPrependDot?1:0), szExtension, MAX_EXTENSION_LENGTH);

	if (RegOpenKeyExA(HKEY_CLASSES_ROOT, szTemp, 0, KEY_READ, &hkey))
	{ 
	  return FALSE;
	}

	if (RegQueryValueA(hkey, NULL, szFileType, &len))
	{ 
	  RegCloseKey(hkey);
	  return FALSE;
	}

	RegCloseKey(hkey);

	TRACE("--UE;\n} %s\n", szFileType);

	return TRUE;
}

BOOL HCR_GetDefaultVerbW( HKEY hkeyClass, LPCWSTR szVerb, LPWSTR szDest, DWORD len )
{
        WCHAR sTemp[MAX_PATH];
        LONG size;
        HKEY hkey;

	TRACE("%p %s %p\n", hkeyClass, debugstr_w(szVerb), szDest);

        if (szVerb && *szVerb)
        {
            lstrcpynW(szDest, szVerb, len);
            return TRUE;
        }

        size=len;
        *szDest='\0';
        if (!RegQueryValueW(hkeyClass, L"shell\\", szDest, &size) && *szDest)
        {
            /* The MSDN says to first try the default verb */
            lstrcpyW(sTemp, L"shell\\");
            lstrcatW(sTemp, szDest);
            lstrcatW(sTemp, L"\\command");
            if (!RegOpenKeyExW(hkeyClass, sTemp, 0, KEY_ENUMERATE_SUB_KEYS, &hkey))
            {
                RegCloseKey(hkey);
                TRACE("default verb=%s\n", debugstr_w(szDest));
                return TRUE;
            }
        }

        /* then fallback to 'open' */
        lstrcpyW(sTemp, L"shell\\open\\command");
        if (!RegOpenKeyExW(hkeyClass, sTemp, 0, KEY_ENUMERATE_SUB_KEYS, &hkey))
        {
            RegCloseKey(hkey);
            lstrcpynW(szDest, L"open", len);
            TRACE("default verb=open\n");
            return TRUE;
        }

        /* and then just use the first verb on Windows >= 2000 */
        if (!RegEnumKeyW(hkeyClass, 0, szDest, len) && *szDest)
        {
            TRACE("default verb=first verb=%s\n", debugstr_w(szDest));
            return TRUE;
        }

        TRACE("no default verb!\n");
	return FALSE;
}

BOOL HCR_GetExecuteCommandW( HKEY hkeyClass, LPCWSTR szClass, LPCWSTR szVerb, LPWSTR szDest, DWORD len )
{
	WCHAR sTempVerb[MAX_PATH];
	BOOL ret;

	TRACE("%p %s %s %p\n", hkeyClass, debugstr_w(szClass), debugstr_w(szVerb), szDest);

	if (szClass)
            RegOpenKeyExW(HKEY_CLASSES_ROOT, szClass, 0, KEY_READ, &hkeyClass);
        if (!hkeyClass)
            return FALSE;
        ret = FALSE;

        if (HCR_GetDefaultVerbW(hkeyClass, szVerb, sTempVerb, ARRAY_SIZE(sTempVerb)))
        {
            WCHAR sTemp[MAX_PATH];
            lstrcpyW(sTemp, L"shell\\");
            lstrcatW(sTemp, sTempVerb);
            lstrcatW(sTemp, L"\\command");
            ret = (ERROR_SUCCESS == SHGetValueW(hkeyClass, sTemp, NULL, NULL, szDest, &len));
        }
        if (szClass)
            RegCloseKey(hkeyClass);

	TRACE("-- %s\n", debugstr_w(szDest) );
	return ret;
}

/***************************************************************************************
*	HCR_GetDefaultIcon	[internal]
*
* Gets the icon for a filetype
*/
static BOOL HCR_RegOpenClassIDKey(REFIID riid, HKEY *hkey)
{
	char	xriid[50];
    sprintf( xriid, "CLSID\\{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                 riid->Data1, riid->Data2, riid->Data3,
                 riid->Data4[0], riid->Data4[1], riid->Data4[2], riid->Data4[3],
                 riid->Data4[4], riid->Data4[5], riid->Data4[6], riid->Data4[7] );

 	TRACE("%s\n",xriid );

	return !RegOpenKeyExA(HKEY_CLASSES_ROOT, xriid, 0, KEY_READ, hkey);
}

static BOOL HCR_RegGetDefaultIconW(HKEY hkey, LPWSTR szDest, DWORD len, int* picon_idx)
{
    DWORD dwType, size = len * sizeof(WCHAR);
    WCHAR sTemp[MAX_PATH];
    WCHAR sNum[5];

    if (!RegQueryValueExW(hkey, NULL, 0, &dwType, (LPBYTE)szDest, &size))
    {
      if (dwType == REG_EXPAND_SZ)
      {
        ExpandEnvironmentStringsW(szDest, sTemp, MAX_PATH);
        lstrcpynW(szDest, sTemp, len);
      }
      if (ParseFieldW (szDest, 2, sNum, 5))
          *picon_idx = wcstol(sNum, NULL, 10);
      else
          *picon_idx=0; /* sometimes the icon number is missing */
      ParseFieldW (szDest, 1, szDest, len);
          PathUnquoteSpacesW(szDest);
      return TRUE;
    }
    return FALSE;
}

static BOOL HCR_RegGetDefaultIconA(HKEY hkey, LPSTR szDest, DWORD len, int* picon_idx)
{
	DWORD dwType;
	char sTemp[MAX_PATH];
	char  sNum[5];

	if (!RegQueryValueExA(hkey, NULL, 0, &dwType, (LPBYTE)szDest, &len))
	{
          if (dwType == REG_EXPAND_SZ)
	  {
	    ExpandEnvironmentStringsA(szDest, sTemp, MAX_PATH);
	    lstrcpynA(szDest, sTemp, len);
	  }
	  if (ParseFieldA (szDest, 2, sNum, 5))
             *picon_idx=atoi(sNum);
          else
             *picon_idx=0; /* sometimes the icon number is missing */
	  ParseFieldA (szDest, 1, szDest, len);
          PathUnquoteSpacesA(szDest);
	  return TRUE;
	}
	return FALSE;
}

BOOL HCR_GetDefaultIconW(LPCWSTR szClass, LPWSTR szDest, DWORD len, int* picon_idx)
{
	HKEY	hkey;
	WCHAR	sTemp[MAX_PATH];
	BOOL	ret = FALSE;

	TRACE("%s\n",debugstr_w(szClass) );

	lstrcpynW(sTemp, szClass, MAX_PATH);
	lstrcatW(sTemp, L"\\DefaultIcon");

	if (!RegOpenKeyExW(HKEY_CLASSES_ROOT, sTemp, 0, KEY_READ, &hkey))
	{
	  ret = HCR_RegGetDefaultIconW(hkey, szDest, len, picon_idx);
	  RegCloseKey(hkey);
	}

        if(ret)
            TRACE("-- %s %i\n", debugstr_w(szDest), *picon_idx);
        else
            TRACE("-- not found\n");

	return ret;
}

BOOL HCR_GetDefaultIconA(LPCSTR szClass, LPSTR szDest, DWORD len, int* picon_idx)
{
	HKEY	hkey;
	char	sTemp[MAX_PATH];
	BOOL	ret = FALSE;

	TRACE("%s\n",szClass );

	sprintf(sTemp, "%s\\DefaultIcon",szClass);

	if (!RegOpenKeyExA(HKEY_CLASSES_ROOT, sTemp, 0, KEY_READ, &hkey))
	{
	  ret = HCR_RegGetDefaultIconA(hkey, szDest, len, picon_idx);
	  RegCloseKey(hkey);
	}

	if (ret)
	  TRACE("-- %s %i\n", szDest, *picon_idx);
	else
	  TRACE("-- not found\n");

	return ret;
}

/***************************************************************************************
*	HCR_GetClassName	[internal]
*
* Gets the name of a registered class
*/
BOOL HCR_GetClassNameW(REFIID riid, LPWSTR szDest, DWORD len)
{	
	HKEY	hkey;
	BOOL ret = FALSE;
	DWORD buflen = len * sizeof(WCHAR);

 	szDest[0] = 0;
	if (HCR_RegOpenClassIDKey(riid, &hkey))
	{
          if (!RegLoadMUIStringW(hkey, L"LocalizedString", szDest, buflen, NULL, 0, NULL) ||
              !RegQueryValueExW(hkey, L"", 0, NULL, (LPBYTE)szDest, &buflen))
          {
	    ret = TRUE;
	  }
	  RegCloseKey(hkey);
	}

	if (!ret || !szDest[0])
	{
	  if(IsEqualIID(riid, &CLSID_ShellDesktop))
	  {
	    if (LoadStringW(shell32_hInstance, IDS_DESKTOP, szDest, len))
	      ret = TRUE;
	  }
	  else if (IsEqualIID(riid, &CLSID_MyComputer))
	  {
	    if(LoadStringW(shell32_hInstance, IDS_MYCOMPUTER, szDest, len))
	      ret = TRUE;
	  }
	}
	TRACE("-- %s\n", debugstr_w(szDest));
	return ret;
}

BOOL HCR_GetClassNameA(REFIID riid, LPSTR szDest, DWORD len)
{	HKEY	hkey;
	BOOL ret = FALSE;
	DWORD buflen = len;

	szDest[0] = 0;
	if (HCR_RegOpenClassIDKey(riid, &hkey))
	{
          if (!RegLoadMUIStringA(hkey,"LocalizedString",szDest,len,NULL,0,NULL) ||
              !RegQueryValueExA(hkey,"",0,NULL,(LPBYTE)szDest,&len))
          {
	    ret = TRUE;
	  }
	  RegCloseKey(hkey);
	}

	if (!ret || !szDest[0])
	{
	  if(IsEqualIID(riid, &CLSID_ShellDesktop))
	  {
	    if (LoadStringA(shell32_hInstance, IDS_DESKTOP, szDest, buflen))
	      ret = TRUE;
	  }
	  else if (IsEqualIID(riid, &CLSID_MyComputer))
	  {
	    if(LoadStringA(shell32_hInstance, IDS_MYCOMPUTER, szDest, buflen))
	      ret = TRUE;
	  }
	}

	TRACE("-- (%s)\n", szDest);

	return ret;
}

/******************************************************************************
 * HCR_GetFolderAttributes [Internal]
 *
 * Query the registry for a shell folders' attributes
 *
 * PARAMS
 *  pidlFolder    [I]  A simple pidl of type PT_GUID. 
 *  pdwAttributes [IO] In: Attributes to be queried, OUT: Resulting attributes.
 *
 * RETURNS
 *  TRUE:  Found information for the attributes in the registry
 *  FALSE: No attribute information found
 *
 * NOTES
 *  If queried for an attribute, which is set in the CallForAttributes registry
 *  value, the function binds to the shellfolder objects and queries it.
 */
BOOL HCR_GetFolderAttributes(LPCITEMIDLIST pidlFolder, LPDWORD pdwAttributes)
{
    HKEY hSFKey;
    LPOLESTR pwszCLSID;
    LONG lResult;
    DWORD dwTemp, dwLen;
    WCHAR wszShellFolderKey[] = L"CLSID\\{00021400-0000-0000-C000-000000000046}\\ShellFolder";

    TRACE("(pidlFolder=%p, pdwAttributes=%p)\n", pidlFolder, pdwAttributes);
       
    if (!_ILIsPidlSimple(pidlFolder)) { 
        static BOOL firstHit = TRUE;
        if (firstHit) {
            ERR("should be called for simple PIDL's only!\n");
            firstHit = FALSE;
        }
        return FALSE;
    }
    
    if (!_ILIsDesktop(pidlFolder)) {
        if (FAILED(StringFromCLSID(_ILGetGUIDPointer(pidlFolder), &pwszCLSID))) return FALSE;
        memcpy(&wszShellFolderKey[6], pwszCLSID, 38 * sizeof(WCHAR));
        CoTaskMemFree(pwszCLSID);
    }
    
    lResult = RegOpenKeyExW(HKEY_CLASSES_ROOT, wszShellFolderKey, 0, KEY_READ, &hSFKey);
    if (lResult != ERROR_SUCCESS) return FALSE;
    
    dwLen = sizeof(DWORD);
    lResult = RegQueryValueExW(hSFKey, L"CallForAttributes", 0, NULL, (LPBYTE)&dwTemp, &dwLen);
    if ((lResult == ERROR_SUCCESS) && (dwTemp & *pdwAttributes)) {
        LPSHELLFOLDER psfDesktop, psfFolder;
        HRESULT hr;

        RegCloseKey(hSFKey);
        hr = SHGetDesktopFolder(&psfDesktop);
        if (SUCCEEDED(hr)) {
            hr = IShellFolder_BindToObject(psfDesktop, pidlFolder, NULL, &IID_IShellFolder, 
                                           (LPVOID*)&psfFolder);
            if (SUCCEEDED(hr)) { 
                hr = IShellFolder_GetAttributesOf(psfFolder, 0, NULL, pdwAttributes);
                IShellFolder_Release(psfFolder);
            }
            IShellFolder_Release(psfDesktop);
        }
        if (FAILED(hr)) return FALSE;
    } else {
        lResult = RegQueryValueExW(hSFKey, L"Attributes", 0, NULL, (LPBYTE)&dwTemp, &dwLen);
        RegCloseKey(hSFKey);
        if (lResult == ERROR_SUCCESS) {
            *pdwAttributes &= dwTemp;
        } else {
            return FALSE;
        }
    }

    TRACE("-- *pdwAttributes == 0x%08lx\n", *pdwAttributes);

    return TRUE;
}
