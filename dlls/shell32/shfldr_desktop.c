
/*
 *	Virtual Desktop Folder
 *
 *	Copyright 1997			Marcus Meissner
 *	Copyright 1998, 1999, 2002	Juergen Schmied
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

#include "winerror.h"
#include "winbase.h"
#include "winreg.h"

#include "ole2.h"
#include "shlguid.h"

#include "pidl.h"
#include "undocshell.h"
#include "shell32_main.h"
#include "shresdef.h"
#include "shlwapi.h"
#include "shellfolder.h"
#include "wine/debug.h"
#include "debughlp.h"
#include "shfldr.h"

WINE_DEFAULT_DEBUG_CHANNEL (shell);

/***********************************************************************
* 	Desktopfolder implementation
*/

typedef struct {
    ICOM_VFIELD (IShellFolder2);
    DWORD ref;

    CLSID *pclsid;

    /* both paths are parsible from the desktop */
    LPSTR sPathTarget;		/* complete path to target used for enumeration and ChangeNotify */
    LPITEMIDLIST pidlRoot;	/* absolute pidl */

    int dwAttributes;		/* attributes returned by GetAttributesOf FIXME: use it */

    UINT cfShellIDList;		/* clipboardformat for IDropTarget */
    BOOL fAcceptFmt;		/* flag for pending Drop */
} IGenericSFImpl;

#define _IUnknown_(This)	(IShellFolder*)&(This->lpVtbl)
#define _IShellFolder_(This)	(IShellFolder*)&(This->lpVtbl)

HRESULT WINAPI ISF_MyComputer_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);

static struct ICOM_VTABLE (IShellFolder2) vt_MCFldr_ShellFolder2;

static shvheader DesktopSFHeader[] = {
    {IDS_SHV_COLUMN1, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15},
    {IDS_SHV_COLUMN2, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN3, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10},
    {IDS_SHV_COLUMN4, SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 12},
    {IDS_SHV_COLUMN5, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 5}
};

#define DESKTOPSHELLVIEWCOLUMNS 5

/**************************************************************************
*	ISF_Desktop_Constructor
*/
HRESULT WINAPI ISF_Desktop_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv)
{
    IGenericSFImpl *sf;
    char szMyPath[MAX_PATH];

    TRACE ("unkOut=%p %s\n", pUnkOuter, shdebugstr_guid (riid));

    if (!ppv)
	return E_POINTER;
    if (pUnkOuter)
	return CLASS_E_NOAGGREGATION;

    if (!SHGetSpecialFolderPathA (0, szMyPath, CSIDL_DESKTOPDIRECTORY, TRUE))
	return E_UNEXPECTED;

    sf = (IGenericSFImpl *) LocalAlloc (GMEM_ZEROINIT, sizeof (IGenericSFImpl));
    if (!sf)
	return E_OUTOFMEMORY;

    sf->ref = 0;
    ICOM_VTBL (sf) = &vt_MCFldr_ShellFolder2;
    sf->pidlRoot = _ILCreateDesktop ();	/* my qualified pidl */
    sf->sPathTarget = SHAlloc (strlen (szMyPath) + 1);
    lstrcpyA (sf->sPathTarget, szMyPath);

    if (!SUCCEEDED (IUnknown_QueryInterface (_IUnknown_ (sf), riid, ppv))) {
	IUnknown_Release (_IUnknown_ (sf));
	return E_NOINTERFACE;
    }

    TRACE ("--(%p)\n", sf);
    return S_OK;
}

/**************************************************************************
 *	ISF_Desktop_fnQueryInterface
 *
 * NOTES supports not IPersist/IPersistFolder
 */
static HRESULT WINAPI ISF_Desktop_fnQueryInterface (IShellFolder2 * iface, REFIID riid, LPVOID * ppvObj)
{
    ICOM_THIS (IGenericSFImpl, iface);

    TRACE ("(%p)->(%s,%p)\n", This, shdebugstr_guid (riid), ppvObj);

    *ppvObj = NULL;

    if (IsEqualIID (riid, &IID_IUnknown) || IsEqualIID (riid, &IID_IShellFolder)
	|| IsEqualIID (riid, &IID_IShellFolder2)) {
	*ppvObj = This;
    }

    if (*ppvObj) {
	IUnknown_AddRef ((IUnknown *) (*ppvObj));
	TRACE ("-- Interface: (%p)->(%p)\n", ppvObj, *ppvObj);
	return S_OK;
    }
    TRACE ("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ISF_Desktop_fnAddRef (IShellFolder2 * iface)
{
    ICOM_THIS (IGenericSFImpl, iface);

    TRACE ("(%p)->(count=%lu)\n", This, This->ref);

    return ++(This->ref);
}

static ULONG WINAPI ISF_Desktop_fnRelease (IShellFolder2 * iface)
{
    ICOM_THIS (IGenericSFImpl, iface);

    TRACE ("(%p)->(count=%lu)\n", This, This->ref);

    if (!--(This->ref)) {
	TRACE ("-- destroying IShellFolder(%p)\n", This);
	if (This->pidlRoot)
	    SHFree (This->pidlRoot);
	if (This->sPathTarget)
	    SHFree (This->sPathTarget);
	LocalFree ((HLOCAL) This);
        return 0;
    }
    return This->ref;
}

/**************************************************************************
*	ISF_Desktop_fnParseDisplayName
*
* NOTES
*	"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}" and "" binds
*	to MyComputer
*/
static HRESULT WINAPI ISF_Desktop_fnParseDisplayName (IShellFolder2 * iface,
						      HWND hwndOwner,
						      LPBC pbcReserved,
						      LPOLESTR lpszDisplayName,
						      DWORD * pchEaten, LPITEMIDLIST * ppidl, DWORD * pdwAttributes)
{
    ICOM_THIS (IGenericSFImpl, iface);

    WCHAR szElement[MAX_PATH];
    LPCWSTR szNext = NULL;
    LPITEMIDLIST pidlTemp = NULL;
    HRESULT hr = E_OUTOFMEMORY;
    CLSID clsid;

    TRACE ("(%p)->(HWND=%p,%p,%p=%s,%p,pidl=%p,%p)\n",
	   This, hwndOwner, pbcReserved, lpszDisplayName, debugstr_w (lpszDisplayName), pchEaten, ppidl, pdwAttributes);

    *ppidl = 0;
    if (pchEaten)
	*pchEaten = 0;		/* strange but like the original */

    if (lpszDisplayName[0] == ':' && lpszDisplayName[1] == ':') {
	szNext = GetNextElementW (lpszDisplayName, szElement, MAX_PATH);
	TRACE ("-- element: %s\n", debugstr_w (szElement));
	CLSIDFromString (szElement + 2, &clsid);
	pidlTemp = _ILCreate (PT_MYCOMP, &clsid, sizeof (clsid));
    } else if (PathGetDriveNumberW (lpszDisplayName) >= 0) {
	/* it's a filesystem path with a drive. Let MyComputer parse it */
	pidlTemp = _ILCreateMyComputer ();
	szNext = lpszDisplayName;
    } else {
	/* it's a filesystem path on the desktop. Let a FSFolder parse it */
	WCHAR szCompletePath[MAX_PATH];

	/* build a complete path to create a simpel pidl */
	MultiByteToWideChar (CP_ACP, 0, This->sPathTarget, -1, szCompletePath, MAX_PATH);
	PathAddBackslashW (szCompletePath);
	lstrcatW (szCompletePath, lpszDisplayName);
	pidlTemp = SHSimpleIDListFromPathW (lpszDisplayName);
	szNext = lpszDisplayName;
    }

    if (pidlTemp) {
	if (szNext && *szNext) {
	    hr = SHELL32_ParseNextElement (hwndOwner, iface, &pidlTemp, (LPOLESTR) szNext, pchEaten, pdwAttributes);
	} else {
	    hr = S_OK;
	    if (pdwAttributes && *pdwAttributes) {
		SHELL32_GetItemAttributes (_IShellFolder_ (This), pidlTemp, pdwAttributes);
	    }
	}
    }

    *ppidl = pidlTemp;

    TRACE ("(%p)->(-- ret=0x%08lx)\n", This, hr);

    return hr;
}

/**************************************************************************
*		ISF_Desktop_fnEnumObjects
*/
static HRESULT WINAPI ISF_Desktop_fnEnumObjects (IShellFolder2 * iface,
						 HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST * ppEnumIDList)
{
    ICOM_THIS (IGenericSFImpl, iface);

    TRACE ("(%p)->(HWND=%p flags=0x%08lx pplist=%p)\n", This, hwndOwner, dwFlags, ppEnumIDList);

    *ppEnumIDList = NULL;
    *ppEnumIDList = IEnumIDList_Constructor (NULL, dwFlags, EIDL_DESK);

    TRACE ("-- (%p)->(new ID List: %p)\n", This, *ppEnumIDList);

    if (!*ppEnumIDList)
	return E_OUTOFMEMORY;

    return S_OK;
}

/**************************************************************************
*		ISF_Desktop_fnBindToObject
*/
static HRESULT WINAPI ISF_Desktop_fnBindToObject (IShellFolder2 * iface,
						  LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    ICOM_THIS (IGenericSFImpl, iface);

    TRACE ("(%p)->(pidl=%p,%p,%s,%p)\n", This, pidl, pbcReserved, shdebugstr_guid (riid), ppvOut);

    return SHELL32_BindToChild (This->pidlRoot, This->sPathTarget, pidl, riid, ppvOut);
}

/**************************************************************************
*	ISF_Desktop_fnBindToStorage
*/
static HRESULT WINAPI ISF_Desktop_fnBindToStorage (IShellFolder2 * iface,
						   LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    ICOM_THIS (IGenericSFImpl, iface);

    FIXME ("(%p)->(pidl=%p,%p,%s,%p) stub\n", This, pidl, pbcReserved, shdebugstr_guid (riid), ppvOut);

    *ppvOut = NULL;
    return E_NOTIMPL;
}

/**************************************************************************
* 	ISF_Desktop_fnCompareIDs
*/

static HRESULT WINAPI ISF_Desktop_fnCompareIDs (IShellFolder2 * iface,
						LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    ICOM_THIS (IGenericSFImpl, iface);

    int nReturn;

    TRACE ("(%p)->(0x%08lx,pidl1=%p,pidl2=%p)\n", This, lParam, pidl1, pidl2);
    nReturn = SHELL32_CompareIDs (_IShellFolder_ (This), lParam, pidl1, pidl2);
    TRACE ("-- %i\n", nReturn);
    return nReturn;
}

/**************************************************************************
*	ISF_Desktop_fnCreateViewObject
*/
static HRESULT WINAPI ISF_Desktop_fnCreateViewObject (IShellFolder2 * iface,
						      HWND hwndOwner, REFIID riid, LPVOID * ppvOut)
{
    ICOM_THIS (IGenericSFImpl, iface);

    LPSHELLVIEW pShellView;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(hwnd=%p,%s,%p)\n", This, hwndOwner, shdebugstr_guid (riid), ppvOut);

    if (ppvOut) {
	*ppvOut = NULL;

	if (IsEqualIID (riid, &IID_IDropTarget)) {
	    WARN ("IDropTarget not implemented\n");
	    hr = E_NOTIMPL;
	} else if (IsEqualIID (riid, &IID_IContextMenu)) {
	    WARN ("IContextMenu not implemented\n");
	    hr = E_NOTIMPL;
	} else if (IsEqualIID (riid, &IID_IShellView)) {
	    pShellView = IShellView_Constructor ((IShellFolder *) iface);
	    if (pShellView) {
		hr = IShellView_QueryInterface (pShellView, riid, ppvOut);
		IShellView_Release (pShellView);
	    }
	}
    }
    TRACE ("-- (%p)->(interface=%p)\n", This, ppvOut);
    return hr;
}

/**************************************************************************
*  ISF_Desktop_fnGetAttributesOf
*/
static HRESULT WINAPI ISF_Desktop_fnGetAttributesOf (IShellFolder2 * iface,
						     UINT cidl, LPCITEMIDLIST * apidl, DWORD * rgfInOut)
{
    ICOM_THIS (IGenericSFImpl, iface);

    HRESULT hr = S_OK;

    TRACE ("(%p)->(cidl=%d apidl=%p mask=0x%08lx)\n", This, cidl, apidl, *rgfInOut);

    if ((!cidl) || (!apidl) || (!rgfInOut))
	return E_INVALIDARG;

    while (cidl > 0 && *apidl) {
	pdump (*apidl);
	SHELL32_GetItemAttributes (_IShellFolder_ (This), *apidl, rgfInOut);
	apidl++;
	cidl--;
    }

    TRACE ("-- result=0x%08lx\n", *rgfInOut);

    return hr;
}

/**************************************************************************
*	ISF_Desktop_fnGetUIObjectOf
*
* PARAMETERS
*  HWND           hwndOwner, //[in ] Parent window for any output
*  UINT           cidl,      //[in ] array size
*  LPCITEMIDLIST* apidl,     //[in ] simple pidl array
*  REFIID         riid,      //[in ] Requested Interface
*  UINT*          prgfInOut, //[   ] reserved
*  LPVOID*        ppvObject) //[out] Resulting Interface
*
*/
static HRESULT WINAPI ISF_Desktop_fnGetUIObjectOf (IShellFolder2 * iface,
						   HWND hwndOwner,
						   UINT cidl,
						   LPCITEMIDLIST * apidl,
						   REFIID riid, UINT * prgfInOut, LPVOID * ppvOut)
{
    ICOM_THIS (IGenericSFImpl, iface);

    LPITEMIDLIST pidl;
    IUnknown *pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(%p,%u,apidl=%p,%s,%p,%p)\n",
	   This, hwndOwner, cidl, apidl, shdebugstr_guid (riid), prgfInOut, ppvOut);

    if (ppvOut) {
	*ppvOut = NULL;

	if (IsEqualIID (riid, &IID_IContextMenu) && (cidl >= 1)) {
	    pObj = (LPUNKNOWN) ISvItemCm_Constructor ((IShellFolder *) iface, This->pidlRoot, apidl, cidl);
	    hr = S_OK;
	} else if (IsEqualIID (riid, &IID_IDataObject) && (cidl >= 1)) {
	    pObj = (LPUNKNOWN) IDataObject_Constructor (hwndOwner, This->pidlRoot, apidl, cidl);
	    hr = S_OK;
	} else if (IsEqualIID (riid, &IID_IExtractIconA) && (cidl == 1)) {
	    pidl = ILCombine (This->pidlRoot, apidl[0]);
	    pObj = (LPUNKNOWN) IExtractIconA_Constructor (pidl);
	    SHFree (pidl);
	    hr = S_OK;
	} else if (IsEqualIID (riid, &IID_IDropTarget) && (cidl >= 1)) {
	    hr = IShellFolder_QueryInterface (iface, &IID_IDropTarget, (LPVOID *) & pObj);
	} else {
	    hr = E_NOINTERFACE;
	}

	if (!pObj)
	    hr = E_OUTOFMEMORY;

	*ppvOut = pObj;
    }
    TRACE ("(%p)->hr=0x%08lx\n", This, hr);
    return hr;
}

/**************************************************************************
*	ISF_Desktop_fnGetDisplayNameOf
*
* NOTES
*	special case: pidl = null gives desktop-name back
*/
DWORD WINAPI __SHGUIDToStringA (REFGUID guid, LPSTR str)
{
    CHAR sFormat[52] = "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}";

    return wsprintfA (str, sFormat,
		      guid->Data1, guid->Data2, guid->Data3,
		      guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
		      guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);

}

static HRESULT WINAPI ISF_Desktop_fnGetDisplayNameOf (IShellFolder2 * iface,
						      LPCITEMIDLIST pidl, DWORD dwFlags, LPSTRRET strRet)
{
    ICOM_THIS (IGenericSFImpl, iface);

    CHAR szPath[MAX_PATH] = "";
    GUID const *clsid;
    HRESULT hr = S_OK;

    TRACE ("(%p)->(pidl=%p,0x%08lx,%p)\n", This, pidl, dwFlags, strRet);
    pdump (pidl);

    if (!strRet)
	return E_INVALIDARG;

    if (_ILIsDesktop (pidl)) {
	if ((GET_SHGDN_RELATION (dwFlags) == SHGDN_NORMAL) && (GET_SHGDN_FOR (dwFlags) == SHGDN_FORPARSING)) {
	    lstrcpyA (szPath, This->sPathTarget);
	} else {
	    HCR_GetClassName (&CLSID_ShellDesktop, szPath, MAX_PATH);
	}
    } else if (_ILIsPidlSimple (pidl)) {
	if ((clsid = _ILGetGUIDPointer (pidl))) {
	    if (GET_SHGDN_FOR (dwFlags) == SHGDN_FORPARSING) {
		int bWantsForParsing;

		/*
		 * we can only get a filesystem path from a shellfolder if the value WantsFORPARSING in
		 * CLSID\\{...}\\shellfolder exists
		 * exception: the MyComputer folder has this keys not but like any filesystem backed
		 *            folder it needs these behaviour
		 */
		if (IsEqualIID (clsid, &CLSID_MyComputer)) {
		    bWantsForParsing = 1;
		} else {
		    /* get the "WantsFORPARSING" flag from the registry */
		    char szRegPath[100];

		    lstrcpyA (szRegPath, "CLSID\\");
		    __SHGUIDToStringA (clsid, &szRegPath[6]);
		    lstrcatA (szRegPath, "\\shellfolder");
		    bWantsForParsing =
			(ERROR_SUCCESS ==
			 SHGetValueA (HKEY_CLASSES_ROOT, szRegPath, "WantsFORPARSING", NULL, NULL, NULL));
		}

		if ((GET_SHGDN_RELATION (dwFlags) == SHGDN_NORMAL) && bWantsForParsing) {
		    /* we need the filesystem path to the destination folder. Only the folder itself can know it */
		    hr = SHELL32_GetDisplayNameOfChild (iface, pidl, dwFlags, szPath, MAX_PATH);
		} else {
		    /* parsing name like ::{...} */
		    lstrcpyA (szPath, "::");
		    __SHGUIDToStringA (clsid, &szPath[2]);
		}
	    } else {
		/* user friendly name */
		HCR_GetClassName (clsid, szPath, MAX_PATH);
	    }
	} else {
	    /* file system folder */
	    _ILSimpleGetText (pidl, szPath, MAX_PATH);
	}
    } else {
	/* a complex pidl, let the subfolder do the work */
	hr = SHELL32_GetDisplayNameOfChild (iface, pidl, dwFlags, szPath, MAX_PATH);
    }

    if (SUCCEEDED (hr)) {
	strRet->uType = STRRET_CSTR;
	lstrcpynA (strRet->u.cStr, szPath, MAX_PATH);
    }

    TRACE ("-- (%p)->(%s,0x%08lx)\n", This, szPath, hr);
    return hr;
}

/**************************************************************************
*  ISF_Desktop_fnSetNameOf
*  Changes the name of a file object or subfolder, possibly changing its item
*  identifier in the process.
*
* PARAMETERS
*  HWND          hwndOwner,  //[in ] Owner window for output
*  LPCITEMIDLIST pidl,       //[in ] simple pidl of item to change
*  LPCOLESTR     lpszName,   //[in ] the items new display name
*  DWORD         dwFlags,    //[in ] SHGNO formatting flags
*  LPITEMIDLIST* ppidlOut)   //[out] simple pidl returned
*/
static HRESULT WINAPI ISF_Desktop_fnSetNameOf (IShellFolder2 * iface, HWND hwndOwner, LPCITEMIDLIST pidl,	/*simple pidl */
					       LPCOLESTR lpName, DWORD dwFlags, LPITEMIDLIST * pPidlOut)
{
    ICOM_THIS (IGenericSFImpl, iface);

    FIXME ("(%p)->(%p,pidl=%p,%s,%lu,%p)\n", This, hwndOwner, pidl, debugstr_w (lpName), dwFlags, pPidlOut);

    return E_FAIL;
}

static HRESULT WINAPI ISF_Desktop_fnGetDefaultSearchGUID (IShellFolder2 * iface, GUID * pguid)
{
    ICOM_THIS (IGenericSFImpl, iface);

    FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}
static HRESULT WINAPI ISF_Desktop_fnEnumSearches (IShellFolder2 * iface, IEnumExtraSearch ** ppenum)
{
    ICOM_THIS (IGenericSFImpl, iface);
    FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}
static HRESULT WINAPI ISF_Desktop_fnGetDefaultColumn (IShellFolder2 * iface,
						      DWORD dwRes, ULONG * pSort, ULONG * pDisplay)
{
    ICOM_THIS (IGenericSFImpl, iface);

    TRACE ("(%p)\n", This);

    if (pSort)
	*pSort = 0;
    if (pDisplay)
	*pDisplay = 0;

    return S_OK;
}
static HRESULT WINAPI ISF_Desktop_fnGetDefaultColumnState (IShellFolder2 * iface, UINT iColumn, DWORD * pcsFlags)
{
    ICOM_THIS (IGenericSFImpl, iface);

    TRACE ("(%p)\n", This);

    if (!pcsFlags || iColumn >= DESKTOPSHELLVIEWCOLUMNS)
	return E_INVALIDARG;

    *pcsFlags = DesktopSFHeader[iColumn].pcsFlags;

    return S_OK;
}
static HRESULT WINAPI ISF_Desktop_fnGetDetailsEx (IShellFolder2 * iface,
						  LPCITEMIDLIST pidl, const SHCOLUMNID * pscid, VARIANT * pv)
{
    ICOM_THIS (IGenericSFImpl, iface);
    FIXME ("(%p)\n", This);

    return E_NOTIMPL;
}
static HRESULT WINAPI ISF_Desktop_fnGetDetailsOf (IShellFolder2 * iface,
						  LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS * psd)
{
    ICOM_THIS (IGenericSFImpl, iface);

    HRESULT hr = E_FAIL;

    TRACE ("(%p)->(%p %i %p)\n", This, pidl, iColumn, psd);

    if (!psd || iColumn >= DESKTOPSHELLVIEWCOLUMNS)
	return E_INVALIDARG;

    if (!pidl) {
	psd->fmt = DesktopSFHeader[iColumn].fmt;
	psd->cxChar = DesktopSFHeader[iColumn].cxChar;
	psd->str.uType = STRRET_CSTR;
	LoadStringA (shell32_hInstance, DesktopSFHeader[iColumn].colnameid, psd->str.u.cStr, MAX_PATH);
	return S_OK;
    } else {
	/* the data from the pidl */
	switch (iColumn) {
	case 0:		/* name */
	    hr = IShellFolder_GetDisplayNameOf (iface, pidl, SHGDN_NORMAL | SHGDN_INFOLDER, &psd->str);
	    break;
	case 1:		/* size */
	    _ILGetFileSize (pidl, psd->str.u.cStr, MAX_PATH);
	    break;
	case 2:		/* type */
	    _ILGetFileType (pidl, psd->str.u.cStr, MAX_PATH);
	    break;
	case 3:		/* date */
	    _ILGetFileDate (pidl, psd->str.u.cStr, MAX_PATH);
	    break;
	case 4:		/* attributes */
	    _ILGetFileAttributes (pidl, psd->str.u.cStr, MAX_PATH);
	    break;
	}
	hr = S_OK;
	psd->str.uType = STRRET_CSTR;
    }

    return hr;
}
static HRESULT WINAPI ISF_Desktop_fnMapNameToSCID (IShellFolder2 * iface, LPCWSTR pwszName, SHCOLUMNID * pscid)
{
    ICOM_THIS (IGenericSFImpl, iface);
    FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}

static ICOM_VTABLE (IShellFolder2) vt_MCFldr_ShellFolder2 =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	ISF_Desktop_fnQueryInterface,
	ISF_Desktop_fnAddRef,
	ISF_Desktop_fnRelease,
	ISF_Desktop_fnParseDisplayName,
	ISF_Desktop_fnEnumObjects,
	ISF_Desktop_fnBindToObject,
	ISF_Desktop_fnBindToStorage,
	ISF_Desktop_fnCompareIDs,
	ISF_Desktop_fnCreateViewObject,
	ISF_Desktop_fnGetAttributesOf,
	ISF_Desktop_fnGetUIObjectOf,
	ISF_Desktop_fnGetDisplayNameOf,
	ISF_Desktop_fnSetNameOf,
	/* ShellFolder2 */
        ISF_Desktop_fnGetDefaultSearchGUID,
	ISF_Desktop_fnEnumSearches,
	ISF_Desktop_fnGetDefaultColumn,
	ISF_Desktop_fnGetDefaultColumnState,
	ISF_Desktop_fnGetDetailsEx,
	ISF_Desktop_fnGetDetailsOf,
	ISF_Desktop_fnMapNameToSCID};
