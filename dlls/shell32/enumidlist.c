/*
 *	IEnumIDList
 *
 *	Copyright 1998	Juergen Schmied <juergen.schmied@metronet.de>
 */

#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "wine/obj_base.h"
#include "winerror.h"

#include "pidl.h"
#include "shlguid.h"
#include "shell32_main.h"

/* IEnumIDList Implementation */
static HRESULT WINAPI IEnumIDList_QueryInterface(LPENUMIDLIST,REFIID,LPVOID*);
static ULONG WINAPI IEnumIDList_AddRef(LPENUMIDLIST);
static ULONG WINAPI IEnumIDList_Release(LPENUMIDLIST);
static HRESULT WINAPI IEnumIDList_Next(LPENUMIDLIST,ULONG,LPITEMIDLIST*,ULONG*);
static HRESULT WINAPI IEnumIDList_Skip(LPENUMIDLIST,ULONG);
static HRESULT WINAPI IEnumIDList_Reset(LPENUMIDLIST);
static HRESULT WINAPI IEnumIDList_Clone(LPENUMIDLIST,LPENUMIDLIST*);
static BOOL WINAPI IEnumIDList_CreateEnumList(LPENUMIDLIST,LPCSTR, DWORD);
static BOOL WINAPI IEnumIDList_AddToEnumList(LPENUMIDLIST,LPITEMIDLIST);
static BOOL WINAPI IEnumIDList_DeleteList(LPENUMIDLIST);

/**************************************************************************
 *  IEnumIDList_VTable
 */
static IEnumIDList_VTable eidlvt = 
{ IEnumIDList_QueryInterface,
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

LPENUMIDLIST IEnumIDList_Constructor( LPCSTR lpszPath, DWORD dwFlags)
{	LPENUMIDLIST	lpeidl;

	lpeidl = (LPENUMIDLIST)HeapAlloc(GetProcessHeap(),0,sizeof(IEnumIDList));
	if (! lpeidl)
	  return NULL;

	lpeidl->ref = 1;
	lpeidl->lpvtbl = &eidlvt;
	lpeidl->mpFirst=NULL;
	lpeidl->mpLast=NULL;
	lpeidl->mpCurrent=NULL;

	TRACE(shell,"(%p)->(%s flags=0x%08lx)\n",lpeidl,debugstr_a(lpszPath),dwFlags);

	if(!IEnumIDList_CreateEnumList(lpeidl, lpszPath, dwFlags))
	{ if (lpeidl)
	  { HeapFree(GetProcessHeap(),0,lpeidl);
	  }
	  return NULL;	  
	}

	TRACE(shell,"-- (%p)->()\n",lpeidl);
	shell32_ObjCount++;
	return lpeidl;
}

/**************************************************************************
 *  EnumIDList_QueryInterface
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
{	TRACE(shell,"(%p)->(%lu)\n",this,this->ref);

	shell32_ObjCount++;
	return ++(this->ref);
}
/******************************************************************************
 * IEnumIDList_Release
 */
static ULONG WINAPI IEnumIDList_Release(LPENUMIDLIST this)
{	TRACE(shell,"(%p)->(%lu)\n",this,this->ref);

	shell32_ObjCount--;

	if (!--(this->ref)) 
	{ TRACE(shell," destroying IEnumIDList(%p)\n",this);
	  IEnumIDList_DeleteList(this);
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
{	ULONG    i;
	HRESULT  hr = S_OK;
	LPITEMIDLIST  temp;

	TRACE(shell,"(%p)->(%ld,%p, %p)\n",this,celt,rgelt,pceltFetched);

/* It is valid to leave pceltFetched NULL when celt is 1. Some of explorer's
 * subsystems actually use it (and so may a third party browser)
 */
	if(pceltFetched)
	  *pceltFetched = 0;

	*rgelt=0;
	
	if(celt > 1 && !pceltFetched)
	{ return E_INVALIDARG;
	}

	for(i = 0; i < celt; i++)
	{ if(!(this->mpCurrent))
	  { hr =  S_FALSE;
	    break;
	  }
	  temp = ILClone(this->mpCurrent->pidl);
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
static BOOL WINAPI IEnumIDList_CreateEnumList(LPENUMIDLIST this, LPCSTR lpszPath, DWORD dwFlags)
{	LPITEMIDLIST	pidl=NULL;
	LPPIDLDATA 	pData=NULL;
	WIN32_FIND_DATAA stffile;	
	HANDLE hFile;
	DWORD dwDrivemap;
	CHAR  szDriveName[4];
	CHAR  szPath[MAX_PATH];
    
	TRACE(shell,"(%p)->(path=%s flags=0x%08lx) \n",this,debugstr_a(lpszPath),dwFlags);

	if (lpszPath && lpszPath[0]!='\0')
	{ strcpy(szPath, lpszPath);
	  PathAddBackslashA(szPath);
	  strcat(szPath,"*.*");
	}

	/*enumerate the folders*/
	if(dwFlags & SHCONTF_FOLDERS)
	{ /* special case - we can't enumerate the Desktop level Objects (MyComputer,Nethood...
	  so we need to fake an enumeration of those.*/
	  if(!lpszPath)
	  { TRACE (shell,"-- (%p)-> enumerate SHCONTF_FOLDERS (special) items\n",this);
  		/*create the pidl for this item */
	    pidl = _ILCreateMyComputer();
	    if(pidl)
	    { if(!IEnumIDList_AddToEnumList(this, pidl))
	        return FALSE;
	    }
	  }   
	  else if (lpszPath[0]=='\0') /* enumerate the drives*/
	  { TRACE (shell,"-- (%p)-> enumerate SHCONTF_FOLDERS (drives)\n",this);
	    dwDrivemap = GetLogicalDrives();
	    strcpy (szDriveName,"A:\\");
	    while (szDriveName[0]<='Z')
	    { if(dwDrivemap & 0x00000001L)
	      { pidl = _ILCreateDrive(szDriveName);
	        if(pidl)
	        { if(!IEnumIDList_AddToEnumList(this, pidl))
	          return FALSE;
	        }
	      }
	      szDriveName[0]++;
	      dwDrivemap = dwDrivemap >> 1;
	    }   
	  }
    	  else
	  { TRACE (shell,"-- (%p)-> enumerate SHCONTF_FOLDERS of %s\n",this,debugstr_a(szPath));
	    hFile = FindFirstFileA(szPath,&stffile);
	    if ( hFile != INVALID_HANDLE_VALUE )
	    { do
	      { if ( (stffile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && strcmp (stffile.cFileName, ".") && strcmp (stffile.cFileName, ".."))
	        { pidl = _ILCreateFolder( stffile.cAlternateFileName, stffile.cFileName);
	          if(pidl)
	          { pData = _ILGetDataPointer(pidl);
		    FileTimeToDosDateTime(&stffile.ftLastWriteTime,&pData->u.folder.uFileDate,&pData->u.folder.uFileTime);
		    pData->u.folder.dwFileSize = stffile.nFileSizeLow;
		    pData->u.folder.uFileAttribs=stffile.dwFileAttributes;
	            if(!IEnumIDList_AddToEnumList(this, pidl))
	            {  return FALSE;
	            }
	          }
	          else
	          { return FALSE;
	          }   
	        }
	      } while( FindNextFileA(hFile,&stffile));
			FindClose (hFile);
	    }
	  }   
	}   
	/*enumerate the non-folder items (values) */
	if(dwFlags & SHCONTF_NONFOLDERS)
	{ if(lpszPath)
	  { TRACE (shell,"-- (%p)-> enumerate SHCONTF_NONFOLDERS of %s\n",this,debugstr_a(szPath));
	    hFile = FindFirstFileA(szPath,&stffile);
	    if ( hFile != INVALID_HANDLE_VALUE )
	    { do
	      { if (! (stffile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
	        { pidl = _ILCreateValue( stffile.cAlternateFileName, stffile.cFileName);
	          if(pidl)
	          { pData = _ILGetDataPointer(pidl);
		    FileTimeToDosDateTime(&stffile.ftLastWriteTime,&pData->u.file.uFileDate,&pData->u.file.uFileTime);
		    pData->u.file.dwFileSize = stffile.nFileSizeLow;
		    pData->u.file.uFileAttribs=stffile.dwFileAttributes;
	            if(!IEnumIDList_AddToEnumList(this, pidl))
	            { return FALSE;
	            }
	          }
	          else
	          { return FALSE;
	          }   
	        }
	      } while( FindNextFileA(hFile,&stffile));
	      FindClose (hFile);
	    } 
	  }
	} 
	return TRUE;
}

/**************************************************************************
 *  EnumIDList_AddToEnumList()
 */
static BOOL WINAPI IEnumIDList_AddToEnumList(LPENUMIDLIST this,LPITEMIDLIST pidl)
{ LPENUMLIST  pNew;

  TRACE(shell,"(%p)->(pidl=%p)\n",this,pidl);
  pNew = (LPENUMLIST)SHAlloc(sizeof(ENUMLIST));
  if(pNew)
  { /*set the next pointer */
    pNew->pNext = NULL;
    pNew->pidl = pidl;

    /*is this the first item in the list? */
    if(!this->mpFirst)
    { this->mpFirst = pNew;
      this->mpCurrent = pNew;
    }
   
    if(this->mpLast)
    { /*add the new item to the end of the list */
      this->mpLast->pNext = pNew;
    }
   
    /*update the last item pointer */
    this->mpLast = pNew;
    TRACE(shell,"-- (%p)->(first=%p, last=%p)\n",this,this->mpFirst,this->mpLast);
    return TRUE;
  }
  return FALSE;
}
/**************************************************************************
*   EnumIDList_DeleteList()
*/
static BOOL WINAPI IEnumIDList_DeleteList(LPENUMIDLIST this)
{ LPENUMLIST  pDelete;

  TRACE(shell,"(%p)->()\n",this);
	
  while(this->mpFirst)
  { pDelete = this->mpFirst;
    this->mpFirst = pDelete->pNext;
    SHFree(pDelete->pidl);
    SHFree(pDelete);
  }
  this->mpFirst = this->mpLast = this->mpCurrent = NULL;
  return TRUE;
}
