/*
 *	IEnumIDList
 *
 *	Copyright 1998	Juergen Schmied <juergen.schmied@metronet.de>
 */

#include <stdlib.h>
#include <string.h>
#include "debugtools.h"
#include "winreg.h"
#include "wine/obj_base.h"
#include "wine/obj_enumidlist.h"
#include "wine/undocshell.h"
#include "winerror.h"

#include "pidl.h"
#include "shlguid.h"
#include "shell32_main.h"

DEFAULT_DEBUG_CHANNEL(shell)

typedef struct tagENUMLIST
{
	struct tagENUMLIST	*pNext;
	LPITEMIDLIST		pidl;

} ENUMLIST, *LPENUMLIST;

typedef struct
{
	ICOM_VFIELD(IEnumIDList);
	DWORD				ref;
	LPENUMLIST			mpFirst;
	LPENUMLIST			mpLast;
	LPENUMLIST			mpCurrent;

} IEnumIDListImpl;

static struct ICOM_VTABLE(IEnumIDList) eidlvt;

/**************************************************************************
 *  AddToEnumList()
 */
static BOOL AddToEnumList(
	IEnumIDList * iface,
	LPITEMIDLIST pidl)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	LPENUMLIST  pNew;

	TRACE("(%p)->(pidl=%p)\n",This,pidl);
	pNew = (LPENUMLIST)SHAlloc(sizeof(ENUMLIST));
	if(pNew)
	{
	  /*set the next pointer */
	  pNew->pNext = NULL;
	  pNew->pidl = pidl;

	  /*is This the first item in the list? */
	  if(!This->mpFirst)
	  {
	    This->mpFirst = pNew;
	    This->mpCurrent = pNew;
	  }

	  if(This->mpLast)
	  {
	    /*add the new item to the end of the list */
	    This->mpLast->pNext = pNew;
	  }
	 
	  /*update the last item pointer */
	  This->mpLast = pNew;
	  TRACE("-- (%p)->(first=%p, last=%p)\n",This,This->mpFirst,This->mpLast);
	  return TRUE;
	}
	return FALSE;
}

/**************************************************************************
 *  CreateFolderEnumList()
 */
static BOOL CreateFolderEnumList(
	IEnumIDList * iface,
	LPCSTR lpszPath,
	DWORD dwFlags)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	LPITEMIDLIST	pidl=NULL;
	WIN32_FIND_DATAA stffile;	
	HANDLE hFile;
	CHAR  szPath[MAX_PATH];

	TRACE("(%p)->(path=%s flags=0x%08lx) \n",This,debugstr_a(lpszPath),dwFlags);

	if(!lpszPath || !lpszPath[0]) return FALSE;

	strcpy(szPath, lpszPath);
	PathAddBackslashA(szPath);
	strcat(szPath,"*.*");

	/*enumerate the folders*/
	if(dwFlags & SHCONTF_FOLDERS)
	{ 
	  TRACE("-- (%p)-> enumerate SHCONTF_FOLDERS of %s\n",This,debugstr_a(szPath));
	  hFile = FindFirstFileA(szPath,&stffile);
	  if ( hFile != INVALID_HANDLE_VALUE )
	  {
	    do
	    {
	      if ( (stffile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && strcmp (stffile.cFileName, ".") && strcmp (stffile.cFileName, ".."))
	      {
		pidl = _ILCreateFolder (&stffile);
		if(pidl && AddToEnumList((IEnumIDList*)This, pidl))
		{
		  continue;
		}
		return FALSE;
	      }
	    } while( FindNextFileA(hFile,&stffile));
	    FindClose (hFile);
	  }
	}

	/*enumerate the non-folder items (values) */
	if(dwFlags & SHCONTF_NONFOLDERS)
	{
	  TRACE("-- (%p)-> enumerate SHCONTF_NONFOLDERS of %s\n",This,debugstr_a(szPath));
	  hFile = FindFirstFileA(szPath,&stffile);
	  if ( hFile != INVALID_HANDLE_VALUE )
	  {
	    do
	    {
	      if (! (stffile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
	      {
		pidl = _ILCreateValue(&stffile);
		if(pidl && AddToEnumList((IEnumIDList*)This, pidl))
		{
		  continue;
		}
		return FALSE;
	      }
	    } while( FindNextFileA(hFile,&stffile));
	    FindClose (hFile);
	  } 
	}
	return TRUE;
}

/**************************************************************************
 *  CreateDesktopEnumList()
 */
static BOOL CreateDesktopEnumList(
	IEnumIDList * iface,
	DWORD dwFlags)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	LPITEMIDLIST	pidl=NULL;
	HKEY hkey;
	char	szPath[MAX_PATH];

	TRACE("(%p)->(flags=0x%08lx) \n",This,dwFlags);

	/*enumerate the root folders */
	if(dwFlags & SHCONTF_FOLDERS)
	{ 
	  /*create the pidl for This item */
	  pidl = _ILCreateMyComputer();
	  if(pidl)
	  {
	    if(!AddToEnumList((IEnumIDList*)This, pidl))
	      return FALSE;
	  }

	  if (! RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\explorer\\desktop\\NameSpace", 0, KEY_READ, &hkey))
	  {
	    char iid[50];
	    int i=0;

	    while (1)
	    {
	      DWORD size = sizeof (iid);

	      if (ERROR_SUCCESS!=RegEnumKeyExA(hkey, i, iid, &size, 0, NULL, NULL, NULL))
	        break;

	      pidl = _ILCreateSpecial(iid);

	      if(pidl)
	        AddToEnumList((IEnumIDList*)This, pidl);

	      i++;
	    }
	    RegCloseKey(hkey);
	  }
	}

	/*enumerate the elements in %windir%\desktop */
	SHGetSpecialFolderPathA(0, szPath, CSIDL_DESKTOPDIRECTORY, FALSE);
	CreateFolderEnumList( (IEnumIDList*)This, szPath, dwFlags);
	
	return TRUE;
}

/**************************************************************************
 *  CreateMyCompEnumList()
 */
static BOOL CreateMyCompEnumList(
	IEnumIDList * iface,
	DWORD dwFlags)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	LPITEMIDLIST	pidl=NULL;
	DWORD		dwDrivemap;
	CHAR		szDriveName[4];
	HKEY		hkey;

	TRACE("(%p)->(flags=0x%08lx) \n",This,dwFlags);

	/*enumerate the folders*/
	if(dwFlags & SHCONTF_FOLDERS)
	{
	  dwDrivemap = GetLogicalDrives();
	  strcpy (szDriveName,"A:\\");
	  while (szDriveName[0]<='Z')
	  {
	    if(dwDrivemap & 0x00000001L)
	    {
	      pidl = _ILCreateDrive(szDriveName);
	      if(pidl)
	      {
		if(!AddToEnumList((IEnumIDList*)This, pidl))
	          return FALSE;
	      }
	    }
	    szDriveName[0]++;
	    dwDrivemap = dwDrivemap >> 1;
	  }

	  TRACE("-- (%p)-> enumerate (mycomputer shell extensions)\n",This);
	  if (! RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\explorer\\mycomputer\\NameSpace", 0, KEY_READ, &hkey))
	  {
	    char iid[50];
	    int i=0;

	    while (1)
	    {
	      DWORD size = sizeof (iid);

	      if (ERROR_SUCCESS!=RegEnumKeyExA(hkey, i, iid, &size, 0, NULL, NULL, NULL))
	        break;

	      pidl = _ILCreateSpecial(iid);

	      if(pidl)
	        AddToEnumList((IEnumIDList*)This, pidl);

	      i++;
	    }
	    RegCloseKey(hkey);
	  }
	}
	return TRUE;
}

/**************************************************************************
*   DeleteList()
*/
static BOOL DeleteList(
	IEnumIDList * iface)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	LPENUMLIST  pDelete;

	TRACE("(%p)->()\n",This);
	
	while(This->mpFirst)
	{ pDelete = This->mpFirst;
	  This->mpFirst = pDelete->pNext;
	  SHFree(pDelete->pidl);
	  SHFree(pDelete);
	}
	This->mpFirst = This->mpLast = This->mpCurrent = NULL;
	return TRUE;
}

/**************************************************************************
 *  IEnumIDList_Folder_Constructor
 *
 */

IEnumIDList * IEnumIDList_Constructor(
	LPCSTR lpszPath,
	DWORD dwFlags,
	DWORD dwKind)
{
	IEnumIDListImpl*	lpeidl;
	BOOL			ret = FALSE;	

	lpeidl = (IEnumIDListImpl*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IEnumIDListImpl));

	TRACE("(%p)->(%s flags=0x%08lx kind=0x%08lx)\n",lpeidl,debugstr_a(lpszPath),dwFlags, dwKind);

	if (lpeidl)
	{
	  lpeidl->ref = 1;
	  ICOM_VTBL(lpeidl) = &eidlvt;

	  switch (dwKind)
	  {
	    case EIDL_DESK:
	      ret = CreateDesktopEnumList((IEnumIDList*)lpeidl, dwFlags);
	      break;

	    case EIDL_MYCOMP:
	      ret = CreateMyCompEnumList((IEnumIDList*)lpeidl, dwFlags);
	      break;

	    case EIDL_FILE:
	      ret = CreateFolderEnumList((IEnumIDList*)lpeidl, lpszPath, dwFlags);
	      break;
	  }

	  if(ret)
	  {
	    shell32_ObjCount++;
	  }
	  else
	  {
	    if (lpeidl)
	    {
	      HeapFree(GetProcessHeap(),0,lpeidl);
	    }
	  }
	}

	TRACE("-- (%p)->()\n",lpeidl);

	return (IEnumIDList*)lpeidl;
}

/**************************************************************************
 *  EnumIDList_QueryInterface
 */
static HRESULT WINAPI IEnumIDList_fnQueryInterface(
	IEnumIDList * iface,
	REFIID riid,
	LPVOID *ppvObj)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	char	xriid[50];
	WINE_StringFromCLSID((LPCLSID)riid,xriid);
	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,xriid,ppvObj);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
	{ *ppvObj = This; 
	}
	else if(IsEqualIID(riid, &IID_IEnumIDList))  /*IEnumIDList*/
	{    *ppvObj = (IEnumIDList*)This;
	}

	if(*ppvObj)
	{ IEnumIDList_AddRef((IEnumIDList*)*ppvObj);
	  TRACE("-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	
	TRACE("-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}

/******************************************************************************
 * IEnumIDList_fnAddRef
 */
static ULONG WINAPI IEnumIDList_fnAddRef(
	IEnumIDList * iface)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	TRACE("(%p)->(%lu)\n",This,This->ref);

	shell32_ObjCount++;
	return ++(This->ref);
}
/******************************************************************************
 * IEnumIDList_fnRelease
 */
static ULONG WINAPI IEnumIDList_fnRelease(
	IEnumIDList * iface)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	TRACE("(%p)->(%lu)\n",This,This->ref);

	shell32_ObjCount--;

	if (!--(This->ref)) 
	{ TRACE(" destroying IEnumIDList(%p)\n",This);
	  DeleteList((IEnumIDList*)This);
	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}
	return This->ref;
}
   
/**************************************************************************
 *  IEnumIDList_fnNext
 */

static HRESULT WINAPI IEnumIDList_fnNext(
	IEnumIDList * iface,
	ULONG celt,
	LPITEMIDLIST * rgelt,
	ULONG *pceltFetched) 
{
	ICOM_THIS(IEnumIDListImpl,iface);

	ULONG    i;
	HRESULT  hr = S_OK;
	LPITEMIDLIST  temp;

	TRACE("(%p)->(%ld,%p, %p)\n",This,celt,rgelt,pceltFetched);

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
	{ if(!(This->mpCurrent))
	  { hr =  S_FALSE;
	    break;
	  }
	  temp = ILClone(This->mpCurrent->pidl);
	  rgelt[i] = temp;
	  This->mpCurrent = This->mpCurrent->pNext;
	}
	if(pceltFetched)
	{  *pceltFetched = i;
	}

	return hr;
}

/**************************************************************************
*  IEnumIDList_fnSkip
*/
static HRESULT WINAPI IEnumIDList_fnSkip(
	IEnumIDList * iface,ULONG celt)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	DWORD    dwIndex;
	HRESULT  hr = S_OK;

	TRACE("(%p)->(%lu)\n",This,celt);

	for(dwIndex = 0; dwIndex < celt; dwIndex++)
	{ if(!This->mpCurrent)
	  { hr = S_FALSE;
	    break;
	  }
	  This->mpCurrent = This->mpCurrent->pNext;
	}
	return hr;
}
/**************************************************************************
*  IEnumIDList_fnReset
*/
static HRESULT WINAPI IEnumIDList_fnReset(
	IEnumIDList * iface)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	TRACE("(%p)\n",This);
	This->mpCurrent = This->mpFirst;
	return S_OK;
}
/**************************************************************************
*  IEnumIDList_fnClone
*/
static HRESULT WINAPI IEnumIDList_fnClone(
	IEnumIDList * iface,LPENUMIDLIST * ppenum)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	TRACE("(%p)->() to (%p)->() E_NOTIMPL\n",This,ppenum);
	return E_NOTIMPL;
}

/**************************************************************************
 *  IEnumIDList_fnVTable
 */
static ICOM_VTABLE (IEnumIDList) eidlvt = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IEnumIDList_fnQueryInterface,
	IEnumIDList_fnAddRef,
	IEnumIDList_fnRelease,
	IEnumIDList_fnNext,
	IEnumIDList_fnSkip,
	IEnumIDList_fnReset,
	IEnumIDList_fnClone,
};
