/*
 *	IEnumIDList
 *
 *	Copyright 1998	Juergen Schmied <juergen.schmied@metronet.de>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define COBJMACROS

#include "wine/debug.h"
#include "wine/unicode.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "shlwapi.h"

#include "pidl.h"
#include "shell32_main.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/**************************************************************************
 *  AddToEnumList()
 */
BOOL AddToEnumList(IEnumIDListImpl *This, LPITEMIDLIST pidl)
{
        struct enumlist *pNew;

	TRACE("(%p)->(pidl=%p)\n",This,pidl);

        if (!This || !pidl)
          return FALSE;

        pNew = SHAlloc(sizeof(*pNew));
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
BOOL CreateFolderEnumList(IEnumIDListImpl *list, LPCWSTR lpszPath, DWORD dwFlags)
{
    LPITEMIDLIST pidl=NULL;
    WIN32_FIND_DATAW stffile;
    HANDLE hFile;
    WCHAR  szPath[MAX_PATH];
    BOOL succeeded = TRUE;
    static const WCHAR stars[] = { '*','.','*',0 };
    static const WCHAR dot[] = { '.',0 };
    static const WCHAR dotdot[] = { '.','.',0 };

    TRACE("(%p)->(path=%s flags=0x%08x)\n", list, debugstr_w(lpszPath), dwFlags);

    if(!lpszPath || !lpszPath[0]) return FALSE;

    strcpyW(szPath, lpszPath);
    PathAddBackslashW(szPath);
    strcatW(szPath,stars);

    hFile = FindFirstFileW(szPath,&stffile);
    if ( hFile != INVALID_HANDLE_VALUE )
    {
        BOOL findFinished = FALSE;

        do
        {
            if ( !(stffile.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) 
             || (dwFlags & SHCONTF_INCLUDEHIDDEN) )
            {
                if ( (stffile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                 dwFlags & SHCONTF_FOLDERS &&
                 strcmpW(stffile.cFileName, dot) && strcmpW(stffile.cFileName, dotdot))
                {
                    pidl = _ILCreateFromFindDataW(&stffile);
                    succeeded = succeeded && AddToEnumList(list, pidl);
                }
                else if (!(stffile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                 && dwFlags & SHCONTF_NONFOLDERS)
                {
                    pidl = _ILCreateFromFindDataW(&stffile);
                    succeeded = succeeded && AddToEnumList(list, pidl);
                }
            }
            if (succeeded)
            {
                if (!FindNextFileW(hFile, &stffile))
                {
                    if (GetLastError() == ERROR_NO_MORE_FILES)
                        findFinished = TRUE;
                    else
                        succeeded = FALSE;
                }
            }
        } while (succeeded && !findFinished);
        FindClose(hFile);
    }
    return succeeded;
}

static BOOL DeleteList(IEnumIDListImpl *This)
{
        struct enumlist *pDelete;

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

static inline IEnumIDListImpl *impl_from_IEnumIDList(IEnumIDList *iface)
{
    return CONTAINING_RECORD(iface, IEnumIDListImpl, IEnumIDList_iface);
}

/**************************************************************************
 *  IEnumIDList::QueryInterface
 */
static HRESULT WINAPI IEnumIDList_fnQueryInterface(IEnumIDList *iface, REFIID riid, void **ppvObj)
{
        IEnumIDListImpl *This = impl_from_IEnumIDList(iface);

	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,debugstr_guid(riid),ppvObj);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown) ||
           IsEqualIID(riid, &IID_IEnumIDList))
	{
            *ppvObj = &This->IEnumIDList_iface;
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
 * IEnumIDList::AddRef
 */
static ULONG WINAPI IEnumIDList_fnAddRef(IEnumIDList *iface)
{
        IEnumIDListImpl *This = impl_from_IEnumIDList(iface);
	ULONG refCount = InterlockedIncrement(&This->ref);

	TRACE("(%p)->(%u)\n", This, refCount - 1);

	return refCount;
}
/******************************************************************************
 * IEnumIDList::Release
 */
static ULONG WINAPI IEnumIDList_fnRelease(IEnumIDList *iface)
{
        IEnumIDListImpl *This = impl_from_IEnumIDList(iface);
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(%u)\n", This, refCount + 1);

	if (!refCount) {
	  TRACE(" destroying IEnumIDList(%p)\n",This);
          DeleteList(This);
	  HeapFree(GetProcessHeap(),0,This);
	}
	return refCount;
}

/**************************************************************************
 *  IEnumIDList::Next
 */

static HRESULT WINAPI IEnumIDList_fnNext(IEnumIDList *iface, ULONG celt, LPITEMIDLIST *rgelt,
        ULONG *pceltFetched)
{
        IEnumIDListImpl *This = impl_from_IEnumIDList(iface);

	ULONG    i;
	HRESULT  hr = S_OK;
	LPITEMIDLIST  temp;

	TRACE("(%p)->(%d,%p, %p)\n",This,celt,rgelt,pceltFetched);

/* It is valid to leave pceltFetched NULL when celt is 1. Some of explorer's
 * subsystems actually use it (and so may a third party browser)
 */
	if(pceltFetched)
	  *pceltFetched = 0;

	*rgelt=0;

	if(celt > 1 && !pceltFetched)
	{ return E_INVALIDARG;
	}

	if(celt > 0 && !This->mpCurrent)
	{ return S_FALSE;
	}

	for(i = 0; i < celt; i++)
	{ if(!(This->mpCurrent))
	    break;

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
*  IEnumIDList::Skip
*/
static HRESULT WINAPI IEnumIDList_fnSkip(IEnumIDList *iface, ULONG celt)
{
        IEnumIDListImpl *This = impl_from_IEnumIDList(iface);

	DWORD    dwIndex;
	HRESULT  hr = S_OK;

	TRACE("(%p)->(%u)\n",This,celt);

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
*  IEnumIDList::Reset
*/
static HRESULT WINAPI IEnumIDList_fnReset(IEnumIDList *iface)
{
        IEnumIDListImpl *This = impl_from_IEnumIDList(iface);

	TRACE("(%p)\n",This);
	This->mpCurrent = This->mpFirst;
	return S_OK;
}
/**************************************************************************
*  IEnumIDList::Clone
*/
static HRESULT WINAPI IEnumIDList_fnClone(IEnumIDList *iface, IEnumIDList **ppenum)
{
        IEnumIDListImpl *This = impl_from_IEnumIDList(iface);

	TRACE("(%p)->() to (%p)->() E_NOTIMPL\n",This,ppenum);
	return E_NOTIMPL;
}

static const IEnumIDListVtbl eidlvt =
{
	IEnumIDList_fnQueryInterface,
	IEnumIDList_fnAddRef,
	IEnumIDList_fnRelease,
	IEnumIDList_fnNext,
	IEnumIDList_fnSkip,
	IEnumIDList_fnReset,
	IEnumIDList_fnClone,
};

IEnumIDListImpl *IEnumIDList_Constructor(void)
{
    IEnumIDListImpl *lpeidl = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*lpeidl));

    if (lpeidl)
    {
        lpeidl->ref = 1;
        lpeidl->IEnumIDList_iface.lpVtbl = &eidlvt;
    }

    TRACE("-- (%p)->()\n",lpeidl);

    return lpeidl;
}
