/*
 *	Shell Folder stuff
 *
 *	Copyright 1997	Marcus Meissner
 *	Copyright 1998, 1999	Juergen Schmied
 *	
 *	IShellFolder2 and related interfaces
 *
 */

#include <stdlib.h>
#include <string.h>

#include "debugtools.h"
#include "winerror.h"

#include "oleidl.h"
#include "shlguid.h"

#include "pidl.h"
#include "wine/obj_base.h"
#include "wine/obj_dragdrop.h"
#include "wine/obj_shellfolder.h"
#include "wine/undocshell.h"
#include "shell32_main.h"
#include "shresdef.h"

DEFAULT_DEBUG_CHANNEL(shell)


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
	
	TRACE("(%p %p %s)\n",psf, pidlInOut? *pidlInOut: NULL, debugstr_w(szNext));


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
	LPITEMIDLIST	absPidl;
	IShellFolder2 	*pShellFolder;
	IPersistFolder	*pPersistFolder;

	TRACE("%p %p\n", pidlRoot, pidlChild);

	*ppvOut = NULL;
	
	/* we have to ask first for IPersistFolder, some special folders are expecting this */
	hr = SHCoCreateInstance(NULL, clsid, NULL, &IID_IPersistFolder, (LPVOID*)&pPersistFolder);
	if (SUCCEEDED(hr))
	{
	  hr = IPersistFolder_QueryInterface(pPersistFolder, iid, (LPVOID*)&pShellFolder);
	  if (SUCCEEDED(hr))
	  {
	    absPidl = ILCombine (pidlRoot, pidlChild);
	    hr = IPersistFolder_Initialize(pPersistFolder, absPidl);
	    IPersistFolder_Release(pPersistFolder);
	    SHFree(absPidl);
	    *ppvOut = pShellFolder;
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
*   IShellFolder implementation
*/

typedef struct 
{
	ICOM_VFIELD(IUnknown);
	DWORD				ref;
	ICOM_VTABLE(IShellFolder2)*	lpvtblShellFolder;;
	ICOM_VTABLE(IPersistFolder)*	lpvtblPersistFolder;
	ICOM_VTABLE(IDropTarget)*	lpvtblDropTarget;
	
	IUnknown			*pUnkOuter;	/* used for aggregation */

	CLSID*				pclsid;

	LPSTR				sMyPath;
	LPITEMIDLIST			absPidl;	/* complete pidl */

	UINT		cfShellIDList;			/* clipboardformat for IDropTarget */
	BOOL		fAcceptFmt;			/* flag for pending Drop */
} IGenericSFImpl;

static struct ICOM_VTABLE(IUnknown) unkvt;
static struct ICOM_VTABLE(IShellFolder2) sfvt;
static struct ICOM_VTABLE(IPersistFolder) psfvt;
static struct ICOM_VTABLE(IDropTarget) dtvt;

static IShellFolder * ISF_MyComputer_Constructor(void);

#define _IShellFolder2_Offset ((int)(&(((IGenericSFImpl*)0)->lpvtblShellFolder))) 
#define _ICOM_THIS_From_IShellFolder2(class, name) class* This = (class*)(((char*)name)-_IShellFolder2_Offset); 
	
#define _IPersistFolder_Offset ((int)(&(((IGenericSFImpl*)0)->lpvtblPersistFolder))) 
#define _ICOM_THIS_From_IPersistFolder(class, name) class* This = (class*)(((char*)name)-_IPersistFolder_Offset); 
	
#define _IDropTarget_Offset ((int)(&(((IGenericSFImpl*)0)->lpvtblDropTarget))) 
#define _ICOM_THIS_From_IDropTarget(class, name) class* This = (class*)(((char*)name)-_IDropTarget_Offset); 

/*
  converts This to a interface pointer
*/
#define _IUnknown_(This)	(IUnknown*)&(This->lpVtbl)
#define _IShellFolder_(This)	(IShellFolder*)&(This->lpvtblShellFolder)
#define _IShellFolder2_(This)	(IShellFolder2*)&(This->lpvtblShellFolder)
#define _IPersist_(This)	(IPersist*)&(This->lpvtblPersistFolder)
#define _IPersistFolder_(This)	(IPersistFolder*)&(This->lpvtblPersistFolder)
#define _IDropTarget_(This)	(IDropTarget*)&(This->lpvtblDropTarget)
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
*	we need a seperate IUnknown to handle aggregation
*	(inner IUnknown)
*/
static HRESULT WINAPI IUnknown_fnQueryInterface(
	IUnknown * iface,
	REFIID riid,
	LPVOID *ppvObj)
{
	ICOM_THIS(IGenericSFImpl, iface);

	char	xriid[50];	
	WINE_StringFromCLSID((LPCLSID)riid,xriid);

	_CALL_TRACE
	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,xriid,ppvObj);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))		*ppvObj = _IUnknown_(This); 
	else if(IsEqualIID(riid, &IID_IShellFolder))	*ppvObj = _IShellFolder_(This);
	else if(IsEqualIID(riid, &IID_IShellFolder2))	*ppvObj = _IShellFolder_(This);
	else if(IsEqualIID(riid, &IID_IPersist))	*ppvObj = _IPersist_(This);
	else if(IsEqualIID(riid, &IID_IPersistFolder))	*ppvObj = _IPersistFolder_(This);
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

	  if (pdesktopfolder == _IShellFolder_(This))
	  {
	    pdesktopfolder=NULL;
	    TRACE("-- destroyed IShellFolder(%p) was Desktopfolder\n",This);
	  }
	  if(This->absPidl) SHFree(This->absPidl);
	  if(This->sMyPath) SHFree(This->sMyPath);
	  HeapFree(GetProcessHeap(),0,This);
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
*	IShellFolder_Constructor
*
* NOTES
*  creating undocumented ShellFS_Folder as part of an aggregation
*  {F3364BA0-65B9-11CE-A9BA-00AA004AE837}
*
* FIXME
*	when pUnkOuter = 0 then rrid = IID_IShellFolder is returned
*/
HRESULT IFSFolder_Constructor(
	IUnknown * pUnkOuter,
	REFIID riid,
	LPVOID * ppv)
{
	IGenericSFImpl *	sf;
	char	xriid[50];	
	HRESULT hr = S_OK;
	WINE_StringFromCLSID((LPCLSID)riid,xriid);

	TRACE("unkOut=%p riid=%s\n",pUnkOuter, xriid);

	if(pUnkOuter && ! IsEqualIID(riid, &IID_IUnknown))
	{
	  hr = CLASS_E_NOAGGREGATION;	/* forbidden by definition */
	}
	else
	{
	  sf=(IGenericSFImpl*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IGenericSFImpl));
	  if (sf)
	  {
	    sf->ref=1;
	    ICOM_VTBL(sf)=&unkvt;
	    sf->lpvtblShellFolder=&sfvt;
	    sf->lpvtblPersistFolder=&psfvt;
	    sf->lpvtblDropTarget=&dtvt;
	    sf->pclsid = (CLSID*)&CLSID_SFFile;
	    sf->pUnkOuter = pUnkOuter ? pUnkOuter : _IUnknown_(sf);
	    *ppv = _IUnknown_(sf);
	    hr = S_OK;
	    shell32_ObjCount++;
	  }
	  else
	  {
	    hr = E_OUTOFMEMORY;
	  }
	}
	return hr;
}
/**************************************************************************
*	  IShellFolder_Constructor
*
* NOTES
*	THIS points to the parent folder
*/

IShellFolder * IShellFolder_Constructor(
	IShellFolder2 * iface,
	LPITEMIDLIST pidl)
{
	IGenericSFImpl *	sf;
	DWORD			dwSize=0;

	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	sf=(IGenericSFImpl*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IGenericSFImpl));
	sf->ref=1;

	ICOM_VTBL(sf)=&unkvt;
	sf->lpvtblShellFolder=&sfvt;
	sf->lpvtblPersistFolder=&psfvt;
	sf->lpvtblDropTarget=&dtvt;
	sf->pclsid = (CLSID*)&CLSID_SFFile;
	sf->pUnkOuter = _IUnknown_(sf);

	TRACE("(%p)->(parent=%p, pidl=%p)\n",sf,This, pidl);
	pdump(pidl);
		
	if(pidl && iface)				/* do we have a pidl? */
	{
	  int len;

	  sf->absPidl = ILCombine(This->absPidl, pidl);	/* build a absolute pidl */

	  if (!_ILIsSpecialFolder(pidl))				/* only file system paths */
	  {
	    if(This->sMyPath)				/* get the size of the parents path */
	    {
	      dwSize += strlen(This->sMyPath) ;
	      TRACE("-- (%p)->(parent's path=%s)\n",sf, debugstr_a(This->sMyPath));
	    }   

	    dwSize += _ILSimpleGetText(pidl,NULL,0);		/* add the size of our name*/
	    sf->sMyPath = SHAlloc(dwSize + 2);			/* '\0' and backslash */

	    if(!sf->sMyPath) return NULL;
	    *(sf->sMyPath)=0x00;

	    if(This->sMyPath)				/* if the parent has a path, get it*/
	    {
	      strcpy(sf->sMyPath, This->sMyPath);
	      PathAddBackslashA (sf->sMyPath);
	    }

	    len = strlen(sf->sMyPath);
	    _ILSimpleGetText(pidl, sf->sMyPath + len, dwSize - len + 1);
	  }

	  TRACE("-- (%p)->(my pidl=%p, my path=%s)\n",sf, sf->absPidl,debugstr_a(sf->sMyPath));

	  pdump (sf->absPidl);
	}

	shell32_ObjCount++;
	return _IShellFolder_(sf);
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

	char	xriid[50];	
	WINE_StringFromCLSID((LPCLSID)riid,xriid);
	_CALL_TRACE
	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,xriid,ppvObj);

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
*  every folder trys to parse only it's own (the leftmost) pidl and creates a 
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
	  WideCharToLocal(szTempA, szElement, lstrlenW(szElement) + 1);
	  strcpy(szPath, This->sMyPath);
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
	       hr = S_OK;
	    }
	  }
	}

	*ppidl = pidlTemp;

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
	*ppEnumIDList = IEnumIDList_Constructor (This->sMyPath, dwFlags, EIDL_FILE);

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
	char		xriid[50];
	IShellFolder	*pShellFolder, *pSubFolder;
	IPersistFolder 	*pPersistFolder;
	LPITEMIDLIST	absPidl;
	
	WINE_StringFromCLSID(riid,xriid);

	TRACE("(%p)->(pidl=%p,%p,\n\tIID:\t%s,%p)\n",This,pidl,pbcReserved,xriid,ppvOut);

	if(!pidl || !ppvOut) return E_INVALIDARG;

	*ppvOut = NULL;

	if ((iid=_ILGetGUIDPointer(pidl)))
	{
	  /* we have to create a alien folder */
	  if (  SUCCEEDED(SHCoCreateInstance(NULL, iid, NULL, riid, (LPVOID*)&pShellFolder))
	     && SUCCEEDED(IShellFolder_QueryInterface(pShellFolder, &IID_IPersistFolder, (LPVOID*)&pPersistFolder)))
	    {
	      absPidl = ILCombine (This->absPidl, pidl);
	      IPersistFolder_Initialize(pPersistFolder, absPidl);
	      IPersistFolder_Release(pPersistFolder);
	      SHFree(absPidl);
	    }
	    else
	    {
	      return E_FAIL;
	    }
	}
	else
	{
	  LPITEMIDLIST pidltemp = ILCloneFirst(pidl);
	  pShellFolder = IShellFolder_Constructor(iface, pidltemp);
	  ILFree(pidltemp);
	}
	
	if (_ILIsPidlSimple(pidl))
	{
	  *ppvOut = pShellFolder;
	}
	else
	{
	  IShellFolder_BindToObject(pShellFolder, ILGetNext(pidl), NULL, &IID_IShellFolder, (LPVOID)&pSubFolder);
	  IShellFolder_Release(pShellFolder);
	  *ppvOut = pSubFolder;
	}

	TRACE("-- (%p) returning (%p)\n",This, *ppvOut);

	return S_OK;
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

	char xriid[50];
	WINE_StringFromCLSID(riid,xriid);

	FIXME("(%p)->(pidl=%p,%p,\n\tIID:%s,%p) stub\n",This,pidl,pbcReserved,xriid,ppvOut);

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

	TRACE("(%p)->(0x%08lx,pidl1=%p,pidl2=%p)\n",This,lParam,pidl1,pidl2);
	pdump (pidl1);
	pdump (pidl2);
	
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
	      else
	      {
	        hr = ResultFromShort(nReturn);		/* two equal simple pidls */
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
	char		xriid[50];
	HRESULT		hr = E_INVALIDARG;

	WINE_StringFromCLSID(riid,xriid);
	TRACE("(%p)->(hwnd=0x%x,\n\tIID:\t%s,%p)\n",This,hwndOwner,xriid,ppvOut);
	
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
	  if (_ILIsFolder( *apidl))
	  {
	    *rgfInOut &= 0xe0000177;
	    goto next;
	  }
	  else if (_ILIsValue( *apidl))
	  {
	    *rgfInOut &= 0x40000177;
	    goto next;
	  }
	  hr = E_INVALIDARG;

next:	  apidl++;
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

	char		xclsid[50];
	LPITEMIDLIST	pidl;
	IUnknown*	pObj = NULL; 
	HRESULT		hr = E_INVALIDARG;
	
	WINE_StringFromCLSID(riid,xclsid);

	TRACE("(%p)->(%u,%u,apidl=%p,\n\tIID:%s,%p,%p)\n",
	  This,hwndOwner,cidl,apidl,xclsid,prgfInOut,ppvOut);

	if (ppvOut)
	{
	  *ppvOut = NULL;

	  if(IsEqualIID(riid, &IID_IContextMenu) && (cidl >= 1))
	  {
	    pObj  = (LPUNKNOWN)IContextMenu_Constructor((IShellFolder*)iface, This->absPidl, apidl, cidl);
	    hr = S_OK;
	  }
	  else if (IsEqualIID(riid, &IID_IDataObject) &&(cidl >= 1))
	  {
	    pObj = (LPUNKNOWN)IDataObject_Constructor (hwndOwner, This->absPidl, apidl, cidl);
	    hr = S_OK;
	  }
	  else if (IsEqualIID(riid, &IID_IExtractIconA) && (cidl == 1))
	  {
	    pidl = ILCombine(This->absPidl,apidl[0]);
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
	  if (!(dwFlags & SHGDN_INFOLDER) && (dwFlags & SHGDN_FORPARSING) && This->sMyPath)
	  {
	    strcpy (szPath, This->sMyPath);			/* get path to root*/
	    PathAddBackslashA(szPath);
	    len = strlen(szPath);
	  }
	  _ILSimpleGetText(pidl, szPath + len, MAX_PATH - len);	/* append my own path */
	}
	
	if ( (dwFlags & SHGDN_FORPARSING) && !bSimplePidl)	/* go deeper if needed */
	{
	  PathAddBackslashA(szPath);
	  len = strlen(szPath);

	  if (!SUCCEEDED(SHELL32_GetDisplayNameOfChild(iface, pidl, dwFlags, szPath + len, MAX_PATH - len)))
	    return E_OUTOFMEMORY;
	}
	strRet->uType = STRRET_CSTRA;
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
	DWORD dw, 
	LPITEMIDLIST *pPidlOut)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	FIXME("(%p)->(%u,pidl=%p,%s,%lu,%p),stub!\n",
	This,hwndOwner,pidl,debugstr_w(lpName),dw,pPidlOut);

	return E_NOTIMPL;
}

/**************************************************************************
*  IShellFolder_fnGetFolderPath
*/
static HRESULT WINAPI IShellFolder_fnGetFolderPath(IShellFolder2 * iface, LPSTR lpszOut, DWORD dwOutSize)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)
	
	TRACE("(%p)->(%p %lu)\n",This, lpszOut, dwOutSize);

	if (!lpszOut) return FALSE;

	*lpszOut=0;

	if (! This->sMyPath) return FALSE;

	lstrcpynA(lpszOut, This->sMyPath, dwOutSize);

	TRACE("-- (%p)->(return=%s)\n",This, lpszOut);
	return TRUE;
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
	  psd->str.uType = STRRET_CSTRA;
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
	  psd->str.uType = STRRET_CSTRA;
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
IShellFolder * ISF_Desktop_Constructor()
{
	IGenericSFImpl *	sf;

	sf=(IGenericSFImpl*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IGenericSFImpl));
	sf->ref=1;
	ICOM_VTBL(sf)=&unkvt;
	sf->lpvtblShellFolder=&sfdvt;
	sf->absPidl=_ILCreateDesktop();	/* my qualified pidl */
	sf->pUnkOuter = (IUnknown *) &sf->lpVtbl;

	TRACE("(%p)\n",sf);

	shell32_ObjCount++;
	return _IShellFolder_(sf);
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

	char	xriid[50];	
	WINE_StringFromCLSID((LPCLSID)riid,xriid);
	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,xriid,ppvObj);

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

	LPCWSTR		szNext=NULL;
	LPITEMIDLIST	pidlTemp=NULL;
	HRESULT		hr=E_OUTOFMEMORY;
	
	TRACE("(%p)->(HWND=0x%08x,%p,%p=%s,%p,pidl=%p,%p)\n",
	This,hwndOwner,pbcReserved,lpszDisplayName,
	debugstr_w(lpszDisplayName),pchEaten,ppidl,pdwAttributes);

	*ppidl = 0;
	if (pchEaten) *pchEaten = 0;	/* strange but like the original */
	
	/* fixme no real parsing implemented */
	pidlTemp = _ILCreateMyComputer();
	szNext = lpszDisplayName;

	if (szNext && *szNext)
	{
	  hr = SHELL32_ParseNextElement(hwndOwner, iface, &pidlTemp, (LPOLESTR)szNext, pchEaten, pdwAttributes);
	}
	else
	{
	  hr = S_OK;
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
	char		xriid[50];
	IShellFolder	*pShellFolder, *pSubFolder;
	
	WINE_StringFromCLSID(riid,xriid);

	TRACE("(%p)->(pidl=%p,%p,\n\tIID:\t%s,%p)\n",This,pidl,pbcReserved,xriid,ppvOut);

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
	     if (!SUCCEEDED(SHELL32_CoCreateInitSF (This->absPidl, pidl, clsid, riid, (LPVOID*)&pShellFolder)))
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
	  SHGetSpecialFolderLocation(0, CSIDL_DESKTOPDIRECTORY, &deskpidl);
	  firstpidl = ILCloneFirst(pidl);
	  completepidl = ILCombine(deskpidl, firstpidl);

	  pShellFolder = IShellFolder_Constructor(NULL, NULL);
	  if (SUCCEEDED(IShellFolder_QueryInterface(pShellFolder, &IID_IPersistFolder, (LPVOID*)&ppf)))
	  {
	    IPersistFolder_Initialize(ppf, completepidl);
	    IPersistFolder_Release(ppf);
	  }
	  ILFree(completepidl);
	  ILFree(deskpidl);
	  ILFree(firstpidl);
	}
	
	if (_ILIsPidlSimple(pidl))	/* no sub folders */
	{
	  *ppvOut = pShellFolder;
	}
	else				/* go deeper */
	{
	  IShellFolder_BindToObject(pShellFolder, ILGetNext(pidl), NULL, riid, (LPVOID)&pSubFolder);
	  IShellFolder_Release(pShellFolder);
	  *ppvOut = pSubFolder;
	}

	TRACE("-- (%p) returning (%p)\n",This, *ppvOut);

	return S_OK;
}

/**************************************************************************
*	ISF_Desktop_fnCreateViewObject
*/
static HRESULT WINAPI ISF_Desktop_fnCreateViewObject( IShellFolder2 * iface,
		 HWND hwndOwner, REFIID riid, LPVOID *ppvOut)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	LPSHELLVIEW	pShellView;
	char		xriid[50];
	HRESULT		hr = E_INVALIDARG;

	WINE_StringFromCLSID(riid,xriid);
	TRACE("(%p)->(hwnd=0x%x,\n\tIID:\t%s,%p)\n",This,hwndOwner,xriid,ppvOut);
	
	if(ppvOut)
	{
	  *ppvOut = NULL;

	  if(IsEqualIID(riid, &IID_IDropTarget))
	  {
	    FIXME("IDropTarget not implemented\n");
	    hr = E_NOTIMPL;
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
*  ISF_Desktop_fnGetAttributesOf
*/
static HRESULT WINAPI ISF_Desktop_fnGetAttributesOf(IShellFolder2 * iface,UINT cidl,LPCITEMIDLIST *apidl,DWORD *rgfInOut)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	GUID		const * clsid;
	DWORD		attributes;
	HRESULT		hr = S_OK;

	TRACE("(%p)->(cidl=%d apidl=%p mask=0x%08lx)\n",This,cidl,apidl, *rgfInOut);

	if ( (!cidl) || (!apidl) || (!rgfInOut))
	  return E_INVALIDARG;

	while (cidl > 0 && *apidl)
	{
	  pdump (*apidl);

	  if ((clsid=_ILGetGUIDPointer(*apidl)))
	  {
	    if (IsEqualIID(clsid, &CLSID_MyComputer))
	    {
	      *rgfInOut &= 0xb0000154;
	      goto next;
	    }
	    else if (HCR_GetFolderAttributes(clsid, &attributes))
	    {
	      *rgfInOut &= attributes;
	      goto next;
	    }
	    else
	    { /* some shell-extension */
	      *rgfInOut &= 0xb0000154;
	    }
	  }
	  else if (_ILIsFolder( *apidl))
	  {
	    *rgfInOut &= 0xe0000177;
	    goto next;
	  }
	  else if (_ILIsValue( *apidl))
	  {
	    *rgfInOut &= 0x40000177;
	    goto next;
	  }
	  hr = E_INVALIDARG;

next:	  apidl++;
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
	strRet->uType = STRRET_CSTRA;
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
	HRESULT hr = E_FAIL;;

	TRACE("(%p)->(%p %i %p)\n",This, pidl, iColumn, psd);

	if (!psd || iColumn >= DESKTOPSHELLVIEWCOLUMNS ) return E_INVALIDARG;
	
	if (!pidl)
	{
	  psd->fmt = DesktopSFHeader[iColumn].fmt;
	  psd->cxChar = DesktopSFHeader[iColumn].cxChar;
	  psd->str.uType = STRRET_CSTRA;
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
	  psd->str.uType = STRRET_CSTRA;
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
	sf->lpvtblPersistFolder = &psfvt;
	sf->pclsid = (CLSID*)&CLSID_SFMyComp;
	sf->absPidl=_ILCreateMyComputer();	/* my qualified pidl */
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
	
	if (PathIsRootW(lpszDisplayName))
	{
	  szNext = GetNextElementW(lpszDisplayName, szElement, MAX_PATH);
	  WideCharToLocal(szTempA, szElement, lstrlenW(szElement) + 1);
	  pidlTemp = _ILCreateDrive(szTempA);

	  if (szNext && *szNext)
	  {
	    hr = SHELL32_ParseNextElement(hwndOwner, iface, &pidlTemp, (LPOLESTR)szNext, pchEaten, pdwAttributes);
	  }
	  else
	  {
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
	char		xriid[50];
	IShellFolder	*pShellFolder, *pSubFolder;
	LPITEMIDLIST	pidltemp;
	
	WINE_StringFromCLSID(riid,xriid);

	TRACE("(%p)->(pidl=%p,%p,\n\tIID:\t%s,%p)\n",This,pidl,pbcReserved,xriid,ppvOut);

	if(!pidl || !ppvOut) return E_INVALIDARG;

	*ppvOut = NULL;

	if ((clsid=_ILGetGUIDPointer(pidl)) && !IsEqualIID(clsid, &CLSID_MyComputer))
	{
	   if (!SUCCEEDED(SHELL32_CoCreateInitSF (This->absPidl, pidl, clsid, riid, (LPVOID*)&pShellFolder)))
	   {
	     return E_FAIL;
	   }
	}
	else
	{
	  if (!_ILIsDrive(pidl)) return E_INVALIDARG;

	  pidltemp = ILCloneFirst(pidl);
	  pShellFolder = IShellFolder_Constructor(iface, pidltemp);
	  ILFree(pidltemp);
	}

	if (_ILIsPidlSimple(pidl))	/* no sub folders */
	{
	  *ppvOut = pShellFolder;
	}
	else				/* go deeper */
	{
	  IShellFolder_BindToObject(pShellFolder, ILGetNext(pidl), NULL, &IID_IShellFolder, (LPVOID)&pSubFolder);
	  IShellFolder_Release(pShellFolder);
	  *ppvOut = pSubFolder;
	}

	TRACE("-- (%p) returning (%p)\n",This, *ppvOut);

	return S_OK;
}

/**************************************************************************
*	ISF_MyComputer_fnCreateViewObject
*/
static HRESULT WINAPI ISF_MyComputer_fnCreateViewObject( IShellFolder2 * iface,
		 HWND hwndOwner, REFIID riid, LPVOID *ppvOut)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	LPSHELLVIEW	pShellView;
	char		xriid[50];
	HRESULT		hr = E_INVALIDARG;

	WINE_StringFromCLSID(riid,xriid);
	TRACE("(%p)->(hwnd=0x%x,\n\tIID:\t%s,%p)\n",This,hwndOwner,xriid,ppvOut);
	
	if(ppvOut)
	{
	  *ppvOut = NULL;

	  if(IsEqualIID(riid, &IID_IDropTarget))
	  {
	    FIXME("IDropTarget not implemented\n");
	    hr = E_NOTIMPL;
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
*  ISF_MyComputer_fnGetAttributesOf
*/
static HRESULT WINAPI ISF_MyComputer_fnGetAttributesOf(IShellFolder2 * iface,UINT cidl,LPCITEMIDLIST *apidl,DWORD *rgfInOut)
{
	_ICOM_THIS_From_IShellFolder2(IGenericSFImpl, iface)

	GUID		const * clsid;
	DWORD		attributes;
	HRESULT		hr = S_OK;

	TRACE("(%p)->(cidl=%d apidl=%p mask=0x%08lx)\n",This,cidl,apidl,*rgfInOut);

	if ( (!cidl) || (!apidl) || (!rgfInOut))
	  return E_INVALIDARG;

	*rgfInOut = 0xffffffff;

	while (cidl > 0 && *apidl)
	{
	  pdump (*apidl);

	  if (_ILIsDrive(*apidl))
	  {
	    *rgfInOut &= 0xf0000144;
	    goto next;
	  }
	  else if ((clsid=_ILGetGUIDPointer(*apidl)))
	  {
	    if (HCR_GetFolderAttributes(clsid, &attributes))
	    {
	      *rgfInOut &= attributes;
	      goto next;
	    }
	  }
	  hr = E_INVALIDARG;

next:	  apidl++;
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

	    GetVolumeInformationA(szPath,szDrive,12,&dwVolumeSerialNumber,&dwMaximumComponetLength,&dwFileSystemFlags,NULL,0);
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
	strRet->uType = STRRET_CSTRA;
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

/* fixme: drive size >4GB is rolling over */
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
	  psd->str.uType = STRRET_CSTRA;
	  LoadStringA(shell32_hInstance, MyComputerSFHeader[iColumn].colnameid, psd->str.u.cStr, MAX_PATH);
	  return S_OK;
	}
	else
	{
	  char szPath[MAX_PATH];
	  ULARGE_INTEGER ulBytes;

	  psd->str.u.cStr[0] = 0x00;
	  psd->str.uType = STRRET_CSTRA;
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
static HRESULT WINAPI ISFPersistFolder_QueryInterface(
	IPersistFolder *	iface,
	REFIID			iid,
	LPVOID*			ppvObj)
{
	_ICOM_THIS_From_IPersistFolder(IGenericSFImpl, iface);

	TRACE("(%p)\n", This);

	return IUnknown_QueryInterface(This->pUnkOuter, iid, ppvObj);
}

/************************************************************************
 * ISFPersistFolder_AddRef (IUnknown)
 *
 */
static ULONG WINAPI ISFPersistFolder_AddRef(
	IPersistFolder *	iface)
{
	_ICOM_THIS_From_IPersistFolder(IGenericSFImpl, iface);

	TRACE("(%p)\n", This);

	return IUnknown_AddRef(This->pUnkOuter);
}

/************************************************************************
 * ISFPersistFolder_Release (IUnknown)
 *
 */
static ULONG WINAPI ISFPersistFolder_Release(
	IPersistFolder *	iface)
{
	_ICOM_THIS_From_IPersistFolder(IGenericSFImpl, iface);

	TRACE("(%p)\n", This);

	return IUnknown_Release(This->pUnkOuter);
}

/************************************************************************
 * ISFPersistFolder_GetClassID (IPersist)
 */
static HRESULT WINAPI ISFPersistFolder_GetClassID(
	IPersistFolder *	iface,
	CLSID *			lpClassId)
{
	_ICOM_THIS_From_IPersistFolder(IGenericSFImpl, iface);

	TRACE("(%p)\n", This);

	if (!lpClassId) return E_POINTER;
	*lpClassId = *This->pclsid;

	return S_OK;
}

/************************************************************************
 * ISFPersistFolder_Initialize (IPersistFolder)
 *
 * NOTES
 *  sMyPath is not set. Don't know how to handle in a non rooted environment.
 */
static HRESULT WINAPI ISFPersistFolder_Initialize(
	IPersistFolder *	iface,
	LPCITEMIDLIST		pidl)
{
	char sTemp[MAX_PATH];
	_ICOM_THIS_From_IPersistFolder(IGenericSFImpl, iface);

	TRACE("(%p)->(%p)\n", This, pidl);

	/* free the old stuff */
	if(This->absPidl)
	{
	  SHFree(This->absPidl);
	  This->absPidl = NULL;
	}
	if(This->sMyPath)
	{
	  SHFree(This->sMyPath);
	  This->sMyPath = NULL;
	}

	/* set my pidl */
	This->absPidl = ILClone(pidl);

	/* set my path */
	if (SHGetPathFromIDListA(pidl, sTemp))
	{
	  This->sMyPath = SHAlloc(strlen(sTemp+1));
	  strcpy(This->sMyPath, sTemp);
	}

	TRACE("--(%p)->(%s)\n", This, This->sMyPath);

	return S_OK;
}

static ICOM_VTABLE(IPersistFolder) psfvt = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	ISFPersistFolder_QueryInterface,
	ISFPersistFolder_AddRef,
	ISFPersistFolder_Release,
	ISFPersistFolder_GetClassID,
	ISFPersistFolder_Initialize
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

