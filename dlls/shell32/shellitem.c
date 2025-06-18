/*
 * IShellItem and IShellItemArray implementations
 *
 * Copyright 2008 Vincent Povirk for CodeWeavers
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

#include <stdio.h>
#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

#include "pidl.h"
#include "shell32_main.h"
#include "debughlp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

struct shell_item {
    IShellItem2             IShellItem2_iface;
    LONG                    ref;
    LPITEMIDLIST            pidl;
    IPersistIDList          IPersistIDList_iface;
    IShellItemImageFactory  IShellItemImageFactory_iface;
};

typedef struct _CustomDestinationList {
    ICustomDestinationList ICustomDestinationList_iface;
    LONG ref;
} CustomDestinationList;

static struct shell_item *impl_from_IShellItem2(IShellItem2 *iface)
{
    return CONTAINING_RECORD(iface, struct shell_item, IShellItem2_iface);
}

static struct shell_item *impl_from_IPersistIDList(IPersistIDList *iface)
{
    return CONTAINING_RECORD(iface, struct shell_item, IPersistIDList_iface);
}

static struct shell_item *impl_from_IShellItemImageFactory(IShellItemImageFactory *iface)
{
    return CONTAINING_RECORD(iface, struct shell_item, IShellItemImageFactory_iface);
}

static inline CustomDestinationList *impl_from_ICustomDestinationList( ICustomDestinationList *iface )
{
    return CONTAINING_RECORD(iface, CustomDestinationList, ICustomDestinationList_iface);
}

static HRESULT WINAPI ShellItem_QueryInterface(IShellItem2 *iface, REFIID riid,
    void **ppv)
{
    struct shell_item *This = impl_from_IShellItem2(iface);

    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, riid) || IsEqualIID(&IID_IShellItem, riid) ||
        IsEqualIID(&IID_IShellItem2, riid))
    {
        *ppv = &This->IShellItem2_iface;
    }
    else if (IsEqualIID(&IID_IPersist, riid) || IsEqualIID(&IID_IPersistIDList, riid))
    {
        *ppv = &This->IPersistIDList_iface;
    }
    else if (IsEqualIID(&IID_IShellItemImageFactory, riid))
    {
        *ppv = &This->IShellItemImageFactory_iface;
    }
    else {
        FIXME("not implemented for %s\n", shdebugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ShellItem_AddRef(IShellItem2 *iface)
{
    struct shell_item *This = impl_from_IShellItem2(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p), new refcount=%li\n", iface, ref);

    return ref;
}

static ULONG WINAPI ShellItem_Release(IShellItem2 *iface)
{
    struct shell_item *This = impl_from_IShellItem2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p), new refcount=%li\n", iface, ref);

    if (ref == 0)
    {
        ILFree(This->pidl);
        free(This);
    }

    return ref;
}

static HRESULT ShellItem_get_parent_pidl(struct shell_item *This, ITEMIDLIST **parent_pidl)
{
    *parent_pidl = ILClone(This->pidl);
    if (*parent_pidl)
    {
        if (ILRemoveLastID(*parent_pidl))
            return S_OK;
        else
        {
            ILFree(*parent_pidl);
            *parent_pidl = NULL;
            return E_INVALIDARG;
        }
    }
    else
    {
        *parent_pidl = NULL;
        return E_OUTOFMEMORY;
    }
}

static HRESULT ShellItem_get_parent_shellfolder(struct shell_item *This, IShellFolder **ppsf)
{
    LPITEMIDLIST parent_pidl;
    IShellFolder *desktop;
    HRESULT ret;

    ret = ShellItem_get_parent_pidl(This, &parent_pidl);
    if (SUCCEEDED(ret))
    {
        ret = SHGetDesktopFolder(&desktop);
        if (SUCCEEDED(ret))
        {
            if (_ILIsDesktop(parent_pidl))
            {
                *ppsf = desktop;
            }
            else
            {
                ret = IShellFolder_BindToObject(desktop, parent_pidl, NULL, &IID_IShellFolder, (void**)ppsf);
                IShellFolder_Release(desktop);
            }
        }
        ILFree(parent_pidl);
    }

    return ret;
}

static HRESULT ShellItem_get_shellfolder(struct shell_item *This, IBindCtx *pbc, IShellFolder **ppsf)
{
    IShellFolder *desktop;
    HRESULT ret;

    ret = SHGetDesktopFolder(&desktop);
    if (SUCCEEDED(ret))
    {
        if (_ILIsDesktop(This->pidl))
        {
            *ppsf = desktop;
            IShellFolder_AddRef(*ppsf);
        }
        else
        {
            ret = IShellFolder_BindToObject(desktop, This->pidl, pbc, &IID_IShellFolder, (void**)ppsf);
        }

        IShellFolder_Release(desktop);
    }

    return ret;
}

static HRESULT WINAPI ShellItem_BindToHandler(IShellItem2 *iface, IBindCtx *pbc,
    REFGUID rbhid, REFIID riid, void **ppvOut)
{
    struct shell_item *This = impl_from_IShellItem2(iface);
    HRESULT ret;
    TRACE("(%p,%p,%s,%p,%p)\n", iface, pbc, shdebugstr_guid(rbhid), riid, ppvOut);

    *ppvOut = NULL;
    if (IsEqualGUID(rbhid, &BHID_SFObject))
    {
        IShellFolder *psf;
        ret = ShellItem_get_shellfolder(This, pbc, &psf);
        if (SUCCEEDED(ret))
        {
            ret = IShellFolder_QueryInterface(psf, riid, ppvOut);
            IShellFolder_Release(psf);
        }
        return ret;
    }
    else if (IsEqualGUID(rbhid, &BHID_SFUIObject))
    {
        IShellFolder *psf_parent;
        if (_ILIsDesktop(This->pidl))
            ret = SHGetDesktopFolder(&psf_parent);
        else
            ret = ShellItem_get_parent_shellfolder(This, &psf_parent);

        if (SUCCEEDED(ret))
        {
            LPCITEMIDLIST pidl = ILFindLastID(This->pidl);
            ret = IShellFolder_GetUIObjectOf(psf_parent, NULL, 1, &pidl, riid, NULL, ppvOut);
            IShellFolder_Release(psf_parent);
        }
        return ret;
    }
    else if (IsEqualGUID(rbhid, &BHID_DataObject))
    {
        return ShellItem_BindToHandler(&This->IShellItem2_iface, pbc, &BHID_SFUIObject,
                                       &IID_IDataObject, ppvOut);
    }

    FIXME("Unsupported BHID %s.\n", debugstr_guid(rbhid));

    return MK_E_NOOBJECT;
}

static HRESULT WINAPI ShellItem_GetParent(IShellItem2 *iface, IShellItem **ppsi)
{
    struct shell_item *This = impl_from_IShellItem2(iface);
    LPITEMIDLIST parent_pidl;
    HRESULT ret;

    TRACE("(%p,%p)\n", iface, ppsi);

    ret = ShellItem_get_parent_pidl(This, &parent_pidl);
    if (SUCCEEDED(ret))
    {
        ret = SHCreateShellItem(NULL, NULL, parent_pidl, ppsi);
        ILFree(parent_pidl);
    }

    return ret;
}

static HRESULT WINAPI ShellItem_GetDisplayName(IShellItem2 *iface, SIGDN sigdnName,
    LPWSTR *ppszName)
{
    struct shell_item *This = impl_from_IShellItem2(iface);
    TRACE("(%p,%x,%p)\n", iface, sigdnName, ppszName);

    return SHGetNameFromIDList(This->pidl, sigdnName, ppszName);
}

static HRESULT WINAPI ShellItem_GetAttributes(IShellItem2 *iface, SFGAOF sfgaoMask,
    SFGAOF *psfgaoAttribs)
{
    struct shell_item *This = impl_from_IShellItem2(iface);
    IShellFolder *parent_folder;
    LPITEMIDLIST child_pidl;
    HRESULT ret;

    TRACE("(%p,%lx,%p)\n", iface, sfgaoMask, psfgaoAttribs);

    if (_ILIsDesktop(This->pidl))
        ret = SHGetDesktopFolder(&parent_folder);
    else
        ret = ShellItem_get_parent_shellfolder(This, &parent_folder);
    if (SUCCEEDED(ret))
    {
        child_pidl = ILFindLastID(This->pidl);
        *psfgaoAttribs = sfgaoMask;
        ret = IShellFolder_GetAttributesOf(parent_folder, 1, (LPCITEMIDLIST*)&child_pidl, psfgaoAttribs);
        *psfgaoAttribs &= sfgaoMask;
        IShellFolder_Release(parent_folder);

        if (SUCCEEDED(ret))
        {
            if(sfgaoMask == *psfgaoAttribs)
                return S_OK;
            else
                return S_FALSE;
        }
    }

    return ret;
}

static HRESULT WINAPI ShellItem_Compare(IShellItem2 *iface, IShellItem *oth,
    SICHINTF hint, int *piOrder)
{
    LPWSTR dispname, dispname_oth;
    HRESULT ret;
    TRACE("(%p,%p,%lx,%p)\n", iface, oth, hint, piOrder);

    if(hint & (SICHINT_CANONICAL | SICHINT_ALLFIELDS))
        FIXME("Unsupported flags 0x%08lx\n", hint);

    ret = IShellItem2_GetDisplayName(iface, SIGDN_DESKTOPABSOLUTEEDITING, &dispname);
    if(SUCCEEDED(ret))
    {
        ret = IShellItem_GetDisplayName(oth, SIGDN_DESKTOPABSOLUTEEDITING, &dispname_oth);
        if(SUCCEEDED(ret))
        {
            *piOrder = lstrcmpiW(dispname, dispname_oth);
            CoTaskMemFree(dispname_oth);
        }
        CoTaskMemFree(dispname);
    }

    if(SUCCEEDED(ret) && *piOrder &&
       (hint & SICHINT_TEST_FILESYSPATH_IF_NOT_EQUAL))
    {
        LPWSTR dispname, dispname_oth;

        TRACE("Testing filesystem path.\n");
        ret = IShellItem2_GetDisplayName(iface, SIGDN_FILESYSPATH, &dispname);
        if(SUCCEEDED(ret))
        {
            ret = IShellItem_GetDisplayName(oth, SIGDN_FILESYSPATH, &dispname_oth);
            if(SUCCEEDED(ret))
            {
                *piOrder = lstrcmpiW(dispname, dispname_oth);
                CoTaskMemFree(dispname_oth);
            }
            CoTaskMemFree(dispname);
        }
    }

    if(FAILED(ret))
        return ret;

    if(*piOrder)
        return S_FALSE;
    else
        return S_OK;
}

static HRESULT WINAPI ShellItem2_GetPropertyStore(IShellItem2 *iface, GETPROPERTYSTOREFLAGS flags,
    REFIID riid, void **ppv)
{
    struct shell_item *This = impl_from_IShellItem2(iface);
    FIXME("Stub: %p (%d, %s, %p)\n", This, flags, shdebugstr_guid(riid), ppv);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellItem2_GetPropertyStoreWithCreateObject(IShellItem2 *iface,
    GETPROPERTYSTOREFLAGS flags, IUnknown *punkCreateObject, REFIID riid, void **ppv)
{
    struct shell_item *This = impl_from_IShellItem2(iface);
    FIXME("Stub: %p (%08x, %p, %s, %p)\n",
          This, flags, punkCreateObject, shdebugstr_guid(riid), ppv);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellItem2_GetPropertyStoreForKeys(IShellItem2 *iface, const PROPERTYKEY *rgKeys,
    UINT cKeys, GETPROPERTYSTOREFLAGS flags, REFIID riid, void **ppv)
{
    struct shell_item *This = impl_from_IShellItem2(iface);
    FIXME("Stub: %p (%p, %d, %08x, %s, %p)\n",
          This, rgKeys, cKeys, flags, shdebugstr_guid(riid), ppv);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellItem2_GetPropertyDescriptionList(IShellItem2 *iface,
    REFPROPERTYKEY keyType, REFIID riid, void **ppv)
{
    struct shell_item *This = impl_from_IShellItem2(iface);
    FIXME("Stub: %p (%p, %s, %p)\n", This, keyType, debugstr_guid(riid), ppv);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellItem2_Update(IShellItem2 *iface, IBindCtx *pbc)
{
    struct shell_item *This = impl_from_IShellItem2(iface);
    FIXME("Stub: %p (%p)\n", This, pbc);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellItem2_GetProperty(IShellItem2 *iface, REFPROPERTYKEY key, PROPVARIANT *ppropvar)
{
    struct shell_item *This = impl_from_IShellItem2(iface);
    FIXME("Stub: %p (%p, %p)\n", This, key, ppropvar);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellItem2_GetCLSID(IShellItem2 *iface, REFPROPERTYKEY key, CLSID *pclsid)
{
    struct shell_item *This = impl_from_IShellItem2(iface);
    FIXME("Stub: %p (%p, %p)\n", This, key, pclsid);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellItem2_GetFileTime(IShellItem2 *iface, REFPROPERTYKEY key, FILETIME *pft)
{
    struct shell_item *This = impl_from_IShellItem2(iface);
    FIXME("Stub: %p (%p, %p)\n", This, key, pft);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellItem2_GetInt32(IShellItem2 *iface, REFPROPERTYKEY key, int *pi)
{
    struct shell_item *This = impl_from_IShellItem2(iface);
    FIXME("Stub: %p (%p, %p)\n", This, key, pi);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellItem2_GetString(IShellItem2 *iface, REFPROPERTYKEY key, LPWSTR *ppsz)
{
    struct shell_item *This = impl_from_IShellItem2(iface);
    FIXME("Stub: %p (%p, %p)\n", This, key, ppsz);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellItem2_GetUInt32(IShellItem2 *iface, REFPROPERTYKEY key, ULONG *pui)
{
    struct shell_item *This = impl_from_IShellItem2(iface);
    FIXME("Stub: %p (%p, %p)\n", This, key, pui);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellItem2_GetUInt64(IShellItem2 *iface, REFPROPERTYKEY key, ULONGLONG *pull)
{
    struct shell_item *This = impl_from_IShellItem2(iface);
    FIXME("Stub: %p (%p, %p)\n", This, key, pull);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellItem2_GetBool(IShellItem2 *iface, REFPROPERTYKEY key, BOOL *pf)
{
    struct shell_item *This = impl_from_IShellItem2(iface);
    FIXME("Stub: %p (%p, %p)\n", This, key, pf);
    return E_NOTIMPL;
}


static const IShellItem2Vtbl ShellItem2_Vtbl = {
    ShellItem_QueryInterface,
    ShellItem_AddRef,
    ShellItem_Release,
    ShellItem_BindToHandler,
    ShellItem_GetParent,
    ShellItem_GetDisplayName,
    ShellItem_GetAttributes,
    ShellItem_Compare,
    ShellItem2_GetPropertyStore,
    ShellItem2_GetPropertyStoreWithCreateObject,
    ShellItem2_GetPropertyStoreForKeys,
    ShellItem2_GetPropertyDescriptionList,
    ShellItem2_Update,
    ShellItem2_GetProperty,
    ShellItem2_GetCLSID,
    ShellItem2_GetFileTime,
    ShellItem2_GetInt32,
    ShellItem2_GetString,
    ShellItem2_GetUInt32,
    ShellItem2_GetUInt64,
    ShellItem2_GetBool
};

static HRESULT WINAPI ShellItem_IPersistIDList_QueryInterface(IPersistIDList *iface,
    REFIID riid, void **ppv)
{
    struct shell_item *This = impl_from_IPersistIDList(iface);
    return IShellItem2_QueryInterface(&This->IShellItem2_iface, riid, ppv);
}

static ULONG WINAPI ShellItem_IPersistIDList_AddRef(IPersistIDList *iface)
{
    struct shell_item *This = impl_from_IPersistIDList(iface);
    return IShellItem2_AddRef(&This->IShellItem2_iface);
}

static ULONG WINAPI ShellItem_IPersistIDList_Release(IPersistIDList *iface)
{
    struct shell_item *This = impl_from_IPersistIDList(iface);
    return IShellItem2_Release(&This->IShellItem2_iface);
}

static HRESULT WINAPI ShellItem_IPersistIDList_GetClassID(IPersistIDList* iface,
    CLSID *pClassID)
{
    *pClassID = CLSID_ShellItem;
    return S_OK;
}

static HRESULT WINAPI ShellItem_IPersistIDList_SetIDList(IPersistIDList* iface,
    LPCITEMIDLIST pidl)
{
    struct shell_item *This = impl_from_IPersistIDList(iface);
    LPITEMIDLIST new_pidl;

    TRACE("(%p,%p)\n", This, pidl);

    new_pidl = ILClone(pidl);

    if (new_pidl)
    {
        ILFree(This->pidl);
        This->pidl = new_pidl;
        return S_OK;
    }
    else
        return E_OUTOFMEMORY;
}

static HRESULT WINAPI ShellItem_IPersistIDList_GetIDList(IPersistIDList* iface,
    LPITEMIDLIST *ppidl)
{
    struct shell_item *This = impl_from_IPersistIDList(iface);

    TRACE("(%p,%p)\n", This, ppidl);

    *ppidl = ILClone(This->pidl);
    if (*ppidl)
        return S_OK;
    else
        return E_OUTOFMEMORY;
}

static const IPersistIDListVtbl ShellItem_IPersistIDList_Vtbl = {
    ShellItem_IPersistIDList_QueryInterface,
    ShellItem_IPersistIDList_AddRef,
    ShellItem_IPersistIDList_Release,
    ShellItem_IPersistIDList_GetClassID,
    ShellItem_IPersistIDList_SetIDList,
    ShellItem_IPersistIDList_GetIDList
};

static HRESULT WINAPI ShellItem_IShellItemImageFactory_QueryInterface(IShellItemImageFactory *iface,
    REFIID riid, void **ppv)
{
    struct shell_item *This = impl_from_IShellItemImageFactory(iface);
    return IShellItem2_QueryInterface(&This->IShellItem2_iface, riid, ppv);
}

static ULONG WINAPI ShellItem_IShellItemImageFactory_AddRef(IShellItemImageFactory *iface)
{
    struct shell_item *This = impl_from_IShellItemImageFactory(iface);
    return IShellItem2_AddRef(&This->IShellItem2_iface);
}

static ULONG WINAPI ShellItem_IShellItemImageFactory_Release(IShellItemImageFactory *iface)
{
    struct shell_item *This = impl_from_IShellItemImageFactory(iface);
    return IShellItem2_Release(&This->IShellItem2_iface);
}

static HRESULT WINAPI ShellItem_IShellItemImageFactory_GetImage(IShellItemImageFactory *iface,
    SIZE size, SIIGBF flags, HBITMAP *phbm)
{
    struct shell_item *This = impl_from_IShellItemImageFactory(iface);
    static const BITMAPINFOHEADER dummy_bmi_header = {
        .biSize = sizeof(dummy_bmi_header),
        .biWidth = 1,
        .biHeight = 1,
        .biPlanes = 1,
        .biBitCount = 32,
        .biCompression = BI_RGB
    };
    static int once;

    if (!once++)
        FIXME("%p ({%lu, %lu} %d %p): stub\n", This, size.cx, size.cy, flags, phbm);

    if (!(*phbm = CreateDIBSection(NULL, (const BITMAPINFO *)&dummy_bmi_header,
                                   DIB_RGB_COLORS, NULL, NULL, 0)))
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

static const IShellItemImageFactoryVtbl ShellItem_IShellItemImageFactory_Vtbl = {
    ShellItem_IShellItemImageFactory_QueryInterface,
    ShellItem_IShellItemImageFactory_AddRef,
    ShellItem_IShellItemImageFactory_Release,
    ShellItem_IShellItemImageFactory_GetImage,
};


HRESULT WINAPI IShellItem_Constructor(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    struct shell_item *This;
    HRESULT ret;

    TRACE("(%p,%s)\n",pUnkOuter, debugstr_guid(riid));

    *ppv = NULL;

    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    This = malloc(sizeof(*This));
    This->IShellItem2_iface.lpVtbl = &ShellItem2_Vtbl;
    This->ref = 1;
    This->pidl = NULL;
    This->IPersistIDList_iface.lpVtbl = &ShellItem_IPersistIDList_Vtbl;
    This->IShellItemImageFactory_iface.lpVtbl = &ShellItem_IShellItemImageFactory_Vtbl;

    ret = IShellItem2_QueryInterface(&This->IShellItem2_iface, riid, ppv);
    IShellItem2_Release(&This->IShellItem2_iface);

    return ret;
}

HRESULT WINAPI SHCreateShellItem(LPCITEMIDLIST pidlParent,
    IShellFolder *psfParent, LPCITEMIDLIST pidl, IShellItem **ppsi)
{
    LPITEMIDLIST new_pidl;
    HRESULT ret;

    TRACE("(%p,%p,%p,%p)\n", pidlParent, psfParent, pidl, ppsi);

    *ppsi = NULL;

    if (!pidl)
    {
        return E_INVALIDARG;
    }
    else if (pidlParent || psfParent)
    {
        LPITEMIDLIST temp_parent=NULL;
        if (!pidlParent)
        {
            IPersistFolder2* ppf2Parent;

            if (FAILED(IShellFolder_QueryInterface(psfParent, &IID_IPersistFolder2, (void**)&ppf2Parent)))
            {
                FIXME("couldn't get IPersistFolder2 interface of parent\n");
                return E_NOINTERFACE;
            }

            if (FAILED(IPersistFolder2_GetCurFolder(ppf2Parent, &temp_parent)))
            {
                FIXME("couldn't get parent PIDL\n");
                IPersistFolder2_Release(ppf2Parent);
                return E_NOINTERFACE;
            }

            pidlParent = temp_parent;
            IPersistFolder2_Release(ppf2Parent);
        }

        new_pidl = ILCombine(pidlParent, pidl);
        ILFree(temp_parent);

        if (!new_pidl)
            return E_OUTOFMEMORY;
    }
    else
    {
        new_pidl = ILClone(pidl);
        if (!new_pidl)
            return E_OUTOFMEMORY;
    }

    ret = SHCreateItemFromIDList(new_pidl, &IID_IShellItem, (void**)ppsi);
    ILFree(new_pidl);

    return ret;
}

HRESULT WINAPI SHCreateItemFromParsingName(PCWSTR pszPath,
    IBindCtx *pbc, REFIID riid, void **ppv)
{
    LPITEMIDLIST pidl;
    HRESULT ret;

    *ppv = NULL;

    ret = SHParseDisplayName(pszPath, pbc, &pidl, 0, NULL);
    if(SUCCEEDED(ret))
    {
        ret = SHCreateItemFromIDList(pidl, riid, ppv);
        ILFree(pidl);
    }
    return ret;
}

HRESULT WINAPI SHCreateItemFromRelativeName(IShellItem *parent, PCWSTR name, IBindCtx *pbc,
                                            REFIID riid, void **ppv)
{
    LPITEMIDLIST pidl_folder = NULL, pidl = NULL;
    IShellFolder *desktop = NULL, *folder = NULL;
    HRESULT hr;

    TRACE("(%p, %s, %p, %s, %p)\n", parent, wine_dbgstr_w(name), pbc, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_INVALIDARG;
    *ppv = NULL;
    if(!name)
        return E_INVALIDARG;

    hr = SHGetIDListFromObject((IUnknown*)parent, &pidl_folder);
    if(hr != S_OK)
        return hr;

    hr = SHGetDesktopFolder(&desktop);
    if(hr != S_OK)
        goto cleanup;

    if(!_ILIsDesktop(pidl_folder))
    {
        hr = IShellFolder_BindToObject(desktop, pidl_folder, NULL, &IID_IShellFolder,
                                       (void**)&folder);
        if(hr != S_OK)
            goto cleanup;
    }

    hr = IShellFolder_ParseDisplayName(folder ? folder : desktop, NULL, pbc, (LPWSTR)name,
                                       NULL, &pidl, NULL);
    if(hr != S_OK)
        goto cleanup;
    hr = SHCreateItemFromIDList(pidl, riid, ppv);

cleanup:
    if(pidl_folder)
        ILFree(pidl_folder);
    if(pidl)
        ILFree(pidl);
    if(desktop)
        IShellFolder_Release(desktop);
    if(folder)
        IShellFolder_Release(folder);
    return hr;
}

HRESULT WINAPI SHCreateItemFromIDList(PCIDLIST_ABSOLUTE pidl, REFIID riid, void **ppv)
{
    IPersistIDList *persist;
    HRESULT ret;

    if(!pidl)
        return E_INVALIDARG;

    *ppv = NULL;
    ret = IShellItem_Constructor(NULL, &IID_IPersistIDList, (void**)&persist);
    if(FAILED(ret))
        return ret;

    ret = IPersistIDList_SetIDList(persist, pidl);
    if(FAILED(ret))
    {
        IPersistIDList_Release(persist);
        return ret;
    }

    ret = IPersistIDList_QueryInterface(persist, riid, ppv);
    IPersistIDList_Release(persist);
    return ret;
}

HRESULT WINAPI SHCreateItemWithParent(PCIDLIST_ABSOLUTE pidl_parent, IShellFolder *psf,
                                PCUITEMID_CHILD pidl, REFIID riid, void **ppv)
{
    FIXME("(%p, %p, %p, %s, %p)\n", pidl_parent, psf, pidl, debugstr_guid(riid), ppv);

    return E_NOTIMPL;
}

HRESULT WINAPI SHCreateItemInKnownFolder(REFKNOWNFOLDERID rfid, DWORD flags,
                                         PCWSTR filename, REFIID riid, void **ppv)
{
    HRESULT hr;
    IShellItem *parent = NULL;
    LPITEMIDLIST pidl = NULL;

    TRACE("(%p, %lx, %s, %s, %p)\n", rfid, flags, wine_dbgstr_w(filename),
          debugstr_guid(riid), ppv);

    if(!rfid || !ppv)
        return E_INVALIDARG;

    *ppv = NULL;
    hr = SHGetKnownFolderIDList(rfid, flags, NULL, &pidl);
    if(hr != S_OK)
        return hr;

    hr = SHCreateItemFromIDList(pidl, &IID_IShellItem, (void**)&parent);
    if(hr != S_OK)
    {
        ILFree(pidl);
        return hr;
    }

    if(filename)
        hr = SHCreateItemFromRelativeName(parent, filename, NULL, riid, ppv);
    else
        hr = IShellItem_QueryInterface(parent, riid, ppv);

    ILFree(pidl);
    IShellItem_Release(parent);
    return hr;
}

HRESULT WINAPI SHGetItemFromDataObject(IDataObject *pdtobj,
    DATAOBJ_GET_ITEM_FLAGS dwFlags, REFIID riid, void **ppv)
{
    FORMATETC fmt;
    STGMEDIUM medium;
    HRESULT ret;

    TRACE("%p, %x, %s, %p\n", pdtobj, dwFlags, debugstr_guid(riid), ppv);

    if(!pdtobj)
        return E_INVALIDARG;

    fmt.cfFormat = RegisterClipboardFormatW(CFSTR_SHELLIDLISTW);
    fmt.ptd = NULL;
    fmt.dwAspect = DVASPECT_CONTENT;
    fmt.lindex = -1;
    fmt.tymed = TYMED_HGLOBAL;

    ret = IDataObject_GetData(pdtobj, &fmt, &medium);
    if(SUCCEEDED(ret))
    {
        LPIDA pida = GlobalLock(medium.hGlobal);

        if((pida->cidl > 1 && !(dwFlags & DOGIF_ONLY_IF_ONE)) ||
           pida->cidl == 1)
        {
            LPITEMIDLIST pidl;

            /* Get the first pidl (parent + child1) */
            pidl = ILCombine((LPCITEMIDLIST) ((LPBYTE)pida+pida->aoffset[0]),
                             (LPCITEMIDLIST) ((LPBYTE)pida+pida->aoffset[1]));

            ret = SHCreateItemFromIDList(pidl, riid, ppv);
            ILFree(pidl);
        }
        else
        {
            ret = E_FAIL;
        }

        GlobalUnlock(medium.hGlobal);
        GlobalFree(medium.hGlobal);
    }

    if(FAILED(ret) && !(dwFlags & DOGIF_NO_HDROP))
    {
        TRACE("Attempting to fall back on CF_HDROP.\n");

        fmt.cfFormat = CF_HDROP;
        fmt.ptd = NULL;
        fmt.dwAspect = DVASPECT_CONTENT;
        fmt.lindex = -1;
        fmt.tymed = TYMED_HGLOBAL;

        ret = IDataObject_GetData(pdtobj, &fmt, &medium);
        if(SUCCEEDED(ret))
        {
            DROPFILES *df = GlobalLock(medium.hGlobal);
            LPBYTE files = (LPBYTE)df + df->pFiles;
            BOOL multiple_files = FALSE;

            ret = E_FAIL;
            if(!df->fWide)
            {
                WCHAR filename[MAX_PATH];
                PCSTR first_file = (PCSTR)files;
                if(*(files + lstrlenA(first_file) + 1) != 0)
                    multiple_files = TRUE;

                if( !(multiple_files && (dwFlags & DOGIF_ONLY_IF_ONE)) )
                {
                    MultiByteToWideChar(CP_ACP, 0, first_file, -1, filename, MAX_PATH);
                    ret = SHCreateItemFromParsingName(filename, NULL, riid, ppv);
                }
            }
            else
            {
                PCWSTR first_file = (PCWSTR)files;
                if(*((PCWSTR)files + lstrlenW(first_file) + 1) != 0)
                    multiple_files = TRUE;

                if( !(multiple_files && (dwFlags & DOGIF_ONLY_IF_ONE)) )
                    ret = SHCreateItemFromParsingName(first_file, NULL, riid, ppv);
            }

            GlobalUnlock(medium.hGlobal);
            GlobalFree(medium.hGlobal);
        }
    }

    if(FAILED(ret) && !(dwFlags & DOGIF_NO_URL))
    {
        FIXME("Failed to create item, should try CF_URL.\n");
    }

    return ret;
}

HRESULT WINAPI SHGetItemFromObject(IUnknown *punk, REFIID riid, void **ppv)
{
    LPITEMIDLIST pidl;
    HRESULT ret;

    ret = SHGetIDListFromObject(punk, &pidl);
    if(SUCCEEDED(ret))
    {
        ret = SHCreateItemFromIDList(pidl, riid, ppv);
        ILFree(pidl);
    }

    return ret;
}

/*************************************************************************
 * IEnumShellItems implementation
 */
typedef struct {
    IEnumShellItems IEnumShellItems_iface;
    LONG ref;

    IShellItemArray *array;
    DWORD count;
    DWORD position;
} IEnumShellItemsImpl;

static inline IEnumShellItemsImpl *impl_from_IEnumShellItems(IEnumShellItems *iface)
{
    return CONTAINING_RECORD(iface, IEnumShellItemsImpl, IEnumShellItems_iface);
}

static HRESULT WINAPI IEnumShellItems_fnQueryInterface(IEnumShellItems *iface,
                                                       REFIID riid,
                                                       void **ppvObject)
{
    IEnumShellItemsImpl *This = impl_from_IEnumShellItems(iface);
    TRACE("%p (%s, %p)\n", This, shdebugstr_guid(riid), ppvObject);

    *ppvObject = NULL;
    if(IsEqualIID(riid, &IID_IEnumShellItems) ||
       IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObject = &This->IEnumShellItems_iface;
    }

    if(*ppvObject)
    {
        IUnknown_AddRef((IUnknown*)*ppvObject);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI IEnumShellItems_fnAddRef(IEnumShellItems *iface)
{
    IEnumShellItemsImpl *This = impl_from_IEnumShellItems(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("%p - ref %ld\n", This, ref);

    return ref;
}

static ULONG WINAPI IEnumShellItems_fnRelease(IEnumShellItems *iface)
{
    IEnumShellItemsImpl *This = impl_from_IEnumShellItems(iface);
    LONG ref = InterlockedDecrement(&This->ref);
    TRACE("%p - ref %ld\n", This, ref);

    if(!ref)
    {
        TRACE("Freeing.\n");
        IShellItemArray_Release(This->array);
        free(This);
        return 0;
    }

    return ref;
}

static HRESULT WINAPI IEnumShellItems_fnNext(IEnumShellItems* iface,
                                             ULONG celt,
                                             IShellItem **rgelt,
                                             ULONG *pceltFetched)
{
    IEnumShellItemsImpl *This = impl_from_IEnumShellItems(iface);
    HRESULT hr = S_FALSE;
    UINT i;
    ULONG fetched = 0;
    TRACE("%p (%ld %p %p)\n", This, celt, rgelt, pceltFetched);

    if(pceltFetched == NULL && celt != 1)
        return E_INVALIDARG;

    for(i = This->position; fetched < celt && i < This->count; i++) {
        hr = IShellItemArray_GetItemAt(This->array, i, &rgelt[fetched]);
        if(FAILED(hr))
            break;
        fetched++;
        This->position++;
    }

    if(SUCCEEDED(hr))
    {
        if(pceltFetched != NULL)
            *pceltFetched = fetched;

        if(fetched > 0)
            return S_OK;

        return S_FALSE;
    }

    return hr;
}

static HRESULT WINAPI IEnumShellItems_fnSkip(IEnumShellItems* iface, ULONG celt)
{
    IEnumShellItemsImpl *This = impl_from_IEnumShellItems(iface);
    TRACE("%p (%ld)\n", This, celt);

    This->position = min(This->position + celt, This->count-1);

    return S_OK;
}

static HRESULT WINAPI IEnumShellItems_fnReset(IEnumShellItems* iface)
{
    IEnumShellItemsImpl *This = impl_from_IEnumShellItems(iface);
    TRACE("%p\n", This);

    This->position = 0;

    return S_OK;
}

static HRESULT WINAPI IEnumShellItems_fnClone(IEnumShellItems* iface, IEnumShellItems **ppenum)
{
    IEnumShellItemsImpl *This = impl_from_IEnumShellItems(iface);
    TRACE("%p (%p)\n", This, ppenum);

    /* Not implemented anywhere */
    *ppenum = NULL;

    return E_NOTIMPL;
}

static const IEnumShellItemsVtbl vt_IEnumShellItems = {
    IEnumShellItems_fnQueryInterface,
    IEnumShellItems_fnAddRef,
    IEnumShellItems_fnRelease,
    IEnumShellItems_fnNext,
    IEnumShellItems_fnSkip,
    IEnumShellItems_fnReset,
    IEnumShellItems_fnClone
};

static HRESULT IEnumShellItems_Constructor(IShellItemArray *array, IEnumShellItems **ppesi)
{
    IEnumShellItemsImpl *This;
    HRESULT ret;

    This = malloc(sizeof(*This));
    if(!This)
        return E_OUTOFMEMORY;

    This->ref = 1;
    This->IEnumShellItems_iface.lpVtbl = &vt_IEnumShellItems;
    This->array = array;
    This->position = 0;

    IShellItemArray_AddRef(This->array);
    IShellItemArray_GetCount(This->array, &This->count);

    ret = IEnumShellItems_QueryInterface(&This->IEnumShellItems_iface, &IID_IEnumShellItems, (void**)ppesi);
    IEnumShellItems_Release(&This->IEnumShellItems_iface);

    return ret;
}


/*************************************************************************
 * IShellItemArray implementation
 */
typedef struct {
    IShellItemArray IShellItemArray_iface;
    LONG ref;

    IShellItem **array;
    DWORD item_count;
} IShellItemArrayImpl;

static inline IShellItemArrayImpl *impl_from_IShellItemArray(IShellItemArray *iface)
{
    return CONTAINING_RECORD(iface, IShellItemArrayImpl, IShellItemArray_iface);
}

static HRESULT WINAPI IShellItemArray_fnQueryInterface(IShellItemArray *iface,
                                                       REFIID riid,
                                                       void **ppvObject)
{
    IShellItemArrayImpl *This = impl_from_IShellItemArray(iface);
    TRACE("%p (%s, %p)\n", This, shdebugstr_guid(riid), ppvObject);

    *ppvObject = NULL;
    if(IsEqualIID(riid, &IID_IShellItemArray) ||
       IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObject = &This->IShellItemArray_iface;
    }

    if(*ppvObject)
    {
        IUnknown_AddRef((IUnknown*)*ppvObject);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI IShellItemArray_fnAddRef(IShellItemArray *iface)
{
    IShellItemArrayImpl *This = impl_from_IShellItemArray(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("%p - ref %ld\n", This, ref);

    return ref;
}

static ULONG WINAPI IShellItemArray_fnRelease(IShellItemArray *iface)
{
    IShellItemArrayImpl *This = impl_from_IShellItemArray(iface);
    LONG ref = InterlockedDecrement(&This->ref);
    TRACE("%p - ref %ld\n", This, ref);

    if(!ref)
    {
        UINT i;
        TRACE("Freeing.\n");

        for(i = 0; i < This->item_count; i++)
            IShellItem_Release(This->array[i]);

        free(This->array);
        free(This);
        return 0;
    }

    return ref;
}

static HRESULT WINAPI IShellItemArray_fnBindToHandler(IShellItemArray *iface,
                                                      IBindCtx *pbc,
                                                      REFGUID bhid,
                                                      REFIID riid,
                                                      void **ppvOut)
{
    IShellItemArrayImpl *This = impl_from_IShellItemArray(iface);
    FIXME("Stub: %p (%p, %s, %s, %p)\n",
          This, pbc, shdebugstr_guid(bhid), shdebugstr_guid(riid), ppvOut);

    return E_NOTIMPL;
}

static HRESULT WINAPI IShellItemArray_fnGetPropertyStore(IShellItemArray *iface,
                                                         GETPROPERTYSTOREFLAGS flags,
                                                         REFIID riid,
                                                         void **ppv)
{
    IShellItemArrayImpl *This = impl_from_IShellItemArray(iface);
    FIXME("Stub: %p (%x, %s, %p)\n", This, flags, shdebugstr_guid(riid), ppv);

    return E_NOTIMPL;
}

static HRESULT WINAPI IShellItemArray_fnGetPropertyDescriptionList(IShellItemArray *iface,
                                                                   REFPROPERTYKEY keyType,
                                                                   REFIID riid,
                                                                   void **ppv)
{
    IShellItemArrayImpl *This = impl_from_IShellItemArray(iface);
    FIXME("Stub: %p (%p, %s, %p)\n",
          This, keyType, shdebugstr_guid(riid), ppv);

    return E_NOTIMPL;
}

static HRESULT WINAPI IShellItemArray_fnGetAttributes(IShellItemArray *iface,
                                                      SIATTRIBFLAGS AttribFlags,
                                                      SFGAOF sfgaoMask,
                                                      SFGAOF *psfgaoAttribs)
{
    IShellItemArrayImpl *This = impl_from_IShellItemArray(iface);
    HRESULT hr = S_OK;
    SFGAOF attr;
    UINT i;
    TRACE("%p (%x, %lx, %p)\n", This, AttribFlags, sfgaoMask, psfgaoAttribs);

    if(AttribFlags & ~(SIATTRIBFLAGS_AND|SIATTRIBFLAGS_OR))
        FIXME("%08x contains unsupported attribution flags\n", AttribFlags);

    for(i = 0; i < This->item_count; i++)
    {
        hr = IShellItem_GetAttributes(This->array[i], sfgaoMask, &attr);
        if(FAILED(hr))
            break;

        if(i == 0)
        {
            *psfgaoAttribs = attr;
            continue;
        }

        switch(AttribFlags & SIATTRIBFLAGS_MASK)
        {
        case SIATTRIBFLAGS_AND:
            *psfgaoAttribs &= attr;
            break;
        case SIATTRIBFLAGS_OR:
            *psfgaoAttribs |= attr;
            break;
        }
    }

    if(SUCCEEDED(hr))
    {
        if(*psfgaoAttribs == sfgaoMask)
            return S_OK;

        return S_FALSE;
    }

    return hr;
}

static HRESULT WINAPI IShellItemArray_fnGetCount(IShellItemArray *iface,
                                                 DWORD *pdwNumItems)
{
    IShellItemArrayImpl *This = impl_from_IShellItemArray(iface);
    TRACE("%p (%p)\n", This, pdwNumItems);

    *pdwNumItems = This->item_count;

    return S_OK;
}

static HRESULT WINAPI IShellItemArray_fnGetItemAt(IShellItemArray *iface,
                                                  DWORD dwIndex,
                                                  IShellItem **ppsi)
{
    IShellItemArrayImpl *This = impl_from_IShellItemArray(iface);
    TRACE("%p (%lx, %p)\n", This, dwIndex, ppsi);

    /* zero indexed */
    if(dwIndex + 1 > This->item_count)
        return E_FAIL;

    *ppsi = This->array[dwIndex];
    IShellItem_AddRef(*ppsi);

    return S_OK;
}

static HRESULT WINAPI IShellItemArray_fnEnumItems(IShellItemArray *iface,
                                                  IEnumShellItems **ppenumShellItems)
{
    IShellItemArrayImpl *This = impl_from_IShellItemArray(iface);
    TRACE("%p (%p)\n", This, ppenumShellItems);
    return IEnumShellItems_Constructor(iface, ppenumShellItems);
}

static const IShellItemArrayVtbl vt_IShellItemArray = {
    IShellItemArray_fnQueryInterface,
    IShellItemArray_fnAddRef,
    IShellItemArray_fnRelease,
    IShellItemArray_fnBindToHandler,
    IShellItemArray_fnGetPropertyStore,
    IShellItemArray_fnGetPropertyDescriptionList,
    IShellItemArray_fnGetAttributes,
    IShellItemArray_fnGetCount,
    IShellItemArray_fnGetItemAt,
    IShellItemArray_fnEnumItems
};

/* Caller is responsible to AddRef all items */
static HRESULT create_shellitemarray(IShellItem **items, DWORD count, IShellItemArray **ret)
{
    IShellItemArrayImpl *This;

    TRACE("(%p, %ld, %p)\n", items, count, ret);

    This = malloc(sizeof(*This));
    if(!This)
        return E_OUTOFMEMORY;

    This->IShellItemArray_iface.lpVtbl = &vt_IShellItemArray;
    This->ref = 1;

    This->array = malloc(count * sizeof(IShellItem*));
    if (!This->array)
    {
        free(This);
        return E_OUTOFMEMORY;
    }
    memcpy(This->array, items, count*sizeof(IShellItem*));
    This->item_count = count;

    *ret = &This->IShellItemArray_iface;
    return S_OK;
}

HRESULT WINAPI SHCreateShellItemArray(PCIDLIST_ABSOLUTE pidlParent,
                                      IShellFolder *psf,
                                      UINT cidl,
                                      PCUITEMID_CHILD_ARRAY ppidl,
                                      IShellItemArray **ppsiItemArray)
{
    IShellItem **array;
    HRESULT ret = E_FAIL;
    UINT i;

    TRACE("%p, %p, %d, %p, %p\n", pidlParent, psf, cidl, ppidl, ppsiItemArray);

    *ppsiItemArray = NULL;

    if(!pidlParent && !psf)
        return E_POINTER;

    if(!ppidl)
        return E_INVALIDARG;

    array = calloc(cidl, sizeof(IShellItem*));
    if(!array)
        return E_OUTOFMEMORY;

    for(i = 0; i < cidl; i++)
    {
        ret = SHCreateShellItem(pidlParent, psf, ppidl[i], &array[i]);
        if(FAILED(ret)) break;
    }

    if(SUCCEEDED(ret))
    {
        ret = create_shellitemarray(array, cidl, ppsiItemArray);
    }

    if(FAILED(ret))
    {
        for(i = 0; i < cidl; i++)
            if(array[i]) IShellItem_Release(array[i]);
    }
    free(array);
    return ret;
}

HRESULT WINAPI SHCreateShellItemArrayFromShellItem(IShellItem *item, REFIID riid, void **ppv)
{
    IShellItemArray *array;
    HRESULT ret;

    TRACE("%p, %s, %p\n", item, shdebugstr_guid(riid), ppv);

    *ppv = NULL;

    IShellItem_AddRef(item);
    ret = create_shellitemarray(&item, 1, &array);
    if(FAILED(ret))
    {
        IShellItem_Release(item);
        return ret;
    }

    ret = IShellItemArray_QueryInterface(array, riid, ppv);
    IShellItemArray_Release(array);
    return ret;
}

HRESULT WINAPI SHCreateShellItemArrayFromDataObject(IDataObject *pdo, REFIID riid, void **ppv)
{
    IShellItemArray *psia;
    FORMATETC fmt;
    STGMEDIUM medium;
    HRESULT ret;

    TRACE("%p, %s, %p\n", pdo, shdebugstr_guid(riid), ppv);

    if(!pdo)
        return E_INVALIDARG;

    *ppv = NULL;

    fmt.cfFormat = RegisterClipboardFormatW(CFSTR_SHELLIDLISTW);
    fmt.ptd = NULL;
    fmt.dwAspect = DVASPECT_CONTENT;
    fmt.lindex = -1;
    fmt.tymed = TYMED_HGLOBAL;

    ret = IDataObject_GetData(pdo, &fmt, &medium);
    if(SUCCEEDED(ret))
    {
        LPIDA pida = GlobalLock(medium.hGlobal);
        LPCITEMIDLIST parent_pidl;
        LPCITEMIDLIST *children;
        UINT i;
        TRACE("Converting %d objects.\n", pida->cidl);

        parent_pidl = (LPCITEMIDLIST) ((LPBYTE)pida+pida->aoffset[0]);

        children = malloc(sizeof(const ITEMIDLIST*) * pida->cidl);
        for(i = 0; i < pida->cidl; i++)
            children[i] = (LPCITEMIDLIST) ((LPBYTE)pida+pida->aoffset[i+1]);

        ret = SHCreateShellItemArray(parent_pidl, NULL, pida->cidl, children, &psia);

        free(children);

        GlobalUnlock(medium.hGlobal);
        GlobalFree(medium.hGlobal);
    }

    if(SUCCEEDED(ret))
    {
        ret = IShellItemArray_QueryInterface(psia, riid, ppv);
        IShellItemArray_Release(psia);
    }

    return ret;
}

HRESULT WINAPI SHCreateShellItemArrayFromIDLists(UINT cidl,
                                                 PCIDLIST_ABSOLUTE_ARRAY pidl_array,
                                                 IShellItemArray **psia)
{
    IShellItem **array;
    HRESULT ret;
    UINT i;
    TRACE("%d, %p, %p\n", cidl, pidl_array, psia);

    *psia = NULL;

    if(cidl == 0)
        return E_INVALIDARG;

    array = calloc(cidl, sizeof(IShellItem*));
    if(!array)
        return E_OUTOFMEMORY;

    for(i = 0; i < cidl; i++)
    {
        ret = SHCreateShellItem(NULL, NULL, pidl_array[i], &array[i]);
        if(FAILED(ret))
            break;
    }

    if(SUCCEEDED(ret))
    {
        ret = create_shellitemarray(array, cidl, psia);
    }

    if(FAILED(ret))
    {
        for(i = 0; i < cidl; i++)
            if(array[i]) IShellItem_Release(array[i]);
        *psia = NULL;
    }
    free(array);
    return ret;
}

HRESULT WINAPI SHGetPropertyStoreFromParsingName(const WCHAR *path, IBindCtx *pbc, GETPROPERTYSTOREFLAGS flags,
    REFIID riid, void **ppv)
{
    IShellItem2 *item;
    HRESULT hr;

    TRACE("(%s %p %#x %p %p)\n", debugstr_w(path), pbc, flags, riid, ppv);

    hr = SHCreateItemFromParsingName(path, pbc, &IID_IShellItem2, (void **)&item);
    if(SUCCEEDED(hr))
    {
        hr = IShellItem2_GetPropertyStore(item, flags, riid, ppv);
        IShellItem2_Release(item);
    }

    return hr;
}

static HRESULT WINAPI CustomDestinationList_QueryInterface(ICustomDestinationList *iface, REFIID riid, void **obj)
{
    CustomDestinationList *This = impl_from_ICustomDestinationList(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(&IID_ICustomDestinationList, riid) || IsEqualIID(&IID_IUnknown, riid))
    {
        *obj = &This->ICustomDestinationList_iface;
    }
    else {
        WARN("Unsupported interface %s.\n", shdebugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*obj);
    return S_OK;
}

static ULONG WINAPI CustomDestinationList_AddRef(ICustomDestinationList *iface)
{
    CustomDestinationList *This = impl_from_ICustomDestinationList(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p), new refcount=%li\n", This, ref);

    return ref;
}

static ULONG WINAPI CustomDestinationList_Release(ICustomDestinationList *iface)
{
    CustomDestinationList *This = impl_from_ICustomDestinationList(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p), new refcount=%li\n", This, ref);

    if (ref == 0)
        free(This);

    return ref;
}

static HRESULT WINAPI CustomDestinationList_SetAppID(ICustomDestinationList *iface, const WCHAR *appid)
{
    CustomDestinationList *This = impl_from_ICustomDestinationList(iface);

    FIXME("%p (%s): stub\n", This, debugstr_w(appid));

    return E_NOTIMPL;
}

static HRESULT WINAPI CustomDestinationList_BeginList(ICustomDestinationList *iface, UINT *min_slots, REFIID riid, void **obj)
{
    CustomDestinationList *This = impl_from_ICustomDestinationList(iface);

    FIXME("%p (%p %s %p): stub\n", This, min_slots, debugstr_guid(riid), obj);

    return E_NOTIMPL;
}

static HRESULT WINAPI CustomDestinationList_AppendCategory(ICustomDestinationList *iface, const WCHAR *category, IObjectArray *array)
{
    CustomDestinationList *This = impl_from_ICustomDestinationList(iface);

    FIXME("%p (%s %p): stub\n", This, debugstr_w(category), array);

    return E_NOTIMPL;
}

static HRESULT WINAPI CustomDestinationList_AppendKnownCategory(ICustomDestinationList *iface, KNOWNDESTCATEGORY category)
{
    CustomDestinationList *This = impl_from_ICustomDestinationList(iface);

    FIXME("%p (%d): stub\n", This, category);

    return E_NOTIMPL;
}

static HRESULT WINAPI CustomDestinationList_AddUserTasks(ICustomDestinationList *iface, IObjectArray *tasks)
{
    CustomDestinationList *This = impl_from_ICustomDestinationList(iface);

    FIXME("%p (%p): stub\n", This, tasks);

    return E_NOTIMPL;
}

static HRESULT WINAPI CustomDestinationList_CommitList(ICustomDestinationList *iface)
{
    CustomDestinationList *This = impl_from_ICustomDestinationList(iface);

    FIXME("%p: stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI CustomDestinationList_GetRemovedDestinations(ICustomDestinationList *iface, REFIID riid, void **obj)
{
    CustomDestinationList *This = impl_from_ICustomDestinationList(iface);

    FIXME("%p (%s %p): stub\n", This, debugstr_guid(riid), obj);

    return E_NOTIMPL;
}

static HRESULT WINAPI CustomDestinationList_DeleteList(ICustomDestinationList *iface, const WCHAR *appid)
{
    CustomDestinationList *This = impl_from_ICustomDestinationList(iface);

    FIXME("%p (%s): stub\n", This, debugstr_w(appid));

    return E_NOTIMPL;
}

static HRESULT WINAPI CustomDestinationList_AbortList(ICustomDestinationList *iface)
{
    CustomDestinationList *This = impl_from_ICustomDestinationList(iface);

    FIXME("%p: stub\n", This);

    return E_NOTIMPL;
}

static const ICustomDestinationListVtbl CustomDestinationListVtbl =
{
    CustomDestinationList_QueryInterface,
    CustomDestinationList_AddRef,
    CustomDestinationList_Release,
    CustomDestinationList_SetAppID,
    CustomDestinationList_BeginList,
    CustomDestinationList_AppendCategory,
    CustomDestinationList_AppendKnownCategory,
    CustomDestinationList_AddUserTasks,
    CustomDestinationList_CommitList,
    CustomDestinationList_GetRemovedDestinations,
    CustomDestinationList_DeleteList,
    CustomDestinationList_AbortList
};

HRESULT WINAPI CustomDestinationList_Constructor(IUnknown *outer, REFIID riid, void **obj)
{
    CustomDestinationList *list;
    HRESULT hr;

    TRACE("%p %s %p\n", outer, debugstr_guid(riid), obj);

    if (outer)
        return CLASS_E_NOAGGREGATION;

    if(!(list = malloc(sizeof(*list))))
        return E_OUTOFMEMORY;

    list->ICustomDestinationList_iface.lpVtbl = &CustomDestinationListVtbl;
    list->ref = 1;

    hr = ICustomDestinationList_QueryInterface(&list->ICustomDestinationList_iface, riid, obj);
    ICustomDestinationList_Release(&list->ICustomDestinationList_iface);
    return hr;
}

HRESULT WINAPI SHSetTemporaryPropertyForItem(IShellItem *psi, REFPROPERTYKEY propkey, REFPROPVARIANT propvar)
{
    FIXME("%p %p %p: stub\n", psi, propkey, propvar);

    return E_NOTIMPL;
}
