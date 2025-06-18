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
BOOL AddToEnumList(IEnumIDListImpl *list, LPITEMIDLIST pidl)
{
    struct pidl_enum_entry *pidl_entry;

    TRACE("(%p)->(pidl=%p)\n", list, pidl);

    if (!list || !pidl)
        return FALSE;

    if (!(pidl_entry = SHAlloc(sizeof(*pidl_entry))))
        return FALSE;

    pidl_entry->pidl = pidl;
    list_add_tail(&list->pidls, &pidl_entry->entry);
    if (!list->current)
        list->current = list_head(&list->pidls);

    return TRUE;
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

    TRACE("(%p)->(path=%s flags=0x%08lx)\n", list, debugstr_w(lpszPath), dwFlags);

    if(!lpszPath || !lpszPath[0]) return FALSE;

    lstrcpyW(szPath, lpszPath);
    PathAddBackslashW(szPath);
    lstrcatW(szPath,L"*");

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
                 wcscmp(stffile.cFileName, L".") && wcscmp(stffile.cFileName, L".."))
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

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppvObj);

    *ppvObj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IEnumIDList))
    {
        *ppvObj = &This->IEnumIDList_iface;
    }

    if (*ppvObj)
    {
        IUnknown_AddRef((IUnknown*)*ppvObj);
        return S_OK;
    }

    WARN("interface %s is not supported\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

/******************************************************************************
 * IEnumIDList::AddRef
 */
static ULONG WINAPI IEnumIDList_fnAddRef(IEnumIDList *iface)
{
    IEnumIDListImpl *This = impl_from_IEnumIDList(iface);
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(%lu)\n", This, refCount - 1);

    return refCount;
}

/******************************************************************************
 * IEnumIDList::Release
 */
static ULONG WINAPI IEnumIDList_fnRelease(IEnumIDList *iface)
{
    IEnumIDListImpl *This = impl_from_IEnumIDList(iface);
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%lu)\n", This, refCount + 1);

    if (!refCount)
    {
        struct pidl_enum_entry *cur, *cur2;

        LIST_FOR_EACH_ENTRY_SAFE(cur, cur2, &This->pidls, struct pidl_enum_entry, entry)
        {
            list_remove(&cur->entry);
            SHFree(cur->pidl);
            SHFree(cur);
        }
        free(This);
    }

    return refCount;
}

/**************************************************************************
 *  IEnumIDList::Next
 */

static HRESULT WINAPI IEnumIDList_fnNext(IEnumIDList *iface, ULONG celt, LPITEMIDLIST *rgelt,
        ULONG *fetched)
{
    IEnumIDListImpl *This = impl_from_IEnumIDList(iface);
    HRESULT hr = S_OK;
    ULONG i;

    TRACE("(%p)->(%ld, %p, %p)\n", This, celt, rgelt, fetched);

    /* It is valid to leave pceltFetched NULL when celt is 1. Some of explorer's
     * subsystems actually use it (and so may a third party browser)
     */
    if (fetched)
        *fetched = 0;

    *rgelt = NULL;

    if (celt > 1 && !fetched)
        return E_INVALIDARG;

    if (celt > 0 && !This->current)
        return S_FALSE;

    for (i = 0; i < celt; i++)
    {
        if (!This->current)
            break;

        rgelt[i] = ILClone(LIST_ENTRY(This->current, struct pidl_enum_entry, entry)->pidl);
        This->current = list_next(&This->pidls, This->current);
    }

    if (fetched)
        *fetched = i;

    return hr;
}

/**************************************************************************
*  IEnumIDList::Skip
*/
static HRESULT WINAPI IEnumIDList_fnSkip(IEnumIDList *iface, ULONG celt)
{
    IEnumIDListImpl *This = impl_from_IEnumIDList(iface);
    HRESULT hr = S_OK;
    ULONG i;

    TRACE("(%p)->(%lu)\n", This, celt);

    for (i = 0; i < celt; i++)
    {
        if (!This->current)
        {
            hr = S_FALSE;
            break;
        }
        This->current = list_next(&This->pidls, This->current);
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
    This->current = list_head(&This->pidls);
    return S_OK;
}

/**************************************************************************
*  IEnumIDList::Clone
*/
static HRESULT WINAPI IEnumIDList_fnClone(IEnumIDList *iface, IEnumIDList **ppenum)
{
    IEnumIDListImpl *This = impl_from_IEnumIDList(iface);

    FIXME("(%p)->(%p): stub\n",This, ppenum);

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
    IEnumIDListImpl *lpeidl = malloc(sizeof(*lpeidl));

    if (lpeidl)
    {
        lpeidl->IEnumIDList_iface.lpVtbl = &eidlvt;
        lpeidl->ref = 1;
        list_init(&lpeidl->pidls);
        lpeidl->current = NULL;
    }

    TRACE("-- (%p)->()\n",lpeidl);

    return lpeidl;
}
