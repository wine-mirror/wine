/*
 * IAssemblyCache implementation
 *
 * Copyright 2008 James Hawkins
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "fusion.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(fusion);

/* IAssemblyCache */

typedef struct {
    const IAssemblyCacheVtbl *lpIAssemblyCacheVtbl;

    LONG ref;
} IAssemblyCacheImpl;

static HRESULT WINAPI IAssemblyCacheImpl_QueryInterface(IAssemblyCache *iface,
                                                        REFIID riid, LPVOID *ppobj)
{
    IAssemblyCacheImpl *This = (IAssemblyCacheImpl *)iface;

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppobj);

    *ppobj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAssemblyCache))
    {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p, %s, %p): not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI IAssemblyCacheImpl_AddRef(IAssemblyCache *iface)
{
    IAssemblyCacheImpl *This = (IAssemblyCacheImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before = %u)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IAssemblyCacheImpl_Release(IAssemblyCache *iface)
{
    IAssemblyCacheImpl *This = (IAssemblyCacheImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before = %u)\n", This, refCount + 1);

    if (!refCount)
        HeapFree(GetProcessHeap(), 0, This);

    return refCount;
}

static HRESULT WINAPI IAssemblyCacheImpl_UninstallAssembly(IAssemblyCache *iface,
                                                           DWORD dwFlags,
                                                           LPCWSTR pszAssemblyName,
                                                           LPCFUSION_INSTALL_REFERENCE pRefData,
                                                           ULONG *pulDisposition)
{
    FIXME("(%p, %d, %s, %p, %p) stub!\n", iface, dwFlags,
          debugstr_w(pszAssemblyName), pRefData, pulDisposition);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAssemblyCacheImpl_QueryAssemblyInfo(IAssemblyCache *iface,
                                                           DWORD dwFlags,
                                                           LPCWSTR pszAssemblyName,
                                                           ASSEMBLY_INFO *pAsmInfo)
{
    FIXME("(%p, %d, %s, %p) stub!\n", iface, dwFlags,
          debugstr_w(pszAssemblyName), pAsmInfo);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAssemblyCacheImpl_CreateAssemblyCacheItem(IAssemblyCache *iface,
                                                                 DWORD dwFlags,
                                                                 PVOID pvReserved,
                                                                 IAssemblyCacheItem **ppAsmItem,
                                                                 LPCWSTR pszAssemblyName)
{
    FIXME("(%p, %d, %p, %p, %s) stub!\n", iface, dwFlags, pvReserved,
          ppAsmItem, debugstr_w(pszAssemblyName));

    return E_NOTIMPL;
}

static HRESULT WINAPI IAssemblyCacheImpl_CreateAssemblyScavenger(IAssemblyCache *iface,
                                                                 IUnknown **ppUnkReserved)
{
    FIXME("(%p, %p) stub!\n", iface, ppUnkReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI IAssemblyCacheImpl_InstallAssembly(IAssemblyCache *iface,
                                                         DWORD dwFlags,
                                                         LPCWSTR pszManifestFilePath,
                                                         LPCFUSION_INSTALL_REFERENCE pRefData)
{
    FIXME("(%p, %d, %s, %p) stub!\n", iface, dwFlags,
          debugstr_w(pszManifestFilePath), pRefData);

    return E_NOTIMPL;
}

static const IAssemblyCacheVtbl AssemblyCacheVtbl = {
    IAssemblyCacheImpl_QueryInterface,
    IAssemblyCacheImpl_AddRef,
    IAssemblyCacheImpl_Release,
    IAssemblyCacheImpl_UninstallAssembly,
    IAssemblyCacheImpl_QueryAssemblyInfo,
    IAssemblyCacheImpl_CreateAssemblyCacheItem,
    IAssemblyCacheImpl_CreateAssemblyScavenger,
    IAssemblyCacheImpl_InstallAssembly
};

/******************************************************************
 *  CreateAssemblyCache   (FUSION.@)
 */
HRESULT WINAPI CreateAssemblyCache(IAssemblyCache **ppAsmCache, DWORD dwReserved)
{
    IAssemblyCacheImpl *cache;

    TRACE("(%p, %d)\n", ppAsmCache, dwReserved);

    if (!ppAsmCache)
        return E_INVALIDARG;

    *ppAsmCache = NULL;

    cache = HeapAlloc(GetProcessHeap(), 0, sizeof(IAssemblyCacheImpl));
    if (!cache)
        return E_OUTOFMEMORY;

    cache->lpIAssemblyCacheVtbl = &AssemblyCacheVtbl;
    cache->ref = 1;

    *ppAsmCache = (IAssemblyCache *)cache;

    return S_OK;
}

/* IAssemblyCacheItem */

typedef struct {
    const IAssemblyCacheItemVtbl *lpIAssemblyCacheItemVtbl;

    LONG ref;
} IAssemblyCacheItemImpl;

static HRESULT WINAPI IAssemblyCacheItemImpl_QueryInterface(IAssemblyCacheItem *iface,
                                                            REFIID riid, LPVOID *ppobj)
{
    IAssemblyCacheItemImpl *This = (IAssemblyCacheItemImpl *)iface;

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppobj);

    *ppobj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAssemblyCacheItem))
    {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p, %s, %p): not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI IAssemblyCacheItemImpl_AddRef(IAssemblyCacheItem *iface)
{
    IAssemblyCacheItemImpl *This = (IAssemblyCacheItemImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before = %u)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IAssemblyCacheItemImpl_Release(IAssemblyCacheItem *iface)
{
    IAssemblyCacheItemImpl *This = (IAssemblyCacheItemImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before = %u)\n", This, refCount + 1);

    if (!refCount)
        HeapFree(GetProcessHeap(), 0, This);

    return refCount;
}

static HRESULT WINAPI IAssemblyCacheItemImpl_CreateStream(IAssemblyCacheItem *iface,
                                                        DWORD dwFlags,
                                                        LPCWSTR pszStreamName,
                                                        DWORD dwFormat,
                                                        DWORD dwFormatFlags,
                                                        IStream **ppIStream,
                                                        ULARGE_INTEGER *puliMaxSize)
{
    FIXME("(%p, %d, %s, %d, %d, %p, %p) stub!\n", iface, dwFlags,
          debugstr_w(pszStreamName), dwFormat, dwFormatFlags, ppIStream, puliMaxSize);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAssemblyCacheItemImpl_Commit(IAssemblyCacheItem *iface,
                                                  DWORD dwFlags,
                                                  ULONG *pulDisposition)
{
    FIXME("(%p, %d, %p) stub!\n", iface, dwFlags, pulDisposition);
    return E_NOTIMPL;
}

static HRESULT WINAPI IAssemblyCacheItemImpl_AbortItem(IAssemblyCacheItem *iface)
{
    FIXME("(%p) stub!\n", iface);
    return E_NOTIMPL;
}

static const IAssemblyCacheItemVtbl AssemblyCacheItemVtbl = {
    IAssemblyCacheItemImpl_QueryInterface,
    IAssemblyCacheItemImpl_AddRef,
    IAssemblyCacheItemImpl_Release,
    IAssemblyCacheItemImpl_CreateStream,
    IAssemblyCacheItemImpl_Commit,
    IAssemblyCacheItemImpl_AbortItem
};

/* IAssemblyEnum */

typedef struct {
    const IAssemblyEnumVtbl *lpIAssemblyEnumVtbl;

    LONG ref;
} IAssemblyEnumImpl;

static HRESULT WINAPI IAssemblyEnumImpl_QueryInterface(IAssemblyEnum *iface,
                                                       REFIID riid, LPVOID *ppobj)
{
    IAssemblyEnumImpl *This = (IAssemblyEnumImpl *)iface;

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppobj);

    *ppobj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAssemblyEnum))
    {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p, %s, %p): not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI IAssemblyEnumImpl_AddRef(IAssemblyEnum *iface)
{
    IAssemblyEnumImpl *This = (IAssemblyEnumImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before = %u)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IAssemblyEnumImpl_Release(IAssemblyEnum *iface)
{
    IAssemblyEnumImpl *This = (IAssemblyEnumImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before = %u)\n", This, refCount + 1);

    if (!refCount)
        HeapFree(GetProcessHeap(), 0, This);

    return refCount;
}

static HRESULT WINAPI IAssemblyEnumImpl_GetNextAssembly(IAssemblyEnum *iface,
                                                        LPVOID pvReserved,
                                                        IAssemblyName **ppName,
                                                        DWORD dwFlags)
{
    FIXME("(%p, %p, %p, %d) stub!\n", iface, pvReserved, ppName, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IAssemblyEnumImpl_Reset(IAssemblyEnum *iface)
{
    FIXME("(%p) stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI IAssemblyEnumImpl_Clone(IAssemblyEnum *iface,
                                               IAssemblyEnum **ppEnum)
{
    FIXME("(%p, %p) stub!\n", iface, ppEnum);
    return E_NOTIMPL;
}

static const IAssemblyEnumVtbl AssemblyEnumVtbl = {
    IAssemblyEnumImpl_QueryInterface,
    IAssemblyEnumImpl_AddRef,
    IAssemblyEnumImpl_Release,
    IAssemblyEnumImpl_GetNextAssembly,
    IAssemblyEnumImpl_Reset,
    IAssemblyEnumImpl_Clone
};
