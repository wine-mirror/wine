/*
 *	Network Places (Neighbourhood) folder
 *
 *	Copyright 1997			Marcus Meissner
 *	Copyright 1998, 1999, 2002	Juergen Schmied
 *	Copyright 2003                  Mike McCormack for CodeWeavers
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

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"

#include "pidl.h"
#include "shell32_main.h"
#include "shresdef.h"
#include "wine/debug.h"
#include "debughlp.h"
#include "shfldr.h"

WINE_DEFAULT_DEBUG_CHANNEL (shell);

/***********************************************************************
*   IShellFolder implementation
*/

typedef struct {
    IShellFolder2   IShellFolder2_iface;
    IPersistFolder2 IPersistFolder2_iface;
    LONG            ref;

    /* both paths are parsible from the desktop */
    LPITEMIDLIST pidlRoot;	/* absolute pidl */
} IGenericSFImpl;

static const IShellFolder2Vtbl vt_ShellFolder2;
static const IPersistFolder2Vtbl vt_NP_PersistFolder2;

static inline IGenericSFImpl *impl_from_IShellFolder2(IShellFolder2 *iface)
{
    return CONTAINING_RECORD(iface, IGenericSFImpl, IShellFolder2_iface);
}

static inline IGenericSFImpl *impl_from_IPersistFolder2(IPersistFolder2 *iface)
{
    return CONTAINING_RECORD(iface, IGenericSFImpl, IPersistFolder2_iface);
}


static const shvheader networkplaces_header[] =
{
    { NULL, 0, IDS_SHV_COLUMN1, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 15 },
    { NULL, 0, IDS_SHV_COLUMN9, SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_RIGHT, 10 },
};

/**************************************************************************
*	ISF_NetworkPlaces_Constructor
*/
HRESULT WINAPI ISF_NetworkPlaces_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv)
{
    IGenericSFImpl *sf;

    TRACE ("unkOut=%p %s\n", pUnkOuter, shdebugstr_guid (riid));

    if (!ppv)
        return E_POINTER;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    sf = calloc (1, sizeof (*sf));
    if (!sf)
        return E_OUTOFMEMORY;

    sf->ref = 0;
    sf->IShellFolder2_iface.lpVtbl = &vt_ShellFolder2;
    sf->IPersistFolder2_iface.lpVtbl = &vt_NP_PersistFolder2;
    sf->pidlRoot = _ILCreateNetHood();	/* my qualified pidl */

    if (FAILED (IShellFolder2_QueryInterface (&sf->IShellFolder2_iface, riid, ppv)))
    {
        IShellFolder2_Release (&sf->IShellFolder2_iface);
        return E_NOINTERFACE;
    }

    TRACE ("--(%p)\n", sf);
    return S_OK;
}

/**************************************************************************
 *	ISF_NetworkPlaces_fnQueryInterface
 *
 * NOTE
 *     supports not IPersist/IPersistFolder
 */
static HRESULT WINAPI ISF_NetworkPlaces_fnQueryInterface (IShellFolder2 *iface, REFIID riid, LPVOID *ppvObj)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);

    TRACE ("(%p)->(%s,%p)\n", This, shdebugstr_guid (riid), ppvObj);

    *ppvObj = NULL;

    if (IsEqualIID (riid, &IID_IUnknown) ||
        IsEqualIID (riid, &IID_IShellFolder) ||
        IsEqualIID (riid, &IID_IShellFolder2))
    {
        *ppvObj = &This->IShellFolder2_iface;
    }
    else if (IsEqualIID (riid, &IID_IPersist) ||
             IsEqualIID (riid, &IID_IPersistFolder) ||
             IsEqualIID (riid, &IID_IPersistFolder2))
    {
        *ppvObj = &This->IPersistFolder2_iface;
    }

    if (*ppvObj)
    {
        IUnknown_AddRef ((IUnknown *) (*ppvObj));
        TRACE ("-- Interface: (%p)->(%p)\n", ppvObj, *ppvObj);
        return S_OK;
    }
    TRACE ("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ISF_NetworkPlaces_fnAddRef (IShellFolder2 * iface)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE ("(%p)->(count=%lu)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI ISF_NetworkPlaces_fnRelease (IShellFolder2 * iface)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE ("(%p)->(count=%lu)\n", This, refCount + 1);

    if (!refCount) {
        TRACE ("-- destroying IShellFolder(%p)\n", This);
        SHFree (This->pidlRoot);
        free (This);
    }
    return refCount;
}

/**************************************************************************
*	ISF_NetworkPlaces_fnParseDisplayName
*/
static HRESULT WINAPI ISF_NetworkPlaces_fnParseDisplayName (IShellFolder2 * iface,
               HWND hwndOwner, LPBC pbcReserved, LPOLESTR lpszDisplayName,
               DWORD * pchEaten, LPITEMIDLIST * ppidl, DWORD * pdwAttributes)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);
    HRESULT hr = E_INVALIDARG;
    LPCWSTR szNext = NULL;
    WCHAR szElement[MAX_PATH];
    LPITEMIDLIST pidlTemp = NULL;

    TRACE ("(%p)->(HWND=%p,%p,%p=%s,%p,pidl=%p,%p)\n", This,
            hwndOwner, pbcReserved, lpszDisplayName, debugstr_w (lpszDisplayName),
            pchEaten, ppidl, pdwAttributes);

    *ppidl = NULL;

    szNext = GetNextElementW (lpszDisplayName, szElement, MAX_PATH);
    if (!wcsicmp(szElement, L"EntireNetwork"))
    {
        pidlTemp = _ILCreateEntireNetwork();
        if (pidlTemp)
            hr = S_OK;
        else
            hr = E_OUTOFMEMORY;
    }
    else
        FIXME("not implemented for %s\n", debugstr_w(lpszDisplayName));

    if (SUCCEEDED(hr) && pidlTemp)
    {
        if (szNext && *szNext)
        {
            hr = SHELL32_ParseNextElement(iface, hwndOwner, pbcReserved,
                    &pidlTemp, (LPOLESTR) szNext, pchEaten, pdwAttributes);
        }
        else
        {
            if (pdwAttributes && *pdwAttributes)
                hr = SHELL32_GetItemAttributes(&This->IShellFolder2_iface, pidlTemp, pdwAttributes);
        }
    }

    if (SUCCEEDED(hr))
        *ppidl = pidlTemp;
    else
        ILFree(pidlTemp);

    TRACE ("(%p)->(-- ret=0x%08lx)\n", This, hr);

    return hr;
}

/**************************************************************************
*		ISF_NetworkPlaces_fnEnumObjects
*/
static HRESULT WINAPI ISF_NetworkPlaces_fnEnumObjects (IShellFolder2 * iface,
               HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST * ppEnumIDList)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);
    IEnumIDListImpl *list;

    TRACE ("(%p)->(HWND=%p flags=0x%08lx pplist=%p)\n", This,
            hwndOwner, dwFlags, ppEnumIDList);

    if (!(list = IEnumIDList_Constructor()))
        return E_OUTOFMEMORY;
    *ppEnumIDList = &list->IEnumIDList_iface;

    TRACE ("-- (%p)->(new ID List: %p)\n", This, *ppEnumIDList);

    return S_OK;
}

/**************************************************************************
*		ISF_NetworkPlaces_fnBindToObject
*/
static HRESULT WINAPI ISF_NetworkPlaces_fnBindToObject (IShellFolder2 * iface,
               LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);

    TRACE ("(%p)->(pidl=%p,%p,%s,%p)\n", This,
            pidl, pbcReserved, shdebugstr_guid (riid), ppvOut);

    return SHELL32_BindToChild (This->pidlRoot, &CLSID_ShellFSFolder, NULL, pidl, riid, ppvOut);
}

/**************************************************************************
*	ISF_NetworkPlaces_fnBindToStorage
*/
static HRESULT WINAPI ISF_NetworkPlaces_fnBindToStorage (IShellFolder2 * iface,
               LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);

    FIXME ("(%p)->(pidl=%p,%p,%s,%p) stub\n", This,
            pidl, pbcReserved, shdebugstr_guid (riid), ppvOut);

    *ppvOut = NULL;
    return E_NOTIMPL;
}

/**************************************************************************
* 	ISF_NetworkPlaces_fnCompareIDs
*/

static HRESULT WINAPI ISF_NetworkPlaces_fnCompareIDs (IShellFolder2 * iface,
               LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);
    int nReturn;

    TRACE ("(%p)->(0x%08Ix,pidl1=%p,pidl2=%p)\n", This, lParam, pidl1, pidl2);
    nReturn = SHELL32_CompareIDs(&This->IShellFolder2_iface, lParam, pidl1, pidl2);
    TRACE ("-- %i\n", nReturn);
    return nReturn;
}

/**************************************************************************
*	ISF_NetworkPlaces_fnCreateViewObject
*/
static HRESULT WINAPI ISF_NetworkPlaces_fnCreateViewObject (IShellFolder2 * iface,
               HWND hwndOwner, REFIID riid, LPVOID * ppvOut)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);
    LPSHELLVIEW pShellView;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(hwnd=%p,%s,%p)\n", This,
           hwndOwner, shdebugstr_guid (riid), ppvOut);

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if (IsEqualIID (riid, &IID_IDropTarget))
    {
        WARN ("IDropTarget not implemented\n");
        hr = E_NOTIMPL;
    }
    else if (IsEqualIID (riid, &IID_IShellView))
    {
        pShellView = IShellView_Constructor ((IShellFolder *) iface);
        if (pShellView)
        {
            hr = IShellView_QueryInterface (pShellView, riid, ppvOut);
            IShellView_Release (pShellView);
        }
    }
    else
    {
        FIXME ("invalid/unsupported interface %s\n", shdebugstr_guid (riid));
        hr = E_NOINTERFACE;
    }
    TRACE ("-- (%p)->(interface=%p)\n", This, ppvOut);
    return hr;
}

/**************************************************************************
*  ISF_NetworkPlaces_fnGetAttributesOf
*/
static HRESULT WINAPI ISF_NetworkPlaces_fnGetAttributesOf (IShellFolder2 * iface,
               UINT cidl, LPCITEMIDLIST * apidl, DWORD * rgfInOut)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);
    HRESULT hr = S_OK;

    TRACE ("(%p)->(cidl=%d apidl=%p mask=%p (0x%08lx))\n", This,
            cidl, apidl, rgfInOut, rgfInOut ? *rgfInOut : 0);

    if (!rgfInOut)
        return E_INVALIDARG;
    if (cidl && !apidl)
        return E_INVALIDARG;

    if (*rgfInOut == 0)
        *rgfInOut = ~0;

    if (cidl == 0)
    {
        IShellFolder2 *parent = NULL;
        LPCITEMIDLIST rpidl = NULL;

        hr = SHBindToParent(This->pidlRoot, &IID_IShellFolder2, (void **)&parent, &rpidl);
        if(SUCCEEDED(hr))
        {
            SHELL32_GetItemAttributes(parent, rpidl, rgfInOut);
            IShellFolder2_Release(parent);
        }
    }
    else
    {
        while (cidl > 0 && *apidl)
        {
            pdump (*apidl);
            SHELL32_GetItemAttributes(&This->IShellFolder2_iface, *apidl, rgfInOut);
            apidl++;
            cidl--;
        }
    }

    /* make sure SFGAO_VALIDATE is cleared, some apps depend on that */
    *rgfInOut &= ~SFGAO_VALIDATE;

    TRACE ("-- result=0x%08lx\n", *rgfInOut);
    return hr;
}

/**************************************************************************
*	ISF_NetworkPlaces_fnGetUIObjectOf
*
* PARAMETERS
*  hwndOwner [in]  Parent window for any output
*  cidl      [in]  array size
*  apidl     [in]  simple pidl array
*  riid      [in]  Requested Interface
*  prgfInOut [   ] reserved
*  ppvObject [out] Resulting Interface
*
*/
static HRESULT WINAPI ISF_NetworkPlaces_fnGetUIObjectOf (IShellFolder2 * iface,
               HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl, REFIID riid,
               UINT * prgfInOut, LPVOID * ppvOut)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);

    LPITEMIDLIST pidl;
    IUnknown *pObj = NULL;
    HRESULT hr = E_INVALIDARG;

    TRACE ("(%p)->(%p,%u,apidl=%p,%s,%p,%p)\n", This,
            hwndOwner, cidl, apidl, shdebugstr_guid (riid), prgfInOut, ppvOut);

    if (!ppvOut)
        return hr;

    *ppvOut = NULL;

    if (IsEqualIID (riid, &IID_IContextMenu) && (cidl >= 1))
    {
        return ItemMenu_Constructor((IShellFolder*)iface, This->pidlRoot, apidl, cidl, riid, ppvOut);
    }
    else if (IsEqualIID (riid, &IID_IDataObject) && (cidl >= 1))
    {
        pObj = (LPUNKNOWN) IDataObject_Constructor (hwndOwner, This->pidlRoot, apidl, cidl);
        hr = S_OK;
    }
    else if (IsEqualIID (riid, &IID_IExtractIconA) && (cidl == 1))
    {
        pidl = ILCombine (This->pidlRoot, apidl[0]);
        pObj = (LPUNKNOWN) IExtractIconA_Constructor (pidl);
        ILFree(pidl);
        hr = S_OK;
    }
    else if (IsEqualIID (riid, &IID_IExtractIconW) && (cidl == 1))
    {
        pidl = ILCombine (This->pidlRoot, apidl[0]);
        pObj = (LPUNKNOWN) IExtractIconW_Constructor (pidl);
        ILFree(pidl);
        hr = S_OK;
    }
    else if (IsEqualIID (riid, &IID_IDropTarget) && (cidl >= 1))
    {
        hr = IShellFolder2_QueryInterface (iface, &IID_IDropTarget, (LPVOID *) & pObj);
    }
    else
        hr = E_NOINTERFACE;

    if (SUCCEEDED(hr) && !pObj)
        hr = E_OUTOFMEMORY;

    *ppvOut = pObj;
    TRACE ("(%p)->hr=0x%08lx\n", This, hr);
    return hr;
}

/**************************************************************************
*	ISF_NetworkPlaces_fnGetDisplayNameOf
*
*/
static HRESULT WINAPI ISF_NetworkPlaces_fnGetDisplayNameOf (IShellFolder2 * iface,
               LPCITEMIDLIST pidl, DWORD dwFlags, LPSTRRET strRet)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);

    FIXME ("(%p)->(pidl=%p,0x%08lx,%p)\n", This, pidl, dwFlags, strRet);
    pdump (pidl);

    if (!strRet)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

/**************************************************************************
*  ISF_NetworkPlaces_fnSetNameOf
*  Changes the name of a file object or subfolder, possibly changing its item
*  identifier in the process.
*
* PARAMETERS
*  hwndOwner [in]  Owner window for output
*  pidl      [in]  simple pidl of item to change
*  lpszName  [in]  the items new display name
*  dwFlags   [in]  SHGNO formatting flags
*  ppidlOut  [out] simple pidl returned
*/
static HRESULT WINAPI ISF_NetworkPlaces_fnSetNameOf (IShellFolder2 * iface,
               HWND hwndOwner, LPCITEMIDLIST pidl,	/*simple pidl */
               LPCOLESTR lpName, DWORD dwFlags, LPITEMIDLIST * pPidlOut)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);

    FIXME ("(%p)->(%p,pidl=%p,%s,%lu,%p)\n", This,
            hwndOwner, pidl, debugstr_w (lpName), dwFlags, pPidlOut);
    return E_FAIL;
}

static HRESULT WINAPI ISF_NetworkPlaces_fnGetDefaultSearchGUID(IShellFolder2 *iface, GUID *guid)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);
    TRACE("(%p)->(%p)\n", This, guid);
    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_NetworkPlaces_fnEnumSearches (IShellFolder2 * iface,
               IEnumExtraSearch ** ppenum)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);

    FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_NetworkPlaces_fnGetDefaultColumn(IShellFolder2 *iface, DWORD reserved,
        ULONG *sort, ULONG *display)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);

    TRACE("(%p)->(%#lx, %p, %p)\n", This, reserved, sort, display);

    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_NetworkPlaces_fnGetDefaultColumnState (
               IShellFolder2 * iface, UINT iColumn, DWORD * pcsFlags)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);

    TRACE ("(%p)->(%d %p)\n", This, iColumn, pcsFlags);

    if (!pcsFlags || iColumn >= ARRAY_SIZE(networkplaces_header))
        return E_INVALIDARG;

    *pcsFlags = networkplaces_header[iColumn].pcsFlags;

    return S_OK;
}

static HRESULT WINAPI ISF_NetworkPlaces_fnGetDetailsEx (IShellFolder2 * iface,
               LPCITEMIDLIST pidl, const SHCOLUMNID * pscid, VARIANT * pv)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);

    FIXME ("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_NetworkPlaces_fnGetDetailsOf (IShellFolder2 * iface,
               LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS * psd)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);

    FIXME ("(%p)->(%p %i %p)\n", This, pidl, iColumn, psd);

    return E_NOTIMPL;
}

static HRESULT WINAPI ISF_NetworkPlaces_fnMapColumnToSCID (IShellFolder2 *iface, UINT column, SHCOLUMNID *scid)
{
    IGenericSFImpl *This = impl_from_IShellFolder2(iface);

    TRACE("(%p)->(%u %p)\n", This, column, scid);

    if (column >= ARRAY_SIZE(networkplaces_header))
        return E_INVALIDARG;

    return shellfolder_map_column_to_scid(networkplaces_header, column, scid);
}

static const IShellFolder2Vtbl vt_ShellFolder2 = {
    ISF_NetworkPlaces_fnQueryInterface,
    ISF_NetworkPlaces_fnAddRef,
    ISF_NetworkPlaces_fnRelease,
    ISF_NetworkPlaces_fnParseDisplayName,
    ISF_NetworkPlaces_fnEnumObjects,
    ISF_NetworkPlaces_fnBindToObject,
    ISF_NetworkPlaces_fnBindToStorage,
    ISF_NetworkPlaces_fnCompareIDs,
    ISF_NetworkPlaces_fnCreateViewObject,
    ISF_NetworkPlaces_fnGetAttributesOf,
    ISF_NetworkPlaces_fnGetUIObjectOf,
    ISF_NetworkPlaces_fnGetDisplayNameOf,
    ISF_NetworkPlaces_fnSetNameOf,
    /* ShellFolder2 */
    ISF_NetworkPlaces_fnGetDefaultSearchGUID,
    ISF_NetworkPlaces_fnEnumSearches,
    ISF_NetworkPlaces_fnGetDefaultColumn,
    ISF_NetworkPlaces_fnGetDefaultColumnState,
    ISF_NetworkPlaces_fnGetDetailsEx,
    ISF_NetworkPlaces_fnGetDetailsOf,
    ISF_NetworkPlaces_fnMapColumnToSCID
};

/************************************************************************
 *	INPFldr_PersistFolder2_QueryInterface
 */
static HRESULT WINAPI INPFldr_PersistFolder2_QueryInterface (IPersistFolder2 * iface,
               REFIID iid, LPVOID * ppvObj)
{
    IGenericSFImpl *This = impl_from_IPersistFolder2(iface);

    TRACE ("(%p)\n", This);

    return IShellFolder2_QueryInterface (&This->IShellFolder2_iface, iid, ppvObj);
}

/************************************************************************
 *	INPFldr_PersistFolder2_AddRef
 */
static ULONG WINAPI INPFldr_PersistFolder2_AddRef (IPersistFolder2 * iface)
{
    IGenericSFImpl *This = impl_from_IPersistFolder2(iface);

    TRACE ("(%p)->(count=%lu)\n", This, This->ref);

    return IShellFolder2_AddRef (&This->IShellFolder2_iface);
}

/************************************************************************
 *	ISFPersistFolder_Release
 */
static ULONG WINAPI INPFldr_PersistFolder2_Release (IPersistFolder2 * iface)
{
    IGenericSFImpl *This = impl_from_IPersistFolder2(iface);

    TRACE ("(%p)->(count=%lu)\n", This, This->ref);

    return IShellFolder2_Release (&This->IShellFolder2_iface);
}

/************************************************************************
 *	INPFldr_PersistFolder2_GetClassID
 */
static HRESULT WINAPI INPFldr_PersistFolder2_GetClassID (
               IPersistFolder2 * iface, CLSID * lpClassId)
{
    IGenericSFImpl *This = impl_from_IPersistFolder2(iface);

    TRACE ("(%p)\n", This);

    if (!lpClassId)
        return E_POINTER;

    *lpClassId = CLSID_NetworkPlaces;

    return S_OK;
}

/************************************************************************
 *	INPFldr_PersistFolder2_Initialize
 *
 * NOTES: it makes no sense to change the pidl
 */
static HRESULT WINAPI INPFldr_PersistFolder2_Initialize (
               IPersistFolder2 * iface, LPCITEMIDLIST pidl)
{
    IGenericSFImpl *This = impl_from_IPersistFolder2(iface);

    TRACE ("(%p)->(%p)\n", This, pidl);

    return E_NOTIMPL;
}

/**************************************************************************
 *	IPersistFolder2_fnGetCurFolder
 */
static HRESULT WINAPI INPFldr_PersistFolder2_GetCurFolder (
               IPersistFolder2 * iface, LPITEMIDLIST * pidl)
{
    IGenericSFImpl *This = impl_from_IPersistFolder2(iface);

    TRACE ("(%p)->(%p)\n", This, pidl);

    if (!pidl)
        return E_POINTER;

    *pidl = ILClone (This->pidlRoot);

    return S_OK;
}

static const IPersistFolder2Vtbl vt_NP_PersistFolder2 =
{
    INPFldr_PersistFolder2_QueryInterface,
    INPFldr_PersistFolder2_AddRef,
    INPFldr_PersistFolder2_Release,
    INPFldr_PersistFolder2_GetClassID,
    INPFldr_PersistFolder2_Initialize,
    INPFldr_PersistFolder2_GetCurFolder
};
