/*
 *	Shell Folder stuff (...and all the OLE-Objects of SHELL32.DLL)
 *
 *	Copyright 1997	Marcus Meissner
 *	Copyright 1998	Juergen Schmied
 *
 *  !!! currently work in progress on all classes 980930 !!!
 *  <contact juergen.schmied@metronet.de>
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "ole.h"
#include "ole2.h"
#include "debug.h"
#include "compobj.h"
#include "interfaces.h"
#include "shlobj.h"
#include "shell.h"
#include "winerror.h"
#include "winnls.h"
#include "winproc.h"
#include "commctrl.h"
#include "pidl.h"
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
	sf->mlpszFolder=NULL;	/* path of the folder */
	sf->mpSFParent=pParent;	/* parrent shellfolder */

	TRACE(shell,"(%p)->(parent=%p, pidl=%p)\n",sf,pParent, pidl);
	
	/* keep a copy of the pidl in the instance*/
	sf->mpidl = ILClone(pidl);
	sf->mpidlNSRoot = NULL;
	
	if(sf->mpidl)        /* do we have a pidl? */
	{ dwSize = 0;
	  if(sf->mpSFParent->mlpszFolder)		/* get the size of the parents path */
	  { dwSize += strlen(sf->mpSFParent->mlpszFolder) + 1;
	    TRACE(shell,"-- (%p)->(parent's path=%s)\n",sf, debugstr_a(sf->mpSFParent->mlpszFolder));
	  }   
	  dwSize += _ILGetFolderText(sf->mpidl,NULL,0); /* add the size of the foldername*/
	  sf->mlpszFolder = SHAlloc(dwSize);
	  if(sf->mlpszFolder)
	  { *(sf->mlpszFolder)=0x00;
	    if(sf->mpSFParent->mlpszFolder)		/* if the parent has a path, get it*/
	    {  strcpy(sf->mlpszFolder, sf->mpSFParent->mlpszFolder);
	       PathAddBackslash (sf->mlpszFolder);
	    }
	    _ILGetFolderText(sf->mpidl, sf->mlpszFolder+strlen(sf->mlpszFolder), dwSize-strlen(sf->mlpszFolder));
	    TRACE(shell,"-- (%p)->(my path=%s)\n",sf, debugstr_a(sf->mlpszFolder));
	  }
	}
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
{  char	xriid[50];
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
{	TRACE(shell,"(%p)->(count=%lu)\n",this,(this->ref)+1);
	return ++(this->ref);
}

/**************************************************************************
 *  IShellFolder_Release
 */
static ULONG WINAPI IShellFolder_Release(LPSHELLFOLDER this) 
{	TRACE(shell,"(%p)->(count=%lu)\n",this,this->ref);
	if (!--(this->ref)) 
	{ TRACE(shell,"-- destroying IShellFolder(%p)\n",this);

	  if (pdesktopfolder==this)
	  { pdesktopfolder=NULL;
	    TRACE(shell,"-- destroyed IShellFolder(%p) was Desktopfolder\n",this);
	  }
	  if(this->mpidlNSRoot)
	  { SHFree(this->mpidlNSRoot);
	  }
	  if(this->mpidl)
	  { SHFree(this->mpidl);
	  }
	  if(this->mlpszFolder)
	  { SHFree(this->mlpszFolder);
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
  CHAR           szElement[MAX_PATH];
  BOOL32         bType;

  DWORD          dwChars=lstrlen32W(lpszDisplayName) + 1;
  LPSTR          pszTemp=(LPSTR)HeapAlloc(GetProcessHeap(),0,dwChars * sizeof(CHAR));
       
  TRACE(shell,"(%p)->(HWND=0x%08x,%p,%p=%s,%p,pidl=%p,%p)\n",
	this,hwndOwner,pbcReserved,lpszDisplayName,debugstr_w(lpszDisplayName),pchEaten,ppidl,pdwAttributes);

	if(pszTemp)
	{ hr = E_FAIL;
	  WideCharToLocal32(pszTemp, lpszDisplayName, dwChars);
	  if(*pszTemp)
	  { if (strcmp(pszTemp,"Desktop")==0)
	    { pidlFull = (LPITEMIDLIST)HeapAlloc(GetProcessHeap(),0,2);
	      pidlFull->mkid.cb = 0;
	    }
	    else
	    { pidlFull = _ILCreateMyComputer();

	      /* check if the lpszDisplayName is Folder or File*/
	      bType = ! (GetFileAttributes32A(pszNext) & FILE_ATTRIBUTE_DIRECTORY);
	      pszNext = GetNextElement(pszTemp, szElement, MAX_PATH);
  
	      pidlTemp = _ILCreateDrive(szElement);			
	      pidlOld = pidlFull;
	      pidlFull = ILCombine(pidlFull,pidlTemp);
	      SHFree(pidlOld);
  
	      if(pidlFull)
	      { while((pszNext=GetNextElement(pszNext, szElement, MAX_PATH)))
	        { if(!*pszNext && bType)
	          { pidlTemp = _ILCreateValue(szElement);
	          }
	          else				
	          { pidlTemp = _ILCreateFolder(szElement);
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
	HeapFree(GetProcessHeap(),0,pszTemp);
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
	*ppEnumIDList = IEnumIDList_Constructor (this->mlpszFolder, dwFlags);
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
static HRESULT WINAPI IShellFolder_Initialize(
	LPSHELLFOLDER this,
	LPCITEMIDLIST pidl)
{ TRACE(shell,"(%p)->(pidl=%p)\n",this,pidl);
  if(this->mpidlNSRoot)
  { SHFree(this->mpidlNSRoot);
    this->mpidlNSRoot = NULL;
  }
  this->mpidlNSRoot=ILClone(pidl);
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
static HRESULT WINAPI IShellFolder_BindToObject(
	LPSHELLFOLDER this,
	LPCITEMIDLIST pidl,
	LPBC pbcReserved,
	REFIID riid,
	LPVOID * ppvOut)
{	char	        xriid[50];
  HRESULT       hr;
	LPSHELLFOLDER pShellFolder;
	
	WINE_StringFromCLSID(riid,xriid);

	TRACE(shell,"(%p)->(pidl=%p,%p,\n\tIID:%s,%p)\n",this,pidl,pbcReserved,xriid,ppvOut);

  *ppvOut = NULL;
  pShellFolder = IShellFolder_Constructor(this, pidl);
  if(!pShellFolder)
    return E_OUTOFMEMORY;
  /*  pShellFolder->lpvtbl->fnInitialize(pShellFolder, this->mpidlNSRoot);*/
  IShellFolder_Initialize(pShellFolder, this->mpidlNSRoot);
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
static HRESULT WINAPI IShellFolder_BindToStorage(
  	LPSHELLFOLDER this,
    LPCITEMIDLIST pidl, /*simple/complex pidl*/
    LPBC pbcReserved, 
    REFIID riid, 
    LPVOID *ppvOut)
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
static HRESULT WINAPI IShellFolder_GetUIObjectOf( LPSHELLFOLDER this,HWND32 hwndOwner,UINT32 cidl,
 LPCITEMIDLIST * apidl, REFIID riid, UINT32 * prgfInOut,LPVOID * ppvOut)
{	char	        xclsid[50];
	LPITEMIDLIST	pidl;
	LPUNKNOWN	pObj = NULL; 
   
	WINE_StringFromCLSID(riid,xclsid);

	TRACE(shell,"(%p)->(%u,%u,pidl=%p,\n\tIID:%s,%p,%p)\n",
	  this,hwndOwner,cidl,apidl,xclsid,prgfInOut,ppvOut);

	*ppvOut = NULL;

	if(IsEqualIID(riid, &IID_IContextMenu))
	{ if(cidl < 1)
	    return E_INVALIDARG;
	  pObj  = (LPUNKNOWN)IContextMenu_Constructor(this, apidl, cidl);
	}
	else if (IsEqualIID(riid, &IID_IDataObject))
        { if (cidl < 1)
	    return(E_INVALIDARG);
	  pObj = (LPUNKNOWN)IDataObject_Constructor (hwndOwner, this, apidl, cidl);
        }
	else if(IsEqualIID(riid, &IID_IExtractIcon))
	{ if (cidl != 1)
	    return(E_INVALIDARG);
	  pidl = ILCombine(this->mpidl, apidl[0]);
	  pObj = (LPUNKNOWN)IExtractIcon_Constructor(pidl);
	  SHFree(pidl);
	} 
	else
	{ ERR(shell,"(%p)->E_NOINTERFACE\n",this);
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
{	CHAR           szText[MAX_PATH];
	CHAR           szTemp[MAX_PATH];
	CHAR           szSpecial[MAX_PATH];
	CHAR           szDrive[MAX_PATH];
	DWORD          dwVolumeSerialNumber,dwMaximumComponetLength,dwFileSystemFlags;
	LPITEMIDLIST   pidlTemp=NULL;
	BOOL32				 bSimplePidl=FALSE;
		
	TRACE(shell,"(%p)->(pidl=%p,0x%08lx,%p)\n",this,pidl,dwFlags,lpName);

	szSpecial[0]=0x00; 
	szDrive[0]=0x00;

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
	{ if (_ILIsMyComputer( pidl))
	  { _ILGetItemText( pidl, szSpecial, MAX_PATH);
	  }
	  if (_ILIsDrive( pidl))
	  { pidlTemp = ILFindLastID(pidl);
	    if (pidlTemp)
	    { _ILGetItemText( pidlTemp, szTemp, MAX_PATH);
	    }
	    if ( dwFlags==SHGDN_NORMAL || dwFlags==SHGDN_INFOLDER)
	    { GetVolumeInformation32A(szTemp,szDrive,MAX_PATH,&dwVolumeSerialNumber,&dwMaximumComponetLength,&dwFileSystemFlags,NULL,0);
	      if (szTemp[2]=='\\')
	      { szTemp[2]=0x00;
	      }
	      strcat (szDrive," (");
	      strcat (szDrive,szTemp);
	      strcat (szDrive,")"); 
	    }
	    else
	    {  PathAddBackslash (szTemp);
	       strcpy(szDrive,szTemp);
	    }
	  }
		
	  switch(dwFlags)
	  { case SHGDN_NORMAL:
	      _ILGetPidlPath( pidl, szText, MAX_PATH);
	      break;

	    case SHGDN_INFOLDER | SHGDN_FORPARSING: /*fall thru*/
	    case SHGDN_INFOLDER:
	      pidlTemp = ILFindLastID(pidl);
	      if (pidlTemp)
	      { _ILGetItemText( pidlTemp, szText, MAX_PATH);
	      }
	      break;				

	    case SHGDN_FORPARSING:
	      if (bSimplePidl)
	      { /* if the IShellFolder has parents, get the path from the
	        parent and add the ItemName*/
	        szText[0]=0x00;
	        if (this->mlpszFolder && strlen (this->mlpszFolder))
	        { if (strcmp(this->mlpszFolder,"My Computer"))
	          { strcpy (szText,this->mlpszFolder);
	            PathAddBackslash (szText);
	          }
	        }
	        pidlTemp = ILFindLastID(pidl);
        	if (pidlTemp)
	        { _ILGetItemText( pidlTemp, szTemp, MAX_PATH );
	        } 
	        strcat(szText,szTemp);
	      }
	      else					
	      { /* if the pidl is absolute, get everything from the pidl*/
	        _ILGetPidlPath( pidl, szText, MAX_PATH);
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
	lpName->uType = STRRET_CSTR;	
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
    
	dwSize = strlen (this->mlpszFolder) +1;
	if ( dwSize > dwOutSize)
	  return FALSE;
	strcpy(lpszOut, this->mlpszFolder);

	TRACE(shell,"-- (%p)->(return=%s)\n",this, lpszOut);
	return TRUE;
}
