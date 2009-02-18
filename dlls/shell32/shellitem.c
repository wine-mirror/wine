/*
 * IShellItem implementation
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

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <stdarg.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

#include "pidl.h"
#include "shell32_main.h"
#include "debughlp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

typedef struct _ShellItem {
    const IShellItemVtbl    *lpIShellItemVtbl;
    LONG                    ref;
    LPITEMIDLIST            pidl;
} ShellItem;


static HRESULT WINAPI ShellItem_QueryInterface(IShellItem *iface, REFIID riid,
    void **ppv)
{
    ShellItem *This = (ShellItem*)iface;

    TRACE("(%p,%p,%p)\n", iface, riid, ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, riid) || IsEqualIID(&IID_IShellItem, riid))
    {
        *ppv = This;
    }
    else {
        FIXME("not implemented for %s\n", shdebugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ShellItem_AddRef(IShellItem *iface)
{
    ShellItem *This = (ShellItem*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p), new refcount=%i\n", iface, ref);

    return ref;
}

static ULONG WINAPI ShellItem_Release(IShellItem *iface)
{
    ShellItem *This = (ShellItem*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p), new refcount=%i\n", iface, ref);

    if (ref == 0)
    {
        ILFree(This->pidl);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI ShellItem_BindToHandler(IShellItem *iface, IBindCtx *pbc,
    REFGUID rbhid, REFIID riid, void **ppvOut)
{
    FIXME("(%p,%p,%s,%p,%p)\n", iface, pbc, shdebugstr_guid(rbhid), riid, ppvOut);

    *ppvOut = NULL;

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellItem_GetParent(IShellItem *iface, IShellItem **ppsi)
{
    FIXME("(%p,%p)\n", iface, ppsi);

    *ppsi = NULL;

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellItem_GetDisplayName(IShellItem *iface, SIGDN sigdnName,
    LPWSTR *ppszName)
{
    FIXME("(%p,%x,%p)\n", iface, sigdnName, ppszName);

    *ppszName = NULL;

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellItem_GetAttributes(IShellItem *iface, SFGAOF sfgaoMask,
    SFGAOF *psfgaoAttribs)
{
    FIXME("(%p,%x,%p)\n", iface, sfgaoMask, psfgaoAttribs);

    *psfgaoAttribs = 0;

    return E_NOTIMPL;
}

static HRESULT WINAPI ShellItem_Compare(IShellItem *iface, IShellItem *oth,
    SICHINTF hint, int *piOrder)
{
    FIXME("(%p,%p,%x,%p)\n", iface, oth, hint, piOrder);

    return E_NOTIMPL;
}

static const IShellItemVtbl ShellItem_Vtbl = {
    ShellItem_QueryInterface,
    ShellItem_AddRef,
    ShellItem_Release,
    ShellItem_BindToHandler,
    ShellItem_GetParent,
    ShellItem_GetDisplayName,
    ShellItem_GetAttributes,
    ShellItem_Compare
};


HRESULT WINAPI IShellItem_Constructor(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    ShellItem *This;
    HRESULT ret;

    TRACE("(%p,%s)\n",pUnkOuter, debugstr_guid(riid));

    *ppv = NULL;

    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(ShellItem));
    This->lpIShellItemVtbl = &ShellItem_Vtbl;
    This->ref = 1;
    This->pidl = NULL;

    ret = ShellItem_QueryInterface((IShellItem*)This, riid, ppv);
    ShellItem_Release((IShellItem*)This);

    return ret;
}

HRESULT WINAPI SHCreateShellItem(LPCITEMIDLIST pidlParent,
    IShellFolder *psfParent, LPCITEMIDLIST pidl, IShellItem **ppsi)
{
    ShellItem *This;
    LPITEMIDLIST new_pidl;
    HRESULT ret;

    TRACE("(%p,%p,%p,%p)\n", pidlParent, psfParent, pidl, ppsi);

    if (!pidlParent && !psfParent && pidl)
    {
        new_pidl = ILClone(pidl);
        if (!new_pidl)
            return E_OUTOFMEMORY;
    }
    else
    {
        FIXME("(%p,%p,%p) not implemented\n", pidlParent, psfParent, pidl);
        return E_NOINTERFACE;
    }

    ret = IShellItem_Constructor(NULL, &IID_IShellItem, (void**)&This);
    if (This)
    {
        *ppsi = (IShellItem*)This;
        This->pidl = new_pidl;
    }
    else
    {
        *ppsi = NULL;
        ILFree(new_pidl);
    }
    return ret;
}
