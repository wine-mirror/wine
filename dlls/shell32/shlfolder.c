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

#include "objbase.h"
#include "wine/obj_base.h"
#include "wine/obj_dragdrop.h"
#include "shlguid.h"

#include "pidl.h"
#include "objbase.h"
#include "shlobj.h"
#include "shell32_main.h"

static HRESULT WINAPI IShellFolder_QueryInterface(LPSHELLFOLDER,REFIID,LPVOID*);
static ULONG WINAPI IShellFolder_AddRef(LPSHELLFOLDER);
static ULONG WINAPI IShellFolder_Release(LPSHELLFOLDER);
static HRESULT WINAPI IShellFolder_Initialize(LPSHELLFOLDER,LPCITEMIDLIST);
static HRESULT WINAPI IShellFolder_ParseDisplayName(LPSHELLFOLDER,HWND32,LPBC,LPOLESTR32,DWORD*,LPITEMIDLIST*,DWORD*);
static HRESULT WINAPI IShellFolder_EnumObjects(LPSHELLFOLDER,HWND32,DWORD,LPENUMIDLIST*);
static HRESULT WINAPI IShellFolder_BindToObject(LPSHELLFOLDER,LPCITEMIDLIST,LPBC,REFIID,LPVOID*);
static HRESULT WINAPI IShellFolder_BindToStorage(LPSHELLFOLDER,LPCITEMIDLIST,LPBC,REFIID,LPVOID*);
static HRESULT WINAPI IShellFolder_CompareIDs(LPSHELLFOLDER,LPARAM,LPCITEMIDLIST,LPCITEMIDLIST);
static HRESULT WINAPI IShellFolder_CreateViewObject(LPSHELLFOLDER,HWND32,REFIID,LPVOID*);
static HRESULT WINAPI IShellFolder_GetAttributesOf(LPSHELLFOLDER,UINT32,LPCITEMIDLIST*,DWORD*);
static HRESULT WINAPI IShellFolder_GetUIObjectOf(LPSHELLFOLDER,HWND32,UINT32,LPCITEMIDLIST*,REFIID,UINT32*,LPVOID*);
static HRESULT WINAPI IShellFolder_GetDisplayNameOf(LPSHELLFOLDER,LPCITEMIDLIST,DWORD,LPSTRRET);
static HRESULT WINAPI IShellFolder_SetNameOf(LPSHELLFOLDER,HWND32,LPCITEMIDLIST,LPCOLESTR32,DWORD,LPITEMIDLIST*);
static BOOL32 WINAPI IShellFolder_GetFolderPath(LPSHELLFOLDER,LPSTR,DWORD);

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
	{ IDropTarget_AddRef((ISFDropTarget*)*ppvObj);
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
	lstrcpyn32A(pszOut, pszNext, (dwOut<dwCopy)? dwOut : dwCopy);

	if(*pszTail)
	{  pszTail++;
	}

	TRACE(shell,"--(%s %s 0x%08lx)\n",debugstr_a(pszNext),debugstr_a(pszOut),dwOut);
	return pszTail;
}

/***********************************************************************
*   IShellFolder implementation
*/
static struct IShellFolder_VTable sfvt = 
{ IShellFolder_QueryInterface,
  IShellFolder_AddRef,
  IShellFolder_Release,
  IShellFolder_ParseDisplayName,
  IShellFolder_EnumObjects,
  IShellFolder_BindToObject,
  IShellFolder_BindToStorage,
  IShellFolder_CompareIDs,
  IShellFolder_CreateViewObject,
  IShellFolder_GetAttributesOf,
  IShellFolder_GetUIObjectOf,
  IShellFolder_GetDisplayNameOf,
  IShellFolder_SetNameOf,
  IShellFolder_GetFolderPath
};
/**************************************************************************
*	  IShellFolder_Constructor
*/

LPSHELLFOLDER IShellFolder_Constructor(LPSHELLFOLDER pParent,LPITEMIDLIST pidl) 
{	LPSHELLFOLDER    sf;
	DWORD dwSize=0;
	sf=(LPSHELLFOLDER)HeapAlloc(GetProcessHeap(),0,sizeof(IShellFolder));
	sf->ref=1;
	sf->lpvtbl=&sfvt;
	sf->sMyPath=NULL;	/* path of the folder */
	sf->pMyPidl=NULL;	/* my qualified pidl */

	TRACE(shell,"(%p)->(parent=%p, pidl=%p)\n",sf,pParent, pidl);
	pdump(pidl);
		
	/* keep a copy of the pidl in the instance*/
	sf->mpidl = ILClone(pidl);		/* my short pidl */
	
	if(sf->mpidl)        			/* do we have a pidl? */
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
	       PathAddBackslash32A (sf->sMyPath);
	    }
	    sf->pMyPidl = ILCombine(pParent->pMyPidl, pidl);
	    len = strlen(sf->sMyPath);
	    _ILGetFolderText(sf->mpidl, sf->sMyPath+len, dwSize-len);
	    TRACE(shell,"-- (%p)->(my pidl=%p, my path=%s)\n",sf, sf->pMyPidl,debugstr_a(sf->sMyPath));
	    pdump (sf->pMyPidl);
	  }
	}
	shell32_ObjCount++;
	return sf;
}
/**************************************************************************
*  IShellFolder::QueryInterface
* PARAMETERS
*  REFIID riid,        //[in ] Requested InterfaceID
*  LPVOID* ppvObject)  //[out] Interface* to hold the result
*/
static HRESULT WINAPI IShellFolder_QueryInterface(
  LPSHELLFOLDER this, REFIID riid, LPVOID *ppvObj)
{	char	xriid[50];
	WINE_StringFromCLSID((LPCLSID)riid,xriid);
	TRACE(shell,"(%p)->(\n\tIID:\t%s,%p)\n",this,xriid,ppvObj);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
	{ *ppvObj = this; 
	}
	else if(IsEqualIID(riid, &IID_IShellFolder))  /*IShellFolder*/
	{    *ppvObj = (IShellFolder*)this;
	}   

	if(*ppvObj)
	{ (*(LPSHELLFOLDER*)ppvObj)->lpvtbl->fnAddRef(this);  	
	  TRACE(shell,"-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE(shell,"-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}

/**************************************************************************
*  IShellFolder::AddRef
*/

static ULONG WINAPI IShellFolder_AddRef(LPSHELLFOLDER this)
{	TRACE(shell,"(%p)->(count=%lu)\n",this,this->ref);
	shell32_ObjCount++;
	return ++(this->ref);
}

/**************************************************************************
 *  IShellFolder_Release
 */
static ULONG WINAPI IShellFolder_Release(LPSHELLFOLDER this) 
{	TRACE(shell,"(%p)->(count=%lu)\n",this,this->ref);

	shell32_ObjCount--;
	if (!--(this->ref)) 
	{ TRACE(shell,"-- destroying IShellFolder(%p)\n",this);

	  if (pdesktopfolder==this)
	  { pdesktopfolder=NULL;
	    TRACE(shell,"-- destroyed IShellFolder(%p) was Desktopfolder\n",this);
	  }
	  if(this->pMyPidl)
	  { SHFree(this->pMyPidl);
	  }
	  if(this->mpidl)
	  { SHFree(this->mpidl);
	  }
	  if(this->sMyPath)
	  { SHFree(this->sMyPath);
	  }

	  HeapFree(GetProcessHeap(),0,this);

	  return 0;
	}
	return this->ref;
}
/**************************************************************************
*		IShellFolder_ParseDisplayName
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
static HRESULT WINAPI IShellFolder_ParseDisplayName(
	LPSHELLFOLDER this,
	HWND32 hwndOwner,
	LPBC pbcReserved,
	LPOLESTR32 lpszDisplayName,
	DWORD *pchEaten,
	LPITEMIDLIST *ppidl,
	DWORD *pdwAttributes)
{	HRESULT        hr=E_OUTOFMEMORY;
	LPITEMIDLIST   pidlFull=NULL, pidlTemp = NULL, pidlOld = NULL;
	LPSTR          pszNext=NULL;
	CHAR           szTemp[MAX_PATH],szElement[MAX_PATH];
	BOOL32         bIsFile;
       
	TRACE(shell,"(%p)->(HWND=0x%08x,%p,%p=%s,%p,pidl=%p,%p)\n",
		this,hwndOwner,pbcReserved,lpszDisplayName,
		debugstr_w(lpszDisplayName),pchEaten,ppidl,pdwAttributes);

	{ hr = E_FAIL;
	  WideCharToLocal32(szTemp, lpszDisplayName, lstrlen32W(lpszDisplayName) + 1);
	  if(szTemp[0])
	  { if (strcmp(szTemp,"Desktop")==0)
	    { pidlFull = _ILCreateDesktop();
	    }
	    else if (strcmp(szTemp,"My Computer")==0)
	    { pidlFull = _ILCreateMyComputer();
	    }
	    else
	    { if (!PathIsRoot32A(szTemp))
	      { if (this->sMyPath && strlen (this->sMyPath))
	        { if (strcmp(this->sMyPath,"My Computer"))
	          { strcpy (szElement,this->sMyPath);
	            PathAddBackslash32A (szElement);
		    strcat (szElement, szTemp);
		    strcpy (szTemp, szElement);
	          }
	        }
	      }
	      
	      /* check if the lpszDisplayName is Folder or File*/
	      bIsFile = ! (GetFileAttributes32A(szTemp) & FILE_ATTRIBUTE_DIRECTORY);
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
*		IShellFolder_EnumObjects
* PARAMETERS
*  HWND          hwndOwner,    //[in ] Parent Window
*  DWORD         grfFlags,     //[in ] SHCONTF enumeration mask
*  LPENUMIDLIST* ppenumIDList  //[out] IEnumIDList interface
*/
static HRESULT WINAPI IShellFolder_EnumObjects(
	LPSHELLFOLDER this,
	HWND32 hwndOwner,
	DWORD dwFlags,
	LPENUMIDLIST* ppEnumIDList)
{	TRACE(shell,"(%p)->(HWND=0x%08x flags=0x%08lx pplist=%p)\n",this,hwndOwner,dwFlags,ppEnumIDList);

	*ppEnumIDList = NULL;
	*ppEnumIDList = IEnumIDList_Constructor (this->sMyPath, dwFlags);
	TRACE(shell,"-- (%p)->(new ID List: %p)\n",this,*ppEnumIDList);
	if(!*ppEnumIDList)
	{ return E_OUTOFMEMORY;
	}
	return S_OK;		
}
/**************************************************************************
 *  IShellFolder_Initialize()
 *  IPersistFolder Method
 */
static HRESULT WINAPI IShellFolder_Initialize( LPSHELLFOLDER this,LPCITEMIDLIST pidl)
{	TRACE(shell,"(%p)->(pidl=%p)\n",this,pidl);

	if(this->pMyPidl)
	{ SHFree(this->pMyPidl);
	  this->pMyPidl = NULL;
	}
	this->pMyPidl = ILClone(pidl);
	return S_OK;
}

/**************************************************************************
*		IShellFolder_BindToObject
* PARAMETERS
*  LPCITEMIDLIST pidl,       //[in ] complex pidl to open
*  LPBC          pbc,        //[in ] reserved
*  REFIID        riid,       //[in ] Initial Interface
*  LPVOID*       ppvObject   //[out] Interface*
*/
static HRESULT WINAPI IShellFolder_BindToObject( LPSHELLFOLDER this, LPCITEMIDLIST pidl,
			LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{	char	        xriid[50];
	HRESULT       hr;
	LPSHELLFOLDER pShellFolder;
	
	WINE_StringFromCLSID(riid,xriid);

	TRACE(shell,"(%p)->(pidl=%p,%p,\n\tIID:%s,%p)\n",this,pidl,pbcReserved,xriid,ppvOut);

	*ppvOut = NULL;

	pShellFolder = IShellFolder_Constructor(this, pidl);

	if(!pShellFolder)
	  return E_OUTOFMEMORY;

	hr = pShellFolder->lpvtbl->fnQueryInterface(pShellFolder, riid, ppvOut);
 	pShellFolder->lpvtbl->fnRelease(pShellFolder);
	TRACE(shell,"-- (%p)->(interface=%p)\n",this, ppvOut);
	return hr;
}

/**************************************************************************
*  IShellFolder_BindToStorage
* PARAMETERS
*  LPCITEMIDLIST pidl,       //[in ] complex pidl to store
*  LPBC          pbc,        //[in ] reserved
*  REFIID        riid,       //[in ] Initial storage interface 
*  LPVOID*       ppvObject   //[out] Interface* returned
*/
static HRESULT WINAPI IShellFolder_BindToStorage(LPSHELLFOLDER this,
	    LPCITEMIDLIST pidl,LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{	char xriid[50];
	WINE_StringFromCLSID(riid,xriid);

	FIXME(shell,"(%p)->(pidl=%p,%p,\n\tIID:%s,%p) stub\n",this,pidl,pbcReserved,xriid,ppvOut);

	*ppvOut = NULL;
	return E_NOTIMPL;
}

/**************************************************************************
*  IShellFolder_CompareIDs
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
static HRESULT WINAPI  IShellFolder_CompareIDs(LPSHELLFOLDER this,
		 LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2) 
{ CHAR szString1[MAX_PATH] = "";
  CHAR szString2[MAX_PATH] = "";
  int   nReturn;
  LPCITEMIDLIST  pidlTemp1 = pidl1, pidlTemp2 = pidl2;

  TRACE(shell,"(%p)->(0x%08lx,pidl1=%p,pidl2=%p)\n",this,lParam,pidl1,pidl2);
  pdump (pidl1);
  pdump (pidl2);

  if (!pidl1 && !pidl2)
    return 0;
  if (!pidl1)	/* Desktop < anything */
    return -1;
  if (!pidl2)
    return 1;
  
  /* get the last item in each list */
  while((ILGetNext(pidlTemp1))->mkid.cb)
    pidlTemp1 = ILGetNext(pidlTemp1);
  while((ILGetNext(pidlTemp2))->mkid.cb)
    pidlTemp2 = ILGetNext(pidlTemp2);

  /* at this point, both pidlTemp1 and pidlTemp2 point to the last item in the list */
  if(_ILIsValue(pidlTemp1) != _ILIsValue(pidlTemp2))
  { if(_ILIsValue(pidlTemp1))
      return 1;
   return -1;
  }

  _ILGetDrive( pidl1,szString1,sizeof(szString1));
  _ILGetDrive( pidl2,szString1,sizeof(szString2));
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
*	  IShellFolder_CreateViewObject
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
static HRESULT WINAPI IShellFolder_CreateViewObject( LPSHELLFOLDER this,
		 HWND32 hwndOwner, REFIID riid, LPVOID *ppvOut)
{	LPSHELLVIEW pShellView;
	char    xriid[50];
	HRESULT       hr;

	WINE_StringFromCLSID(riid,xriid);
	TRACE(shell,"(%p)->(hwnd=0x%x,\n\tIID:\t%s,%p)\n",this,hwndOwner,xriid,ppvOut);
	
	*ppvOut = NULL;

	pShellView = IShellView_Constructor(this, this->mpidl);

	if(!pShellView)
	  return E_OUTOFMEMORY;
	  
	hr = pShellView->lpvtbl->fnQueryInterface(pShellView, riid, ppvOut);
	pShellView->lpvtbl->fnRelease(pShellView);
	TRACE(shell,"-- (%p)->(interface=%p)\n",this, ppvOut);
	return hr; 
}

/**************************************************************************
*  IShellFolder_GetAttributesOf
*
* PARAMETERS
*  UINT            cidl,     //[in ] num elements in pidl array
+  LPCITEMIDLIST*  apidl,    //[in ] simple pidl array 
*  ULONG*          rgfInOut) //[out] result array  
*
* FIXME: quick hack
*  Note: rgfInOut is documented as being an array of ULONGS.
*  This does not seem to be the case. Testing this function using the shell to 
*  call it with cidl > 1 (by deleting multiple items) reveals that the shell
*  passes ONE element in the array and writing to further elements will
*  cause the shell to fail later.
*/
static HRESULT WINAPI IShellFolder_GetAttributesOf(LPSHELLFOLDER this,UINT32 cidl,LPCITEMIDLIST *apidl,DWORD *rgfInOut)
{ LPCITEMIDLIST * pidltemp;
  DWORD i;

  TRACE(shell,"(%p)->(%d,%p,%p)\n",this,cidl,apidl,rgfInOut);

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
      { *rgfInOut |= ( SFGAO_HASSUBFOLDER | SFGAO_FOLDER | SFGAO_FILESYSANCESTOR
      			| SFGAO_DROPTARGET | SFGAO_HASPROPSHEET | SFGAO_CANRENAME | SFGAO_CANLINK );
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
*  IShellFolder_GetUIObjectOf
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
static HRESULT WINAPI IShellFolder_GetUIObjectOf( 
  LPSHELLFOLDER this,
  HWND32        hwndOwner,
  UINT32        cidl,
  LPCITEMIDLIST * apidl, 
  REFIID        riid, 
  UINT32        *  prgfInOut,
  LPVOID        * ppvOut)
{	
	char		xclsid[50];
	LPITEMIDLIST	pidl;
	LPUNKNOWN	pObj = NULL; 

	WINE_StringFromCLSID(riid,xclsid);

	TRACE(shell,"(%p)->(%u,%u,apidl=%p,\n\tIID:%s,%p,%p)\n",
	  this,hwndOwner,cidl,apidl,xclsid,prgfInOut,ppvOut);

	*ppvOut = NULL;

	if(IsEqualIID(riid, &IID_IContextMenu))
	{ 
	  if(cidl < 1)
	    return E_INVALIDARG;

	  pObj  = (LPUNKNOWN)IContextMenu_Constructor(this, apidl, cidl);
	}
	else if (IsEqualIID(riid, &IID_IDataObject))
	{ 
	  if (cidl < 1)
	    return(E_INVALIDARG);

	  pObj = (LPUNKNOWN)IDataObject_Constructor (hwndOwner, this, apidl, cidl);
	}
	else if(IsEqualIID(riid, &IID_IExtractIcon))
	{ 
	  if (cidl != 1)
	    return(E_INVALIDARG);

	  pidl = ILCombine(this->pMyPidl,apidl[0]);
	  pObj = (LPUNKNOWN)IExtractIcon_Constructor( pidl );
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
	  ERR(shell,"(%p)->E_NOINTERFACE\n",this);
	  return E_NOINTERFACE;
	}

	if(!pObj)
	  return E_OUTOFMEMORY;

	*ppvOut = pObj;
	return S_OK;
}
/**************************************************************************
*  IShellFolder_GetDisplayNameOf
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

static HRESULT WINAPI IShellFolder_GetDisplayNameOf( LPSHELLFOLDER this, LPCITEMIDLIST pidl, DWORD dwFlags, LPSTRRET lpName)
{	CHAR	szText[MAX_PATH];
	CHAR	szTemp[MAX_PATH];
	CHAR	szSpecial[MAX_PATH];
	CHAR	szDrive[MAX_PATH];
	DWORD	dwVolumeSerialNumber,dwMaximumComponetLength,dwFileSystemFlags;
	LPITEMIDLIST	pidlTemp=NULL;
	BOOL32	bSimplePidl=FALSE;
		
	TRACE(shell,"(%p)->(pidl=%p,0x%08lx,%p)\n",this,pidl,dwFlags,lpName);
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
	    { GetVolumeInformation32A(szTemp,szDrive,MAX_PATH,&dwVolumeSerialNumber,&dwMaximumComponetLength,&dwFileSystemFlags,NULL,0);
	      szTemp[2]=0x00;						/* overwrite '\' */
	      strcat (szDrive," (");
	      strcat (szDrive,szTemp);
	      strcat (szDrive,")"); 
	    }
	    else							/* like "C:\" */
	    {  PathAddBackslash32A (szTemp);
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
	        if (this->sMyPath && strlen (this->sMyPath))
	        { if (strcmp(this->sMyPath,"My Computer"))
	          { strcpy (szText,this->sMyPath);
	            PathAddBackslash32A (szText);
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

	TRACE(shell,"-- (%p)->(%s)\n",this,szText);

	if(!(lpName))
	{  return E_OUTOFMEMORY;
	}
	lpName->uType = STRRET_CSTRA;	
	strcpy(lpName->u.cStr,szText);
	return S_OK;
}

/**************************************************************************
*  IShellFolder_SetNameOf
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
static HRESULT WINAPI IShellFolder_SetNameOf(
  	LPSHELLFOLDER this,
		HWND32 hwndOwner, 
    LPCITEMIDLIST pidl, /*simple pidl*/
    LPCOLESTR32 lpName, 
    DWORD dw, 
    LPITEMIDLIST *pPidlOut)
{  FIXME(shell,"(%p)->(%u,pidl=%p,%s,%lu,%p),stub!\n",
	  this,hwndOwner,pidl,debugstr_w(lpName),dw,pPidlOut);
	 return E_NOTIMPL;
}
/**************************************************************************
*  IShellFolder_GetFolderPath
*  FIXME: drive not included
*/
static BOOL32 WINAPI IShellFolder_GetFolderPath(LPSHELLFOLDER this, LPSTR lpszOut, DWORD dwOutSize)
{	DWORD	dwSize;

	TRACE(shell,"(%p)->(%p %lu)\n",this, lpszOut, dwOutSize);
	if (!lpszOut)
	{ return FALSE;
	}
    
	*lpszOut=0;

	if (! this->sMyPath)
	  return FALSE;
	  
	dwSize = strlen (this->sMyPath) +1;
	if ( dwSize > dwOutSize)
	  return FALSE;
	strcpy(lpszOut, this->sMyPath);

	TRACE(shell,"-- (%p)->(return=%s)\n",this, lpszOut);
	return TRUE;
}


