/*
 *	Shell Folder stuff
 *
 *	Copyright 1997	Marcus Meissner
 *	Copyright 1998, 1999	Juergen Schmied
 *
 *	IShellFolder2 and related interfaces
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

#include "oleidl.h"
#include "shlguid.h"

#include "pidl.h"
#include "wine/obj_base.h"
#include "wine/obj_dragdrop.h"
#include "wine/obj_shellfolder.h"
#include "undocshell.h"
#include "shell32_main.h"
#include "shresdef.h"
#include "shlwapi.h"
#include "shellfolder.h"
#include "wine/debug.h"
#include "debughlp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);


/***************************************************************************
 * debughelper: print out the return adress
 *  helps especially to track down unbalanced AddRef/Release
 */
#define MEM_DEBUG 0

#if MEM_DEBUG
#define _CALL_TRACE TRACE("called from: 0x%08x\n", *( ((UINT*)&iface)-1 ));
#else
#define _CALL_TRACE
#endif

typedef struct
{
	int 	colnameid;
	int	pcsFlags;
	int	fmt;
	int	cxChar;

} shvheader;

/***************************************************************************
 *  GetNextElement (internal function)
 *
 * gets a part of a string till the first backslash
 *
 * PARAMETERS
 *  pszNext [IN] string to get the element from
 *  pszOut  [IN] pointer to buffer whitch receives string
 *  dwOut   [IN] length of pszOut
 *
 *  RETURNS
 *    LPSTR pointer to first, not yet parsed char
 */

static LPCWSTR GetNextElementW(LPCWSTR pszNext,LPWSTR pszOut,DWORD dwOut)
{	LPCWSTR   pszTail = pszNext;
	DWORD dwCopy;
	TRACE("(%s %p 0x%08lx)\n",debugstr_w(pszNext),pszOut,dwOut);

	*pszOut=0x0000;

	if(!pszNext || !*pszNext)
	  return NULL;

	while(*pszTail && (*pszTail != (WCHAR)'\\'))
	  pszTail++;

	dwCopy = (WCHAR*)pszTail - (WCHAR*)pszNext + 1;
	lstrcpynW(pszOut, pszNext, (dwOut<dwCopy)? dwOut : dwCopy);

	if(*pszTail)
	   pszTail++;
	else
	   pszTail = NULL;

	TRACE("--(%s %s 0x%08lx %p)\n",debugstr_w(pszNext),debugstr_w(pszOut),dwOut,pszTail);
	return pszTail;
}

static HRESULT SHELL32_ParseNextElement(
	HWND hwndOwner,
	IShellFolder2 * psf,
	LPITEMIDLIST * pidlInOut,
	LPOLESTR szNext,
	DWORD *pEaten,
	DWORD *pdwAttributes)
{
	HRESULT		hr = E_OUTOFMEMORY;
	LPITEMIDLIST	pidlOut, pidlTemp = NULL;
	IShellFolder	*psfChild;

	TRACE("(%p, %p, %s)\n",psf, pidlInOut ? *pidlInOut : NULL, debugstr_w(szNext));


	/* get the shellfolder for the child pidl and let it analyse further */
	hr = IShellFolder_BindToObject(psf, *pidlInOut, NULL, &IID_IShellFolder, (LPVOID*)&psfChild);

	if (psfChild)
	{
	  hr = IShellFolder_ParseDisplayName(psfChild, hwndOwner, NULL, szNext, pEaten, &pidlOut, pdwAttributes);
	  IShellFolder_Release(psfChild);

	  pidlTemp = ILCombine(*pidlInOut, pidlOut);

	  if (pidlOut)
	    ILFree(pidlOut);
	}

	ILFree(*pidlInOut);
	*pidlInOut = pidlTemp;

	TRACE("-- pidl=%p ret=0x%08lx\n", pidlInOut? *pidlInOut: NULL, hr);
	return hr;
}

/***********************************************************************
 *	SHELL32_CoCreateInitSF
 *
 *	creates a initialized shell folder
 */
static HRESULT SHELL32_CoCreateInitSF (
	LPITEMIDLIST pidlRoot,
	LPITEMIDLIST pidlChild,
	REFCLSID clsid,
	REFIID iid,
	LPVOID * ppvOut)
{
	HRESULT hr;
	LPITEMIDLIST	pidlAbsolute;
	IPersistFolder	*pPF;

	TRACE("%p %p\n", pidlRoot, pidlChild);

	*ppvOut = NULL;

	if (SUCCEEDED((hr = SHCoCreateInstance(NULL, clsid, NULL, &IID_IPersistFolder, (LPVOID*)&pPF)))) {
	  if(SUCCEEDED((hr = IPersistFolder_QueryInterface(pPF, iid, ppvOut)))) {
	    pidlAbsolute = ILCombine (pidlRoot, pidlChild);
	    hr = IPersistFolder_Initialize(pPF, pidlAbsolute);
	    IPersistFolder_Release(pPF);
	    SHFree(pidlAbsolute);
	  }
	}

	TRACE("-- ret=0x%08lx\n", hr);
	return hr;
}

static HRESULT SHELL32_GetDisplayNameOfChild(
	IShellFolder2 * psf,
	LPCITEMIDLIST pidl,
	DWORD dwFlags,
	LPSTR szOut,
	DWORD dwOutLen)
{
	LPITEMIDLIST	pidlFirst, pidlNext;
	IShellFolder2 *	psfChild;
	HRESULT		hr = E_OUTOFMEMORY;
	STRRET strTemp;

	TRACE("(%p)->(pidl=%p 0x%08lx %p 0x%08lx)\n",psf,pidl,dwFlags,szOut, dwOutLen);
	pdump(pidl);

	if ((pidlFirst = ILCloneFirst(pidl)))
	{
	  hr = IShellFolder_BindToObject(psf, pidlFirst, NULL, &IID_IShellFolder, (LPVOID*)&psfChild);
	  if (SUCCEEDED(hr))
	  {
	    pidlNext = ILGetNext(pidl);

	    hr = IShellFolder_GetDisplayNameOf(psfChild, pidlNext, dwFlags | SHGDN_INFOLDER, &strTemp);
	    if (SUCCEEDED(hr))
	    {
	      hr = StrRetToStrNA(szOut, dwOutLen, &strTemp, pidlNext);
	    }

	    IShellFolder_Release(psfChild);
	  }
	  ILFree(pidlFirst);
	}

	TRACE("-- ret=0x%08lx %s\n", hr, szOut);

	return hr;
}

/***********************************************************************
 *  SHELL32_GetItemAttributes
 *
 * NOTES
 * observerd values:
 *  folder:	0xE0000177	FILESYSTEM | HASSUBFOLDER | FOLDER
 *  file:	0x40000177	FILESYSTEM
 *  drive:	0xf0000144	FILESYSTEM | HASSUBFOLDER | FOLDER | FILESYSANCESTOR
 *  mycomputer:	0xb0000154	HASSUBFOLDER | FOLDER | FILESYSANCESTOR
 *  (seems to be default for shell extensions if no registry entry exists)
 *
 * This functions does not set flags!! It only resets flags when nessesary.
 */
static HRESULT SHELL32_GetItemAttributes(
	IShellFolder * psf,
	LPITEMIDLIST pidl,
	LPDWORD pdwAttributes)
{
        GUID const * clsid;
	DWORD dwAttributes;

	TRACE("0x%08lx\n", *pdwAttributes);

	if (*pdwAttributes & (0xcff3fe88))
	  WARN("attribute 0x%08lx not implemented\n", *pdwAttributes);
	*pdwAttributes &= ~SFGAO_LINK; /* FIXME: for native filedialogs */

	if (_ILIsDrive(pidl))
	{
	  *pdwAttributes &= 0xf0000144;
	}
	else if ((clsid=_ILGetGUIDPointer(pidl)))
	{
	  if (HCR_GetFolderAttributes(clsid, &dwAttributes))
	  {
	    *pdwAttributes &= dwAttributes;
	  }
	  else
	  {
	    *pdwAttributes &= 0xb0000154;
	  }
	}
	else if (_ILGetDataPointer(pidl))
	{
	  dwAttributes = _ILGetFileAttributes(pidl, NULL, 0);
	  *pdwAttributes &= ~SFGAO_FILESYSANCESTOR;

	  if(( SFGAO_FOLDER & *pdwAttributes) && !(dwAttributes & FILE_ATTRIBUTE_DIRECTORY))
	      *pdwAttributes &= ~(SFGAO_FOLDER|SFGAO_HASSUBFOLDER);

	  if(( SFGAO_HIDDEN & *pdwAttributes) && !(dwAttributes & FILE_ATTRIBUTE_HIDDEN))
	      *pdwAttributes &= ~SFGAO_HIDDEN;

	  if(( SFGAO_READONLY & *pdwAttributes) && !(dwAttributes & FILE_ATTRIBUTE_READONLY))
	      *pdwAttributes &= ~SFGAO_READONLY;
	}
	else
	{
	  *pdwAttributes &= 0xb0000154;
	}
	TRACE("-- 0x%08lx\n", *pdwAttributes);
	return S_OK;
}

/***********************************************************************
*   IShellFolder implementation
*/

typedef struct
{
	ICOM_VFIELD(IUnknown);
	DWORD				ref;
	ICOM_VTABLE(IShellFolder2)*	lpvtblShellFolder;
	ICOM_VTABLE(IPersistFolder3)*	lpvtblPersistFolder3;
	ICOM_VTABLE(IDropTarget)*	lpvtblDropTarget;
	ICOM_VTABLE(ISFHelper)*		lpvtblSFHelper;

	IUnknown			*pUnkOuter;	/* used for aggregation */

	CLSID*				pclsid;

	/* both paths are parsible from the desktop */
	LPSTR		sPathRoot;	/* complete path used as return value */
	LPSTR		sPathTarget;	/* complete path to target used for enumeration and ChangeNotify */

	LPITEMIDLIST	pidlRoot;	/* absolute pidl */
	LPITEMIDLIST	pidlTarget;	/* absolute pidl */

	int		dwAttributes;	/* attributes returned by GetAttributesOf FIXME: use it */

	UINT		cfShellIDList;	/* clipboardformat for IDropTarget */
	BOOL		fAcceptFmt;	/* flag for pending Drop */
} IGenericSFImpl;

static struct ICOM_VTABLE(IUnknown) unkvt;
static struct ICOM_VTABLE(IShellFolder2) sfvt;
static struct ICOM_VTABLE(IPersistFolder3) psfvt;
static struct ICOM_VTABLE(IDropTarget) dtvt;
static struct ICOM_VTABLE(ISFHelper) shvt;

static IShellFolder * ISF_MyComputer_Constructor(void);

#define _IShellFolder2_Offset ((int)(&(((IGenericSFImpl*)0)->lpvtblShellFolder)))
#define _ICOM_THIS_From_IShellFolder2(class, name) class* This = (class*)(((char*)name)-_IShellFolder2_Offset);

#define _IPersistFolder_Offset ((int)(&(((IGenericSFImpl*)0)->lpvtblPersistFolder3)))
#define _ICOM_THIS_From_IPersistFolder3(class, name) class* This = (class*)(((char*)name)-_IPersistFolder_Offset);

#define _IDropTarget_Offset ((int)(&(((IGenericSFImpl*)0)->lpvtblDropTarget)))
#define _ICOM_THIS_From_IDropTarget(class, name) class* This = (class*)(((char*)name)-_IDropTarget_Offset);

#define _ISFHelper_Offset ((int)(&(((IGenericSFImpl*)0)->lpvtblSFHelper)))
#define _ICOM_THIS_From_ISFHelper(class, name) class* This = (class*)(((char*)name)-_ISFHelper_Offset);
/*
  converts This to a interface pointer
*/
#define _IUnknown_(This)	(IUnknown*)&(This->lpVtbl)
#define _IShellFolder_(This)	(IShellFolder*)&(This->lpvtblShellFolder)
#define _IShellFolder2_(This)	(IShellFolder2*)&(This->lpvtblShellFolder)
#define _IPersist_(This)	(IPersist*)&(This->lpvtblPersistFolder3)
#define _IPersistFolder_(This)	(IPersistFolder*)&(This->lpvtblPersistFolder3)
#define _IPersistFolder2_(This)	(IPersistFolder2*)&(This->lpvtblPersistFolder3)
#define _IPersistFolder3_(This)	(IPersistFolder3*)&(This->lpvtblPersistFolder3)
#define _IDropTarget_(This)	(IDropTarget*)&(This->lpvtblDropTarget)
#define _ISFHelper_(This)	(ISFHelper*)&(This->lpvtblSFHelper)
/**************************************************************************
*	registers clipboardformat once
*/
static void SF_RegisterClipFmt (IGenericSFImpl * This)
{
	TRACE("(%p)\n", This);

	if (!This->cfShellIDList)
	{
	  This->cfShellIDList = RegisterClipboardFormatA(CFSTR_SHELLIDLIST);
	}
}

/**************************************************************************
*	we need a separate IUnknown to handle aggregation
*	(inner IUnknown)
*/
static HRESULT WINAPI IUnknown_fnQueryInterface(
	IUnknown * iface,
	REFIID riid,
	LPVOID *ppvObj)
{
	ICOM_THIS(IGenericSFImpl, iface);

	_CALL_TRACE
	TRACE("(%p)->(%s,%p)\n",This,shdebugstr_guid(riid),ppvObj);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))		*ppvObj = _IUnknown_(This);
	else if(IsEqualIID(riid, &IID_IShellFolder))	*ppvObj = _IShellFolder_(This);
	else if(IsEqualIID(riid, &IID_IShellFolder2))	*ppvObj = _IShellFolder_(This);
	else if(IsEqualIID(riid, &IID_IPersist))	*ppvObj = _IPersist_(This);
	else if(IsEqualIID(riid, &IID_IPersistFolder))	*ppvObj = _IPersistFolder_(This);
	else if(IsEqualIID(riid, &IID_IPersistFolder2))	*ppvObj = _IPersistFolder2_(This);
	else if(IsEqualIID(riid, &IID_IPersistFolder3))	*ppvObj = _IPersistFolder3_(This);
	else if(IsEqualIID(riid, &IID_ISFHelper))	*ppvObj = _ISFHelper_(This);
	else if(IsEqualIID(riid, &IID_IDropTarget))
	{
	  *ppvObj = _IDropTarget_(This);
	  SF_RegisterClipFmt(This);
	}

	if(*ppvObj)
	{
	  IUnknown_AddRef((IUnknown*)(*ppvObj));
	  TRACE("-- Interface = %p\n", *ppvObj);
	  return S_OK;
	}
	TRACE("-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}

static ULONG WINAPI IUnknown_fnAddRef(IUnknown * iface)
{
	ICOM_THIS(IGenericSFImpl, iface);

	_CALL_TRACE
	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	shell32_ObjCount++;
	return ++(This->ref);
}

static ULONG WINAPI IUnknown_fnRelease(IUnknown * iface)
{
	ICOM_THIS(IGenericSFImpl, iface);

	_CALL_TRACE
	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	shell32_ObjCount--;
	if (!--(This->ref))
	{
	  TRACE("-- destroying IShellFolder(%p)\n",This);

	  if(This->pidlRoot) SHFree(This->pidlRoot);
	  if(This->sPathRoot) SHFree(This->sPathRoot);
	  LocalFree((HLOCAL)This);
	  return 0;
	}
	return This->ref;
}

static ICOM_VTABLE(IUnknown) unkvt =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IUnknown_fnQueryInterface,
	IUnknown_fnAddRef,
	IUnknown_fnRelease,
};

static shvheader GenericSFHeader [] =
{
 { IDS_SHV_COLUMN1, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15 },
 { IDS_SHV_COLUMN2, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10 },
 { IDS_SHV_COLUMN3, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10 },
 { IDS_SHV_COLUMN4, SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 12 },
 { IDS_SHV_COLUMN5, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 5 }
};
#define GENERICSHELLVIEWCOLUMNS 5

/**************************************************************************
*	IFSFolder_Constructor
*
* NOTES
*  creating undocumented ShellFS_Folder as part of an aggregation
*  {F3364BA0-65B9-11CE-A9BA-00AA004AE837}
*
*/
HRESULT WINAPI IFSFolder_Constructor(
	IUnknown * pUnkOuter,
	REFIID riid,
	LPVOID * ppv)
{
	IGenericSFImpl * sf;

	TRACE("unkOut=%p %s\n",pUnkOuter, shdebugstr_guid(riid));

	if(pUnkOuter && ! IsEqualIID(riid, &IID_IUnknown)) return CLASS_E_NOAGGREGATION;
	sf=(IGenericSFImpl*) LocalAlloc(GMEM_ZEROINIT,sizeof(IGenericSFImpl));
	if (!sf) return E_OUTOFMEMORY;

	sf->ref=1;
	ICOM_VTBL(sf)=&unkvt;
	sf->lpvtblShellFolder=&sfvt;
	sf->lpvtblPersistFolder3=&psfvt;
	sf->lpvtblDropTarget=&dtvt;
	sf->lpvtblSFHelper=&shvt;
	sf->pclsid = (CLSID*)&CLSID_ShellFSFolder;
	sf->pUnkOuter = pUnkOuter ? pUnkOuter : _IUnknown_(sf);

	/* we have to return the inner IUnknown */
	*ppv = _IUnknown_(sf);

	shell32_ObjCount++;
	TRACE("--%p\n", *ppv);
	return S_OK;
}

/**************************************************************************
*	InitializeGenericSF
*/
static HRESULT InitializeGenericSF(IGenericSFImpl * sf, LPITEMIDLIST pidlRoot, LPITEMIDLIST pidlFolder, LPCSTR sPathRoot)
{
	TRACE("(%p)->(pidl=%p, path=%s)\n",sf,pidlRoot, sPathRoot);
	pdump(pidlRoot);
	pdump(pidlFolder);

	sf->pidlRoot = ILCombine(pidlRoot, pidlFolder);
	if (!_ILIsSpecialFolder(pidlFolder)) {				/* only file system paths */
	    char sNewPath[MAX_PATH];
	    char * sPos;

	    if (sPathRoot) {
	        strcpy(sNewPath, sPathRoot);
	        if (!((sPos = PathAddBackslashA (sNewPath)))) return E_UNEXPECTED;
	    } else {
	        sPos = sNewPath;
	    }
	    _ILSimpleGetText(pidlFolder, sPos, MAX_PATH - (sPos - sNewPath));
	    if(!((sf->sPathRoot = SHAlloc(strlen(sNewPath+1))))) return E_OUTOFMEMORY;
	    strcpy(sf->sPathRoot, sNewPath);
	    TRACE("-- %s\n", sNewPath);
	}
	return S_OK;
}

/**************************************************************************
*	  IShellFolder_Constructor
*/
IGenericSFImpl * IShellFolder_Constructor()
{
	IGenericSFImpl * sf=(IGenericSFImpl*) LocalAlloc(GMEM_ZEROINIT,sizeof(IGenericSFImpl));
	sf->ref=1;

	ICOM_VTBL(sf)=&unkvt;
	sf->lpvtblShellFolder=&sfvt;
	sf->lpvtblPersistFolder3=&psfvt;
	sf->lpvtblDropTarget=&dtvt;
	sf->lpvtblSFHelper=&shvt;

	sf->pclsid = (CLSID*)&CLSID_ShellFSFolder;
	sf->pUnkOuter = _IUnknown_(sf);

	TRACE("(%p)->()\n",sf);

	shell32_ObjCount++;
	return sf;
}

/**************************************************************************
 *  IShellFolder_fnQueryInterface
 *
 * PARAMETERS
 *  REFIID riid		[in ] Requested InterfaceID
 *  LPVOID* ppvObject	[out] Interface* to hold the result
 */
static HRESULT WINAPI IShellFolder_fnQueryInterface(
	IShellFolder2 * iface,
	REFIID riid,
	LPVOID *ppvObj)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	_CALL_TRACE
	TRACE("(%p)->(%s,%p)\n",This,shdebugstr_guid(riid),ppvObj);

	return IUnknown_QueryInterface(This->pUnkOuter, riid, ppvObj);
}

/**************************************************************************
*  IShellFolder_AddRef
*/

static ULONG WINAPI IShellFolder_fnAddRef(IShellFolder2 * iface)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	_CALL_TRACE
	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	return IUnknown_AddRef(This->pUnkOuter);
}

/**************************************************************************
 *  IShellFolder_fnRelease
 */
static ULONG WINAPI IShellFolder_fnRelease(IShellFolder2 * iface)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	_CALL_TRACE
	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	return IUnknown_Release(This->pUnkOuter);
}

/**************************************************************************
*		IShellFolder_fnParseDisplayName
* PARAMETERS
*  HWND          hwndOwner,      //[in ] Parent window for any message's
*  LPBC          pbc,            //[in ] reserved
*  LPOLESTR      lpszDisplayName,//[in ] "Unicode" displayname.
*  ULONG*        pchEaten,       //[out] (unicode) characters processed
*  LPITEMIDLIST* ppidl,          //[out] complex pidl to item
*  ULONG*        pdwAttributes   //[out] items attributes
*
* NOTES
*  every folder tries to parse only its own (the leftmost) pidl and creates a
*  subfolder to evaluate the remaining parts
*  now we can parse into namespaces implemented by shell extensions
*
*  behaviour on win98:	lpszDisplayName=NULL -> chrash
*			lpszDisplayName="" -> returns mycoputer-pidl
*
* FIXME:
*    pdwAttributes: not set
*    pchEaten: not set like in windows
*/
static HRESULT WINAPI IShellFolder_fnParseDisplayName(
	IShellFolder2 * iface,
	HWND hwndOwner,
	LPBC pbcReserved,
	LPOLESTR lpszDisplayName,
	DWORD *pchEaten,
	LPITEMIDLIST *ppidl,
	DWORD *pdwAttributes)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	HRESULT		hr = E_OUTOFMEMORY;
	LPCWSTR		szNext=NULL;
	WCHAR		szElement[MAX_PATH];
	CHAR		szTempA[MAX_PATH], szPath[MAX_PATH];
	LPITEMIDLIST	pidlTemp=NULL;

	TRACE("(%p)->(HWND=0x%08x,%p,%p=%s,%p,pidl=%p,%p)\n",
	This,hwndOwner,pbcReserved,lpszDisplayName,
	debugstr_w(lpszDisplayName),pchEaten,ppidl,pdwAttributes);

	if (!lpszDisplayName || !ppidl) return E_INVALIDARG;

	if (pchEaten) *pchEaten = 0;	/* strange but like the original */

	if (*lpszDisplayName)
	{
	  /* get the next element */
	  szNext = GetNextElementW(lpszDisplayName, szElement, MAX_PATH);

	  /* build the full pathname to the element */
          WideCharToMultiByte( CP_ACP, 0, szElement, -1, szTempA, MAX_PATH, NULL, NULL );
	  strcpy(szPath, This->sPathRoot);
	  PathAddBackslashA(szPath);
	  strcat(szPath, szTempA);

	  /* get the pidl */
	  pidlTemp = SHSimpleIDListFromPathA(szPath);

	  if (pidlTemp)
	  {
	    /* try to analyse the next element */
	    if (szNext && *szNext)
	    {
	      hr = SHELL32_ParseNextElement(hwndOwner, iface, &pidlTemp, (LPOLESTR)szNext, pchEaten, pdwAttributes);
	    }
	    else
	    {
	       if (pdwAttributes && *pdwAttributes)
	       {
	         SHELL32_GetItemAttributes(_IShellFolder_(This), pidlTemp, pdwAttributes);
/*	         WIN32_FIND_DATAA fd;
	         SHGetDataFromIDListA(_IShellFolder_(This), pidlTemp, SHGDFIL_FINDDATA, &fd, sizeof(fd));
		 if (!(FILE_ATTRIBUTE_DIRECTORY & fd.dwFileAttributes))
		   *pdwAttributes &= ~SFGAO_FOLDER;
		 if (FILE_ATTRIBUTE_READONLY  & fd.dwFileAttributes)
		   *pdwAttributes &= ~(SFGAO_CANDELETE|SFGAO_CANMOVE|SFGAO_CANRENAME );
*/
	       }
	       hr = S_OK;
	    }
	  }
	}

        if (!hr)
	  *ppidl = pidlTemp;
	else
	  *ppidl = NULL;

	TRACE("(%p)->(-- pidl=%p ret=0x%08lx)\n", This, ppidl? *ppidl:0, hr);

	return hr;
}

/**************************************************************************
*		IShellFolder_fnEnumObjects
* PARAMETERS
*  HWND          hwndOwner,    //[in ] Parent Window
*  DWORD         grfFlags,     //[in ] SHCONTF enumeration mask
*  LPENUMIDLIST* ppenumIDList  //[out] IEnumIDList interface
*/
static HRESULT WINAPI IShellFolder_fnEnumObjects(
	IShellFolder2 * iface,
	HWND hwndOwner,
	DWORD dwFlags,
	LPENUMIDLIST* ppEnumIDList)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	TRACE("(%p)->(HWND=0x%08x flags=0x%08lx pplist=%p)\n",This,hwndOwner,dwFlags,ppEnumIDList);

	*ppEnumIDList = NULL;
	*ppEnumIDList = IEnumIDList_Constructor (This->sPathRoot, dwFlags, EIDL_FILE);

	TRACE("-- (%p)->(new ID List: %p)\n",This,*ppEnumIDList);

	if(!*ppEnumIDList) return E_OUTOFMEMORY;

	return S_OK;
}

/**************************************************************************
*		IShellFolder_fnBindToObject
* PARAMETERS
*  LPCITEMIDLIST pidl,       //[in ] relative pidl to open
*  LPBC          pbc,        //[in ] reserved
*  REFIID        riid,       //[in ] Initial Interface
*  LPVOID*       ppvObject   //[out] Interface*
*/
static HRESULT WINAPI IShellFolder_fnBindToObject( IShellFolder2 * iface, LPCITEMIDLIST pidl,
			LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)
	GUID		const * iid;
	IShellFolder	*pShellFolder, *pSubFolder;
        IGenericSFImpl  *pSFImpl;
	IPersistFolder 	*pPersistFolder;
	LPITEMIDLIST	pidlRoot;
	HRESULT         hr;

	TRACE("(%p)->(pidl=%p,%p,%s,%p)\n",This,pidl,pbcReserved,shdebugstr_guid(riid),ppvOut);

	if(!pidl || !ppvOut) return E_INVALIDARG;

	*ppvOut = NULL;

	if ((iid=_ILGetGUIDPointer(pidl)))
	{
	  /* we have to create a alien folder */
	  if (  SUCCEEDED(SHCoCreateInstance(NULL, iid, NULL, riid, (LPVOID*)&pShellFolder))
	     && SUCCEEDED(IShellFolder_QueryInterface(pShellFolder, &IID_IPersistFolder, (LPVOID*)&pPersistFolder)))
	    {
	      pidlRoot = ILCombine (This->pidlRoot, pidl);
	      IPersistFolder_Initialize(pPersistFolder, pidlRoot);
	      IPersistFolder_Release(pPersistFolder);
	      SHFree(pidlRoot);
	    }
	    else
	    {
	      return E_FAIL;
	    }
	}
	else
	{
	  if ((pSFImpl = IShellFolder_Constructor())) {
	    LPITEMIDLIST pidltemp = ILCloneFirst(pidl);
	    hr = InitializeGenericSF(pSFImpl, This->pidlRoot, pidltemp, This->sPathRoot);
	    ILFree(pidltemp);
	    pShellFolder = _IShellFolder_(pSFImpl);
	  }
	}

	if (_ILIsPidlSimple(pidl))
	{
	  if(IsEqualIID(riid, &IID_IShellFolder))
	  {
	    *ppvOut = pShellFolder;
	    hr = S_OK;
	  }
	  else
	  {
	    hr = IShellFolder_QueryInterface(pShellFolder, riid, ppvOut);
	    IShellFolder_Release(pShellFolder);
	  }
	}
	else
	{
	  hr = IShellFolder_BindToObject(pShellFolder, ILGetNext(pidl), NULL, riid, (LPVOID)&pSubFolder);
	  IShellFolder_Release(pShellFolder);
	  *ppvOut = pSubFolder;
	}

	TRACE("-- (%p) returning (%p) %08lx\n",This, *ppvOut, hr);

	return hr;
}

/**************************************************************************
*  IShellFolder_fnBindToStorage
* PARAMETERS
*  LPCITEMIDLIST pidl,       //[in ] complex pidl to store
*  LPBC          pbc,        //[in ] reserved
*  REFIID        riid,       //[in ] Initial storage interface
*  LPVOID*       ppvObject   //[out] Interface* returned
*/
static HRESULT WINAPI IShellFolder_fnBindToStorage(
	IShellFolder2 * iface,
	LPCITEMIDLIST pidl,
	LPBC pbcReserved,
	REFIID riid,
	LPVOID *ppvOut)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	FIXME("(%p)->(pidl=%p,%p,%s,%p) stub\n",
              This,pidl,pbcReserved,shdebugstr_guid(riid),ppvOut);

	*ppvOut = NULL;
	return E_NOTIMPL;
}

/**************************************************************************
*  IShellFolder_fnCompareIDs
*
* PARMETERS
*  LPARAM        lParam, //[in ] Column?
*  LPCITEMIDLIST pidl1,  //[in ] simple pidl
*  LPCITEMIDLIST pidl2)  //[in ] simple pidl
*
* NOTES
*   Special case - If one of the items is a Path and the other is a File,
*   always make the Path come before the File.
*
* NOTES
*  use SCODE_CODE() on the return value to get the result
*/

static HRESULT WINAPI  IShellFolder_fnCompareIDs(
	IShellFolder2 * iface,
	LPARAM lParam,
	LPCITEMIDLIST pidl1,
	LPCITEMIDLIST pidl2)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	CHAR szTemp1[MAX_PATH];
	CHAR szTemp2[MAX_PATH];
	int   nReturn;
	IShellFolder * psf;
	HRESULT hr = E_OUTOFMEMORY;
	LPCITEMIDLIST  pidlTemp;
	PIDLTYPE pt1, pt2;

	if (TRACE_ON(shell)) {
	    TRACE("(%p)->(0x%08lx,pidl1=%p,pidl2=%p)\n",This,lParam,pidl1,pidl2);
	    pdump (pidl1);
	    pdump (pidl2);
	}

	if (!pidl1 && !pidl2)
	{
	  hr = ResultFromShort(0);
	}
	else if (!pidl1)
	{
	  hr = ResultFromShort(-1);
	}
	else if (!pidl2)
	{
	  hr = ResultFromShort(1);
	}
	else
	{
	  LPPIDLDATA pd1, pd2;
	  pd1 = _ILGetDataPointer(pidl1);
	  pd2 = _ILGetDataPointer(pidl2);

	  /* compate the types. sort order is the PT_* constant */
	  pt1 = ( pd1 ? pd1->type: PT_DESKTOP);
	  pt2 = ( pd2 ? pd2->type: PT_DESKTOP);

	  if (pt1 != pt2)
	  {
	    hr = ResultFromShort(pt1-pt2);
	  }
	  else						/* same type of pidl */
	  {
	    _ILSimpleGetText(pidl1, szTemp1, MAX_PATH);
	    _ILSimpleGetText(pidl2, szTemp2, MAX_PATH);
	    nReturn = strcasecmp(szTemp1, szTemp2);

	    if (nReturn == 0)				/* first pidl different ? */
	    {
	      pidl1 = ILGetNext(pidl1);

	      if (pidl1 && pidl1->mkid.cb)		/* go deeper? */
	      {
	        pidlTemp = ILCloneFirst(pidl1);
	        pidl2 = ILGetNext(pidl2);

	        hr = IShellFolder_BindToObject(iface, pidlTemp, NULL, &IID_IShellFolder, (LPVOID*)&psf);
	        if (SUCCEEDED(hr))
	        {
	          nReturn = IShellFolder_CompareIDs(psf, 0, pidl1, pidl2);
	          IShellFolder_Release(psf);
	          hr = ResultFromShort(nReturn);
	        }
	        ILFree(pidlTemp);
	      }
	      else                                      /* no deeper on #1  */
	      {
	        pidl2 = ILGetNext(pidl2);
		if (pidl2 && pidl2->mkid.cb)		/* go deeper on #2 ? */
		    hr = ResultFromShort(-1);           /* two different */
		else
		    hr = ResultFromShort(nReturn);      /* two equal simple pidls */
	      }
	    }
	    else
	    {
	      hr = ResultFromShort(nReturn);		/* two different simple pidls */
	    }
	  }
	}

	TRACE("-- res=0x%08lx\n", hr);
	return hr;
}

/**************************************************************************
*	IShellFolder_fnCreateViewObject
*/
static HRESULT WINAPI IShellFolder_fnCreateViewObject(
	IShellFolder2 * iface,
	HWND hwndOwner,
	REFIID riid,
	LPVOID *ppvOut)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	LPSHELLVIEW	pShellView;
	HRESULT		hr = E_INVALIDARG;

	TRACE("(%p)->(hwnd=0x%x,%s,%p)\n",This,hwndOwner,shdebugstr_guid(riid),ppvOut);

	if(ppvOut)
	{
	  *ppvOut = NULL;

	  if(IsEqualIID(riid, &IID_IDropTarget))
	  {
	    hr = IShellFolder_QueryInterface(iface, &IID_IDropTarget, ppvOut);
	  }
	  else if(IsEqualIID(riid, &IID_IContextMenu))
	  {
	    FIXME("IContextMenu not implemented\n");
	    hr = E_NOTIMPL;
	  }
	  else if(IsEqualIID(riid, &IID_IShellView))
	  {
	    pShellView = IShellView_Constructor((IShellFolder*)iface);
	    if(pShellView)
	    {
	      hr = IShellView_QueryInterface(pShellView, riid, ppvOut);
	      IShellView_Release(pShellView);
	    }
	  }
	}
	TRACE("-- (%p)->(interface=%p)\n",This, ppvOut);
	return hr;
}

/**************************************************************************
*  IShellFolder_fnGetAttributesOf
*
* PARAMETERS
*  UINT            cidl,     //[in ] num elements in pidl array
*  LPCITEMIDLIST*  apidl,    //[in ] simple pidl array
*  ULONG*          rgfInOut) //[out] result array
*
*/
static HRESULT WINAPI IShellFolder_fnGetAttributesOf(
	IShellFolder2 * iface,
	UINT cidl,
	LPCITEMIDLIST *apidl,
	DWORD *rgfInOut)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	HRESULT hr = S_OK;

	TRACE("(%p)->(cidl=%d apidl=%p mask=0x%08lx)\n",This,cidl,apidl,*rgfInOut);

	if ( (!cidl) || (!apidl) || (!rgfInOut))
	  return E_INVALIDARG;

	while (cidl > 0 && *apidl)
	{
	  pdump (*apidl);
	  SHELL32_GetItemAttributes(_IShellFolder_(This), *apidl, rgfInOut);
	  apidl++;
	  cidl--;
	}

	TRACE("-- result=0x%08lx\n",*rgfInOut);

	return hr;
}
/**************************************************************************
*  IShellFolder_fnGetUIObjectOf
*
* PARAMETERS
*  HWND           hwndOwner, //[in ] Parent window for any output
*  UINT           cidl,      //[in ] array size
*  LPCITEMIDLIST* apidl,     //[in ] simple pidl array
*  REFIID         riid,      //[in ] Requested Interface
*  UINT*          prgfInOut, //[   ] reserved
*  LPVOID*        ppvObject) //[out] Resulting Interface
*
* NOTES
*  This function gets asked to return "view objects" for one or more (multiple select)
*  items:
*  The viewobject typically is an COM object with one of the following interfaces:
*  IExtractIcon,IDataObject,IContextMenu
*  In order to support icon positions in the default Listview your DataObject
*  must implement the SetData method (in addition to GetData :) - the shell passes
*  a barely documented "Icon positions" structure to SetData when the drag starts,
*  and GetData's it if the drop is in another explorer window that needs the positions.
*/
static HRESULT WINAPI IShellFolder_fnGetUIObjectOf(
	IShellFolder2 *	iface,
	HWND		hwndOwner,
	UINT		cidl,
	LPCITEMIDLIST *	apidl,
	REFIID		riid,
	UINT *		prgfInOut,
	LPVOID *	ppvOut)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	LPITEMIDLIST	pidl;
	IUnknown*	pObj = NULL;
	HRESULT		hr = E_INVALIDARG;

	TRACE("(%p)->(0x%04x,%u,apidl=%p,%s,%p,%p)\n",
	  This,hwndOwner,cidl,apidl,shdebugstr_guid(riid),prgfInOut,ppvOut);

	if (ppvOut)
	{
	  *ppvOut = NULL;

	  if(IsEqualIID(riid, &IID_IContextMenu) && (cidl >= 1))
	  {
	    pObj  = (LPUNKNOWN)ISvItemCm_Constructor((IShellFolder*)iface, This->pidlRoot, apidl, cidl);
	    hr = S_OK;
	  }
	  else if (IsEqualIID(riid, &IID_IDataObject) &&(cidl >= 1))
	  {
	    pObj = (LPUNKNOWN)IDataObject_Constructor (hwndOwner, This->pidlRoot, apidl, cidl);
	    hr = S_OK;
	  }
	  else if (IsEqualIID(riid, &IID_IExtractIconA) && (cidl == 1))
	  {
	    pidl = ILCombine(This->pidlRoot,apidl[0]);
	    pObj = (LPUNKNOWN)IExtractIconA_Constructor( pidl );
	    SHFree(pidl);
	    hr = S_OK;
	  }
	  else if (IsEqualIID(riid, &IID_IDropTarget) && (cidl >= 1))
	  {
	    hr = IShellFolder_QueryInterface(iface, &IID_IDropTarget, (LPVOID*)&pObj);
	  }
	  else
	  {
	    hr = E_NOINTERFACE;
	  }

	  if(!pObj)
	    hr = E_OUTOFMEMORY;

	  *ppvOut = pObj;
	}
	TRACE("(%p)->hr=0x%08lx\n",This, hr);
	return hr;
}

/**************************************************************************
*  IShellFolder_fnGetDisplayNameOf
*  Retrieves the display name for the specified file object or subfolder
*
* PARAMETERS
*  LPCITEMIDLIST pidl,    //[in ] complex pidl to item
*  DWORD         dwFlags, //[in ] SHGNO formatting flags
*  LPSTRRET      lpName)  //[out] Returned display name
*
* FIXME
*  if the name is in the pidl the ret value should be a STRRET_OFFSET
*/
#define GET_SHGDN_FOR(dwFlags)         ((DWORD)dwFlags & (DWORD)0x0000FF00)
#define GET_SHGDN_RELATION(dwFlags)    ((DWORD)dwFlags & (DWORD)0x000000FF)

static HRESULT WINAPI IShellFolder_fnGetDisplayNameOf(
	IShellFolder2 * iface,
	LPCITEMIDLIST pidl,
	DWORD dwFlags,
	LPSTRRET strRet)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	CHAR		szPath[MAX_PATH]= "";
	int		len = 0;
	BOOL		bSimplePidl;

	TRACE("(%p)->(pidl=%p,0x%08lx,%p)\n",This,pidl,dwFlags,strRet);
	pdump(pidl);

	if(!pidl || !strRet) return E_INVALIDARG;

	bSimplePidl = _ILIsPidlSimple(pidl);

	/* take names of special folders only if its only this folder */
	if (_ILIsSpecialFolder(pidl))
	{
	  if ( bSimplePidl)
	  {
	    _ILSimpleGetText(pidl, szPath, MAX_PATH); /* append my own path */
	  }
	}
	else
	{
	  if (!(dwFlags & SHGDN_INFOLDER) && (dwFlags & SHGDN_FORPARSING) && This->sPathRoot)
	  {
	    strcpy (szPath, This->sPathRoot);			/* get path to root*/
	    PathAddBackslashA(szPath);
	    len = strlen(szPath);
	  }
	  _ILSimpleGetText(pidl, szPath + len, MAX_PATH - len);	/* append my own path */

          /* MSDN also mentions SHGDN_FOREDITING, which isn't defined in wine */
          if(!_ILIsFolder(pidl) && !(dwFlags & SHGDN_FORPARSING) &&
             ((dwFlags & SHGDN_INFOLDER) || (dwFlags == SHGDN_NORMAL)))
          {
              HKEY hKey;
              DWORD dwData;
              DWORD dwDataSize = sizeof(DWORD);
              BOOL  doHide = 0; /* The default value is FALSE (win98 at least) */

              /* XXX should it do this only for known file types? -- that would make it even slower! */
			  /* XXX That's what the prompt says!! */
              if(!RegCreateKeyExA(HKEY_CURRENT_USER,
                                  "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
                                  0, 0, 0, KEY_ALL_ACCESS, 0, &hKey, 0))
              {
                  if(!RegQueryValueExA(hKey, "HideFileExt", 0, 0, (LPBYTE)&dwData, &dwDataSize))
                      doHide = dwData;
                  RegCloseKey(hKey);
              }
              if(doHide && szPath[0]!='.') PathRemoveExtensionA(szPath);
          }
	}

	if ( (dwFlags & SHGDN_FORPARSING) && !bSimplePidl)	/* go deeper if needed */
	{
	  PathAddBackslashA(szPath);
	  len = strlen(szPath);

	  if (!SUCCEEDED(SHELL32_GetDisplayNameOfChild(iface, pidl, dwFlags, szPath + len, MAX_PATH - len)))
	    return E_OUTOFMEMORY;
	}
	strRet->uType = STRRET_CSTR;
	lstrcpynA(strRet->u.cStr, szPath, MAX_PATH);

	TRACE("-- (%p)->(%s)\n", This, szPath);
	return S_OK;
}

/**************************************************************************
*  IShellFolder_fnSetNameOf
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
static HRESULT WINAPI IShellFolder_fnSetNameOf(
	IShellFolder2 * iface,
	HWND hwndOwner,
	LPCITEMIDLIST pidl, /*simple pidl*/
	LPCOLESTR lpName,
	DWORD dwFlags,
	LPITEMIDLIST *pPidlOut)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)
	char szSrc[MAX_PATH], szDest[MAX_PATH];
	int len;
	BOOL bIsFolder = _ILIsFolder(ILFindLastID(pidl));

	TRACE("(%p)->(%u,pidl=%p,%s,%lu,%p)\n",
	  This,hwndOwner,pidl,debugstr_w(lpName),dwFlags,pPidlOut);

	/* build source path */
	if (dwFlags & SHGDN_INFOLDER)
	{
	  strcpy(szSrc, This->sPathRoot);
	  PathAddBackslashA(szSrc);
	  len = strlen (szSrc);
	  _ILSimpleGetText(pidl, szSrc+len, MAX_PATH-len);
	}
	else
	{
	  SHGetPathFromIDListA(pidl, szSrc);
	}

	/* build destination path */
	strcpy(szDest, This->sPathRoot);
	PathAddBackslashA(szDest);
	len = strlen (szDest);
        WideCharToMultiByte( CP_ACP, 0, lpName, -1, szDest+len, MAX_PATH-len, NULL, NULL );
        szDest[MAX_PATH-1] = 0;
	TRACE("src=%s dest=%s\n", szSrc, szDest);
	if ( MoveFileA(szSrc, szDest) )
	{
	  if (pPidlOut) *pPidlOut = SHSimpleIDListFromPathA(szDest);
	  SHChangeNotifyA( bIsFolder?SHCNE_RENAMEFOLDER:SHCNE_RENAMEITEM, SHCNF_PATHA, szSrc, szDest);
	  return S_OK;
	}
	return E_FAIL;
}

static HRESULT WINAPI IShellFolder_fnGetDefaultSearchGUID(
	IShellFolder2 * iface,
	GUID *pguid)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)
	FIXME("(%p)\n",This);
	return E_NOTIMPL;
}
static HRESULT WINAPI IShellFolder_fnEnumSearches(
	IShellFolder2 * iface,
	IEnumExtraSearch **ppenum)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)
	FIXME("(%p)\n",This);
	return E_NOTIMPL;
}
static HRESULT WINAPI IShellFolder_fnGetDefaultColumn(
	IShellFolder2 * iface,
	DWORD dwRes,
	ULONG *pSort,
	ULONG *pDisplay)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	TRACE("(%p)\n",This);

	if (pSort) *pSort = 0;
	if (pDisplay) *pDisplay = 0;

	return S_OK;
}
static HRESULT WINAPI IShellFolder_fnGetDefaultColumnState(
	IShellFolder2 * iface,
	UINT iColumn,
	DWORD *pcsFlags)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	TRACE("(%p)\n",This);

	if (!pcsFlags || iColumn >= GENERICSHELLVIEWCOLUMNS ) return E_INVALIDARG;

	*pcsFlags = GenericSFHeader[iColumn].pcsFlags;

	return S_OK;
}
static HRESULT WINAPI IShellFolder_fnGetDetailsEx(
	IShellFolder2 * iface,
	LPCITEMIDLIST pidl,
	const SHCOLUMNID *pscid,
	VARIANT *pv)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)
	FIXME("(%p)\n",This);

	return E_NOTIMPL;
}
static HRESULT WINAPI IShellFolder_fnGetDetailsOf(
	IShellFolder2 * iface,
	LPCITEMIDLIST pidl,
	UINT iColumn,
	SHELLDETAILS *psd)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)
	HRESULT hr = E_FAIL;

	TRACE("(%p)->(%p %i %p)\n",This, pidl, iColumn, psd);

	if (!psd || iColumn >= GENERICSHELLVIEWCOLUMNS ) return E_INVALIDARG;

	if (!pidl)
	{
	  /* the header titles */
	  psd->fmt = GenericSFHeader[iColumn].fmt;
	  psd->cxChar = GenericSFHeader[iColumn].cxChar;
	  psd->str.uType = STRRET_CSTR;
	  LoadStringA(shell32_hInstance, GenericSFHeader[iColumn].colnameid, psd->str.u.cStr, MAX_PATH);
	  return S_OK;
	}
	else
	{
	  /* the data from the pidl */
	  switch(iColumn)
	  {
	    case 0:	/* name */
	      hr = IShellFolder_GetDisplayNameOf(iface, pidl, SHGDN_NORMAL | SHGDN_INFOLDER, &psd->str);
	      break;
	    case 1:	/* size */
	      _ILGetFileSize (pidl, psd->str.u.cStr, MAX_PATH);
	      break;
	    case 2:	/* type */
	      _ILGetFileType(pidl, psd->str.u.cStr, MAX_PATH);
	      break;
	    case 3:	/* date */
	      _ILGetFileDate(pidl, psd->str.u.cStr, MAX_PATH);
	      break;
	    case 4:	/* attributes */
	      _ILGetFileAttributes(pidl, psd->str.u.cStr, MAX_PATH);
	      break;
	  }
	  hr = S_OK;
	  psd->str.uType = STRRET_CSTR;
	}

	return hr;
}
static HRESULT WINAPI IShellFolder_fnMapNameToSCID(
	IShellFolder2 * iface,
	LPCWSTR pwszName,
	SHCOLUMNID *pscid)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)
	FIXME("(%p)\n",This);
	return E_NOTIMPL;
}

static ICOM_VTABLE(IShellFolder2) sfvt =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IShellFolder_fnQueryInterface,
	IShellFolder_fnAddRef,
	IShellFolder_fnRelease,
	IShellFolder_fnParseDisplayName,
	IShellFolder_fnEnumObjects,
	IShellFolder_fnBindToObject,
	IShellFolder_fnBindToStorage,
	IShellFolder_fnCompareIDs,
	IShellFolder_fnCreateViewObject,
	IShellFolder_fnGetAttributesOf,
	IShellFolder_fnGetUIObjectOf,
	IShellFolder_fnGetDisplayNameOf,
	IShellFolder_fnSetNameOf,

	/* ShellFolder2 */
	IShellFolder_fnGetDefaultSearchGUID,
	IShellFolder_fnEnumSearches,
	IShellFolder_fnGetDefaultColumn,
	IShellFolder_fnGetDefaultColumnState,
	IShellFolder_fnGetDetailsEx,
	IShellFolder_fnGetDetailsOf,
	IShellFolder_fnMapNameToSCID
};

/****************************************************************************
 * ISFHelper for IShellFolder implementation
 */

static HRESULT WINAPI ISFHelper_fnQueryInterface(
	ISFHelper *iface,
	REFIID riid,
	LPVOID *ppvObj)
{
	_ICOM_THIS_From_ISFHelper(IGenericSFImpl,iface);

	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	return IUnknown_QueryInterface(This->pUnkOuter, riid, ppvObj);
}

static ULONG WINAPI ISFHelper_fnAddRef(
	ISFHelper *iface)
{
	_ICOM_THIS_From_ISFHelper(IGenericSFImpl,iface);

	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	return IUnknown_AddRef(This->pUnkOuter);
}

static ULONG WINAPI ISFHelper_fnRelease(
	ISFHelper *iface)
{
	_ICOM_THIS_From_ISFHelper(IGenericSFImpl,iface);

	TRACE("(%p)\n", This);

	return IUnknown_Release(This->pUnkOuter);
}


/****************************************************************************
 * ISFHelper_fnAddFolder
 *
 * creates a unique folder name
 */

static HRESULT WINAPI ISFHelper_fnGetUniqueName(
	ISFHelper *iface,
	LPSTR lpName,
	UINT uLen)
{
	_ICOM_THIS_From_ISFHelper(IGenericSFImpl,iface)
	IEnumIDList * penum;
	HRESULT hr;
	char szText[MAX_PATH];
	char * szNewFolder = "New Folder";

	TRACE("(%p)(%s %u)\n", This, lpName, uLen);

	if (uLen < strlen(szNewFolder) + 4) return E_POINTER;

	strcpy(lpName, szNewFolder);

	hr = IShellFolder_fnEnumObjects(_IShellFolder2_(This), 0, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN, &penum);
	if (penum)
	{
	  LPITEMIDLIST pidl;
	  DWORD dwFetched;
	  int i=1;

next:     IEnumIDList_Reset(penum);
	  while(S_OK == IEnumIDList_Next(penum, 1, &pidl, &dwFetched) && dwFetched)
	  {
	    _ILSimpleGetText(pidl, szText, MAX_PATH);
	    if (0 == strcasecmp(szText, lpName))
	    {
	      sprintf(lpName, "%s %d", szNewFolder, i++);
	      if (i > 99)
	      {
	        hr = E_FAIL;
	        break;
	      }
	      goto next;
	    }
	  }

	  IEnumIDList_Release(penum);
	}
	return hr;
}

/****************************************************************************
 * ISFHelper_fnAddFolder
 *
 * adds a new folder.
 */

static HRESULT WINAPI ISFHelper_fnAddFolder(
	ISFHelper *iface,
	HWND hwnd,
	LPCSTR lpName,
	LPITEMIDLIST* ppidlOut)
{
	_ICOM_THIS_From_ISFHelper(IGenericSFImpl,iface)
	char lpstrNewDir[MAX_PATH];
	DWORD bRes;
	HRESULT hres = E_FAIL;

	TRACE("(%p)(%s %p)\n", This, lpName, ppidlOut);

	strcpy(lpstrNewDir, This->sPathRoot);
	PathAddBackslashA(lpstrNewDir);
	strcat(lpstrNewDir, lpName);

	bRes = CreateDirectoryA(lpstrNewDir, NULL);

	if (bRes)
	{
	  LPITEMIDLIST pidl, pidlitem;

	  pidlitem = SHSimpleIDListFromPathA(lpstrNewDir);

	  pidl = ILCombine(This->pidlRoot, pidlitem);
	  SHChangeNotifyA(SHCNE_MKDIR, SHCNF_IDLIST, pidl, NULL);
	  SHFree(pidl);

	  if (ppidlOut) *ppidlOut = pidlitem;
	  hres = S_OK;
	}
	else
	{
	  char lpstrText[128+MAX_PATH];
	  char lpstrTempText[128];
	  char lpstrCaption[256];

	  /* Cannot Create folder because of permissions */
	  LoadStringA(shell32_hInstance, IDS_CREATEFOLDER_DENIED, lpstrTempText, sizeof(lpstrTempText));
	  LoadStringA(shell32_hInstance, IDS_CREATEFOLDER_CAPTION, lpstrCaption, sizeof(lpstrCaption));
	  sprintf(lpstrText,lpstrTempText, lpstrNewDir);
	  MessageBoxA(hwnd,lpstrText, lpstrCaption, MB_OK | MB_ICONEXCLAMATION);
	}

	return hres;
}

/****************************************************************************
 * ISFHelper_fnDeleteItems
 *
 * deletes items in folder
 */
static HRESULT WINAPI ISFHelper_fnDeleteItems(
	ISFHelper *iface,
	UINT cidl,
	LPCITEMIDLIST* apidl)
{
	_ICOM_THIS_From_ISFHelper(IGenericSFImpl,iface)
	int i;
	char szPath[MAX_PATH];
        BOOL bConfirm = TRUE;

	TRACE("(%p)(%u %p)\n", This, cidl, apidl);

	/* deleting multiple items so give a slightly different warning */
	if(cidl != 1)
	{
          char tmp[8];
          snprintf(tmp, sizeof(tmp), "%d", cidl);
	  if(!SHELL_WarnItemDelete(ASK_DELETE_MULTIPLE_ITEM, tmp))
            return E_FAIL;
          bConfirm = FALSE;
	}

	for(i=0; i< cidl; i++)
	{
	  strcpy(szPath, This->sPathRoot);
	  PathAddBackslashA(szPath);
	  _ILSimpleGetText(apidl[i], szPath+strlen(szPath), MAX_PATH);

	  if (_ILIsFolder(apidl[i]))
	  {
	    LPITEMIDLIST pidl;
	    TRACE("delete %s\n", szPath);
	    if (! SHELL_DeleteDirectoryA(szPath, bConfirm))
	    {
              TRACE("delete %s failed, bConfirm=%d\n", szPath, bConfirm);
	      return E_FAIL;
	    }
	    pidl = ILCombine(This->pidlRoot, apidl[i]);
	    SHChangeNotifyA(SHCNE_RMDIR, SHCNF_IDLIST, pidl, NULL);
	    SHFree(pidl);
	  }
	  else if (_ILIsValue(apidl[i]))
	  {
	    LPITEMIDLIST pidl;

	    TRACE("delete %s\n", szPath);
	    if (! SHELL_DeleteFileA(szPath, bConfirm))
	    {
              TRACE("delete %s failed, bConfirm=%d\n", szPath, bConfirm);
	      return E_FAIL;
	    }
	    pidl = ILCombine(This->pidlRoot, apidl[i]);
	    SHChangeNotifyA(SHCNE_DELETE, SHCNF_IDLIST, pidl, NULL);
	    SHFree(pidl);
	  }

	}
	return S_OK;
}

/****************************************************************************
 * ISFHelper_fnCopyItems
 *
 * copies items to this folder
 */
static HRESULT WINAPI ISFHelper_fnCopyItems(
	ISFHelper *iface,
	IShellFolder* pSFFrom,
	UINT cidl,
	LPCITEMIDLIST *apidl)
{
	int i;
	IPersistFolder2 * ppf2=NULL;
	char szSrcPath[MAX_PATH], szDstPath[MAX_PATH];
	_ICOM_THIS_From_ISFHelper(IGenericSFImpl,iface);

	TRACE("(%p)->(%p,%u,%p)\n", This, pSFFrom, cidl, apidl);

	IShellFolder_QueryInterface(pSFFrom, &IID_IPersistFolder2, (LPVOID*)&ppf2);
	if (ppf2)
	{
	  LPITEMIDLIST pidl;
	  if (SUCCEEDED(IPersistFolder2_GetCurFolder(ppf2, &pidl)))
	  {
	    for (i=0; i<cidl; i++)
	    {
	      SHGetPathFromIDListA(pidl, szSrcPath);
	      PathAddBackslashA(szSrcPath);
	      _ILSimpleGetText(apidl[i], szSrcPath+strlen(szSrcPath), MAX_PATH);

	      strcpy(szDstPath, This->sPathRoot);
	      PathAddBackslashA(szDstPath);
	      _ILSimpleGetText(apidl[i], szDstPath+strlen(szDstPath), MAX_PATH);
	      MESSAGE("would copy %s to %s\n", szSrcPath, szDstPath);
	    }
	    SHFree(pidl);
	  }
	  IPersistFolder2_Release(ppf2);
	}
	return S_OK;
}

static ICOM_VTABLE(ISFHelper) shvt =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	ISFHelper_fnQueryInterface,
	ISFHelper_fnAddRef,
	ISFHelper_fnRelease,
	ISFHelper_fnGetUniqueName,
	ISFHelper_fnAddFolder,
	ISFHelper_fnDeleteItems,
	ISFHelper_fnCopyItems,
};

/***********************************************************************
* 	[Desktopfolder]	IShellFolder implementation
*/
static struct ICOM_VTABLE(IShellFolder2) sfdvt;

static shvheader DesktopSFHeader [] =
{
 { IDS_SHV_COLUMN1, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15 },
 { IDS_SHV_COLUMN2, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10 },
 { IDS_SHV_COLUMN3, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10 },
 { IDS_SHV_COLUMN4, SHCOLSTATE_TYPE_DATE | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 12 },
 { IDS_SHV_COLUMN5, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 5 }
};
#define DESKTOPSHELLVIEWCOLUMNS 5

/**************************************************************************
*	ISF_Desktop_Constructor
*
*/
HRESULT WINAPI ISF_Desktop_Constructor (
	IUnknown * pUnkOuter,
	REFIID riid,
	LPVOID * ppv)
{
	IGenericSFImpl *	sf;

	TRACE("unkOut=%p %s\n",pUnkOuter, shdebugstr_guid(riid));

	if(!ppv) return E_POINTER;

	if(pUnkOuter ) {
	    FIXME("CLASS_E_NOAGGREGATION\n");
	    return CLASS_E_NOAGGREGATION;
	}

	sf=(IGenericSFImpl*) LocalAlloc(GMEM_ZEROINIT,sizeof(IGenericSFImpl));
	sf->ref=1;
	ICOM_VTBL(sf)=&unkvt;
	sf->lpvtblShellFolder=&sfdvt;
	sf->pidlRoot=_ILCreateDesktop();	/* my qualified pidl */
	sf->pUnkOuter = (IUnknown *) &sf->lpVtbl;

	*ppv = _IShellFolder_(sf);
	shell32_ObjCount++;

	TRACE("--(%p)\n",sf);
	return S_OK;
}

/**************************************************************************
 *	ISF_Desktop_fnQueryInterface
 *
 * NOTES supports not IPersist/IPersistFolder
 */
static HRESULT WINAPI ISF_Desktop_fnQueryInterface(
	IShellFolder2 * iface,
	REFIID riid,
	LPVOID *ppvObj)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	TRACE("(%p)->(%s,%p)\n",This,shdebugstr_guid(riid),ppvObj);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
	{
	  *ppvObj = _IUnknown_(This);
	}
	else if(IsEqualIID(riid, &IID_IShellFolder))  /*IShellFolder*/
	{
	  *ppvObj = _IShellFolder_(This);
	}
	else if(IsEqualIID(riid, &IID_IShellFolder2))  /*IShellFolder2*/
	{
	  *ppvObj = _IShellFolder_(This);
	}

	if(*ppvObj)
	{
	  IUnknown_AddRef((IUnknown*)(*ppvObj));
	  TRACE("-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE("-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}

/**************************************************************************
*	ISF_Desktop_fnParseDisplayName
*
* NOTES
*	"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}" and "" binds
*	to MyComputer
*/
static HRESULT WINAPI ISF_Desktop_fnParseDisplayName(
	IShellFolder2 * iface,
	HWND hwndOwner,
	LPBC pbcReserved,
	LPOLESTR lpszDisplayName,
	DWORD *pchEaten,
	LPITEMIDLIST *ppidl,
	DWORD *pdwAttributes)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

        WCHAR           szElement[MAX_PATH];
	LPCWSTR		szNext=NULL;
	LPITEMIDLIST	pidlTemp=NULL;
	HRESULT		hr=E_OUTOFMEMORY;
	CLSID		clsid;

	TRACE("(%p)->(HWND=0x%08x,%p,%p=%s,%p,pidl=%p,%p)\n",
	This,hwndOwner,pbcReserved,lpszDisplayName,
	debugstr_w(lpszDisplayName),pchEaten,ppidl,pdwAttributes);

	*ppidl = 0;
	if (pchEaten) *pchEaten = 0;	/* strange but like the original */

	if(lpszDisplayName[0] == ':' && lpszDisplayName[1] == ':') {
	    szNext = GetNextElementW(lpszDisplayName, szElement, MAX_PATH);
	    TRACE("-- element: %s\n", debugstr_w(szElement));
	    CLSIDFromString(szElement+2, &clsid);
	    TRACE("-- %s\n", shdebugstr_guid(&clsid));
	    pidlTemp = _ILCreate(PT_MYCOMP, &clsid, sizeof(clsid));
	} else {
	    pidlTemp = _ILCreateMyComputer();
	    /* it's a filesystem path, so we cant cut anything away */
	    szNext = lpszDisplayName;
	}


	if (szNext && *szNext)
	{
	  hr = SHELL32_ParseNextElement(hwndOwner, iface, &pidlTemp, (LPOLESTR)szNext, pchEaten, pdwAttributes);
	}
	else
	{
	  hr = S_OK;

	  if (pdwAttributes && *pdwAttributes)
	  {
	    SHELL32_GetItemAttributes(_IShellFolder_(This), pidlTemp, pdwAttributes);
	  }
	}

	*ppidl = pidlTemp;

	TRACE("(%p)->(-- ret=0x%08lx)\n", This, hr);

	return hr;
}

/**************************************************************************
*		ISF_Desktop_fnEnumObjects
*/
static HRESULT WINAPI ISF_Desktop_fnEnumObjects(
	IShellFolder2 * iface,
	HWND hwndOwner,
	DWORD dwFlags,
	LPENUMIDLIST* ppEnumIDList)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	TRACE("(%p)->(HWND=0x%08x flags=0x%08lx pplist=%p)\n",This,hwndOwner,dwFlags,ppEnumIDList);

	*ppEnumIDList = NULL;
	*ppEnumIDList = IEnumIDList_Constructor (NULL, dwFlags, EIDL_DESK);

	TRACE("-- (%p)->(new ID List: %p)\n",This,*ppEnumIDList);

	if(!*ppEnumIDList) return E_OUTOFMEMORY;

	return S_OK;
}

/**************************************************************************
*		ISF_Desktop_fnBindToObject
*/
static HRESULT WINAPI ISF_Desktop_fnBindToObject( IShellFolder2 * iface, LPCITEMIDLIST pidl,
			LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)
	GUID		const * clsid;
	IShellFolder	*pShellFolder, *pSubFolder;
        IGenericSFImpl  *pSFImpl;
	HRESULT         hr;

	TRACE("(%p)->(pidl=%p,%p,%s,%p)\n",
              This,pidl,pbcReserved,shdebugstr_guid(riid),ppvOut);

	*ppvOut = NULL;

	if ((clsid=_ILGetGUIDPointer(pidl)))
	{
	  if ( IsEqualIID(clsid, &CLSID_MyComputer))
	  {
	    pShellFolder = ISF_MyComputer_Constructor();
	  }
	  else
	  {
	     /* shell extension */
	     if (!SUCCEEDED(SHELL32_CoCreateInitSF (This->pidlRoot, pidl, clsid, riid, (LPVOID*)&pShellFolder)))
	     {
	       return E_INVALIDARG;
	     }
	  }
	}
	else
	{
	  /* file system folder on the desktop */
	  LPITEMIDLIST deskpidl, firstpidl, completepidl;
	  IPersistFolder * ppf;

	  /* combine pidls */
	  if ((pSFImpl = IShellFolder_Constructor())) {
	    pShellFolder = _IShellFolder_(pSFImpl);
	    if (SUCCEEDED(IShellFolder_QueryInterface(pShellFolder, &IID_IPersistFolder, (LPVOID*)&ppf))) {
	      SHGetSpecialFolderLocation(0, CSIDL_DESKTOPDIRECTORY, &deskpidl);
	      firstpidl = ILCloneFirst(pidl);
	      completepidl = ILCombine(deskpidl, firstpidl);
	      IPersistFolder_Initialize(ppf, completepidl);
	      IPersistFolder_Release(ppf);
	      ILFree(completepidl);
	      ILFree(deskpidl);
	      ILFree(firstpidl);
	    }
	  }
	}

	if (_ILIsPidlSimple(pidl))	/* no sub folders */
	{
	  *ppvOut = pShellFolder;
	  hr = S_OK;
	}
	else				/* go deeper */
	{
	  hr = IShellFolder_BindToObject(pShellFolder, ILGetNext(pidl), NULL, riid, (LPVOID)&pSubFolder);
	  IShellFolder_Release(pShellFolder);
	  *ppvOut = pSubFolder;
	}

	TRACE("-- (%p) returning (%p) %08lx\n",This, *ppvOut, hr);

	return hr;
}

/**************************************************************************
*	ISF_Desktop_fnCreateViewObject
*/
static HRESULT WINAPI ISF_Desktop_fnCreateViewObject( IShellFolder2 * iface,
		 HWND hwndOwner, REFIID riid, LPVOID *ppvOut)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	LPSHELLVIEW	pShellView;
	HRESULT		hr = E_INVALIDARG;

	TRACE("(%p)->(hwnd=0x%x,%s,%p)\n",This,hwndOwner,shdebugstr_guid(riid),ppvOut);

	if(ppvOut)
	{
	  *ppvOut = NULL;

	  if(IsEqualIID(riid, &IID_IDropTarget))
	  {
	    WARN("IDropTarget not implemented\n");
	    hr = E_NOTIMPL;
	  }
	  else if(IsEqualIID(riid, &IID_IContextMenu))
	  {
	    WARN("IContextMenu not implemented\n");
	    hr = E_NOTIMPL;
	  }
	  else if(IsEqualIID(riid, &IID_IShellView))
	  {
	    pShellView = IShellView_Constructor((IShellFolder*)iface);
	    if(pShellView)
	    {
	      hr = IShellView_QueryInterface(pShellView, riid, ppvOut);
	      IShellView_Release(pShellView);
	    }
	  }
	}
	TRACE("-- (%p)->(interface=%p)\n",This, ppvOut);
	return hr;
}

/**************************************************************************
*  ISF_Desktop_fnGetAttributesOf
*/
static HRESULT WINAPI ISF_Desktop_fnGetAttributesOf(
	IShellFolder2 * iface,
	UINT cidl,
	LPCITEMIDLIST *apidl,
	DWORD *rgfInOut)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	HRESULT		hr = S_OK;

	TRACE("(%p)->(cidl=%d apidl=%p mask=0x%08lx)\n",This,cidl,apidl, *rgfInOut);

	if ( (!cidl) || (!apidl) || (!rgfInOut))
	  return E_INVALIDARG;

	while (cidl > 0 && *apidl)
	{
	  pdump (*apidl);
	  SHELL32_GetItemAttributes(_IShellFolder_(This), *apidl, rgfInOut);
	  apidl++;
	  cidl--;
	}

	TRACE("-- result=0x%08lx\n",*rgfInOut);

	return hr;
}

/**************************************************************************
*	ISF_Desktop_fnGetDisplayNameOf
*
* NOTES
*	special case: pidl = null gives desktop-name back
*/
static HRESULT WINAPI ISF_Desktop_fnGetDisplayNameOf(
	IShellFolder2 * iface,
	LPCITEMIDLIST pidl,
	DWORD dwFlags,
	LPSTRRET strRet)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	CHAR		szPath[MAX_PATH]= "";

	TRACE("(%p)->(pidl=%p,0x%08lx,%p)\n",This,pidl,dwFlags,strRet);
	pdump(pidl);

	if(!strRet) return E_INVALIDARG;

	if(!pidl)
	{
	  HCR_GetClassName(&CLSID_ShellDesktop, szPath, MAX_PATH);
	}
	else if ( _ILIsPidlSimple(pidl) )
	{
	  _ILSimpleGetText(pidl, szPath, MAX_PATH);
	}
	else
	{
	  if (!SUCCEEDED(SHELL32_GetDisplayNameOfChild(iface, pidl, dwFlags, szPath, MAX_PATH)))
	    return E_OUTOFMEMORY;
	}
	strRet->uType = STRRET_CSTR;
	lstrcpynA(strRet->u.cStr, szPath, MAX_PATH);


	TRACE("-- (%p)->(%s)\n", This, szPath);
	return S_OK;
}

static HRESULT WINAPI ISF_Desktop_fnGetDefaultSearchGUID(
	IShellFolder2 * iface,
	GUID *pguid)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)
	FIXME("(%p)\n",This);
	return E_NOTIMPL;
}
static HRESULT WINAPI ISF_Desktop_fnEnumSearches(
	IShellFolder2 * iface,
	IEnumExtraSearch **ppenum)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)
	FIXME("(%p)\n",This);
	return E_NOTIMPL;
}
static HRESULT WINAPI ISF_Desktop_fnGetDefaultColumn(
	IShellFolder2 * iface,
	DWORD dwRes,
	ULONG *pSort,
	ULONG *pDisplay)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	TRACE("(%p)\n",This);

	if (pSort) *pSort = 0;
	if (pDisplay) *pDisplay = 0;

	return S_OK;
}
static HRESULT WINAPI ISF_Desktop_fnGetDefaultColumnState(
	IShellFolder2 * iface,
	UINT iColumn,
	DWORD *pcsFlags)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	TRACE("(%p)\n",This);

	if (!pcsFlags || iColumn >= DESKTOPSHELLVIEWCOLUMNS ) return E_INVALIDARG;

	*pcsFlags = DesktopSFHeader[iColumn].pcsFlags;

	return S_OK;
}
static HRESULT WINAPI ISF_Desktop_fnGetDetailsEx(
	IShellFolder2 * iface,
	LPCITEMIDLIST pidl,
	const SHCOLUMNID *pscid,
	VARIANT *pv)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)
	FIXME("(%p)\n",This);

	return E_NOTIMPL;
}
static HRESULT WINAPI ISF_Desktop_fnGetDetailsOf(
	IShellFolder2 * iface,
	LPCITEMIDLIST pidl,
	UINT iColumn,
	SHELLDETAILS *psd)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)
	HRESULT hr = E_FAIL;

	TRACE("(%p)->(%p %i %p)\n",This, pidl, iColumn, psd);

	if (!psd || iColumn >= DESKTOPSHELLVIEWCOLUMNS ) return E_INVALIDARG;

	if (!pidl)
	{
	  psd->fmt = DesktopSFHeader[iColumn].fmt;
	  psd->cxChar = DesktopSFHeader[iColumn].cxChar;
	  psd->str.uType = STRRET_CSTR;
	  LoadStringA(shell32_hInstance, DesktopSFHeader[iColumn].colnameid, psd->str.u.cStr, MAX_PATH);
	  return S_OK;
	}
	else
	{
	  /* the data from the pidl */
	  switch(iColumn)
	  {
	    case 0:	/* name */
	      hr = IShellFolder_GetDisplayNameOf(iface, pidl, SHGDN_NORMAL | SHGDN_INFOLDER, &psd->str);
	      break;
	    case 1:	/* size */
	      _ILGetFileSize (pidl, psd->str.u.cStr, MAX_PATH);
	      break;
	    case 2:	/* type */
	      _ILGetFileType(pidl, psd->str.u.cStr, MAX_PATH);
	      break;
	    case 3:	/* date */
	      _ILGetFileDate(pidl, psd->str.u.cStr, MAX_PATH);
	      break;
	    case 4:	/* attributes */
	      _ILGetFileAttributes(pidl, psd->str.u.cStr, MAX_PATH);
	      break;
	  }
	  hr = S_OK;
	  psd->str.uType = STRRET_CSTR;
	}

	return hr;
}
static HRESULT WINAPI ISF_Desktop_fnMapNameToSCID(
	IShellFolder2 * iface,
	LPCWSTR pwszName,
	SHCOLUMNID *pscid)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)
	FIXME("(%p)\n",This);
	return E_NOTIMPL;
}

static ICOM_VTABLE(IShellFolder2) sfdvt =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	ISF_Desktop_fnQueryInterface,
	IShellFolder_fnAddRef,
	IShellFolder_fnRelease,
	ISF_Desktop_fnParseDisplayName,
	ISF_Desktop_fnEnumObjects,
	ISF_Desktop_fnBindToObject,
	IShellFolder_fnBindToStorage,
	IShellFolder_fnCompareIDs,
	ISF_Desktop_fnCreateViewObject,
	ISF_Desktop_fnGetAttributesOf,
	IShellFolder_fnGetUIObjectOf,
	ISF_Desktop_fnGetDisplayNameOf,
	IShellFolder_fnSetNameOf,

	/* ShellFolder2 */
	ISF_Desktop_fnGetDefaultSearchGUID,
	ISF_Desktop_fnEnumSearches,
	ISF_Desktop_fnGetDefaultColumn,
	ISF_Desktop_fnGetDefaultColumnState,
	ISF_Desktop_fnGetDetailsEx,
	ISF_Desktop_fnGetDetailsOf,
	ISF_Desktop_fnMapNameToSCID
};


/***********************************************************************
*   IShellFolder [MyComputer] implementation
*/

static struct ICOM_VTABLE(IShellFolder2) sfmcvt;

static shvheader MyComputerSFHeader [] =
{
 { IDS_SHV_COLUMN1, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15 },
 { IDS_SHV_COLUMN3, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10 },
 { IDS_SHV_COLUMN6, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10 },
 { IDS_SHV_COLUMN7, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10 },
};
#define MYCOMPUTERSHELLVIEWCOLUMNS 4

/**************************************************************************
*	ISF_MyComputer_Constructor
*/
static IShellFolder * ISF_MyComputer_Constructor(void)
{
	IGenericSFImpl *	sf;

	sf=(IGenericSFImpl*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IGenericSFImpl));
	sf->ref=1;

	ICOM_VTBL(sf)=&unkvt;
	sf->lpvtblShellFolder=&sfmcvt;
	sf->lpvtblPersistFolder3 = &psfvt;
	sf->pclsid = (CLSID*)&CLSID_MyComputer;
	sf->pidlRoot=_ILCreateMyComputer();	/* my qualified pidl */
	sf->pUnkOuter = (IUnknown *) &sf->lpVtbl;

	TRACE("(%p)\n",sf);

	shell32_ObjCount++;
	return _IShellFolder_(sf);
}

/**************************************************************************
*	ISF_MyComputer_fnParseDisplayName
*/
static HRESULT WINAPI ISF_MyComputer_fnParseDisplayName(
	IShellFolder2 * iface,
	HWND hwndOwner,
	LPBC pbcReserved,
	LPOLESTR lpszDisplayName,
	DWORD *pchEaten,
	LPITEMIDLIST *ppidl,
	DWORD *pdwAttributes)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	HRESULT		hr = E_OUTOFMEMORY;
	LPCWSTR		szNext=NULL;
	WCHAR		szElement[MAX_PATH];
	CHAR		szTempA[MAX_PATH];
	LPITEMIDLIST	pidlTemp;

	TRACE("(%p)->(HWND=0x%08x,%p,%p=%s,%p,pidl=%p,%p)\n",
	This,hwndOwner,pbcReserved,lpszDisplayName,
	debugstr_w(lpszDisplayName),pchEaten,ppidl,pdwAttributes);

	*ppidl = 0;
	if (pchEaten) *pchEaten = 0;	/* strange but like the original */

	/* do we have an absolute path name ? */
	if (PathGetDriveNumberW(lpszDisplayName) >= 0 &&
	    lpszDisplayName[2] == (WCHAR)'\\')
	{
	  szNext = GetNextElementW(lpszDisplayName, szElement, MAX_PATH);
          WideCharToMultiByte( CP_ACP, 0, szElement, -1, szTempA, MAX_PATH, NULL, NULL );
	  pidlTemp = _ILCreateDrive(szTempA);

	  if (szNext && *szNext)
	  {
	    hr = SHELL32_ParseNextElement(hwndOwner, iface, &pidlTemp, (LPOLESTR)szNext, pchEaten, pdwAttributes);
	  }
	  else
	  {
	    if (pdwAttributes && *pdwAttributes)
	    {
	      SHELL32_GetItemAttributes(_IShellFolder_(This), pidlTemp, pdwAttributes);
	    }
	    hr = S_OK;
	  }
	  *ppidl = pidlTemp;
	}

	TRACE("(%p)->(-- ret=0x%08lx)\n", This, hr);

	return hr;
}

/**************************************************************************
*		ISF_MyComputer_fnEnumObjects
*/
static HRESULT WINAPI ISF_MyComputer_fnEnumObjects(
	IShellFolder2 * iface,
	HWND hwndOwner,
	DWORD dwFlags,
	LPENUMIDLIST* ppEnumIDList)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	TRACE("(%p)->(HWND=0x%08x flags=0x%08lx pplist=%p)\n",This,hwndOwner,dwFlags,ppEnumIDList);

	*ppEnumIDList = NULL;
	*ppEnumIDList = IEnumIDList_Constructor (NULL, dwFlags, EIDL_MYCOMP);

	TRACE("-- (%p)->(new ID List: %p)\n",This,*ppEnumIDList);

	if(!*ppEnumIDList) return E_OUTOFMEMORY;

	return S_OK;
}

/**************************************************************************
*		ISF_MyComputer_fnBindToObject
*/
static HRESULT WINAPI ISF_MyComputer_fnBindToObject( IShellFolder2 * iface, LPCITEMIDLIST pidl,
			LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)
	GUID		const * clsid;
	IShellFolder	*pShellFolder, *pSubFolder;
        IGenericSFImpl  *pSFImpl;
	LPITEMIDLIST	pidltemp;
	HRESULT         hr;

	TRACE("(%p)->(pidl=%p,%p,%s,%p)\n",
              This,pidl,pbcReserved,shdebugstr_guid(riid),ppvOut);

	if(!pidl || !ppvOut) return E_INVALIDARG;

	*ppvOut = NULL;

	if ((clsid=_ILGetGUIDPointer(pidl)) && !IsEqualIID(clsid, &CLSID_MyComputer))
	{
	   if (!SUCCEEDED(SHELL32_CoCreateInitSF (This->pidlRoot, pidl, clsid, riid, (LPVOID*)&pShellFolder)))
	   {
	     return E_FAIL;
	   }
	}
	else
	{
	  if (!_ILIsDrive(pidl)) return E_INVALIDARG;

	  if ((pSFImpl = IShellFolder_Constructor())) {
	    pidltemp = ILCloneFirst(pidl);
	    InitializeGenericSF(pSFImpl, This->pidlRoot, pidltemp, This->sPathRoot);
	    ILFree(pidltemp);
	    pShellFolder = _IShellFolder_(pSFImpl);
	  }
	}

	if (_ILIsPidlSimple(pidl))	/* no sub folders */
	{
	  *ppvOut = pShellFolder;
	  hr = S_OK;
	}
	else				/* go deeper */
	{
	  hr = IShellFolder_BindToObject(pShellFolder, ILGetNext(pidl), NULL,
					 riid, (LPVOID)&pSubFolder);
	  IShellFolder_Release(pShellFolder);
	  *ppvOut = pSubFolder;
	}

	TRACE("-- (%p) returning (%p) %08lx\n",This, *ppvOut, hr);

	return hr;
}

/**************************************************************************
*	ISF_MyComputer_fnCreateViewObject
*/
static HRESULT WINAPI ISF_MyComputer_fnCreateViewObject( IShellFolder2 * iface,
		 HWND hwndOwner, REFIID riid, LPVOID *ppvOut)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	LPSHELLVIEW	pShellView;
	HRESULT		hr = E_INVALIDARG;

	TRACE("(%p)->(hwnd=0x%x,%s,%p)\n",This,hwndOwner,shdebugstr_guid(riid),ppvOut);

	if(ppvOut)
	{
	  *ppvOut = NULL;

	  if(IsEqualIID(riid, &IID_IDropTarget))
	  {
	    WARN("IDropTarget not implemented\n");
	    hr = E_NOTIMPL;
	  }
	  else if(IsEqualIID(riid, &IID_IContextMenu))
	  {
	    WARN("IContextMenu not implemented\n");
	    hr = E_NOTIMPL;
	  }
	  else if(IsEqualIID(riid, &IID_IShellView))
	  {
	    pShellView = IShellView_Constructor((IShellFolder*)iface);
	    if(pShellView)
	    {
	      hr = IShellView_QueryInterface(pShellView, riid, ppvOut);
	      IShellView_Release(pShellView);
	    }
	  }
	}
	TRACE("-- (%p)->(interface=%p)\n",This, ppvOut);
	return hr;
}

/**************************************************************************
*  ISF_MyComputer_fnGetAttributesOf
*/
static HRESULT WINAPI ISF_MyComputer_fnGetAttributesOf(
	IShellFolder2 * iface,
	UINT cidl,
	LPCITEMIDLIST *apidl,
	DWORD *rgfInOut)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	HRESULT		hr = S_OK;

	TRACE("(%p)->(cidl=%d apidl=%p mask=0x%08lx)\n",This,cidl,apidl,*rgfInOut);

	if ( (!cidl) || (!apidl) || (!rgfInOut))
	  return E_INVALIDARG;

	while (cidl > 0 && *apidl)
	{
	  pdump (*apidl);
	  SHELL32_GetItemAttributes(_IShellFolder_(This), *apidl, rgfInOut);
	  apidl++;
	  cidl--;
	}

	TRACE("-- result=0x%08lx\n",*rgfInOut);
	return hr;
}

/**************************************************************************
*	ISF_MyComputer_fnGetDisplayNameOf
*
* NOTES
*	The desktopfolder creates only complete paths (SHGDN_FORPARSING).
*	SHGDN_INFOLDER makes no sense.
*/
static HRESULT WINAPI ISF_MyComputer_fnGetDisplayNameOf(
	IShellFolder2 * iface,
	LPCITEMIDLIST pidl,
	DWORD dwFlags,
	LPSTRRET strRet)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	char		szPath[MAX_PATH], szDrive[18];
	int		len = 0;
	BOOL		bSimplePidl;

	TRACE("(%p)->(pidl=%p,0x%08lx,%p)\n",This,pidl,dwFlags,strRet);
	pdump(pidl);

	if(!strRet) return E_INVALIDARG;

	szPath[0]=0x00; szDrive[0]=0x00;


	bSimplePidl = _ILIsPidlSimple(pidl);

	if (_ILIsSpecialFolder(pidl))
	{
	  /* take names of special folders only if its only this folder */
	  if ( bSimplePidl )
	  {
	    _ILSimpleGetText(pidl, szPath, MAX_PATH); /* append my own path */
	  }
	}
	else
	{
	  if (!_ILIsDrive(pidl))
	  {
	    ERR("Wrong pidl type\n");
	    return E_INVALIDARG;
	  }

	  _ILSimpleGetText(pidl, szPath, MAX_PATH);	/* append my own path */

	  /* long view "lw_name (C:)" */
	  if ( bSimplePidl && !(dwFlags & SHGDN_FORPARSING))
	  {
	    DWORD dwVolumeSerialNumber,dwMaximumComponetLength,dwFileSystemFlags;

	    GetVolumeInformationA(szPath,szDrive,sizeof(szDrive)-6,&dwVolumeSerialNumber,&dwMaximumComponetLength,&dwFileSystemFlags,NULL,0);
	    strcat (szDrive," (");
	    strncat (szDrive, szPath, 2);
	    strcat (szDrive,")");
	    strcpy (szPath, szDrive);
	  }
	}

	if (!bSimplePidl)	/* go deeper if needed */
	{
	  PathAddBackslashA(szPath);
	  len = strlen(szPath);

	  if (!SUCCEEDED(SHELL32_GetDisplayNameOfChild(iface, pidl, dwFlags | SHGDN_FORPARSING, szPath + len, MAX_PATH - len)))
	    return E_OUTOFMEMORY;
	}
	strRet->uType = STRRET_CSTR;
	lstrcpynA(strRet->u.cStr, szPath, MAX_PATH);


	TRACE("-- (%p)->(%s)\n", This, szPath);
	return S_OK;
}

static HRESULT WINAPI ISF_MyComputer_fnGetDefaultSearchGUID(
	IShellFolder2 * iface,
	GUID *pguid)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)
	FIXME("(%p)\n",This);
	return E_NOTIMPL;
}
static HRESULT WINAPI ISF_MyComputer_fnEnumSearches(
	IShellFolder2 * iface,
	IEnumExtraSearch **ppenum)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)
	FIXME("(%p)\n",This);
	return E_NOTIMPL;
}
static HRESULT WINAPI ISF_MyComputer_fnGetDefaultColumn(
	IShellFolder2 * iface,
	DWORD dwRes,
	ULONG *pSort,
	ULONG *pDisplay)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	TRACE("(%p)\n",This);

	if (pSort) *pSort = 0;
	if (pDisplay) *pDisplay = 0;

	return S_OK;
}
static HRESULT WINAPI ISF_MyComputer_fnGetDefaultColumnState(
	IShellFolder2 * iface,
	UINT iColumn,
	DWORD *pcsFlags)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	TRACE("(%p)\n",This);

	if (!pcsFlags || iColumn >= MYCOMPUTERSHELLVIEWCOLUMNS ) return E_INVALIDARG;

	*pcsFlags = MyComputerSFHeader[iColumn].pcsFlags;

	return S_OK;
}
static HRESULT WINAPI ISF_MyComputer_fnGetDetailsEx(
	IShellFolder2 * iface,
	LPCITEMIDLIST pidl,
	const SHCOLUMNID *pscid,
	VARIANT *pv)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)
	FIXME("(%p)\n",This);

	return E_NOTIMPL;
}

/* FIXME: drive size >4GB is rolling over */
static HRESULT WINAPI ISF_MyComputer_fnGetDetailsOf(
	IShellFolder2 * iface,
	LPCITEMIDLIST pidl,
	UINT iColumn,
	SHELLDETAILS *psd)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)
	HRESULT hr;

	TRACE("(%p)->(%p %i %p)\n",This, pidl, iColumn, psd);

	if (!psd || iColumn >= MYCOMPUTERSHELLVIEWCOLUMNS ) return E_INVALIDARG;

	if (!pidl)
	{
	  psd->fmt = MyComputerSFHeader[iColumn].fmt;
	  psd->cxChar = MyComputerSFHeader[iColumn].cxChar;
	  psd->str.uType = STRRET_CSTR;
	  LoadStringA(shell32_hInstance, MyComputerSFHeader[iColumn].colnameid, psd->str.u.cStr, MAX_PATH);
	  return S_OK;
	}
	else
	{
	  char szPath[MAX_PATH];
	  ULARGE_INTEGER ulBytes;

	  psd->str.u.cStr[0] = 0x00;
	  psd->str.uType = STRRET_CSTR;
	  switch(iColumn)
	  {
	    case 0:	/* name */
	      hr = IShellFolder_GetDisplayNameOf(iface, pidl, SHGDN_NORMAL | SHGDN_INFOLDER, &psd->str);
	      break;
	    case 1:	/* type */
	      _ILGetFileType(pidl, psd->str.u.cStr, MAX_PATH);
	      break;
	    case 2:	/* total size */
	      if (_ILIsDrive(pidl))
	      {
	        _ILSimpleGetText(pidl, szPath, MAX_PATH);
	        GetDiskFreeSpaceExA(szPath, NULL, &ulBytes, NULL);
	        StrFormatByteSizeA(ulBytes.s.LowPart, psd->str.u.cStr, MAX_PATH);
	      }
	      break;
	    case 3:	/* free size */
	      if (_ILIsDrive(pidl))
	      {
	        _ILSimpleGetText(pidl, szPath, MAX_PATH);
	        GetDiskFreeSpaceExA(szPath, &ulBytes, NULL, NULL);
	        StrFormatByteSizeA(ulBytes.s.LowPart, psd->str.u.cStr, MAX_PATH);
	      }
	      break;
	  }
	  hr = S_OK;
	}

	return hr;
}
static HRESULT WINAPI ISF_MyComputer_fnMapNameToSCID(
	IShellFolder2 * iface,
	LPCWSTR pwszName,
	SHCOLUMNID *pscid)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)
	FIXME("(%p)\n",This);
	return E_NOTIMPL;
}

static ICOM_VTABLE(IShellFolder2) sfmcvt =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IShellFolder_fnQueryInterface,
	IShellFolder_fnAddRef,
	IShellFolder_fnRelease,
	ISF_MyComputer_fnParseDisplayName,
	ISF_MyComputer_fnEnumObjects,
	ISF_MyComputer_fnBindToObject,
	IShellFolder_fnBindToStorage,
	IShellFolder_fnCompareIDs,
	ISF_MyComputer_fnCreateViewObject,
	ISF_MyComputer_fnGetAttributesOf,
	IShellFolder_fnGetUIObjectOf,
	ISF_MyComputer_fnGetDisplayNameOf,
	IShellFolder_fnSetNameOf,

	/* ShellFolder2 */
	ISF_MyComputer_fnGetDefaultSearchGUID,
	ISF_MyComputer_fnEnumSearches,
	ISF_MyComputer_fnGetDefaultColumn,
	ISF_MyComputer_fnGetDefaultColumnState,
	ISF_MyComputer_fnGetDetailsEx,
	ISF_MyComputer_fnGetDetailsOf,
	ISF_MyComputer_fnMapNameToSCID
};


/************************************************************************
 * ISFPersistFolder_QueryInterface (IUnknown)
 *
 */
static HRESULT WINAPI ISFPersistFolder3_QueryInterface(
	IPersistFolder3 *	iface,
	REFIID			iid,
	LPVOID*			ppvObj)
{
	_ICOM_THIS_From_IPersistFolder3(IGenericSFImpl, iface);

	TRACE("(%p)\n", This);

	return IUnknown_QueryInterface(This->pUnkOuter, iid, ppvObj);
}

/************************************************************************
 * ISFPersistFolder_AddRef (IUnknown)
 *
 */
static ULONG WINAPI ISFPersistFolder3_AddRef(
	IPersistFolder3 *	iface)
{
	_ICOM_THIS_From_IPersistFolder3(IGenericSFImpl, iface);

	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	return IUnknown_AddRef(This->pUnkOuter);
}

/************************************************************************
 * ISFPersistFolder_Release (IUnknown)
 *
 */
static ULONG WINAPI ISFPersistFolder3_Release(
	IPersistFolder3 *	iface)
{
	_ICOM_THIS_From_IPersistFolder3(IGenericSFImpl, iface);

	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	return IUnknown_Release(This->pUnkOuter);
}

/************************************************************************
 * ISFPersistFolder_GetClassID (IPersist)
 */
static HRESULT WINAPI ISFPersistFolder3_GetClassID(
	IPersistFolder3 *	iface,
	CLSID *			lpClassId)
{
	_ICOM_THIS_From_IPersistFolder3(IGenericSFImpl, iface);

	TRACE("(%p)\n", This);

	if (!lpClassId) return E_POINTER;
	*lpClassId = *This->pclsid;

	return S_OK;
}

/************************************************************************
 * ISFPersistFolder_Initialize (IPersistFolder)
 *
 * NOTES
 *  sPathRoot is not set. Don't know how to handle in a non rooted environment.
 */
static HRESULT WINAPI ISFPersistFolder3_Initialize(
	IPersistFolder3 *	iface,
	LPCITEMIDLIST		pidl)
{
	char sTemp[MAX_PATH];
	_ICOM_THIS_From_IPersistFolder3(IGenericSFImpl, iface);

	TRACE("(%p)->(%p)\n", This, pidl);

	if(This->pidlRoot) SHFree(This->pidlRoot);	/* free the old pidl */
	This->pidlRoot = ILClone(pidl);			/* set my pidl */

	if(This->sPathRoot) SHFree(This->sPathRoot);

	/* set my path */
	if (SHGetPathFromIDListA(pidl, sTemp))
	{
	  This->sPathRoot = SHAlloc(strlen(sTemp)+1);
	  strcpy(This->sPathRoot, sTemp);
	}

	TRACE("--(%p)->(%s)\n", This, This->sPathRoot);
	return S_OK;
}

/**************************************************************************
*  IPersistFolder2_fnGetCurFolder
*/
static HRESULT WINAPI ISFPersistFolder3_fnGetCurFolder(
	IPersistFolder3 *	iface,
	LPITEMIDLIST * pidl)
{
	_ICOM_THIS_From_IPersistFolder3(IGenericSFImpl, iface);

	TRACE("(%p)->(%p)\n",This, pidl);

	if (!pidl) return E_POINTER;

	*pidl = ILClone(This->pidlRoot);

	return S_OK;
}

static HRESULT WINAPI ISFPersistFolder3_InitializeEx(
	IPersistFolder3 *	iface,
	IBindCtx *		pbc,
	LPCITEMIDLIST		pidlRoot,
	const PERSIST_FOLDER_TARGET_INFO *ppfti)
{
	char sTemp[MAX_PATH];
	_ICOM_THIS_From_IPersistFolder3(IGenericSFImpl, iface);

	FIXME("(%p)->(%p,%p,%p)\n",This,pbc,pidlRoot,ppfti);
	TRACE("--%p %s %s 0x%08lx 0x%08x\n",
	  ppfti->pidlTargetFolder, debugstr_w(ppfti->szTargetParsingName),
	  debugstr_w(ppfti->szNetworkProvider), ppfti->dwAttributes, ppfti->csidl);

	pdump(pidlRoot);
	if(ppfti->pidlTargetFolder) pdump(ppfti->pidlTargetFolder);

	if(This->pidlRoot) SHFree(This->pidlRoot);	/* free the old pidl */

	if(ppfti->csidl == -1) {			/* set my pidl */
	    This->pidlRoot = ILClone(ppfti->pidlTargetFolder);
	} else {
	    SHGetSpecialFolderLocation(0, ppfti->csidl, &This->pidlRoot);
	}

	if(This->sPathRoot) SHFree(This->sPathRoot);

	/* set my path */
	if (SHGetPathFromIDListA(This->pidlRoot, sTemp))
	{
	  This->sPathRoot = SHAlloc(strlen(sTemp)+1);
	  strcpy(This->sPathRoot, sTemp);
	}

	TRACE("--(%p)->(%s)\n", This, This->sPathRoot);
	return S_OK;
}

static HRESULT WINAPI ISFPersistFolder3_GetFolderTargetInfo(
	IPersistFolder3 *iface,
	PERSIST_FOLDER_TARGET_INFO *ppfti)
{
	_ICOM_THIS_From_IPersistFolder3(IGenericSFImpl, iface);
	FIXME("(%p)->(%p)\n",This,ppfti);
	ZeroMemory(ppfti, sizeof(ppfti));
	return E_NOTIMPL;
}

static ICOM_VTABLE(IPersistFolder3) psfvt =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	ISFPersistFolder3_QueryInterface,
	ISFPersistFolder3_AddRef,
	ISFPersistFolder3_Release,
	ISFPersistFolder3_GetClassID,
	ISFPersistFolder3_Initialize,
	ISFPersistFolder3_fnGetCurFolder,
	ISFPersistFolder3_InitializeEx,
	ISFPersistFolder3_GetFolderTargetInfo
};

/****************************************************************************
 * ISFDropTarget implementation
 */
static BOOL ISFDropTarget_QueryDrop(
	IDropTarget *iface,
	DWORD dwKeyState,
	LPDWORD pdwEffect)
{
	DWORD dwEffect = *pdwEffect;

	_ICOM_THIS_From_IDropTarget(IGenericSFImpl,iface);

	*pdwEffect = DROPEFFECT_NONE;

	if (This->fAcceptFmt)
	{ /* Does our interpretation of the keystate ... */
	  *pdwEffect = KeyStateToDropEffect(dwKeyState);

	  /* ... matches the desired effect ? */
	  if (dwEffect & *pdwEffect)
	  {
	    return TRUE;
	  }
	}
	return FALSE;
}

static HRESULT WINAPI ISFDropTarget_QueryInterface(
	IDropTarget *iface,
	REFIID riid,
	LPVOID *ppvObj)
{
	_ICOM_THIS_From_IDropTarget(IGenericSFImpl,iface);

	TRACE("(%p)\n", This);

	return IUnknown_QueryInterface(This->pUnkOuter, riid, ppvObj);
}

static ULONG WINAPI ISFDropTarget_AddRef( IDropTarget *iface)
{
	_ICOM_THIS_From_IDropTarget(IGenericSFImpl,iface);

	TRACE("(%p)\n", This);

	return IUnknown_AddRef(This->pUnkOuter);
}

static ULONG WINAPI ISFDropTarget_Release( IDropTarget *iface)
{
	_ICOM_THIS_From_IDropTarget(IGenericSFImpl,iface);

	TRACE("(%p)\n", This);

	return IUnknown_Release(This->pUnkOuter);
}

static HRESULT WINAPI ISFDropTarget_DragEnter(
	IDropTarget 	*iface,
	IDataObject	*pDataObject,
	DWORD		dwKeyState,
	POINTL		pt,
	DWORD		*pdwEffect)
{
	FORMATETC	fmt;

	_ICOM_THIS_From_IDropTarget(IGenericSFImpl,iface);

	TRACE("(%p)->(DataObject=%p)\n",This,pDataObject);

	InitFormatEtc(fmt, This->cfShellIDList, TYMED_HGLOBAL);

	This->fAcceptFmt = (S_OK == IDataObject_QueryGetData(pDataObject, &fmt)) ? TRUE : FALSE;

	ISFDropTarget_QueryDrop(iface, dwKeyState, pdwEffect);

	return S_OK;
}

static HRESULT WINAPI ISFDropTarget_DragOver(
	IDropTarget	*iface,
	DWORD		dwKeyState,
	POINTL		pt,
	DWORD		*pdwEffect)
{
	_ICOM_THIS_From_IDropTarget(IGenericSFImpl,iface);

	TRACE("(%p)\n",This);

	if(!pdwEffect) return E_INVALIDARG;

	ISFDropTarget_QueryDrop(iface, dwKeyState, pdwEffect);

	return S_OK;
}

static HRESULT WINAPI ISFDropTarget_DragLeave(
	IDropTarget	*iface)
{
	_ICOM_THIS_From_IDropTarget(IGenericSFImpl,iface);

	TRACE("(%p)\n",This);

	This->fAcceptFmt = FALSE;

	return S_OK;
}

static HRESULT WINAPI ISFDropTarget_Drop(
	IDropTarget	*iface,
	IDataObject*	pDataObject,
	DWORD		dwKeyState,
	POINTL		pt,
	DWORD		*pdwEffect)
{
	_ICOM_THIS_From_IDropTarget(IGenericSFImpl,iface);

	FIXME("(%p) object dropped\n",This);

	return E_NOTIMPL;
}

static struct ICOM_VTABLE(IDropTarget) dtvt =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	ISFDropTarget_QueryInterface,
	ISFDropTarget_AddRef,
	ISFDropTarget_Release,
	ISFDropTarget_DragEnter,
	ISFDropTarget_DragOver,
	ISFDropTarget_DragLeave,
 	ISFDropTarget_Drop
};
