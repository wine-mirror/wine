/*
 *	Shell Folder stuff (...and all the OLE-Objects of SHELL32.DLL)
 *
 *	Copyright 1997	Marcus Meissner
 *	Copyright 1998	Juergen Schmied
 *
 *  !!! currently work in progress on all classes !!!
 *  <contact juergen.schmied@metronet.de, 980624>
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
#include "winerror.h"
#include "winnls.h"

/* FIXME should be moved to a header file. IsEqualGUID 
is declared but not exported in compobj.c !!!*/
#define IsEqualGUID(rguid1, rguid2) (!memcmp(rguid1, rguid2, sizeof(GUID)))
#define IsEqualIID(riid1, riid2) IsEqualGUID(riid1, riid2)
#define IsEqualCLSID(rclsid1, rclsid2) IsEqualGUID(rclsid1, rclsid2)
/***************************************************************************
 *  GetNextElement (internal function)
 *
 *  RETURNS
 *    LPSTR pointer to first, not yet parsed char
 */
LPSTR GetNextElement(
    LPSTR pszNext, /*[IN] string to get the element from*/
		LPSTR pszOut,  /*[IN] pointer to buffer whitch receives string*/
		DWORD dwOut)   /*[IN] length of pszOut*/
{ LPSTR   pszTail = pszNext;
  DWORD dwCopy;
  TRACE(shell,"(%s %p %lx)\n",pszNext,	pszOut, dwOut);

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

  TRACE(shell,"--(%s %s %lx)\n",pszNext,	pszOut, dwOut);
  return pszTail;
}

/**************************************************************************
*  IClassFactory Implementation
*/
static HRESULT WINAPI IClassFactory_QueryInterface(LPCLASSFACTORY,REFIID,LPVOID*);
static ULONG WINAPI IClassFactory_AddRef(LPCLASSFACTORY);
static ULONG WINAPI IClassFactory_Release(LPCLASSFACTORY);
static HRESULT WINAPI IClassFactory_CreateInstance();
/*static HRESULT WINAPI IClassFactory_LockServer();*/
/**************************************************************************
 *  IClassFactory_VTable
 */
static IClassFactory_VTable clfvt = {
	IClassFactory_QueryInterface,
	IClassFactory_AddRef,
	IClassFactory_Release,
	IClassFactory_CreateInstance,
/*	IClassFactory_LockServer*/
};

/**************************************************************************
 *  IClassFactory_Constructor
 */

LPCLASSFACTORY IClassFactory_Constructor()
{	LPCLASSFACTORY	lpclf;

	lpclf= (LPCLASSFACTORY)HeapAlloc(GetProcessHeap(),0,sizeof(IClassFactory));
	lpclf->ref = 1;
	lpclf->lpvtbl = &clfvt;
  TRACE(shell,"(%p)->()\n",lpclf);
	return lpclf;
}
/**************************************************************************
 *  IClassFactory::QueryInterface
 */
static HRESULT WINAPI IClassFactory_QueryInterface(
  LPCLASSFACTORY this, REFIID riid, LPVOID *ppvObj)
{  char	xriid[50];
   WINE_StringFromCLSID((LPCLSID)riid,xriid);
   TRACE(shell,"(%p)->(\n\tIID:\t%s)\n",this,xriid);

  *ppvObj = NULL;

  if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
  { *ppvObj = this; 
  }
  else if(IsEqualIID(riid, &IID_IClassFactory))  /*IClassFactory*/
  {    *ppvObj = (IClassFactory*)this;
  }   

  if(*ppvObj)
  { (*(LPCLASSFACTORY*)ppvObj)->lpvtbl->fnAddRef(this);  	
    TRACE(shell,"-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
    return S_OK;
  }
	TRACE(shell,"-- Interface: E_NOINTERFACE\n");
  return E_NOINTERFACE;
}  
/******************************************************************************
 * IClassFactory_AddRef
 */
static ULONG WINAPI IClassFactory_AddRef(LPCLASSFACTORY this)
{ TRACE(shell,"(%p)->(count=%lu)\n",this,this->ref);
	return ++(this->ref);
}
/******************************************************************************
 * IClassFactory_Release
 */
static ULONG WINAPI IClassFactory_Release(LPCLASSFACTORY this)
{	TRACE(shell,"(%p)->(count=%lu)\n",this,this->ref);
	if (!--(this->ref)) 
	{ TRACE(shell,"-- destroying IClassFactory(%p)\n",this);
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
	return this->ref;
}
/******************************************************************************
 * IClassFactory_CreateInstance
 */
static HRESULT WINAPI IClassFactory_CreateInstance(
  LPCLASSFACTORY this, LPUNKNOWN pUnknown, REFIID riid, LPVOID *ppObject)
{ LPSHELLFOLDER pSHFolder;
  HRESULT hResult=E_OUTOFMEMORY;
  
	char	xriid[50];
  WINE_StringFromCLSID((LPCLSID)riid,xriid);
  TRACE(shell,"%p->(%p,\n\tIID:\t%s)\n",this,pUnknown,xriid);

  *ppObject = NULL;
	
	if(pUnknown)
	{	return CLASS_E_NOAGGREGATION;
	}

	if (IsEqualIID(riid, &IID_IShellFolder))
  { pSHFolder = IShellFolder_Constructor(NULL,NULL);
    if(pSHFolder)
  	{ hResult = pSHFolder->lpvtbl->fnQueryInterface(pSHFolder,riid, ppObject);
      pSHFolder->lpvtbl->fnRelease(pSHFolder);
  		TRACE(shell,"-- ShellFolder created: (%p)->%p\n",this,*ppObject);
  	}
	}	
	else
	{  FIXME(shell,"unknown IID requested\n\tIID:\t%s\n",xriid);
	   hResult=E_NOINTERFACE;
	}
  return hResult;
}
/******************************************************************************
 * IClassFactory_LockServer
 */
/*static HRESULT WINAPI IClassFactory_LockServer(LPCLASSFACTORY this, BOOL)
{ TRACE(shell,"%p->(), not implemented\n",this)
  return E_NOTIMPL;
}

*/
/**************************************************************************
 *  IEnumIDList Implementation
 */
static HRESULT WINAPI IEnumIDList_QueryInterface(LPENUMIDLIST,REFIID,LPVOID*);
static ULONG WINAPI IEnumIDList_AddRef(LPENUMIDLIST);
static ULONG WINAPI IEnumIDList_Release(LPENUMIDLIST);
static HRESULT WINAPI IEnumIDList_Next(LPENUMIDLIST,ULONG,LPITEMIDLIST*,ULONG*);
static HRESULT WINAPI IEnumIDList_Skip(LPENUMIDLIST,ULONG);
static HRESULT WINAPI IEnumIDList_Reset(LPENUMIDLIST);
static HRESULT WINAPI IEnumIDList_Clone(LPENUMIDLIST,LPENUMIDLIST*);
static BOOL32 WINAPI IEnumIDList_CreateEnumList(LPENUMIDLIST,LPCSTR, DWORD);
static BOOL32 WINAPI IEnumIDList_AddToEnumList(LPENUMIDLIST,LPITEMIDLIST);
static BOOL32 WINAPI IEnumIDList_DeleteList(LPENUMIDLIST);
/**************************************************************************
 *  IEnumIDList_VTable
 */
static IEnumIDList_VTable eidlvt = {
	IEnumIDList_QueryInterface,
	IEnumIDList_AddRef,
	IEnumIDList_Release,
	IEnumIDList_Next,
	IEnumIDList_Skip,
	IEnumIDList_Reset,
  IEnumIDList_Clone,
	IEnumIDList_CreateEnumList,
  IEnumIDList_AddToEnumList,
	IEnumIDList_DeleteList
};

/**************************************************************************
 *  IEnumIDList_Constructor
 */

LPENUMIDLIST IEnumIDList_Constructor( LPCSTR lpszPath, DWORD dwFlags, HRESULT* pResult)
{	LPENUMIDLIST	lpeidl;

	lpeidl = (LPENUMIDLIST)HeapAlloc(GetProcessHeap(),0,sizeof(IEnumIDList));
	lpeidl->ref = 1;
	lpeidl->lpvtbl = &eidlvt;
	lpeidl->mpFirst=NULL;
	lpeidl->mpLast=NULL;
	lpeidl->mpCurrent=NULL;

  TRACE(shell,"(%p)->(%s %lx %p)\n",lpeidl,lpszPath,dwFlags,pResult);

	lpeidl->mpPidlMgr=PidlMgr_Constructor();
  if (!lpeidl->mpPidlMgr)
	{ if (pResult)
	  { *pResult=E_OUTOFMEMORY;
			HeapFree(GetProcessHeap(),0,lpeidl);
			return NULL;
		}
	}

	if(!IEnumIDList_CreateEnumList(lpeidl, lpszPath, dwFlags))
  { if(pResult)
    { *pResult = E_OUTOFMEMORY;
			HeapFree(GetProcessHeap(),0,lpeidl->mpPidlMgr);
			HeapFree(GetProcessHeap(),0,lpeidl);
			return NULL;
		}
  }

  TRACE(shell,"-- (%p)->()\n",lpeidl);
	return lpeidl;
}

/**************************************************************************
 *  EnumIDList::QueryInterface
 */
static HRESULT WINAPI IEnumIDList_QueryInterface(
  LPENUMIDLIST this, REFIID riid, LPVOID *ppvObj)
{  char	xriid[50];
   WINE_StringFromCLSID((LPCLSID)riid,xriid);
   TRACE(shell,"(%p)->(\n\tIID:\t%s,%p)\n",this,xriid,ppvObj);

  *ppvObj = NULL;

  if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
  { *ppvObj = this; 
  }
  else if(IsEqualIID(riid, &IID_IEnumIDList))  /*IEnumIDList*/
  {    *ppvObj = (IEnumIDList*)this;
  }   

  if(*ppvObj)
  { (*(LPENUMIDLIST*)ppvObj)->lpvtbl->fnAddRef(this);  	
    TRACE(shell,"-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
    return S_OK;
  }
	TRACE(shell,"-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}   

/******************************************************************************
 * IEnumIDList_AddRef
 */
static ULONG WINAPI IEnumIDList_AddRef(LPENUMIDLIST this)
{ TRACE(shell,"(%p)->()\n",this);
	return ++(this->ref);
}
/******************************************************************************
 * IEnumIDList_Release
 */
static ULONG WINAPI IEnumIDList_Release(LPENUMIDLIST this)
{	TRACE(shell,"(%p)->()\n",this);
	if (!--(this->ref)) 
	{ TRACE(shell," destroying IEnumIDList(%p)\n",this);
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
	return this->ref;
}
   
/**************************************************************************
 *  IEnumIDList_Next
 */

static HRESULT WINAPI IEnumIDList_Next(
	LPENUMIDLIST this,ULONG celt,LPITEMIDLIST * rgelt,ULONG *pceltFetched) 
{ ULONG    i;
  HRESULT  hr = S_OK;

  LPITEMIDLIST  temp;

	TRACE(shell,"(%p)->(%ld,%p, %p)\n",this,celt,rgelt,pceltFetched);

	*rgelt=0;
	
  if(celt > 1 && !pceltFetched)
  { return E_INVALIDARG;
	}

  for(i = 0; i < celt; i++)
  { if(!(this->mpCurrent))
    { hr =  S_FALSE;
      break;
    }
		temp = this->mpPidlMgr->lpvtbl->fnCopy(this->mpPidlMgr, this->mpCurrent->pidl);
    rgelt[i] = temp;
    this->mpCurrent = this->mpCurrent->pNext;
  }
  if(pceltFetched)
  {  *pceltFetched = i;
	}

  return hr;
}

/**************************************************************************
*  IEnumIDList_Skip
*/
static HRESULT WINAPI IEnumIDList_Skip(
	LPENUMIDLIST this,ULONG celt)
{ DWORD    dwIndex;
  HRESULT  hr = S_OK;

  TRACE(shell,"(%p)->(%lu)\n",this,celt);

  for(dwIndex = 0; dwIndex < celt; dwIndex++)
  { if(!this->mpCurrent)
    { hr = S_FALSE;
      break;
    }
    this->mpCurrent = this->mpCurrent->pNext;
  }
  return hr;
}
/**************************************************************************
*  IEnumIDList_Reset
*/
static HRESULT WINAPI IEnumIDList_Reset(LPENUMIDLIST this)
{ TRACE(shell,"(%p)\n",this);
  this->mpCurrent = this->mpFirst;
  return S_OK;
}
/**************************************************************************
*  IEnumIDList_Clone
*/
static HRESULT WINAPI IEnumIDList_Clone(
	LPENUMIDLIST this,LPENUMIDLIST * ppenum)
{ TRACE(shell,"(%p)->() to (%p)->() E_NOTIMPL\n",this,ppenum);
	return E_NOTIMPL;
}
/**************************************************************************
 *  EnumIDList_CreateEnumList()
 *  fixme: devices not handled
 *  fixme: add wildcards to path
 */
static BOOL32 WINAPI IEnumIDList_CreateEnumList(LPENUMIDLIST this, LPCSTR lpszPath, DWORD dwFlags)
{ LPITEMIDLIST   pidl=NULL;
  WIN32_FIND_DATA32A stffile;	
  HANDLE32 hFile;
	
  TRACE(shell,"(%p)->(%s %lx) \n",this,lpszPath,dwFlags);

  /*enumerate the folders*/
  if(dwFlags & SHCONTF_FOLDERS)
  { /* special case - we can't enumerate the Desktop level Objects (MyComputer,Nethood...
   so we need to fake an enumeration of those.*/
   if(!lpszPath)
   { //create the pidl for this item
     pidl = this->mpPidlMgr->lpvtbl->fnCreateDesktop(this->mpPidlMgr);
     if(pidl)
     { if(!IEnumIDList_AddToEnumList(this, pidl))
         return FALSE;
     }
     else
     { return FALSE;
     }
   }   
   else
   {  hFile = FindFirstFile32A(lpszPath,&stffile);
      do
      { if (stffile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			  //create the pidl for this item
				/* fixme: the shortname should be given too*/
        pidl = this->mpPidlMgr->lpvtbl->fnCreateFolder(this->mpPidlMgr, stffile.cFileName);
        if(pidl)
        { if(!IEnumIDList_AddToEnumList(this, pidl))
            return FALSE;
        }
        else
        { return FALSE;
        }   
      } while( FindNextFile32A(hFile,&stffile));
			FindClose32 (hFile);
    }
  }   
  //enumerate the non-folder items (values)
  if(dwFlags & SHCONTF_NONFOLDERS)
  {   hFile = FindFirstFile32A(lpszPath,&stffile);
      do
      { if (! (stffile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
			  //create the pidl for this item
				/* fixme: the shortname should be given too*/
        pidl = this->mpPidlMgr->lpvtbl->fnCreateFolder(this->mpPidlMgr, stffile.cFileName);
        if(pidl)
        { if(!IEnumIDList_AddToEnumList(this, pidl))
          { return FALSE;
					}
        }
        else
        { return FALSE;
        }   
      } while( FindNextFile32A(hFile,&stffile));
			FindClose32 (hFile);
   } 
  return TRUE;
}

/**************************************************************************
 *  EnumIDList_AddToEnumList()
 */
static BOOL32 WINAPI IEnumIDList_AddToEnumList(LPENUMIDLIST this,LPITEMIDLIST pidl)
{ LPENUMLIST  pNew;

  TRACE(shell,"(%p)->(pidl=%p)\n",this,pidl);
  pNew = (LPENUMLIST)HeapAlloc(GetProcessHeap(),0,sizeof(ENUMLIST));
  if(pNew)
  { //set the next pointer
    pNew->pNext = NULL;
    pNew->pidl = pidl;

    //is this the first item in the list?
    if(!this->mpFirst)
    { this->mpFirst = pNew;
      this->mpCurrent = pNew;
    }
   
    if(this->mpLast)
    { //add the new item to the end of the list
      this->mpLast->pNext = pNew;
    }
   
    //update the last item pointer
    this->mpLast = pNew;
		TRACE(shell,"(%p)->(first=%p, last=%p)\n",this,this->mpFirst,this->mpLast);
    return TRUE;
  }
  return FALSE;
}
/**************************************************************************
*   EnumIDList_DeleteList()
*/
static BOOL32 WINAPI IEnumIDList_DeleteList(LPENUMIDLIST this)
{ LPENUMLIST  pDelete;

  TRACE(shell,"(%p)->()\n",this);
	
  while(this->mpFirst)
  { pDelete = this->mpFirst;
    this->mpFirst = pDelete->pNext;

    //free the pidl
    this->mpPidlMgr->lpvtbl->fnDelete(this->mpPidlMgr,pDelete->pidl);
   
    //free the list item
		HeapFree(GetProcessHeap(),0,pDelete);
  }
  this->mpFirst = this->mpLast = this->mpCurrent = NULL;
  return TRUE;
}

/***********************************************************************
*   IShellFolder implementation
*/
/*LPSHELLFOLDER IShellFolder_Constructor();*/
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
/***********************************************************************
*
*	  IShellFolder_VTable
*/
static struct IShellFolder_VTable sfvt = {
  IShellFolder_QueryInterface,
	IShellFolder_AddRef,
	IShellFolder_Release,
	IShellFolder_Initialize,
	IShellFolder_ParseDisplayName,
	IShellFolder_EnumObjects,
	IShellFolder_BindToObject,
  IShellFolder_BindToStorage,
  IShellFolder_CompareIDs,
	IShellFolder_CreateViewObject,
	IShellFolder_GetAttributesOf,
  IShellFolder_GetUIObjectOf,
  IShellFolder_GetDisplayNameOf,
  IShellFolder_SetNameOf
};
/**************************************************************************
*	  IShellFolder_Constructor
*/

LPSHELLFOLDER IShellFolder_Constructor(LPSHELLFOLDER pParent,LPITEMIDLIST pidl) {
	LPSHELLFOLDER	sf;
	DWORD dwSize=0;
	WORD wLen;
	sf = (LPSHELLFOLDER)HeapAlloc(GetProcessHeap(),0,sizeof(IShellFolder));
	sf->ref		= 1;
	sf->lpvtbl	= &sfvt;
  sf->mlpszFolder=NULL;
	sf->mpSFParent=pParent;

	TRACE(shell,"(%p)->(parent:%p, pidl=%p)\n",sf,pParent, pidl);
	
  sf->pPidlMgr  = PidlMgr_Constructor();
	if (! sf->pPidlMgr )
	{ HeapFree(GetProcessHeap(),0,sf);
	  ERR (shell,"-- Could not initialize PidMGR\n");
	  return NULL;
	}

	sf->mpidl = sf->pPidlMgr->lpvtbl->fnCopy(sf->pPidlMgr, pidl);
	sf->mpidlNSRoot = NULL;
	
  if(sf->mpidl)
  { /*if(sf->pPidlMgr->lpvtbl->fnIsDesktop(sf->pPidlMgr,sf->mpidl))
    { sf->pPidlMgr->lpvtbl->fnGetDesktop(sf->pPidlMgr,sf->mpidl);
    }*/
    dwSize = 0;
    if(sf->mpSFParent->mlpszFolder)
    { dwSize += strlen(sf->mpSFParent->mlpszFolder) + 1;
    }   
    dwSize += sf->pPidlMgr->lpvtbl->fnGetFolderText(sf->pPidlMgr,sf->mpidl,NULL,0);
    sf->mlpszFolder = (LPSTR)HeapAlloc(GetProcessHeap(),0,dwSize);
    if(sf->mlpszFolder)
    { *(sf->mlpszFolder) = 0;
      if(sf->mpSFParent->mlpszFolder)
      {  strcpy(sf->mlpszFolder, sf->mpSFParent->mlpszFolder);
         wLen = strlen(sf->mlpszFolder);
         if (wLen && sf->mlpszFolder[wLen-1]!='\\')
         { sf->mlpszFolder[wLen+0]='\\';
           sf->mlpszFolder[wLen+1]='\0';
         }
      }
      sf->pPidlMgr->lpvtbl->fnGetFolderText(sf->pPidlMgr, sf->mpidl, sf->mlpszFolder+strlen(sf->mlpszFolder), dwSize-strlen(sf->mlpszFolder));
    }
  }
	
	TRACE(shell,"-- (%p)->(%p,%p)\n",sf,pParent, pidl);
	return sf;
}
/**************************************************************************
 *  IShellFolder::QueryInterface
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
{ TRACE(shell,"(%p)->(count=%lu)\n",this,this->ref);
	if (!--(this->ref)) {
		TRACE(shell,"-- destroying IShellFolder(%p)\n",this);
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
	return this->ref;
}
/**************************************************************************
*		IShellFolder_ParseDisplayName
*
* FIXME: 
*    pdwAttributes: not used
*/
static HRESULT WINAPI IShellFolder_ParseDisplayName(
	LPSHELLFOLDER this,
	HWND32 hwndOwner,
	LPBC pbcReserved,
	LPOLESTR32 lpszDisplayName,    /* [in]  name of file or folder*/
	DWORD *pchEaten,               /* [out] number of chars parsed*/
	LPITEMIDLIST *ppidl,           /* [out] the pidl*/
	DWORD *pdwAttributes)
{	HRESULT        hr=E_OUTOFMEMORY;
  LPITEMIDLIST   pidlFull=NULL;
  DWORD          dwChars=lstrlen32W(lpszDisplayName) + 1;
  LPSTR          pszTemp=(LPSTR)HeapAlloc(GetProcessHeap(),0,dwChars * sizeof(CHAR));
  LPSTR          pszNext=NULL;
  CHAR           szElement[MAX_PATH];
  BOOL32         bType;
  LPITEMIDLIST   pidlTemp = NULL;
  LPITEMIDLIST   pidlOld = NULL;
       
  TRACE(shell,"(%p)->(%x,%p,%p=%s,%p,pidl=%p,%p)\n",
	this,hwndOwner,pbcReserved,lpszDisplayName,debugstr_w(lpszDisplayName),pchEaten,ppidl,pdwAttributes
	);
  if(pszTemp)
  { hr = E_FAIL;
    WideCharToLocal32(pszTemp, lpszDisplayName, dwChars);
    if(*pszTemp)
    { pidlFull = this->pPidlMgr->lpvtbl->fnCreateDesktop(this->pPidlMgr);

      /* check if the lpszDisplayName is Folder or File*/
			bType = ! (GetFileAttributes32A(pszNext)&FILE_ATTRIBUTE_DIRECTORY);
			
			pszNext = GetNextElement(pszTemp, szElement, MAX_PATH);

      pidlTemp = this->pPidlMgr->lpvtbl->fnCreateDrive(this->pPidlMgr,szElement);			
      pidlOld = pidlFull;
      pidlFull = this->pPidlMgr->lpvtbl->fnConcatenate(this->pPidlMgr,pidlFull,pidlTemp);
	    this->pPidlMgr->lpvtbl->fnDelete(this->pPidlMgr,pidlOld);

			if(pidlFull)
      { while((pszNext=GetNextElement(pszNext, szElement, MAX_PATH)))
        { if(!*pszNext && bType)
					{ pidlTemp = this->pPidlMgr->lpvtbl->fnCreateValue(this->pPidlMgr,szElement);
					}
					else				
          { pidlTemp = this->pPidlMgr->lpvtbl->fnCreateFolder(this->pPidlMgr,szElement);
					}
          pidlOld = pidlFull;
          pidlFull = this->pPidlMgr->lpvtbl->fnConcatenate(this->pPidlMgr,pidlFull,pidlTemp);
          this->pPidlMgr->lpvtbl->fnDelete(this->pPidlMgr,pidlOld);
         }
         hr = S_OK;
       }
     }
		HeapFree(GetProcessHeap(),0,pszTemp);
  }
  *ppidl = pidlFull;
  return hr;



}

/**************************************************************************
*		IShellFolder_EnumObjects
*/
static HRESULT WINAPI IShellFolder_EnumObjects(
	LPSHELLFOLDER this,HWND32 hwndOwner,DWORD dwFlags,
	LPENUMIDLIST* ppEnumIDList)
{ HRESULT  hr;
	TRACE(shell,"(%p)->(%x,%lx,%p)\n",this,hwndOwner,dwFlags,ppEnumIDList);

  *ppEnumIDList = NULL;
	*ppEnumIDList = IEnumIDList_Constructor (this->mlpszFolder, dwFlags, &hr);
  TRACE(shell,"-- (%p)->(new ID List: %p)\n",this,*ppEnumIDList);
  if(!*ppEnumIDList)
  { return hr;
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
  { this->pPidlMgr->lpvtbl->fnDelete(this->pPidlMgr, this->mpidlNSRoot);
    this->mpidlNSRoot = NULL;
  }
  this->mpidlNSRoot = this->pPidlMgr->lpvtbl->fnCopy(this->pPidlMgr, pidl);
  return S_OK;
}

/**************************************************************************
*		IShellFolder_BindToObject
*/
static HRESULT WINAPI IShellFolder_BindToObject(
	LPSHELLFOLDER this,
	LPCITEMIDLIST pidl,
	LPBC pbcReserved,
	REFIID riid,
	LPVOID * ppvOut)
{	char	        xclsid[50];
  HRESULT       hr;
	LPSHELLFOLDER pShellFolder;
	
	WINE_StringFromCLSID(riid,xclsid);

	TRACE(shell,"(%p)->(pidl=%p,%p,\n\tSID:%s,%p)\n",this,pidl,pbcReserved,xclsid,ppvOut);

  *ppvOut = NULL;
  pShellFolder = IShellFolder_Constructor(this, pidl);
  if(!pShellFolder)
    return E_OUTOFMEMORY;
  pShellFolder->lpvtbl->fnInitialize(pShellFolder, this->mpidlNSRoot);
  hr = pShellFolder->lpvtbl->fnQueryInterface(pShellFolder, riid, ppvOut);
  pShellFolder->lpvtbl->fnRelease(pShellFolder);
	TRACE(shell,"-- (%p)->(interface=%p)\n",this, ppvOut);
  return hr;
}

/**************************************************************************
*  IShellFolder_BindToStorage
*/
static HRESULT WINAPI IShellFolder_BindToStorage(
  	LPSHELLFOLDER this,
    LPCITEMIDLIST pidl, 
    LPBC pbcReserved, 
    REFIID riid, 
    LPVOID *ppvOut)
{	char	        xclsid[50];
	WINE_StringFromCLSID(riid,xclsid);

	FIXME(shell,"(%p)->(pidl=%p,%p,\n\tSID:%s,%p) stub\n",this,pidl,pbcReserved,xclsid,ppvOut);

  *ppvOut = NULL;
  return E_NOTIMPL;
}

/**************************************************************************
*  IShellFolder_CompareIDs
*/
static HRESULT WINAPI  IShellFolder_CompareIDs(
  	LPSHELLFOLDER this,
		LPARAM lParam, 
    LPCITEMIDLIST pidl1, 
    LPCITEMIDLIST pidl2)
{ CHAR szString1[MAX_PATH] = "";
  CHAR szString2[MAX_PATH] = "";
  int   nReturn;
  LPCITEMIDLIST  pidlTemp1 = pidl1, pidlTemp2 = pidl2;

  TRACE(shell,"(%p)->(%lx,pidl1=%p,pidl2=%p) stub\n",this,lParam,pidl1,pidl2);

  /*Special case - If one of the items is a Path and the other is a File, always 
  make the Path come before the File.*/

  //get the last item in each list
  while((this->pPidlMgr->lpvtbl->fnGetNextItem(this->pPidlMgr,pidlTemp1))->mkid.cb)
    pidlTemp1 = this->pPidlMgr->lpvtbl->fnGetNextItem(this->pPidlMgr,pidlTemp1);
  while((this->pPidlMgr->lpvtbl->fnGetNextItem(this->pPidlMgr,pidlTemp2))->mkid.cb)
    pidlTemp2 = this->pPidlMgr->lpvtbl->fnGetNextItem(this->pPidlMgr,pidlTemp2);

  //at this point, both pidlTemp1 and pidlTemp2 point to the last item in the list
  if(this->pPidlMgr->lpvtbl->fnIsValue(this->pPidlMgr,pidlTemp1) != this->pPidlMgr->lpvtbl->fnIsValue(this->pPidlMgr,pidlTemp2))
  { if(this->pPidlMgr->lpvtbl->fnIsValue(this->pPidlMgr,pidlTemp1))
      return 1;
   return -1;
  }

  this->pPidlMgr->lpvtbl->fnGetDrive(this->pPidlMgr, pidl1,szString1,sizeof(szString1));
  this->pPidlMgr->lpvtbl->fnGetDrive(this->pPidlMgr, pidl2,szString1,sizeof(szString2));
  nReturn = strcasecmp(szString1, szString2);
  if(nReturn)
    return nReturn;

  this->pPidlMgr->lpvtbl->fnGetFolderText(this->pPidlMgr, pidl1,szString1,sizeof(szString1));
  this->pPidlMgr->lpvtbl->fnGetFolderText(this->pPidlMgr, pidl2,szString2,sizeof(szString2));
  nReturn = strcasecmp(szString1, szString2);
  if(nReturn)
    return nReturn;

  this->pPidlMgr->lpvtbl->fnGetValueText(this->pPidlMgr,pidl1,szString1,sizeof(szString1));
  this->pPidlMgr->lpvtbl->fnGetValueText(this->pPidlMgr,pidl2,szString2,sizeof(szString2));
  return strcasecmp(szString1, szString2);
}

/**************************************************************************
*	  IShellFolder_CreateViewObject
*/
static HRESULT WINAPI IShellFolder_CreateViewObject(
	LPSHELLFOLDER this,HWND32 hwndOwner,REFIID riid,LPVOID *ppv)
{	char	xclsid[50];

	WINE_StringFromCLSID(riid,xclsid);
	FIXME(shell,"(%p)->(0x%04x,\n\tIID:\t%s,%p),stub!\n",this,hwndOwner,xclsid,ppv);
	
	*(DWORD*)ppv = 0;
	return E_OUTOFMEMORY;
}

/**************************************************************************
*  IShellFolder_GetAttributesOf
*/
static HRESULT WINAPI IShellFolder_GetAttributesOf(
	LPSHELLFOLDER this,
	UINT32 cidl,
	LPCITEMIDLIST *apidl,
	DWORD *rgfInOut)
{ FIXME(shell,"(%p)->(%d,%p,%p),stub!\n",this,cidl,apidl,rgfInOut);
	return E_NOTIMPL;
}
/**************************************************************************
*  IShellFolder_GetUIObjectOf
*/
static HRESULT WINAPI IShellFolder_GetUIObjectOf(
	LPSHELLFOLDER this,
	HWND32 hwndOwner,
	UINT32 cidl, 
	LPCITEMIDLIST * apidl,
  REFIID riid,
	UINT32 * prgfInOut,
	LPVOID * ppvOut)
{ char	        xclsid[50];
  
	WINE_StringFromCLSID(riid,xclsid);

  FIXME(shell,"(%p)->(%u %u,pidl=%p,\n\tIID:%s,%p,%p),stub!\n",
	  this,hwndOwner,cidl,apidl,xclsid,prgfInOut,ppvOut);
	return E_FAIL;
}
/**************************************************************************
*  IShellFolder_GetDisplayNameOf
*/
#define GET_SHGDN_FOR(dwFlags)         ((DWORD)dwFlags & (DWORD)0x0000FF00)
#define GET_SHGDN_RELATION(dwFlags)    ((DWORD)dwFlags & (DWORD)0x000000FF)

static HRESULT WINAPI IShellFolder_GetDisplayNameOf( 
  	LPSHELLFOLDER this,
    LPCITEMIDLIST pidl, 
    DWORD dwFlags, 
    LPSTRRET lpName)
{ CHAR szText[MAX_PATH];
  int   cchOleStr;
  LPITEMIDLIST   pidlTemp;

  TRACE(shell,"(%p)->(pidl=%p,%lx,%p)\n",this,pidl,dwFlags,lpName);
  switch(GET_SHGDN_RELATION(dwFlags))
  { case SHGDN_NORMAL:
    //get the full name
    this->pPidlMgr->lpvtbl->fnGetPidlPath(this->pPidlMgr, pidl, szText, sizeof(szText));
/*  FIXME if the text is NULL and this is a value, then is something wrong*/
/*  if(!*szText && this->pPidlMgr->lpvtbl->fnIsValue(this->pPidlMgr, this->pPidlMgr->lpvtbl->fnGetLastItem(this->pPidlMgr, pidl)))
    { do_something()
    }*/  
    break;

    case SHGDN_INFOLDER:
      pidlTemp = this->pPidlMgr->lpvtbl->fnGetLastItem(this->pPidlMgr,pidl);
      //get the relative name
      this->pPidlMgr->lpvtbl->fnGetItemText(this->pPidlMgr, pidlTemp, szText, sizeof(szText));
/*    FIXME if the text is NULL and this is a value, then is something wrong*/
      if(!*szText && this->pPidlMgr->lpvtbl->fnIsValue(this->pPidlMgr,pidlTemp))
/*    { do_something()
      }*/   
     break;
   default:    return E_INVALIDARG;
  }

  //get the number of characters required
  cchOleStr = strlen(szText) + 1;

  TRACE(shell,"-- (%p)->(%s)\n",this,szText);
  //allocate the wide character string
  lpName->u.pOleStr = (LPWSTR)HeapAlloc(GetProcessHeap(),0,cchOleStr * sizeof(WCHAR));

  if(!(lpName->u.pOleStr))
  {  return E_OUTOFMEMORY;
	}
  lpName->uType = STRRET_WSTR;
  LocalToWideChar32(lpName->u.pOleStr, szText, cchOleStr);
  return S_OK;
}

/**************************************************************************
*  IShellFolder_SetNameOf
*/
static HRESULT WINAPI IShellFolder_SetNameOf(
  	LPSHELLFOLDER this,
		HWND32 hwndOwner, 
    LPCITEMIDLIST pidl, 
    LPCOLESTR32 lpName, 
    DWORD dw, 
    LPITEMIDLIST *pPidlOut)
{  FIXME(shell,"(%p)->(%u,pidl=%p,%s,%lu,%p),stub!\n",
	  this,hwndOwner,pidl,debugstr_w(lpName),dw,pPidlOut);
	 return E_NOTIMPL;
}

/**************************************************************************
*	  IShellLink_VTable
*/
static struct IShellLink_VTable slvt = {
    (void *)1,
    (void *)2,
    (void *)3,
    (void *)4,
    (void *)5,
    (void *)6,
    (void *)7,
    (void *)8,
    (void *)9,
    (void *)10,
    (void *)11,
    (void *)12,
    (void *)13,
    (void *)14,
    (void *)15,
    (void *)16,
    (void *)17,
    (void *)18,
    (void *)19,
    (void *)20,
    (void *)21
};

/**************************************************************************
 *	  IShellLink_Constructor
 */
LPSHELLLINK IShellLink_Constructor() 
{	LPSHELLLINK sl;

	sl = (LPSHELLLINK)HeapAlloc(GetProcessHeap(),0,sizeof(IShellLink));
	sl->ref = 1;
	sl->lpvtbl = &slvt;
	TRACE(shell,"(%p)->()\n",sl);
	return sl;
}

/**************************************************************************
*	  INTERNAL CLASS pidlmgr
*/
LPITEMIDLIST PidlMgr_CreateDesktop(LPPIDLMGR);
LPITEMIDLIST PidlMgr_CreateDrive(LPPIDLMGR,LPCSTR);
LPITEMIDLIST PidlMgr_CreateFolder(LPPIDLMGR,LPCSTR);
LPITEMIDLIST PidlMgr_CreateValue(LPPIDLMGR,LPCSTR);
void PidlMgr_Delete(LPPIDLMGR,LPITEMIDLIST);
LPITEMIDLIST PidlMgr_GetNextItem(LPPIDLMGR,LPITEMIDLIST);
LPITEMIDLIST PidlMgr_Copy(LPPIDLMGR,LPITEMIDLIST);
UINT16 PidlMgr_GetSize(LPPIDLMGR,LPITEMIDLIST);
BOOL32 PidlMgr_GetDesktop(LPPIDLMGR,LPCITEMIDLIST,LPSTR);
BOOL32 PidlMgr_GetDrive(LPPIDLMGR,LPCITEMIDLIST,LPSTR,UINT16);
LPITEMIDLIST PidlMgr_GetLastItem(LPPIDLMGR,LPCITEMIDLIST);
DWORD PidlMgr_GetItemText(LPPIDLMGR,LPCITEMIDLIST,LPSTR,UINT16);
BOOL32 PidlMgr_IsDesktop(LPPIDLMGR,LPCITEMIDLIST);
BOOL32 PidlMgr_IsDrive(LPPIDLMGR,LPCITEMIDLIST);
BOOL32 PidlMgr_IsFolder(LPPIDLMGR,LPCITEMIDLIST);
BOOL32 PidlMgr_IsValue(LPPIDLMGR,LPCITEMIDLIST);
BOOL32 PidlMgr_HasFolders(LPPIDLMGR,LPSTR,LPCITEMIDLIST);
DWORD PidlMgr_GetFolderText(LPPIDLMGR,LPCITEMIDLIST,LPSTR,DWORD);
DWORD PidlMgr_GetValueText(LPPIDLMGR,LPCITEMIDLIST,LPSTR,DWORD);
BOOL32 PidlMgr_GetValueType(LPPIDLMGR,LPCITEMIDLIST,LPCITEMIDLIST,LPDWORD);
DWORD PidlMgr_GetDataText(LPPIDLMGR,LPCITEMIDLIST,LPCITEMIDLIST,LPSTR,DWORD);
DWORD PidlMgr_GetPidlPath(LPPIDLMGR,LPCITEMIDLIST,LPSTR,DWORD);
LPITEMIDLIST PidlMgr_Concatenate(LPPIDLMGR,LPITEMIDLIST,LPITEMIDLIST);
LPITEMIDLIST PidlMgr_Create(LPPIDLMGR,PIDLTYPE,LPVOID,UINT16);
DWORD PidlMgr_GetData(LPPIDLMGR,PIDLTYPE,LPCITEMIDLIST,LPVOID,UINT16);
LPPIDLDATA PidlMgr_GetDataPointer(LPPIDLMGR,LPCITEMIDLIST);
BOOL32 PidlMgr_SeparatePathAndValue(LPPIDLMGR,LPITEMIDLIST,LPITEMIDLIST*,LPITEMIDLIST*);

static struct PidlMgr_VTable pmgrvt = {
    PidlMgr_CreateDesktop,
    PidlMgr_CreateDrive,
    PidlMgr_CreateFolder,
    PidlMgr_CreateValue,
		PidlMgr_Delete,
    PidlMgr_GetNextItem,
    PidlMgr_Copy,
    PidlMgr_GetSize,
    PidlMgr_GetDesktop,
		PidlMgr_GetDrive,
    PidlMgr_GetLastItem,
    PidlMgr_GetItemText,
    PidlMgr_IsDesktop,
    PidlMgr_IsDrive,
    PidlMgr_IsFolder,
    PidlMgr_IsValue,
    PidlMgr_HasFolders,
    PidlMgr_GetFolderText,
    PidlMgr_GetValueText,
    PidlMgr_GetValueType,
    PidlMgr_GetDataText,
    PidlMgr_GetPidlPath,
		PidlMgr_Concatenate,
    PidlMgr_Create,
    PidlMgr_GetData,
    PidlMgr_GetDataPointer,
    PidlMgr_SeparatePathAndValue
};
/**************************************************************************
 *	  PidlMgr_Constructor
 */
LPPIDLMGR PidlMgr_Constructor() 
{	LPPIDLMGR pmgr;
	pmgr = (LPPIDLMGR)HeapAlloc(GetProcessHeap(),0,sizeof(pidlmgr));
	pmgr->lpvtbl = &pmgrvt;
	TRACE(shell,"(%p)->()\n",pmgr);
  /** FIXME DllRefCount++;*/
	return pmgr;
}
/**************************************************************************
 *	  PidlMgr_Destructor
 */
void PidlMgr_Destructor(LPPIDLMGR this) 
{	HeapFree(GetProcessHeap(),0,this);
 	TRACE(shell,"(%p)->()\n",this);
  /** FIXME DllRefCount--;*/
}

/**************************************************************************
 *  PidlMgr_CreateDesktop()
 *  PidlMgr_CreateDrive()
 *  PidlMgr_CreateFolder() 
 *  PidlMgr_CreateValue()
 */
LPITEMIDLIST PidlMgr_CreateDesktop(LPPIDLMGR this)
{ TRACE(shell,"(%p)->()\n",this);
    return PidlMgr_Create(this,PT_DESKTOP, (void *)"Desktop", sizeof("Desktop"));
}
LPITEMIDLIST PidlMgr_CreateDrive(LPPIDLMGR this, LPCSTR lpszNew)
{ TRACE(shell,"(%p)->(%s)\n",this,lpszNew);
  return PidlMgr_Create(this,PT_DRIVE, (LPVOID)lpszNew , strlen(lpszNew)+1);
}
LPITEMIDLIST PidlMgr_CreateFolder(LPPIDLMGR this, LPCSTR lpszNew)
{ TRACE(shell,"(%p)->(%s)\n",this,lpszNew);
  return PidlMgr_Create(this,PT_FOLDER, (LPVOID)lpszNew, strlen(lpszNew)+1);
}
LPITEMIDLIST PidlMgr_CreateValue(LPPIDLMGR this,LPCSTR lpszNew)
{ TRACE(shell,"(%p)->(%s)\n",this,lpszNew);
  return PidlMgr_Create(this,PT_VALUE, (LPVOID)lpszNew, strlen(lpszNew)+1);
}
/**************************************************************************
 *  PidlMgr_Delete()
 *  Deletes a PIDL
 */
void PidlMgr_Delete(LPPIDLMGR this,LPITEMIDLIST pidl)
{ TRACE(shell,"(%p)->(pidl=%p)\n",this,pidl);
  HeapFree(GetProcessHeap(),0,pidl);
}

/**************************************************************************
 *  PidlMgr_GetNextItem()
 */
LPITEMIDLIST PidlMgr_GetNextItem(LPPIDLMGR this, LPITEMIDLIST pidl)
{ TRACE(shell,"(%p)->(pidl=%p)\n",this,pidl);
  if(pidl)
  {  return (LPITEMIDLIST)(LPBYTE)(((LPBYTE)pidl) + pidl->mkid.cb);
	}
  else
  {  return (NULL);
	}
}

/**************************************************************************
 *  PidlMgr_Copy()
 */
LPITEMIDLIST PidlMgr_Copy(LPPIDLMGR this, LPITEMIDLIST pidlSource)
{ LPITEMIDLIST pidlTarget = NULL;
  UINT16 cbSource = 0;

	TRACE(shell,"-- (%p)->(pidl=%p)\n",this,pidlSource);

  if(NULL == pidlSource)
 	{  ERR(shell,"-- (%p)->(%p)\n",this,pidlSource);
     return (NULL);
	}

  cbSource = PidlMgr_GetSize(this, pidlSource);
  pidlTarget = (LPITEMIDLIST)HeapAlloc(GetProcessHeap(),0,cbSource);
  if(!pidlTarget)
  { return (NULL);
	}

  memcpy(pidlTarget, pidlSource, cbSource);

	TRACE(shell,"-- (%p)->(pidl=%p to new pidl=%p)\n",this,pidlSource,pidlTarget);
  return pidlTarget;
}
/**************************************************************************
 *  PidlMgr_GetSize()
 *  calculates the size of the complete pidl
 */
UINT16 PidlMgr_GetSize(LPPIDLMGR this, LPITEMIDLIST pidl)
{ UINT16 cbTotal = 0;
  LPITEMIDLIST pidlTemp = pidl;

  TRACE(shell,"(%p)->(pidl=%p)\n",this,pidl);
  if(pidlTemp)
  { while(pidlTemp->mkid.cb)
    { cbTotal += pidlTemp->mkid.cb;
      pidlTemp = PidlMgr_GetNextItem(this, pidlTemp);
    }  
    //add the size of the NULL terminating ITEMIDLIST
    cbTotal += sizeof(ITEMIDLIST);
  }
	TRACE(shell,"-- size %u\n",cbTotal);
  return (cbTotal);
}
/**************************************************************************
 *  PidlMgr_GetDesktop()
 * 
 *  FIXME: quick hack
 */
BOOL32 PidlMgr_GetDesktop(LPPIDLMGR this,LPCITEMIDLIST pidl,LPSTR pOut)
{ TRACE(shell,"(%p)->(%p %p)\n",this,pidl,pOut);
  return (BOOL32)PidlMgr_GetData(this,PT_DESKTOP, pidl, (LPVOID)pOut, 255);
}
/**************************************************************************
 *  PidlMgr_GetDrive()
 *
 *  FIXME: quick hack
 */
BOOL32 PidlMgr_GetDrive(LPPIDLMGR this,LPCITEMIDLIST pidl,LPSTR pOut, UINT16 uSize)
{ LPITEMIDLIST   pidlTemp=NULL;

  TRACE(shell,"(%p)->(%p,%p,%u)\n",this,pidl,pOut,uSize);
  if(PidlMgr_IsDesktop(this,pidl))
  { pidlTemp = PidlMgr_GetNextItem(this,pidl);
	}
	else if (PidlMgr_IsDrive(this,pidl))
  { pidlTemp = PidlMgr_GetNextItem(this,pidl);	  	
    return (BOOL32)PidlMgr_GetData(this,PT_DRIVE, pidlTemp, (LPVOID)pOut, uSize);
	}
	return FALSE;
}
/**************************************************************************
 *  PidlMgr_GetLastItem()
 *  Gets the last item in the list
 */
LPITEMIDLIST PidlMgr_GetLastItem(LPPIDLMGR this,LPCITEMIDLIST pidl)
{ LPITEMIDLIST   pidlLast = NULL;

  TRACE(shell,"(%p)->(pidl=%p)\n",this,pidl);

  if(pidl)
  { while(pidl->mkid.cb)
    { pidlLast = (LPITEMIDLIST)pidl;
      pidl = PidlMgr_GetNextItem(this,pidl);
    }  
  }
  return pidlLast;
}
/**************************************************************************
 *  PidlMgr_GetItemText()
 *  Gets the text for only this item
 */
DWORD PidlMgr_GetItemText(LPPIDLMGR this,LPCITEMIDLIST pidl, LPSTR lpszText, UINT16 uSize)
{ TRACE(shell,"(%p)->(pidl=%p %p %x)\n",this,pidl,lpszText,uSize);
  if (PidlMgr_IsDesktop(this, pidl))
  { return PidlMgr_GetData(this,PT_DESKTOP, pidl, (LPVOID)lpszText, uSize);
	}
	if (PidlMgr_IsDrive(this, pidl))
	{ return PidlMgr_GetData(this,PT_DRIVE, pidl, (LPVOID)lpszText, uSize);
	}
	return PidlMgr_GetData(this,PT_TEXT, pidl, (LPVOID)lpszText, uSize);	
}
/**************************************************************************
 *  PidlMgr_IsDesktop()
 *  PidlMgr_IsDrive()
 *  PidlMgr_IsFolder()
 *  PidlMgr_IsValue()
*/
BOOL32 PidlMgr_IsDesktop(LPPIDLMGR this,LPCITEMIDLIST pidl)
{ LPPIDLDATA  pData;
  TRACE(shell,"%p->(%p)\n",this,pidl);
  pData = PidlMgr_GetDataPointer(this,pidl);
  return (PT_DESKTOP == pData->type);
}

BOOL32 PidlMgr_IsDrive(LPPIDLMGR this,LPCITEMIDLIST pidl)
{ LPPIDLDATA  pData;
  TRACE(shell,"%p->(%p)\n",this,pidl);
  pData = PidlMgr_GetDataPointer(this,pidl);
  return (PT_DRIVE == pData->type);
}

BOOL32 PidlMgr_IsFolder(LPPIDLMGR this,LPCITEMIDLIST pidl)
{ LPPIDLDATA  pData;
  TRACE(shell,"%p->(%p)\n",this,pidl);
  pData = PidlMgr_GetDataPointer(this,pidl);
  return (PT_FOLDER == pData->type);
}

BOOL32 PidlMgr_IsValue(LPPIDLMGR this,LPCITEMIDLIST pidl)
{ LPPIDLDATA  pData;
  TRACE(shell,"%p->(%p)\n",this,pidl);
  pData = PidlMgr_GetDataPointer(this,pidl);
  return (PT_VALUE == pData->type);
}
/**************************************************************************
 * PidlMgr_HasFolders()
 * fixme: quick hack
 */
BOOL32 PidlMgr_HasFolders(LPPIDLMGR this, LPSTR pszPath, LPCITEMIDLIST pidl)
{ BOOL32 bResult= FALSE;
  WIN32_FIND_DATA32A stffile;	
  HANDLE32 hFile;

  TRACE (shell,"(%p)->%p %p\n",this, pszPath, pidl);
	
  hFile = FindFirstFile32A(pszPath,&stffile);
  do
  { if (! (stffile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
    {  bResult= TRUE;
		}
  } while( FindNextFile32A(hFile,&stffile));
  FindClose32 (hFile);
	
	return bResult;
}


/**************************************************************************
 *  PidlMgr_GetFolderText()
 *  Creates a Path string from a PIDL, filtering out the Desktop and the Drive
 *  value, if either is present.
 */
DWORD PidlMgr_GetFolderText(LPPIDLMGR this,LPCITEMIDLIST pidl,
   LPSTR lpszPath, DWORD dwSize)
{ LPITEMIDLIST   pidlTemp;
  DWORD          dwCopied = 0;
 
  TRACE(shell,"(%p)->(%p)\n",this,pidl);
 
  if(!pidl)
  { return 0;
	}

  if(PidlMgr_IsDesktop(this,pidl))
  { pidlTemp = PidlMgr_GetNextItem(this,pidl);
	}
	else if (PidlMgr_IsFolder(this,pidl))
  { pidlTemp = PidlMgr_GetNextItem(this,pidl);	  	
	}
  else
  { pidlTemp = (LPITEMIDLIST)pidl;
	}

  //if this is NULL, return the required size of the buffer
  if(!lpszPath)
  { while(pidlTemp->mkid.cb)
    { LPPIDLDATA  pData = PidlMgr_GetDataPointer(this,pidlTemp);
      
      //add the length of this item plus one for the backslash
      dwCopied += strlen(pData->szText) + 1;  /* fixme pData->szText is not every time a string*/

      pidlTemp = PidlMgr_GetNextItem(this,pidlTemp);
    }

    //add one for the NULL terminator
    return dwCopied + 1;
  }

  *lpszPath = 0;

  while(pidlTemp->mkid.cb && (dwCopied < dwSize))
  { LPPIDLDATA  pData = PidlMgr_GetDataPointer(this,pidlTemp);

    //if this item is a value, then skip it and finish
    if(PT_VALUE == pData->type)
    { break;
		}
   
    strcat(lpszPath, pData->szText);
    strcat(lpszPath, "\\");

    dwCopied += strlen(pData->szText) + 1;

    pidlTemp = PidlMgr_GetNextItem(this,pidlTemp);
  }

  //remove the last backslash if necessary
  if(dwCopied)
  { if(*(lpszPath + strlen(lpszPath) - 1) == '\\')
    { *(lpszPath + strlen(lpszPath) - 1) = 0;
      dwCopied--;
    }
  }
  return dwCopied;
}


/**************************************************************************
 *  PidlMgr_GetValueText()
 *  Gets the text for the last item in the list
 */
DWORD PidlMgr_GetValueText(LPPIDLMGR this,
    LPCITEMIDLIST pidl, LPSTR lpszValue, DWORD dwSize)
{ LPITEMIDLIST  pidlTemp=pidl;
  CHAR          szText[MAX_PATH];

  TRACE(shell,"(%p)->(pidl=%p %p %lx)\n",this,pidl,lpszValue,dwSize);

  if(!pidl)
  { return 0;
	}
		
  while(pidlTemp->mkid.cb && !PidlMgr_IsValue(this,pidlTemp))
  { pidlTemp = PidlMgr_GetNextItem(this,pidlTemp);
  }

  if(!pidlTemp->mkid.cb)
  { return 0;
	}

  PidlMgr_GetItemText(this, pidlTemp, szText, sizeof(szText));

  if(!lpszValue)
  { return strlen(szText) + 1;
  }
  strcpy(lpszValue, szText);
	TRACE(shell,"-- (%p)->(pidl=%p %p=%s %lx)\n",this,pidl,lpszValue,lpszValue,dwSize);
  return strlen(lpszValue);
}
/**************************************************************************
 *  PidlMgr_GetValueType()
 */
BOOL32 PidlMgr_GetValueType( LPPIDLMGR this,
 LPCITEMIDLIST pidlPath, LPCITEMIDLIST pidlValue, LPDWORD pdwType)
{ LPSTR    lpszFolder,
           lpszValueName;
  DWORD    dwNameSize;
 
  FIXME(shell,"(%p)->(%p %p %p) stub\n",this,pidlPath,pidlValue,pdwType);
	
  if(!pidlPath)
  { return FALSE;
	}

  if(!pidlValue)
  { return FALSE;
	}

  if(!pdwType)
  { return FALSE;
	}

  //get the Desktop
  //PidlMgr_GetDesktop(this,pidlPath);

  /* fixme: add the driveletter here*/
	
  //assemble the Folder string
  dwNameSize = PidlMgr_GetFolderText(this,pidlPath, NULL, 0);
  lpszFolder = (LPSTR)HeapAlloc(GetProcessHeap(),0,dwNameSize);
  if(!lpszFolder)
  {  return FALSE;
	}
  PidlMgr_GetFolderText(this,pidlPath, lpszFolder, dwNameSize);

  //assemble the value name
  dwNameSize = PidlMgr_GetValueText(this,pidlValue, NULL, 0);
  lpszValueName = (LPSTR)HeapAlloc(GetProcessHeap(),0,dwNameSize);
  if(!lpszValueName)
  { HeapFree(GetProcessHeap(),0,lpszFolder);
    return FALSE;
  }
  PidlMgr_GetValueText(this,pidlValue, lpszValueName, dwNameSize);

  /* fixme: we've got the path now do something with it
	  -like get the filetype*/
	
	pdwType=NULL;
	
  HeapFree(GetProcessHeap(),0,lpszFolder);
  HeapFree(GetProcessHeap(),0,lpszValueName);
  return TRUE;
}
/**************************************************************************
 *  PidlMgr_GetDataText()
 */
DWORD PidlMgr_GetDataText( LPPIDLMGR this,
 LPCITEMIDLIST pidlPath, LPCITEMIDLIST pidlValue, LPSTR lpszOut, DWORD dwOutSize)
{ LPSTR    lpszFolder,
           lpszValueName;
  DWORD    dwNameSize;

  FIXME(shell,"(%p)->(pidl=%p pidl=%p) stub\n",this,pidlPath,pidlValue);

  if(!lpszOut)
  { return FALSE;
	}

  if(!pidlPath)
  { return FALSE;
	}

  if(!pidlValue)
  { return FALSE;
	}

  /* fixme: get the driveletter*/

  //assemble the Folder string
  dwNameSize = PidlMgr_GetFolderText(this,pidlPath, NULL, 0);
  lpszFolder = (LPSTR)HeapAlloc(GetProcessHeap(),0,dwNameSize);
  if(!lpszFolder)
  {  return FALSE;
	}
  PidlMgr_GetFolderText(this,pidlPath, lpszFolder, dwNameSize);

  //assemble the value name
  dwNameSize = PidlMgr_GetValueText(this,pidlValue, NULL, 0);
  lpszValueName = (LPSTR)HeapAlloc(GetProcessHeap(),0,dwNameSize);
  if(!lpszValueName)
  { HeapFree(GetProcessHeap(),0,lpszFolder);
    return FALSE;
  }
  PidlMgr_GetValueText(this,pidlValue, lpszValueName, dwNameSize);

  /* fixme: we've got the path now do something with it*/
	
  HeapFree(GetProcessHeap(),0,lpszFolder);
  HeapFree(GetProcessHeap(),0,lpszValueName);

  TRACE(shell,"-- (%p)->(%p=%s %lx)\n",this,lpszOut,lpszOut,dwOutSize);

	return TRUE;
}

/**************************************************************************
 *   CPidlMgr::GetPidlPath()
 *  Create a string that includes the Drive name, the folder text and 
 *  the value text.
 */
DWORD PidlMgr_GetPidlPath(LPPIDLMGR this,
    LPCITEMIDLIST pidl, LPSTR lpszOut, DWORD dwOutSize)
{ LPSTR lpszTemp;
  WORD	len;

  TRACE(shell,"(%p)->(%p,%lu)\n",this,lpszOut,dwOutSize);

  if(!lpszOut)
  { return 0;
	}

  *lpszOut = 0;
  lpszTemp = lpszOut;

  dwOutSize -= PidlMgr_GetFolderText(this,pidl, lpszTemp, dwOutSize);

  //add a backslash if necessary
  len = strlen(lpszTemp);
  if (len && lpszTemp[len-1]!='\\')
	{ lpszTemp[len+0]='\\';
    lpszTemp[len+1]='\0';
		dwOutSize--;
  }

  lpszTemp = lpszOut + strlen(lpszOut);

  //add the value string
  PidlMgr_GetValueText(this,pidl, lpszTemp, dwOutSize);

  //remove the last backslash if necessary
  if(*(lpszOut + strlen(lpszOut) - 1) == '\\')
  { *(lpszOut + strlen(lpszOut) - 1) = 0;
  }

  TRACE(shell,"-- (%p)->(%p=%s,%lu)\n",this,lpszOut,lpszOut,dwOutSize);

  return strlen(lpszOut);

}
/**************************************************************************
 * PidlMgr_Concatenate()
 * Create a new PIDL by combining two existing PIDLs.
 */  
LPITEMIDLIST PidlMgr_Concatenate(LPPIDLMGR this,LPITEMIDLIST pidl1, LPITEMIDLIST pidl2)
{ LPITEMIDLIST   pidlNew;
  UINT32         cb1 = 0, cb2 = 0;

  TRACE(shell,"(%p)->(%p,%p)\n",this,pidl1,pidl2);

  if(!pidl1 && !pidl2)
  {  return NULL;
	}

  if(!pidl1)
  { pidlNew = PidlMgr_Copy(this,pidl2);
    return pidlNew;
  }

  if(!pidl2)
  { pidlNew = PidlMgr_Copy(this,pidl1);
    return pidlNew;
  }

  cb1 = PidlMgr_GetSize(this,pidl1) - sizeof(ITEMIDLIST);
  cb2 = PidlMgr_GetSize(this,pidl2);

  pidlNew = (LPITEMIDLIST)HeapAlloc(GetProcessHeap(),0,cb1+cb2);

  if(pidlNew)
  { memcpy(pidlNew, pidl1, cb1);
    memcpy(((LPBYTE)pidlNew) + cb1, pidl2, cb2);
  }
  TRACE(shell,"-- (%p)-> (new pidl=%p)\n",this,pidlNew);
	return pidlNew;
}
/**************************************************************************
 *  PidlMgr_Create()
 *  Creates a new PIDL
 *  type = PT_DESKTOP | PT_DRIVE | PT_FOLDER | PT_VALUE
 *  pIn = data
 *  uInSize = size of data
 */

LPITEMIDLIST PidlMgr_Create(LPPIDLMGR this,PIDLTYPE type, LPVOID pIn, UINT16 uInSize)
{ LPITEMIDLIST   pidlOut=NULL;
  UINT16         uSize;
  LPITEMIDLIST   pidlTemp=NULL;
  LPPIDLDATA     pData;

  TRACE(shell,"(%p)->(%x %p %x)\n",this,type,pIn,uInSize);

  if (! pIn)
	{ return NULL;
	}	
  uSize = sizeof(ITEMIDLIST) + (sizeof(PIDLTYPE)) + uInSize;

  /* Allocate the memory, adding an additional ITEMIDLIST for the NULL terminating 
  ID List. */
  pidlOut = (LPITEMIDLIST)HeapAlloc(GetProcessHeap(),0,uSize + sizeof(ITEMIDLIST));
  pidlTemp = pidlOut;
  if(pidlOut)
  { pidlTemp->mkid.cb = uSize;
    pData = PidlMgr_GetDataPointer(this,pidlTemp);
    pData->type = type;
    switch(type)
    { case PT_DESKTOP: memcpy(pData->szText, pIn, uInSize);
                       TRACE(shell,"-- (%p)->create Desktop: %s\n",this,debugstr_a(pData->szText));
                       break;
			case PT_DRIVE:	 memcpy(pData->szText, pIn, uInSize);
                       TRACE(shell,"-- (%p)->create Drive: %s\n",this,debugstr_a(pData->szText));
											 break;
      case PT_FOLDER:
      case PT_VALUE:   memcpy(pData->szText, pIn, uInSize);
                       TRACE(shell,"-- (%p)->create Value: %s\n",this,debugstr_a(pData->szText));
											 break;
		  default:         FIXME(shell,"-- (%p) wrong argument\n",this);
			                 break;
    }
   
    pidlTemp = PidlMgr_GetNextItem(this,pidlTemp);
    pidlTemp->mkid.cb = 0;
    pidlTemp->mkid.abID[0] = 0;
  }
	TRACE(shell,"-- (%p)->(pidl=%p)\n",this,pidlOut);
  return pidlOut;
}
/**************************************************************************
 *  PidlMgr_GetData(PIDLTYPE, LPCITEMIDLIST, LPVOID, UINT16)
 */
DWORD PidlMgr_GetData(
    LPPIDLMGR this,
		PIDLTYPE type, 
    LPCITEMIDLIST pidl,
		LPVOID pOut,
		UINT16 uOutSize)
{ LPPIDLDATA  pData;
  DWORD       dwReturn=0; 

	TRACE(shell,"(%p)->(%x %p %p %x)\n",this,type,pidl,pOut,uOutSize);
	
  if(!pidl)
  {  return 0;
	}

  pData = PidlMgr_GetDataPointer(this,pidl);

  //copy the data
  switch(type)
  { case PT_DESKTOP: if(uOutSize < 1)
                       return 0;
                     if(PT_DESKTOP != pData->type)
                       return 0;
										 *(LPSTR)pOut = 0;	 
                     strncpy((LPSTR)pOut, "DESKTOP", uOutSize);
										 dwReturn = strlen((LPSTR)pOut);
                     break;

	 case PT_DRIVE:    if(uOutSize < 1)
                       return 0;
                     if(PT_DESKTOP != pData->type)
                       return 0;
										 *(LPSTR)pOut = 0;	 
                     strncpy((LPSTR)pOut, "DRIVE", uOutSize);
										 dwReturn = strlen((LPSTR)pOut);
                     break;

   case PT_FOLDER:
   case PT_VALUE: 
	 case PT_TEXT:     *(LPSTR)pOut = 0;
                     strncpy((LPSTR)pOut, pData->szText, uOutSize);
                     dwReturn = strlen((LPSTR)pOut);
                     break;
  }
	TRACE(shell,"-- (%p)->(%p:%s %lx)\n",this,pOut,(char*)pOut,dwReturn);
  return dwReturn;
}


/**************************************************************************
 *  PidlMgr_GetDataPointer()
 */
LPPIDLDATA PidlMgr_GetDataPointer(LPPIDLMGR this,LPITEMIDLIST pidl)
{ if(!pidl)
  {  return NULL;
	}
	TRACE (shell,"(%p)->(%p)\n"	,this, pidl);
  return (LPPIDLDATA)(pidl->mkid.abID);
}

/**************************************************************************
 *  CPidlMgr_SeparatePathAndValue)
 *  Creates a separate path and value PIDL from a fully qualified PIDL.
 */
BOOL32 PidlMgr_SeparatePathAndValue(LPPIDLMGR this, 
   LPITEMIDLIST pidlFQ, LPITEMIDLIST *ppidlPath, LPITEMIDLIST *ppidlValue)
{ LPITEMIDLIST   pidlTemp;
	TRACE (shell,"(%p)->(pidl=%p pidl=%p pidl=%p)",this,pidlFQ,ppidlPath,ppidlValue);
  if(!pidlFQ)
  {  return FALSE;
	}

  *ppidlValue = PidlMgr_GetLastItem(this,pidlFQ);

  if(!PidlMgr_IsValue(this,*ppidlValue))
  { return FALSE;
	}

  *ppidlValue = PidlMgr_Copy(this,*ppidlValue);
  *ppidlPath = PidlMgr_Copy(this,pidlFQ);

  pidlTemp = PidlMgr_GetLastItem(this,*ppidlPath);
  pidlTemp->mkid.cb = 0;
  pidlTemp->mkid.abID[0] = 0;

  return TRUE;
}
