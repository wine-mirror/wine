/*
 *	Shell Folder stuff
 *
 *	Copyright 1997	Marcus Meissner
 *	Copyright 1998	Juergen Schmied
 *	
 *	IShellFolder with IDropTarget, IPersistFolder
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

DEFAULT_DEBUG_CHANNEL(shell)

/***************************************************************************
 * IDropTarget interface definition for the ShellFolder
 */

typedef struct
{	ICOM_VTABLE(IDropTarget)* lpvtbl;
	ULONG ref;
} ISFDropTarget;

static struct ICOM_VTABLE(IDropTarget) dtvt;


/****************************************************************************
 * ISFDropTarget implementation
 */

static IDropTarget * WINAPI ISFDropTarget_Constructor(void)
{
	ISFDropTarget* sf;

	sf = HeapAlloc(GetProcessHeap(), 0, sizeof(ISFDropTarget));

	if (sf)
	{ sf->lpvtbl = &dtvt;
	  sf->ref    = 1;
	}

	return (IDropTarget *)sf;
}

static HRESULT WINAPI ISFDropTarget_QueryInterface(
	IDropTarget *iface,
	REFIID riid,
	LPVOID *ppvObj)
{
	ICOM_THIS(ISFDropTarget,iface);

	char	xriid[50];
	WINE_StringFromCLSID((LPCLSID)riid,xriid);

	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,xriid,ppvObj);

	if ( !This || !ppvObj)
	  return E_INVALIDARG;

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
	{ *ppvObj = This; 
	}
	else if(IsEqualIID(riid, &IID_IDropTarget))  /*IShellFolder*/
	{    *ppvObj = (ISFDropTarget*)This;
	}   

	if(*ppvObj)
	{ IDropTarget_AddRef((IDropTarget*)*ppvObj);
	  TRACE("-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}

	TRACE("-- Interface: E_NOINTERFACE\n");

	return E_NOINTERFACE;
}

static ULONG WINAPI ISFDropTarget_AddRef( IDropTarget *iface)
{
	ICOM_THIS(ISFDropTarget,iface);

	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	shell32_ObjCount++;

	return ++(This->ref);
}

static ULONG WINAPI ISFDropTarget_Release( IDropTarget *iface)
{
	ICOM_THIS(ISFDropTarget,iface);

	shell32_ObjCount--;

	if (!--(This->ref)) 
	{ TRACE("-- destroying ISFDropTarget (%p)\n",This);
	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}
	return This->ref;
}

static HRESULT WINAPI ISFDropTarget_DragEnter(
	IDropTarget 	*iface,
	IDataObject	*pDataObject,
	DWORD		grfKeyState,
	POINTL		pt,
	DWORD		*pdwEffect)
{	

	ICOM_THIS(ISFDropTarget,iface);

	FIXME("Stub: This=%p, DataObject=%p\n",This,pDataObject);

	return E_NOTIMPL;
}

static HRESULT WINAPI ISFDropTarget_DragOver(
	IDropTarget	*iface,
	DWORD		grfKeyState,
	POINTL		pt,
	DWORD		*pdwEffect)
{
	ICOM_THIS(ISFDropTarget,iface);

	FIXME("Stub: This=%p\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI ISFDropTarget_DragLeave(
	IDropTarget	*iface)
{
	ICOM_THIS(ISFDropTarget,iface);

	FIXME("Stub: This=%p\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI ISFDropTarget_Drop(
	IDropTarget	*iface,
	IDataObject*	pDataObject,
	DWORD		grfKeyState,
	POINTL		pt,
	DWORD		*pdwEffect)
{
	ICOM_THIS(ISFDropTarget,iface);

	FIXME("Stub: This=%p\n",This);

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
	IShellFolder * psf,
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
	IShellFolder 	*pShellFolder;
	IPersistFolder	*pPersistFolder;

	TRACE("%p %p\n", pidlRoot, pidlChild);

	*ppvOut = NULL;
	
	hr = SHCoCreateInstance(NULL, clsid, NULL, iid, (LPVOID*)&pShellFolder);
	if (SUCCEEDED(hr))
	{
	  hr = IShellFolder_QueryInterface(pShellFolder, &IID_IPersistFolder, (LPVOID*)&pPersistFolder);
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
	IShellFolder * psf,
	LPCITEMIDLIST pidl,
	DWORD dwFlags,
	LPSTR szOut,
	DWORD dwOutLen)
{
	LPITEMIDLIST	pidlFirst, pidlNext;
	IShellFolder * 	psfChild;
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
	      if (strTemp.uType == STRRET_CSTRA)
	      {
		lstrcpynA(szOut , strTemp.u.cStr, dwOutLen);
	      }
	      else
	      {
	        FIXME("wrong return type");
	      }
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
	ICOM_VTABLE(IShellFolder)*	lpvtbl;
	DWORD				ref;

	ICOM_VTABLE(IPersistFolder)*	lpvtblPersistFolder;
	CLSID*				pclsid;

	LPSTR				sMyPath;
	LPITEMIDLIST			absPidl;	/* complete pidl */
} IGenericSFImpl;

static struct ICOM_VTABLE(IShellFolder) sfvt;

static struct ICOM_VTABLE(IPersistFolder) psfvt;

static IShellFolder * ISF_MyComputer_Constructor(void);

#define _IPersistFolder_Offset ((int)(&(((IGenericSFImpl*)0)->lpvtblPersistFolder))) 
#define _ICOM_THIS_From_IPersistFolder(class, name) class* This = (class*)(((char*)name)-_IPersistFolder_Offset); 

/**************************************************************************
*	  IShellFolder_Constructor
*
*/

static IShellFolder * IShellFolder_Constructor(
	IShellFolder * psf,
	LPITEMIDLIST pidl)
{
	IGenericSFImpl *	sf;
	IGenericSFImpl * 	sfParent = (IGenericSFImpl*) psf;
	DWORD			dwSize=0;

	sf=(IGenericSFImpl*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IGenericSFImpl));
	sf->ref=1;

	sf->lpvtbl=&sfvt;
	sf->lpvtblPersistFolder=&psfvt;
	sf->pclsid = (CLSID*)&CLSID_SFFile;
	
	TRACE("(%p)->(parent=%p, pidl=%p)\n",sf,sfParent, pidl);
	pdump(pidl);
		
	if(pidl)						/* do we have a pidl? */
	{
	  int len;

	  sf->absPidl = ILCombine(sfParent->absPidl, pidl);	/* build a absolute pidl */

	  if (!_ILIsSpecialFolder(pidl))				/* only file system paths */
	  {
	    if(sfParent->sMyPath)				/* get the size of the parents path */
	    {
	      dwSize += strlen(sfParent->sMyPath) ;
	      TRACE("-- (%p)->(parent's path=%s)\n",sf, debugstr_a(sfParent->sMyPath));
	    }   

	    dwSize += _ILSimpleGetText(pidl,NULL,0);		/* add the size of our name*/
	    sf->sMyPath = SHAlloc(dwSize + 2);			/* '\0' and backslash */

	    if(!sf->sMyPath) return NULL;
	    *(sf->sMyPath)=0x00;

	    if(sfParent->sMyPath)				/* if the parent has a path, get it*/
	    {
	      strcpy(sf->sMyPath, sfParent->sMyPath);
	      PathAddBackslashA (sf->sMyPath);
	    }

	    len = strlen(sf->sMyPath);
	    _ILSimpleGetText(pidl, sf->sMyPath + len, dwSize - len + 1);
	  }

	  TRACE("-- (%p)->(my pidl=%p, my path=%s)\n",sf, sf->absPidl,debugstr_a(sf->sMyPath));

	  pdump (sf->absPidl);
	}

	shell32_ObjCount++;
	return (IShellFolder *)sf;
}
/**************************************************************************
 *  IShellFolder_fnQueryInterface
 *
 * PARAMETERS
 *  REFIID riid		[in ] Requested InterfaceID
 *  LPVOID* ppvObject	[out] Interface* to hold the result
 */
static HRESULT WINAPI IShellFolder_fnQueryInterface(
	IShellFolder * iface,
	REFIID riid,
	LPVOID *ppvObj)
{
	ICOM_THIS(IGenericSFImpl, iface);

	char	xriid[50];	
	WINE_StringFromCLSID((LPCLSID)riid,xriid);
	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,xriid,ppvObj);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))
	{ *ppvObj = This; 
	}
	else if(IsEqualIID(riid, &IID_IShellFolder))
	{    *ppvObj = (IShellFolder*)This;
	}   
	else if(IsEqualIID(riid, &IID_IPersist))
	{    *ppvObj = (IPersistFolder*)&(This->lpvtblPersistFolder);
	}   
	else if(IsEqualIID(riid, &IID_IPersistFolder))
	{    *ppvObj = (IPersistFolder*)&(This->lpvtblPersistFolder);
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
*  IShellFolder_AddRef
*/

static ULONG WINAPI IShellFolder_fnAddRef(IShellFolder * iface)
{
	ICOM_THIS(IGenericSFImpl, iface);

	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	shell32_ObjCount++;
	return ++(This->ref);
}

/**************************************************************************
 *  IShellFolder_fnRelease
 */
static ULONG WINAPI IShellFolder_fnRelease(IShellFolder * iface) 
{
	ICOM_THIS(IGenericSFImpl, iface);

	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	shell32_ObjCount--;
	if (!--(This->ref)) 
	{ TRACE("-- destroying IShellFolder(%p)\n",This);

	  if (pdesktopfolder == iface)
	  { pdesktopfolder=NULL;
	    TRACE("-- destroyed IShellFolder(%p) was Desktopfolder\n",This);
	  }
	  if(This->absPidl)
	  { SHFree(This->absPidl);
	  }
	  if(This->sMyPath)
	  { SHFree(This->sMyPath);
	  }

	  HeapFree(GetProcessHeap(),0,This);

	  return 0;
	}
	return This->ref;
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
	IShellFolder * iface,
	HWND hwndOwner,
	LPBC pbcReserved,
	LPOLESTR lpszDisplayName,
	DWORD *pchEaten,
	LPITEMIDLIST *ppidl,
	DWORD *pdwAttributes)
{
	ICOM_THIS(IGenericSFImpl, iface);

	HRESULT		hr = E_OUTOFMEMORY;
	LPCWSTR		szNext=NULL;
	WCHAR		szElement[MAX_PATH];
	CHAR		szTempA[MAX_PATH], szPath[MAX_PATH];
	LPITEMIDLIST	pidlTemp=NULL;
	
	TRACE("(%p)->(HWND=0x%08x,%p,%p=%s,%p,pidl=%p,%p)\n",
	This,hwndOwner,pbcReserved,lpszDisplayName,
	debugstr_w(lpszDisplayName),pchEaten,ppidl,pdwAttributes);

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
	      hr = SHELL32_ParseNextElement(hwndOwner, (IShellFolder*)This, &pidlTemp, (LPOLESTR)szNext, pchEaten, pdwAttributes);
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
	IShellFolder * iface,
	HWND hwndOwner,
	DWORD dwFlags,
	LPENUMIDLIST* ppEnumIDList)
{
	ICOM_THIS(IGenericSFImpl, iface);

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
static HRESULT WINAPI IShellFolder_fnBindToObject( IShellFolder * iface, LPCITEMIDLIST pidl,
			LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
	ICOM_THIS(IGenericSFImpl, iface);
	GUID		const * iid;
	char		xriid[50];
	IShellFolder	*pShellFolder, *pSubFolder;
	IPersistFolder 	*pPersistFolder;
	LPITEMIDLIST	absPidl;
	
	WINE_StringFromCLSID(riid,xriid);

	TRACE("(%p)->(pidl=%p,%p,\n\tIID:\t%s,%p)\n",This,pidl,pbcReserved,xriid,ppvOut);

	*ppvOut = NULL;

	if ((iid=_ILGetGUIDPointer(pidl)) && !IsEqualIID(iid, &IID_MyComputer))
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
	  pShellFolder = IShellFolder_Constructor((IShellFolder*)This, pidltemp);
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
	IShellFolder * iface,
	LPCITEMIDLIST pidl,
	LPBC pbcReserved,
	REFIID riid,
	LPVOID *ppvOut)
{
	ICOM_THIS(IGenericSFImpl, iface);

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
	IShellFolder * iface,
	LPARAM lParam,
	LPCITEMIDLIST pidl1,
	LPCITEMIDLIST pidl2)
{
	ICOM_THIS(IGenericSFImpl, iface);

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
	
	        hr = IShellFolder_BindToObject((IShellFolder*)This, pidlTemp, NULL, &IID_IShellFolder, (LPVOID*)&psf);
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
*	  IShellFolder_fnCreateViewObject
* Creates an View Object representing the ShellFolder
*  IShellView / IShellBrowser / IContextMenu
*
* PARAMETERS
*  HWND    hwndOwner,  // Handle of owner window
*  REFIID  riid,       // Requested initial interface
*  LPVOID* ppvObject)  // Resultant interface*
*
* NOTES
*  the same as SHCreateShellFolderViewEx ???
*/
static HRESULT WINAPI IShellFolder_fnCreateViewObject( IShellFolder * iface,
		 HWND hwndOwner, REFIID riid, LPVOID *ppvOut)
{
	ICOM_THIS(IGenericSFImpl, iface);

	LPSHELLVIEW pShellView;
	char    xriid[50];
	HRESULT       hr;

	WINE_StringFromCLSID(riid,xriid);
	TRACE("(%p)->(hwnd=0x%x,\n\tIID:\t%s,%p)\n",This,hwndOwner,xriid,ppvOut);
	
	*ppvOut = NULL;

	pShellView = IShellView_Constructor((IShellFolder *) This);

	if(!pShellView)
	  return E_OUTOFMEMORY;
	  
	hr = pShellView->lpvtbl->fnQueryInterface(pShellView, riid, ppvOut);
	pShellView->lpvtbl->fnRelease(pShellView);
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
static HRESULT WINAPI IShellFolder_fnGetAttributesOf(IShellFolder * iface,UINT cidl,LPCITEMIDLIST *apidl,DWORD *rgfInOut)
{
	ICOM_THIS(IGenericSFImpl, iface);

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
	IShellFolder *	iface,
	HWND		hwndOwner,
	UINT		cidl,
	LPCITEMIDLIST *	apidl, 
	REFIID		riid, 
	UINT *		prgfInOut,
	LPVOID *	ppvOut)
{	
	ICOM_THIS(IGenericSFImpl, iface);

	char		xclsid[50];
	LPITEMIDLIST	pidl;
	LPUNKNOWN	pObj = NULL; 

	WINE_StringFromCLSID(riid,xclsid);

	TRACE("(%p)->(%u,%u,apidl=%p,\n\tIID:%s,%p,%p)\n",
	  This,hwndOwner,cidl,apidl,xclsid,prgfInOut,ppvOut);

	if (!ppvOut)
	  return E_INVALIDARG;

	*ppvOut = NULL;

	if(IsEqualIID(riid, &IID_IContextMenu))
	{ 
	  if(cidl < 1)
	    return E_INVALIDARG;

	  pObj  = (LPUNKNOWN)IContextMenu_Constructor((IShellFolder *)This, This->absPidl, apidl, cidl);
	}
	else if (IsEqualIID(riid, &IID_IDataObject))
	{ 
	  if (cidl < 1)
	    return E_INVALIDARG;

	  pObj = (LPUNKNOWN)IDataObject_Constructor (hwndOwner, This->absPidl, apidl, cidl);
	}
	else if(IsEqualIID(riid, &IID_IExtractIconA))
	{ 
	  if (cidl != 1)
	    return E_INVALIDARG;

	  pidl = ILCombine(This->absPidl,apidl[0]);
	  pObj = (LPUNKNOWN)IExtractIconA_Constructor( pidl );
	  SHFree(pidl);
	} 
	else if (IsEqualIID(riid, &IID_IDropTarget))
	{ 
	  if (cidl < 1)
	    return E_INVALIDARG;

	  pObj = (LPUNKNOWN)ISFDropTarget_Constructor();
	}
	else
	{ 
	  ERR("(%p)->E_NOINTERFACE\n",This);
	  return E_NOINTERFACE;
	}

	if(!pObj)
	  return E_OUTOFMEMORY;

	*ppvOut = pObj;
	return S_OK;
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
	IShellFolder * iface,
	LPCITEMIDLIST pidl,
	DWORD dwFlags,
	LPSTRRET strRet)
{
	ICOM_THIS(IGenericSFImpl, iface);

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

	  if (!SUCCEEDED(SHELL32_GetDisplayNameOfChild((IShellFolder*)This, pidl, dwFlags, szPath + len, MAX_PATH - len)))
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
	IShellFolder * iface,
	HWND hwndOwner, 
	LPCITEMIDLIST pidl, /*simple pidl*/
	LPCOLESTR lpName, 
	DWORD dw, 
	LPITEMIDLIST *pPidlOut)
{
	ICOM_THIS(IGenericSFImpl, iface);

	FIXME("(%p)->(%u,pidl=%p,%s,%lu,%p),stub!\n",
	This,hwndOwner,pidl,debugstr_w(lpName),dw,pPidlOut);

	return E_NOTIMPL;
}

/**************************************************************************
*  IShellFolder_fnGetFolderPath
*/
static HRESULT WINAPI IShellFolder_fnGetFolderPath(IShellFolder * iface, LPSTR lpszOut, DWORD dwOutSize)
{
	ICOM_THIS(IGenericSFImpl, iface);
	
	TRACE("(%p)->(%p %lu)\n",This, lpszOut, dwOutSize);

	if (!lpszOut) return FALSE;

	*lpszOut=0;

	if (! This->sMyPath) return FALSE;

	lstrcpynA(lpszOut, This->sMyPath, dwOutSize);

	TRACE("-- (%p)->(return=%s)\n",This, lpszOut);
	return TRUE;
}

static ICOM_VTABLE(IShellFolder) sfvt = 
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
	IShellFolder_fnGetFolderPath
};

/***********************************************************************
* 	[Desktopfolder]	IShellFolder implementation
*/
static struct ICOM_VTABLE(IShellFolder) sfdvt;

/**************************************************************************
*	ISF_Desktop_Constructor
*
*/
IShellFolder * ISF_Desktop_Constructor()
{
	IGenericSFImpl *	sf;

	sf=(IGenericSFImpl*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IGenericSFImpl));
	sf->ref=1;
	sf->lpvtbl=&sfdvt;
	sf->absPidl=_ILCreateDesktop();	/* my qualified pidl */

	TRACE("(%p)\n",sf);

	shell32_ObjCount++;
	return (IShellFolder *)sf;
}

/**************************************************************************
 *	ISF_Desktop_fnQueryInterface
 *
 * NOTES supports not IPersist/IPersistFolder
 */
static HRESULT WINAPI ISF_Desktop_fnQueryInterface(
	IShellFolder * iface,
	REFIID riid,
	LPVOID *ppvObj)
{
	ICOM_THIS(IGenericSFImpl, iface);

	char	xriid[50];	
	WINE_StringFromCLSID((LPCLSID)riid,xriid);
	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,xriid,ppvObj);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
	{ *ppvObj = This; 
	}
	else if(IsEqualIID(riid, &IID_IShellFolder))  /*IShellFolder*/
	{    *ppvObj = (IShellFolder*)This;
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
	IShellFolder * iface,
	HWND hwndOwner,
	LPBC pbcReserved,
	LPOLESTR lpszDisplayName,
	DWORD *pchEaten,
	LPITEMIDLIST *ppidl,
	DWORD *pdwAttributes)
{
	ICOM_THIS(IGenericSFImpl, iface);

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
	  hr = SHELL32_ParseNextElement(hwndOwner, (IShellFolder*)This, &pidlTemp, (LPOLESTR)szNext, pchEaten, pdwAttributes);
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
	IShellFolder * iface,
	HWND hwndOwner,
	DWORD dwFlags,
	LPENUMIDLIST* ppEnumIDList)
{
	ICOM_THIS(IGenericSFImpl, iface);

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
static HRESULT WINAPI ISF_Desktop_fnBindToObject( IShellFolder * iface, LPCITEMIDLIST pidl,
			LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
	ICOM_THIS(IGenericSFImpl, iface);
	GUID		const * clsid;
	char		xriid[50];
	IShellFolder	*pShellFolder, *pSubFolder;
	
	WINE_StringFromCLSID(riid,xriid);

	TRACE("(%p)->(pidl=%p,%p,\n\tIID:\t%s,%p)\n",This,pidl,pbcReserved,xriid,ppvOut);

	*ppvOut = NULL;

	if (!(clsid=_ILGetGUIDPointer(pidl))) return E_INVALIDARG;

	if ( IsEqualIID(clsid, &IID_MyComputer))
	{
	  pShellFolder = ISF_MyComputer_Constructor();
	}
	else
	{
	   if (!SUCCEEDED(SHELL32_CoCreateInitSF (This->absPidl, pidl, clsid, riid, (LPVOID*)&pShellFolder)))
	   {
	     return E_INVALIDARG;
	   }
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
*  ISF_Desktop_fnGetAttributesOf
*/
static HRESULT WINAPI ISF_Desktop_fnGetAttributesOf(IShellFolder * iface,UINT cidl,LPCITEMIDLIST *apidl,DWORD *rgfInOut)
{
	ICOM_THIS(IGenericSFImpl, iface);

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
	    if (IsEqualIID(clsid, &IID_MyComputer))
	    {
	      *rgfInOut &= 0xb0000154;
	      goto next;
	    }
	    else if (HCR_GetFolderAttributes(clsid, &attributes))
	    {
	      *rgfInOut &= attributes;
	      goto next;
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
	IShellFolder * iface,
	LPCITEMIDLIST pidl,
	DWORD dwFlags,
	LPSTRRET strRet)
{
	ICOM_THIS(IGenericSFImpl, iface);

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
	  if (!SUCCEEDED(SHELL32_GetDisplayNameOfChild((IShellFolder*)This, pidl, dwFlags, szPath, MAX_PATH)))
	    return E_OUTOFMEMORY;
	}
	strRet->uType = STRRET_CSTRA;
	lstrcpynA(strRet->u.cStr, szPath, MAX_PATH);


	TRACE("-- (%p)->(%s)\n", This, szPath);
	return S_OK;
}

static ICOM_VTABLE(IShellFolder) sfdvt = 
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
	IShellFolder_fnCreateViewObject,
	ISF_Desktop_fnGetAttributesOf,
	IShellFolder_fnGetUIObjectOf,
	ISF_Desktop_fnGetDisplayNameOf,
	IShellFolder_fnSetNameOf,
	IShellFolder_fnGetFolderPath
};


/***********************************************************************
*   IShellFolder [MyComputer] implementation
*/

static struct ICOM_VTABLE(IShellFolder) sfmcvt;

/**************************************************************************
*	ISF_MyComputer_Constructor
*/
static IShellFolder * ISF_MyComputer_Constructor(void)
{
	IGenericSFImpl *	sf;

	sf=(IGenericSFImpl*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IGenericSFImpl));
	sf->ref=1;

	sf->lpvtbl = &sfmcvt;
	sf->lpvtblPersistFolder = &psfvt;
	sf->pclsid = (CLSID*)&CLSID_SFMyComp;
	sf->absPidl=_ILCreateMyComputer();	/* my qualified pidl */

	TRACE("(%p)\n",sf);

	shell32_ObjCount++;
	return (IShellFolder *)sf;
}

/**************************************************************************
*	ISF_MyComputer_fnParseDisplayName
*/
static HRESULT WINAPI ISF_MyComputer_fnParseDisplayName(
	IShellFolder * iface,
	HWND hwndOwner,
	LPBC pbcReserved,
	LPOLESTR lpszDisplayName,
	DWORD *pchEaten,
	LPITEMIDLIST *ppidl,
	DWORD *pdwAttributes)
{
	ICOM_THIS(IGenericSFImpl, iface);

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
	    hr = SHELL32_ParseNextElement(hwndOwner, (IShellFolder*)This, &pidlTemp, (LPOLESTR)szNext, pchEaten, pdwAttributes);
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
	IShellFolder * iface,
	HWND hwndOwner,
	DWORD dwFlags,
	LPENUMIDLIST* ppEnumIDList)
{
	ICOM_THIS(IGenericSFImpl, iface);

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
static HRESULT WINAPI ISF_MyComputer_fnBindToObject( IShellFolder * iface, LPCITEMIDLIST pidl,
			LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
	ICOM_THIS(IGenericSFImpl, iface);
	GUID		const * clsid;
	char		xriid[50];
	IShellFolder	*pShellFolder, *pSubFolder;
	LPITEMIDLIST	pidltemp;
	
	WINE_StringFromCLSID(riid,xriid);

	TRACE("(%p)->(pidl=%p,%p,\n\tIID:\t%s,%p)\n",This,pidl,pbcReserved,xriid,ppvOut);

	*ppvOut = NULL;

	if ((clsid=_ILGetGUIDPointer(pidl)) && !IsEqualIID(clsid, &IID_MyComputer))
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
	  pShellFolder = IShellFolder_Constructor((IShellFolder*)This, pidltemp);
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
*  ISF_MyComputer_fnGetAttributesOf
*/
static HRESULT WINAPI ISF_MyComputer_fnGetAttributesOf(IShellFolder * iface,UINT cidl,LPCITEMIDLIST *apidl,DWORD *rgfInOut)
{
	ICOM_THIS(IGenericSFImpl, iface);

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
	IShellFolder * iface,
	LPCITEMIDLIST pidl,
	DWORD dwFlags,
	LPSTRRET strRet)
{
	ICOM_THIS(IGenericSFImpl, iface);

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

	  if (!SUCCEEDED(SHELL32_GetDisplayNameOfChild((IShellFolder*)This, pidl, dwFlags | SHGDN_FORPARSING, szPath + len, MAX_PATH - len)))
	    return E_OUTOFMEMORY;
	}
	strRet->uType = STRRET_CSTRA;
	lstrcpynA(strRet->u.cStr, szPath, MAX_PATH);


	TRACE("-- (%p)->(%s)\n", This, szPath);
	return S_OK;
}

static ICOM_VTABLE(IShellFolder) sfmcvt = 
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
	IShellFolder_fnCreateViewObject,
	ISF_MyComputer_fnGetAttributesOf,
	IShellFolder_fnGetUIObjectOf,
	ISF_MyComputer_fnGetDisplayNameOf,
	IShellFolder_fnSetNameOf,
	IShellFolder_fnGetFolderPath
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

	return IShellFolder_QueryInterface((IShellFolder*)This, iid, ppvObj);
}

/************************************************************************
 * ISFPersistFolder_AddRef (IUnknown)
 *
 */
static ULONG WINAPI ISFPersistFolder_AddRef(
	IPersistFolder *	iface)
{
	_ICOM_THIS_From_IPersistFolder(IShellFolder, iface);

	return IShellFolder_AddRef((IShellFolder*)This);
}

/************************************************************************
 * ISFPersistFolder_Release (IUnknown)
 *
 */
static ULONG WINAPI ISFPersistFolder_Release(
	IPersistFolder *	iface)
{
	_ICOM_THIS_From_IPersistFolder(IGenericSFImpl, iface);

	return IShellFolder_Release((IShellFolder*)This);
}

/************************************************************************
 * ISFPersistFolder_GetClassID (IPersist)
 */
static HRESULT WINAPI ISFPersistFolder_GetClassID(
	IPersistFolder *	iface,
	CLSID *			lpClassId)
{
	_ICOM_THIS_From_IPersistFolder(IGenericSFImpl, iface);

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
	_ICOM_THIS_From_IPersistFolder(IGenericSFImpl, iface);

	if(This->absPidl)
	{
	  SHFree(This->absPidl);
	  This->absPidl = NULL;
	}
	This->absPidl = ILClone(pidl);
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

