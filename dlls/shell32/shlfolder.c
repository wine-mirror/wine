/*
 *	Shell Folder stuff
 *
 *	Copyright 1997	Marcus Meissner
 *	Copyright 1998	Juergen Schmied
 *
 */

#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "winerror.h"

#include "oleidl.h"
#include "shlguid.h"

#include "pidl.h"
#include "wine/obj_base.h"
#include "wine/obj_dragdrop.h"
#include "wine/obj_shellfolder.h"
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

	TRACE(shell,"(%p)->(\n\tIID:\t%s,%p)\n",This,xriid,ppvObj);

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
	  TRACE(shell,"-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}

	TRACE(shell,"-- Interface: E_NOINTERFACE\n");

	return E_NOINTERFACE;
}

static ULONG WINAPI ISFDropTarget_AddRef( IDropTarget *iface)
{
	ICOM_THIS(ISFDropTarget,iface);

	TRACE(shell,"(%p)->(count=%lu)\n",This,This->ref);

	shell32_ObjCount++;

	return ++(This->ref);
}

static ULONG WINAPI ISFDropTarget_Release( IDropTarget *iface)
{
	ICOM_THIS(ISFDropTarget,iface);

	shell32_ObjCount--;

	if (!--(This->ref)) 
	{ TRACE(shell,"-- destroying ISFDropTarget (%p)\n",This);
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

	FIXME(shell, "Stub: This=%p, DataObject=%p\n",This,pDataObject);

	return E_NOTIMPL;
}

static HRESULT WINAPI ISFDropTarget_DragOver(
	IDropTarget	*iface,
	DWORD		grfKeyState,
	POINTL		pt,
	DWORD		*pdwEffect)
{
	ICOM_THIS(ISFDropTarget,iface);

	FIXME(shell, "Stub: This=%p\n",This);

	return E_NOTIMPL;
}

static HRESULT WINAPI ISFDropTarget_DragLeave(
	IDropTarget	*iface)
{
	ICOM_THIS(ISFDropTarget,iface);

	FIXME(shell, "Stub: This=%p\n",This);

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

	FIXME(shell, "Stub: This=%p\n",This);

	return E_NOTIMPL;
}

static struct ICOM_VTABLE(IDropTarget) dtvt = 
{
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
LPSTR GetNextElement(LPSTR pszNext,LPSTR pszOut,DWORD dwOut)
{	LPSTR   pszTail = pszNext;
	DWORD dwCopy;
	TRACE(shell,"(%s %p 0x%08lx)\n",debugstr_a(pszNext),pszOut,dwOut);

	if(!pszNext || !*pszNext)
	  return NULL;

	while(*pszTail && (*pszTail != '\\'))
	{ pszTail++;
	}
	dwCopy=((LPBYTE)pszTail-(LPBYTE)pszNext)/sizeof(CHAR)+1;
	lstrcpynA(pszOut, pszNext, (dwOut<dwCopy)? dwOut : dwCopy);

	if(*pszTail)
	{  pszTail++;
	}

	TRACE(shell,"--(%s %s 0x%08lx)\n",debugstr_a(pszNext),debugstr_a(pszOut),dwOut);
	return pszTail;
}

/***********************************************************************
*   IShellFolder implementation
*/

static struct ICOM_VTABLE(IShellFolder) sfvt;
static struct ICOM_VTABLE(IPersistFolder) psfvt;

#define _IPersistFolder_Offset ((int)(&(((IGenericSFImpl*)0)->lpvtblPersistFolder))) 
#define _ICOM_THIS_From_IPersistFolder(class, name) class* This = (class*)(((void*)name)-_IPersistFolder_Offset); 

/**************************************************************************
*	  IShellFolder_Constructor
*/

IShellFolder * IShellFolder_Constructor(
	IGenericSFImpl * pParent,
	LPITEMIDLIST pidl) 
{
	IGenericSFImpl *	sf;
	DWORD			dwSize=0;

	sf=(IGenericSFImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IGenericSFImpl));
	sf->ref=1;
	sf->lpvtbl=&sfvt;
	sf->lpvtblPersistFolder=&psfvt;
	sf->sMyPath=NULL;	/* path of the folder */
	sf->pMyPidl=NULL;	/* my qualified pidl */

	TRACE(shell,"(%p)->(parent=%p, pidl=%p)\n",sf,pParent, pidl);
	pdump(pidl);
		
	/* keep a copy of the pidl in the instance*/
	sf->mpidl = ILClone(pidl);		/* my short pidl */
	
	if(sf->mpidl)				/* do we have a pidl? */
	{ dwSize = 0;
	  if(pParent->sMyPath)			/* get the size of the parents path */
	  { dwSize += strlen(pParent->sMyPath) ;
	    TRACE(shell,"-- (%p)->(parent's path=%s)\n",sf, debugstr_a(pParent->sMyPath));
	  }   
	  dwSize += _ILGetFolderText(sf->mpidl,NULL,0); /* add the size of the foldername*/
	  sf->sMyPath = SHAlloc(dwSize+2);		/* '\0' and backslash */
	  if(sf->sMyPath)
	  { int len;
	    *(sf->sMyPath)=0x00;
	    if(pParent->sMyPath)			/* if the parent has a path, get it*/
	    {  strcpy(sf->sMyPath, pParent->sMyPath);
	       PathAddBackslashA (sf->sMyPath);
	    }
	    sf->pMyPidl = ILCombine(pParent->pMyPidl, pidl);
	    len = strlen(sf->sMyPath);
	    _ILGetFolderText(sf->mpidl, sf->sMyPath+len, dwSize-len);
	    TRACE(shell,"-- (%p)->(my pidl=%p, my path=%s)\n",sf, sf->pMyPidl,debugstr_a(sf->sMyPath));
	    pdump (sf->pMyPidl);
	  }
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
	TRACE(shell,"(%p)->(\n\tIID:\t%s,%p)\n",This,xriid,ppvObj);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
	{ *ppvObj = This; 
	}
	else if(IsEqualIID(riid, &IID_IShellFolder))  /*IShellFolder*/
	{    *ppvObj = (IShellFolder*)This;
	}   
	else if(IsEqualIID(riid, &IID_IPersistFolder))  /*IPersistFolder*/
	{    *ppvObj = (IPersistFolder*)&(This->lpvtblPersistFolder);
	}   

	if(*ppvObj)
	{ IShellFolder_AddRef((IShellFolder*)*ppvObj);
	  TRACE(shell,"-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE(shell,"-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}

/**************************************************************************
*  IShellFolder::AddRef
*/

static ULONG WINAPI IShellFolder_fnAddRef(IShellFolder * iface)
{
	ICOM_THIS(IGenericSFImpl, iface);

	TRACE(shell,"(%p)->(count=%lu)\n",This,This->ref);

	shell32_ObjCount++;
	return ++(This->ref);
}

/**************************************************************************
 *  IShellFolder_fnRelease
 */
static ULONG WINAPI IShellFolder_fnRelease(IShellFolder * iface) 
{
	ICOM_THIS(IGenericSFImpl, iface);

	TRACE(shell,"(%p)->(count=%lu)\n",This,This->ref);

	shell32_ObjCount--;
	if (!--(This->ref)) 
	{ TRACE(shell,"-- destroying IShellFolder(%p)\n",This);

	  if (pdesktopfolder == iface)
	  { pdesktopfolder=NULL;
	    TRACE(shell,"-- destroyed IShellFolder(%p) was Desktopfolder\n",This);
	  }
	  if(This->pMyPidl)
	  { SHFree(This->pMyPidl);
	  }
	  if(This->mpidl)
	  { SHFree(This->mpidl);
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
* FIXME: 
*    pdwAttributes: not used
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

	HRESULT		hr=E_OUTOFMEMORY;
	LPITEMIDLIST	pidlFull=NULL, pidlTemp = NULL, pidlOld = NULL;
	LPSTR		pszNext=NULL;
	CHAR		szTemp[MAX_PATH],szElement[MAX_PATH];
	BOOL		bIsFile;

	TRACE(shell,"(%p)->(HWND=0x%08x,%p,%p=%s,%p,pidl=%p,%p)\n",
	This,hwndOwner,pbcReserved,lpszDisplayName,
	debugstr_w(lpszDisplayName),pchEaten,ppidl,pdwAttributes);

	{ hr = E_FAIL;
	  WideCharToLocal(szTemp, lpszDisplayName, lstrlenW(lpszDisplayName) + 1);
	  if(szTemp[0])
	  { if (strcmp(szTemp,"Desktop")==0)
	    { pidlFull = _ILCreateDesktop();
	    }
	    else if (strcmp(szTemp,"My Computer")==0)
	    { pidlFull = _ILCreateMyComputer();
	    }
	    else
	    { if (!PathIsRootA(szTemp))
	      { if (This->sMyPath && strlen (This->sMyPath))
	        { if (strcmp(This->sMyPath,"My Computer"))
	          { strcpy (szElement,This->sMyPath);
	            PathAddBackslashA (szElement);
	            strcat (szElement, szTemp);
	            strcpy (szTemp, szElement);
	          }
	        }
	      }
	      
	      /* check if the lpszDisplayName is Folder or File*/
	      bIsFile = ! (GetFileAttributesA(szTemp) & FILE_ATTRIBUTE_DIRECTORY);
	      pszNext = GetNextElement(szTemp, szElement, MAX_PATH);

	      pidlFull = _ILCreateMyComputer();
	      pidlTemp = _ILCreateDrive(szElement);			
	      pidlOld = pidlFull;
	      pidlFull = ILCombine(pidlFull,pidlTemp);
	      SHFree(pidlOld);

	      if(pidlFull)
	      { while((pszNext=GetNextElement(pszNext, szElement, MAX_PATH)))
	        { if(!*pszNext && bIsFile)
	          { pidlTemp = _ILCreateValue(NULL, szElement);		/* FIXME: shortname */
	          }
	          else				
	          { pidlTemp = _ILCreateFolder(NULL, szElement);	/* FIXME: shortname */
	          }
	          pidlOld = pidlFull;
	          pidlFull = ILCombine(pidlFull,pidlTemp);
	          SHFree(pidlOld);
	        }
	        hr = S_OK;
	      }
	    }
	  }
	}
	*ppidl = pidlFull;
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

	TRACE(shell,"(%p)->(HWND=0x%08x flags=0x%08lx pplist=%p)\n",This,hwndOwner,dwFlags,ppEnumIDList);

	*ppEnumIDList = NULL;
	*ppEnumIDList = IEnumIDList_Constructor (This->sMyPath, dwFlags);
	TRACE(shell,"-- (%p)->(new ID List: %p)\n",This,*ppEnumIDList);
	if(!*ppEnumIDList)
	{ return E_OUTOFMEMORY;
	}
	return S_OK;		
}

/**************************************************************************
*		IShellFolder_fnBindToObject
* PARAMETERS
*  LPCITEMIDLIST pidl,       //[in ] complex pidl to open
*  LPBC          pbc,        //[in ] reserved
*  REFIID        riid,       //[in ] Initial Interface
*  LPVOID*       ppvObject   //[out] Interface*
*/
static HRESULT WINAPI IShellFolder_fnBindToObject( IShellFolder * iface, LPCITEMIDLIST pidl,
			LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
	ICOM_THIS(IGenericSFImpl, iface);

	char		xriid[50];
	HRESULT		hr;
	LPSHELLFOLDER	pShellFolder;
	
	WINE_StringFromCLSID(riid,xriid);

	TRACE(shell,"(%p)->(pidl=%p,%p,\n\tIID:%s,%p)\n",This,pidl,pbcReserved,xriid,ppvOut);

	*ppvOut = NULL;

	pShellFolder = IShellFolder_Constructor(This, pidl);

	if(!pShellFolder)
	  return E_OUTOFMEMORY;

	hr = pShellFolder->lpvtbl->fnQueryInterface(pShellFolder, riid, ppvOut);
	pShellFolder->lpvtbl->fnRelease(pShellFolder);
	TRACE(shell,"-- (%p)->(interface=%p)\n",This, ppvOut);
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
	IShellFolder * iface,
	LPCITEMIDLIST pidl,
	LPBC pbcReserved,
	REFIID riid,
	LPVOID *ppvOut)
{
	ICOM_THIS(IGenericSFImpl, iface);

	char xriid[50];
	WINE_StringFromCLSID(riid,xriid);

	FIXME(shell,"(%p)->(pidl=%p,%p,\n\tIID:%s,%p) stub\n",This,pidl,pbcReserved,xriid,ppvOut);

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
* FIXME
*  we have to handle simple pidl's only (?)
*/
static HRESULT WINAPI  IShellFolder_fnCompareIDs(
	IShellFolder * iface,
	LPARAM lParam,
	LPCITEMIDLIST pidl1,
	LPCITEMIDLIST pidl2)
{
	ICOM_THIS(IGenericSFImpl, iface);

	CHAR szString1[MAX_PATH] = "";
	CHAR szString2[MAX_PATH] = "";
	int   nReturn;
	LPCITEMIDLIST  pidlTemp1 = pidl1, pidlTemp2 = pidl2;

	TRACE(shell,"(%p)->(0x%08lx,pidl1=%p,pidl2=%p)\n",This,lParam,pidl1,pidl2);
	pdump (pidl1);
	pdump (pidl2);

	if (!pidl1 && !pidl2)
	  return 0;
	if (!pidl1)	/* Desktop < anything */
	  return -1;
	if (!pidl2)
	  return 1;

	/* get the last item in each list */
	pidlTemp1 = ILFindLastID(pidlTemp1);
	pidlTemp2 = ILFindLastID(pidlTemp2);

	/* at This point, both pidlTemp1 and pidlTemp2 point to the last item in the list */
	if(_ILIsValue(pidlTemp1) != _ILIsValue(pidlTemp2))
	{ if(_ILIsValue(pidlTemp1))
	    return 1;
	  return -1;
	}

	_ILGetDrive( pidl1,szString1,sizeof(szString1));
	_ILGetDrive( pidl2,szString2,sizeof(szString2));
	nReturn = strcasecmp(szString1, szString2);

	if(nReturn)
	  return nReturn;

	_ILGetFolderText( pidl1,szString1,sizeof(szString1));
	_ILGetFolderText( pidl2,szString2,sizeof(szString2));
	nReturn = strcasecmp(szString1, szString2);

	if(nReturn)
	  return nReturn;

	_ILGetValueText(pidl1,szString1,sizeof(szString1));
	_ILGetValueText(pidl2,szString2,sizeof(szString2));
	return strcasecmp(szString1, szString2);
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
	TRACE(shell,"(%p)->(hwnd=0x%x,\n\tIID:\t%s,%p)\n",This,hwndOwner,xriid,ppvOut);
	
	*ppvOut = NULL;

	pShellView = IShellView_Constructor((IShellFolder *) This, This->mpidl);

	if(!pShellView)
	  return E_OUTOFMEMORY;
	  
	hr = pShellView->lpvtbl->fnQueryInterface(pShellView, riid, ppvOut);
	pShellView->lpvtbl->fnRelease(pShellView);
	TRACE(shell,"-- (%p)->(interface=%p)\n",This, ppvOut);
	return hr; 
}

/**************************************************************************
*  IShellFolder_fnGetAttributesOf
*
* PARAMETERS
*  UINT            cidl,     //[in ] num elements in pidl array
+  LPCITEMIDLIST*  apidl,    //[in ] simple pidl array 
*  ULONG*          rgfInOut) //[out] result array  
*
* FIXME: quick hack
*  Note: rgfInOut is documented as being an array of ULONGS.
*  This does not seem to be the case. Testing This function using the shell to 
*  call it with cidl > 1 (by deleting multiple items) reveals that the shell
*  passes ONE element in the array and writing to further elements will
*  cause the shell to fail later.
*/
static HRESULT WINAPI IShellFolder_fnGetAttributesOf(IShellFolder * iface,UINT cidl,LPCITEMIDLIST *apidl,DWORD *rgfInOut)
{
	ICOM_THIS(IGenericSFImpl, iface);

	LPCITEMIDLIST * pidltemp;
	DWORD i;

	TRACE(shell,"(%p)->(%d,%p,%p)\n",This,cidl,apidl,rgfInOut);

	if ((! cidl )| (!apidl) | (!rgfInOut))
	  return E_INVALIDARG;

	pidltemp=apidl;
	*rgfInOut = 0x00;
	i=cidl;

	TRACE(shell,"-- mask=0x%08lx\n",*rgfInOut);

	do
	{ if (*pidltemp)
	  { pdump (*pidltemp);
	    if (_ILIsDesktop( *pidltemp))
	    { *rgfInOut |= ( SFGAO_HASSUBFOLDER | SFGAO_FOLDER | SFGAO_DROPTARGET | SFGAO_HASPROPSHEET | SFGAO_CANLINK );
	    }
	    else if (_ILIsMyComputer( *pidltemp))
	    { *rgfInOut |= ( SFGAO_HASSUBFOLDER | SFGAO_FOLDER | SFGAO_FILESYSANCESTOR |
	                     SFGAO_DROPTARGET | SFGAO_HASPROPSHEET | SFGAO_CANRENAME | SFGAO_CANLINK );
	    }
	    else if (_ILIsDrive( *pidltemp))
	    { *rgfInOut |= ( SFGAO_HASSUBFOLDER | SFGAO_FILESYSTEM  | SFGAO_FOLDER | SFGAO_FILESYSANCESTOR  | 
	                     SFGAO_DROPTARGET | SFGAO_HASPROPSHEET | SFGAO_CANLINK );
	    }
	    else if (_ILIsFolder( *pidltemp))
	    { *rgfInOut |= ( SFGAO_HASSUBFOLDER | SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_CAPABILITYMASK );
	    }
	    else if (_ILIsValue( *pidltemp))
	    { *rgfInOut |= (SFGAO_FILESYSTEM | SFGAO_CAPABILITYMASK );
	    }
	  }
	  pidltemp++;
	  cidl--;
	} while (cidl > 0 && *pidltemp);

	return S_OK;
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

	TRACE(shell,"(%p)->(%u,%u,apidl=%p,\n\tIID:%s,%p,%p)\n",
	  This,hwndOwner,cidl,apidl,xclsid,prgfInOut,ppvOut);

	*ppvOut = NULL;

	if(IsEqualIID(riid, &IID_IContextMenu))
	{ 
	  if(cidl < 1)
	    return E_INVALIDARG;

	  pObj  = (LPUNKNOWN)IContextMenu_Constructor((IShellFolder *)This, apidl, cidl);
	}
	else if (IsEqualIID(riid, &IID_IDataObject))
	{ 
	  if (cidl < 1)
	    return(E_INVALIDARG);

	  pObj = (LPUNKNOWN)IDataObject_Constructor (hwndOwner, (IShellFolder *)This, apidl, cidl);
	}
	else if(IsEqualIID(riid, &IID_IExtractIconA))
	{ 
	  if (cidl != 1)
	    return(E_INVALIDARG);

	  pidl = ILCombine(This->pMyPidl,apidl[0]);
	  pObj = (LPUNKNOWN)IExtractIconA_Constructor( pidl );
	  SHFree(pidl);
	} 
	else if (IsEqualIID(riid, &IID_IDropTarget))
	{ 
	  if (cidl < 1)
	    return(E_INVALIDARG);

	  pObj = (LPUNKNOWN)ISFDropTarget_Constructor();
	}
	else
	{ 
	  ERR(shell,"(%p)->E_NOINTERFACE\n",This);
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
	LPSTRRET lpName)
{
	ICOM_THIS(IGenericSFImpl, iface);

	CHAR	szText[MAX_PATH];
	CHAR	szTemp[MAX_PATH];
	CHAR	szSpecial[MAX_PATH];
	CHAR	szDrive[MAX_PATH];
	DWORD	dwVolumeSerialNumber,dwMaximumComponetLength,dwFileSystemFlags;
	LPITEMIDLIST	pidlTemp=NULL;
	BOOL	bSimplePidl=FALSE;
		
	TRACE(shell,"(%p)->(pidl=%p,0x%08lx,%p)\n",This,pidl,dwFlags,lpName);
	pdump(pidl);
	
	szSpecial[0]=0x00; 
	szDrive[0]=0x00;
	szText[0]=0x00;
	szTemp[0]=0x00;
	
	/* test if simple(relative) or complex(absolute) pidl */
	pidlTemp = ILGetNext(pidl);
	if (pidlTemp && pidlTemp->mkid.cb==0x00)
	{ bSimplePidl = TRUE;
	  TRACE(shell,"-- simple pidl\n");
	}

	if (_ILIsDesktop( pidl))
	{ strcpy (szText,"Desktop");
	}	
	else
	{ if (_ILIsMyComputer(pidl))
	  { _ILGetItemText(pidl, szSpecial, MAX_PATH);
	    pidl = ILGetNext(pidl);
	  }

	  if (_ILIsDrive(pidl))
	  { _ILGetDrive( pidl, szTemp, MAX_PATH);

	    if ( dwFlags==SHGDN_NORMAL || dwFlags==SHGDN_INFOLDER)	/* like "A1-dos (C:)" */
	    { GetVolumeInformationA(szTemp,szDrive,MAX_PATH,&dwVolumeSerialNumber,&dwMaximumComponetLength,&dwFileSystemFlags,NULL,0);
	      szTemp[2]=0x00;						/* overwrite '\' */
	      strcat (szDrive," (");
	      strcat (szDrive,szTemp);
	      strcat (szDrive,")"); 
	    }
	    else							/* like "C:\" */
	    {  PathAddBackslashA (szTemp);
	       strcpy(szDrive,szTemp);
	    }
	  }

		
	  switch(dwFlags)
	  { case SHGDN_NORMAL:				/* 0x0000 */
	      _ILGetPidlPath( pidl, szText, MAX_PATH);
	      break;

	    case SHGDN_INFOLDER | SHGDN_FORPARSING:	/* 0x8001 */
	    case SHGDN_INFOLDER:			/* 0x0001 */
	      pidlTemp = ILFindLastID(pidl);
	      if (pidlTemp)
	      { _ILGetItemText( pidlTemp, szText, MAX_PATH);
	      }
	      break;				

	    case SHGDN_FORPARSING:			/* 0x8000 */
	      if (bSimplePidl)
	      { /* if the IShellFolder has parents, get the path from the
	        parent and add the ItemName*/
	        szText[0]=0x00;
	        if (This->sMyPath && strlen (This->sMyPath))
	        { if (strcmp(This->sMyPath,"My Computer"))
	          { strcpy (szText,This->sMyPath);
	            PathAddBackslashA (szText);
	          }
	        }
	        pidlTemp = ILFindLastID(pidl);
	        if (pidlTemp)
	        { _ILGetItemText( pidlTemp, szTemp, MAX_PATH );
	        } 
	        strcat(szText,szTemp);
	      }
	      else	/* if the pidl is absolute, get everything from the pidl*/					
	      {	_ILGetPidlPath( pidl, szText, MAX_PATH);
	      }
	      break;
	    default:
	      TRACE(shell,"--- wrong flags=%lx\n", dwFlags);
	      return E_INVALIDARG;
	  }
	  if ((szText[0]==0x00 && szDrive[0]!=0x00)|| (bSimplePidl && szDrive[0]!=0x00))
	  { strcpy(szText,szDrive);
	  }
	  if (szText[0]==0x00 && szSpecial[0]!=0x00)
	  { strcpy(szText,szSpecial);
	  }
	}

	TRACE(shell,"-- (%p)->(%s)\n",This,szText);

	if(!(lpName))
	{  return E_OUTOFMEMORY;
	}
	lpName->uType = STRRET_CSTRA;	
	strcpy(lpName->u.cStr,szText);
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

	FIXME(shell,"(%p)->(%u,pidl=%p,%s,%lu,%p),stub!\n",
	This,hwndOwner,pidl,debugstr_w(lpName),dw,pPidlOut);

	return E_NOTIMPL;
}

/**************************************************************************
*  IShellFolder_fnGetFolderPath
*  FIXME: drive not included
*/
static HRESULT WINAPI IShellFolder_fnGetFolderPath(IShellFolder * iface, LPSTR lpszOut, DWORD dwOutSize)
{
	ICOM_THIS(IGenericSFImpl, iface);
	DWORD	dwSize;
	
	TRACE(shell,"(%p)->(%p %lu)\n",This, lpszOut, dwOutSize);
	if (!lpszOut)
	{ return FALSE;
	}

	*lpszOut=0;

	if (! This->sMyPath)
	  return FALSE;
	  
	dwSize = strlen (This->sMyPath) +1;
	if ( dwSize > dwOutSize)
	  return FALSE;
	strcpy(lpszOut, This->sMyPath);

	TRACE(shell,"-- (%p)->(return=%s)\n",This, lpszOut);
	return TRUE;
}

static ICOM_VTABLE(IShellFolder) sfvt = 
{	IShellFolder_fnQueryInterface,
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

/************************************************************************
 * ISFPersistFolder_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
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
 * See Windows documentation for more details on IUnknown methods.
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
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI ISFPersistFolder_Release(
	IPersistFolder *	iface)
{
	_ICOM_THIS_From_IPersistFolder(IGenericSFImpl, iface);

	return IShellFolder_Release((IShellFolder*)This);
}

/************************************************************************
 * ISFPersistFolder_GetClassID (IPersist)
 *
 * See Windows documentation for more details on IPersist methods.
 */
static HRESULT WINAPI ISFPersistFolder_GetClassID(
	const IPersistFolder *	iface,
	LPCLSID               lpClassId)
{
	/* This ID is not documented anywhere but some tests in Windows tell 
	 * me that This is the ID for the "standard" implementation of the 
	 * IFolder interface. 
	 */

	CLSID StdFolderID = { 0xF3364BA0, 0x65B9, 0x11CE, {0xA9, 0xBA, 0x00, 0xAA, 0x00, 0x4A, 0xE8, 0x37} };

	if (lpClassId==NULL)
	  return E_POINTER;

	memcpy(lpClassId, &StdFolderID, sizeof(StdFolderID));

	return S_OK;
}

/************************************************************************
 * ISFPersistFolder_Initialize (IPersistFolder)
 *
 * See Windows documentation for more details on IPersistFolder methods.
 */
static HRESULT WINAPI ISFPersistFolder_Initialize(
	IPersistFolder *	iface,
	LPCITEMIDLIST		pidl)
{
	_ICOM_THIS_From_IPersistFolder(IGenericSFImpl, iface);

	if(This->pMyPidl)
	{ SHFree(This->pMyPidl);
	  This->pMyPidl = NULL;
	}
	This->pMyPidl = ILClone(pidl);
	return S_OK;
}

static ICOM_VTABLE(IPersistFolder) psfvt = 
{
	ISFPersistFolder_QueryInterface,
	ISFPersistFolder_AddRef,
	ISFPersistFolder_Release,
	ISFPersistFolder_GetClassID,
	ISFPersistFolder_Initialize
};
