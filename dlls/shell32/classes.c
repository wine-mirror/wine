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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "wine/debug.h"
#include "winerror.h"
#include "winreg.h"

#include "shlobj.h"
#include "shell32_main.h"
#include "shlguid.h"
#include "shresdef.h"
#include "shlwapi.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define MAX_EXTENSION_LENGTH 20

BOOL HCR_MapTypeToValueW(LPCWSTR szExtension, LPWSTR szFileType, DWORD len, BOOL bPrependDot)
{	
	HKEY	hkey;
	WCHAR	szTemp[MAX_EXTENSION_LENGTH + 2];

	TRACE("%s %p\n", debugstr_w(szExtension), debugstr_w(szFileType));

    /* added because we do not want to have double dots */
    if (szExtension[0] == '.')
        bPrependDot = 0;

	if (bPrependDot)
	  szTemp[0] = '.';

	lstrcpynW(szTemp + (bPrependDot?1:0), szExtension, MAX_EXTENSION_LENGTH);

	if (RegOpenKeyExW(HKEY_CLASSES_ROOT, szTemp, 0, 0x02000000, &hkey))
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

BOOL HCR_MapTypeToValueA(LPCSTR szExtension, LPSTR szFileType, DWORD len, BOOL bPrependDot)
{
	HKEY	hkey;
	char	szTemp[MAX_EXTENSION_LENGTH + 2];

	TRACE("%s %p\n", szExtension, szFileType);

    /* added because we do not want to have double dots */
    if (szExtension[0] == '.')
        bPrependDot = 0;

	if (bPrependDot)
	  szTemp[0] = '.';

	lstrcpynA(szTemp + (bPrependDot?1:0), szExtension, MAX_EXTENSION_LENGTH);

	if (RegOpenKeyExA(HKEY_CLASSES_ROOT, szTemp, 0, 0x02000000, &hkey))
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


BOOL HCR_GetExecuteCommandW(LPCWSTR szClass, LPCWSTR szVerb, LPWSTR szDest, DWORD len)
{
        static const WCHAR swShell[] = {'\\','s','h','e','l','l','\\',0};
        static const WCHAR swCommand[] = {'\\','c','o','m','m','a','n','d',0};
	WCHAR	sTemp[MAX_PATH];

	TRACE("%s %s %p\n",debugstr_w(szClass), debugstr_w(szVerb), szDest);

	lstrcpyW(sTemp, szClass);
	lstrcatW(sTemp, swShell);
	lstrcatW(sTemp, szVerb);
	lstrcatW(sTemp, swCommand);

	if (ERROR_SUCCESS == SHGetValueW(HKEY_CLASSES_ROOT, sTemp, NULL, NULL, szDest, &len)) {
	    TRACE("-- %s\n", debugstr_w(szDest) );
	    return TRUE;
	}
	return FALSE;
}

BOOL HCR_GetExecuteCommandA(LPCSTR szClass, LPCSTR szVerb, LPSTR szDest, DWORD len)
{
	char	sTemp[MAX_PATH];

	TRACE("%s %s\n",szClass, szVerb );

	snprintf(sTemp, MAX_PATH, "%s\\shell\\%s\\command",szClass, szVerb);

	if (ERROR_SUCCESS == SHGetValueA(HKEY_CLASSES_ROOT, sTemp, NULL, NULL, szDest, &len)) {
	    TRACE("-- %s\n", debugstr_a(szDest) );
	    return TRUE;
	}
	return FALSE;
}

BOOL HCR_GetExecuteCommandEx( HKEY hkeyClass, LPCSTR szClass, LPCSTR szVerb, LPSTR szDest, DWORD len )
{
	BOOL	ret = FALSE;

	TRACE("%p %s %s\n", hkeyClass, szClass, szVerb );

	if (szClass)
            RegOpenKeyExA(hkeyClass,szClass,0,0x02000000,&hkeyClass);

        if (hkeyClass)
	{
	    char sTemp[MAX_PATH];

	    snprintf(sTemp, MAX_PATH, "shell\\%s\\command", szVerb);

            ret = (ERROR_SUCCESS == SHGetValueA(hkeyClass, sTemp, NULL, NULL, szDest, &len));

	    if (szClass)
	       RegCloseKey(hkeyClass);
	}

	TRACE("-- %s\n", szDest );
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

static BOOL HCR_RegGetDefaultIconW(HKEY hkey, LPWSTR szDest, DWORD len, LPDWORD dwNr)
{
	DWORD dwType;
	WCHAR sTemp[MAX_PATH];
	WCHAR sNum[5];

	if (!RegQueryValueExW(hkey, NULL, 0, &dwType, (LPBYTE)szDest, &len))
	{
      if (dwType == REG_EXPAND_SZ)
	  {
	    ExpandEnvironmentStringsW(szDest, sTemp, MAX_PATH);
	    lstrcpynW(szDest, sTemp, len);
	  }
	  if (ParseFieldW (szDest, 2, sNum, 5))
             *dwNr = atoiW(sNum);
          else
             *dwNr=0; /* sometimes the icon number is missing */
	  ParseFieldW (szDest, 1, szDest, len);
	  return TRUE;
	}
	return FALSE;
}

static BOOL HCR_RegGetDefaultIconA(HKEY hkey, LPSTR szDest, DWORD len, LPDWORD dwNr)
{
	DWORD dwType;
	char sTemp[MAX_PATH];
	char  sNum[5];

	if (!RegQueryValueExA(hkey, NULL, 0, &dwType, szDest, &len))
	{
      if (dwType == REG_EXPAND_SZ)
	  {
	    ExpandEnvironmentStringsA(szDest, sTemp, MAX_PATH);
	    lstrcpynA(szDest, sTemp, len);
	  }
	  if (ParseFieldA (szDest, 2, sNum, 5))
             *dwNr=atoi(sNum);
          else
             *dwNr=0; /* sometimes the icon number is missing */
	  ParseFieldA (szDest, 1, szDest, len);
	  return TRUE;
	}
	return FALSE;
}

BOOL HCR_GetDefaultIconW(LPCWSTR szClass, LPWSTR szDest, DWORD len, LPDWORD dwNr)
{
        static const WCHAR swDefaultIcon[] = {'\\','D','e','f','a','u','l','t','I','c','o','n',0};
	HKEY	hkey;
	WCHAR	sTemp[MAX_PATH];
	BOOL	ret = FALSE;

	TRACE("%s\n",debugstr_w(szClass) );

	lstrcpynW(sTemp, szClass, MAX_PATH);
	lstrcatW(sTemp, swDefaultIcon);

	if (!RegOpenKeyExW(HKEY_CLASSES_ROOT, sTemp, 0, 0x02000000, &hkey))
	{
	  ret = HCR_RegGetDefaultIconW(hkey, szDest, len, dwNr);
	  RegCloseKey(hkey);
	}
	TRACE("-- %s %li\n", debugstr_w(szDest), *dwNr );
	return ret;
}

BOOL HCR_GetDefaultIconA(LPCSTR szClass, LPSTR szDest, DWORD len, LPDWORD dwNr)
{
	HKEY	hkey;
	char	sTemp[MAX_PATH];
	BOOL	ret = FALSE;

	TRACE("%s\n",szClass );

	sprintf(sTemp, "%s\\DefaultIcon",szClass);

	if (!RegOpenKeyExA(HKEY_CLASSES_ROOT, sTemp, 0, 0x02000000, &hkey))
	{
	  ret = HCR_RegGetDefaultIconA(hkey, szDest, len, dwNr);
	  RegCloseKey(hkey);
	}
	TRACE("-- %s %li\n", szDest, *dwNr );
	return ret;
}

BOOL HCR_GetDefaultIconFromGUIDW(REFIID riid, LPWSTR szDest, DWORD len, LPDWORD dwNr)
{
	HKEY	hkey;
	BOOL	ret = FALSE;

	if (HCR_RegOpenClassIDKey(riid, &hkey))
	{
	  ret = HCR_RegGetDefaultIconW(hkey, szDest, len, dwNr);
	  RegCloseKey(hkey);
	}
	TRACE("-- %s %li\n", debugstr_w(szDest), *dwNr );
	return ret;
}

/***************************************************************************************
*	HCR_GetClassName	[internal]
*
* Gets the name of a registred class
*/
static WCHAR swEmpty[] = {0};

BOOL HCR_GetClassNameW(REFIID riid, LPWSTR szDest, DWORD len)
{	
	HKEY	hkey;
	BOOL ret = FALSE;
	DWORD buflen = len;

 	szDest[0] = 0;
	if (HCR_RegOpenClassIDKey(riid, &hkey))
	{
	  if (!RegQueryValueExW(hkey, swEmpty, 0, NULL, (LPBYTE)szDest, &len))
	  {
	    ret = TRUE;
	  }
	  RegCloseKey(hkey);
	}

	if (!ret || !szDest[0])
	{
	  if(IsEqualIID(riid, &CLSID_ShellDesktop))
	  {
	    if (LoadStringW(shell32_hInstance, IDS_DESKTOP, szDest, buflen))
	      ret = TRUE;
	  }
	  else if (IsEqualIID(riid, &CLSID_MyComputer))
	  {
	    if(LoadStringW(shell32_hInstance, IDS_MYCOMPUTER, szDest, buflen))
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
	  if (!RegQueryValueExA(hkey,"",0,NULL,szDest,&len))
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

	TRACE("-- %s\n", szDest);

	return ret;
}

/***************************************************************************************
*	HCR_GetFolderAttributes	[internal]
*
* gets the folder attributes of a class
*
* FIXME
*	verify the defaultvalue for *szDest
*/
BOOL HCR_GetFolderAttributes (REFIID riid, LPDWORD szDest)
{	HKEY	hkey;
	char	xriid[60];
	DWORD	attributes;
	DWORD	len = 4;

        sprintf( xriid, "CLSID\\{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                 riid->Data1, riid->Data2, riid->Data3,
                 riid->Data4[0], riid->Data4[1], riid->Data4[2], riid->Data4[3],
                 riid->Data4[4], riid->Data4[5], riid->Data4[6], riid->Data4[7] );
	TRACE("%s\n",xriid );

	if (!szDest) return FALSE;
	*szDest = SFGAO_FOLDER|SFGAO_FILESYSTEM;

	strcat (xriid, "\\ShellFolder");

	if (RegOpenKeyExA(HKEY_CLASSES_ROOT,xriid,0,KEY_READ,&hkey))
	{
	  return FALSE;
	}

	if (RegQueryValueExA(hkey,"Attributes",0,NULL,(LPBYTE)&attributes,&len))
	{
	  RegCloseKey(hkey);
	  return FALSE;
	}

	RegCloseKey(hkey);

	TRACE("-- 0x%08lx\n", attributes);

	*szDest = attributes;

	return TRUE;
}

typedef struct
{	ICOM_VFIELD(IQueryAssociations);
	DWORD	ref;
} IQueryAssociationsImpl;

static struct ICOM_VTABLE(IQueryAssociations) qavt;

/**************************************************************************
*  IQueryAssociations_Constructor
*/
IQueryAssociations* IQueryAssociations_Constructor(void)
{
	IQueryAssociationsImpl* ei;

	ei=(IQueryAssociationsImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IQueryAssociationsImpl));
	ei->ref=1;
	ei->lpVtbl = &qavt;

	TRACE("(%p)\n",ei);
	return (IQueryAssociations *)ei;
}
/**************************************************************************
 *  IQueryAssociations_QueryInterface
 */
static HRESULT WINAPI IQueryAssociations_fnQueryInterface(
	IQueryAssociations * iface,
	REFIID riid,
	LPVOID *ppvObj)
{
	ICOM_THIS(IQueryAssociationsImpl,iface);

	 TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,debugstr_guid(riid),ppvObj);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))		/*IUnknown*/
	{
	  *ppvObj = This;
	}
	else if(IsEqualIID(riid, &IID_IQueryAssociations))	/*IExtractIcon*/
	{
	  *ppvObj = (IQueryAssociations*)This;
	}

	if(*ppvObj)
	{
	  IQueryAssociations_AddRef((IQueryAssociations*) *ppvObj);
	  TRACE("-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE("-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}

/**************************************************************************
*  IQueryAssociations_AddRef
*/
static ULONG WINAPI IQueryAssociations_fnAddRef(IQueryAssociations * iface)
{
	ICOM_THIS(IQueryAssociationsImpl,iface);

	TRACE("(%p)->(count=%lu)\n",This, This->ref );

	return ++(This->ref);
}
/**************************************************************************
*  IQueryAssociations_Release
*/
static ULONG WINAPI IQueryAssociations_fnRelease(IQueryAssociations * iface)
{
	ICOM_THIS(IQueryAssociationsImpl,iface);

	TRACE("(%p)->()\n",This);

	if (!--(This->ref))
	{
	  TRACE(" destroying IExtractIcon(%p)\n",This);
	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}
	return This->ref;
}

static HRESULT WINAPI IQueryAssociations_fnInit(
	IQueryAssociations * iface,
	ASSOCF flags,
	LPCWSTR pszAssoc,
	HKEY hkProgid,
	HWND hwnd)
{
	return E_NOTIMPL;
}

static HRESULT WINAPI IQueryAssociations_fnGetString(
	IQueryAssociations * iface,
	ASSOCF flags,
	ASSOCSTR str,
	LPCWSTR pszExtra,
	LPWSTR pszOut,
	DWORD *pcchOut)
{
	return E_NOTIMPL;
}

static HRESULT WINAPI IQueryAssociations_fnGetKey(
	IQueryAssociations * iface,
	ASSOCF flags,
	ASSOCKEY key,
	LPCWSTR pszExtra,
	HKEY *phkeyOut)
{
	return E_NOTIMPL;
}

static HRESULT WINAPI IQueryAssociations_fnGetData(
	IQueryAssociations * iface,
	ASSOCF flags,
	ASSOCDATA data,
	LPCWSTR pszExtra,
	LPVOID pvOut,
	DWORD *pcbOut)
{
	return E_NOTIMPL;
}
static HRESULT WINAPI IQueryAssociations_fnGetEnum(
	IQueryAssociations * iface,
	ASSOCF flags,
	ASSOCENUM assocenum,
	LPCWSTR pszExtra,
	REFIID riid,
	LPVOID *ppvOut)
{
	return E_NOTIMPL;
}

static struct ICOM_VTABLE(IQueryAssociations) qavt =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IQueryAssociations_fnQueryInterface,
	IQueryAssociations_fnAddRef,
	IQueryAssociations_fnRelease,
	IQueryAssociations_fnInit,
	IQueryAssociations_fnGetString,
	IQueryAssociations_fnGetKey,
	IQueryAssociations_fnGetData,
	IQueryAssociations_fnGetEnum
};
