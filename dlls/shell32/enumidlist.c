/*
 *	IEnumIDList
 *
 *	Copyright 1998	Juergen Schmied <juergen.schmied@metronet.de>
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

#include "shell32_main.h"

/* IEnumIDList Implementation */
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

LPENUMIDLIST IEnumIDList_Constructor( LPCSTR lpszPath, DWORD dwFlags, HRESULT* pResult)
{	LPENUMIDLIST	lpeidl;

	lpeidl = (LPENUMIDLIST)HeapAlloc(GetProcessHeap(),0,sizeof(IEnumIDList));
	lpeidl->ref = 1;
	lpeidl->lpvtbl = &eidlvt;
	lpeidl->mpFirst=NULL;
	lpeidl->mpLast=NULL;
	lpeidl->mpCurrent=NULL;

  TRACE(shell,"(%p)->(%s 0x%08lx %p)\n",lpeidl,debugstr_a(lpszPath),dwFlags,pResult);

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

  /* It is valid to leave pceltFetched NULL when celt is 1. Some of explorer's
     subsystems actually use it (and so may a third party browser)
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
static BOOL32 WINAPI IEnumIDList_CreateEnumList(LPENUMIDLIST this, LPCSTR lpszPath, DWORD dwFlags)
{ LPITEMIDLIST   pidl=NULL;
  WIN32_FIND_DATA32A stffile;	
  HANDLE32 hFile;
  DWORD dwDrivemap;
  CHAR  szDriveName[4];
  CHAR  szPath[MAX_PATH];
    
  TRACE(shell,"(%p)->(%s 0x%08lx) \n",this,debugstr_a(lpszPath),dwFlags);

  if (lpszPath && lpszPath[0]!='\0')
  { strcpy(szPath, lpszPath);
    PathAddBackslash(szPath);
    strcat(szPath,"*.*");
  }

  /*enumerate the folders*/
  if(dwFlags & SHCONTF_FOLDERS)
  {	/* special case - we can't enumerate the Desktop level Objects (MyComputer,Nethood...
    so we need to fake an enumeration of those.*/
	  if(!lpszPath)
    { TRACE (shell,"-- (%p)-> enumerate SHCONTF_FOLDERS (special) items\n",this);
  		//create the pidl for this item
      pidl = this->mpPidlMgr->lpvtbl->fnCreateMyComputer(this->mpPidlMgr);
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
        { pidl = this->mpPidlMgr->lpvtbl->fnCreateDrive(this->mpPidlMgr,szDriveName );
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
      hFile = FindFirstFile32A(szPath,&stffile);
      if ( hFile != INVALID_HANDLE_VALUE32 )
      { do
        { if ( (stffile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && strcmp (stffile.cFileName, ".") && strcmp (stffile.cFileName, ".."))
          { pidl = this->mpPidlMgr->lpvtbl->fnCreateFolder(this->mpPidlMgr, stffile.cFileName);
         if(pidl)
         { if(!IEnumIDList_AddToEnumList(this, pidl))
               {  return FALSE;
               }
         }
         else
         { return FALSE;
         }   
           }
      } while( FindNextFile32A(hFile,&stffile));
			FindClose32 (hFile);
    }
  }   
  }   
  //enumerate the non-folder items (values)
  if(dwFlags & SHCONTF_NONFOLDERS)
  { if(lpszPath)
    { TRACE (shell,"-- (%p)-> enumerate SHCONTF_NONFOLDERS of %s\n",this,debugstr_a(szPath));
      hFile = FindFirstFile32A(szPath,&stffile);
      if ( hFile != INVALID_HANDLE_VALUE32 )
      { do
    { if (! (stffile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
          { pidl = this->mpPidlMgr->lpvtbl->fnCreateValue(this->mpPidlMgr, stffile.cFileName);
      if(pidl)
      { if(!IEnumIDList_AddToEnumList(this, pidl))
        { return FALSE;
			  }
      }
      else
      { return FALSE;
      }   
          }
    } while( FindNextFile32A(hFile,&stffile));
	  FindClose32 (hFile);
  } 
    }
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
    TRACE(shell,"-- (%p)->(first=%p, last=%p)\n",this,this->mpFirst,this->mpLast);
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
    SHFree(pDelete->pidl);
    SHFree(pDelete);
  }
  this->mpFirst = this->mpLast = this->mpCurrent = NULL;
  return TRUE;
}
