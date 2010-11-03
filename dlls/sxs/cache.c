/*
 * IAssemblyCache implementation
 *
 * Copyright 2010 Hans Leidekker for CodeWeavers
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
#define INITGUID

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "winsxs.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(sxs);

struct cache
{
    const IAssemblyCacheVtbl *vtbl;
    LONG refs;
};

static HRESULT WINAPI cache_QueryInterface(
    IAssemblyCache *iface,
    REFIID riid,
    void **obj )
{
    struct cache *cache = (struct cache *)iface;

    TRACE("%p, %s, %p\n", cache, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAssemblyCache))
    {
        IUnknown_AddRef( iface );
        *obj = cache;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI cache_AddRef( IAssemblyCache *iface )
{
    struct cache *cache = (struct cache *)iface;
    return InterlockedIncrement( &cache->refs );
}

static ULONG WINAPI cache_Release( IAssemblyCache *iface )
{
    struct cache *cache = (struct cache *)iface;
    ULONG refs = InterlockedDecrement( &cache->refs );

    if (!refs)
    {
        TRACE("destroying %p\n", cache);
        HeapFree( GetProcessHeap(), 0, cache );
    }
    return refs;
}

static HRESULT WINAPI cache_UninstallAssembly(
    IAssemblyCache *iface,
    DWORD flags,
    LPCWSTR name,
    LPCFUSION_INSTALL_REFERENCE ref,
    ULONG *disp )
{
    FIXME("%p, 0x%08x, %s, %p, %p\n", iface, flags, debugstr_w(name), ref, disp);
    return E_NOTIMPL;
}

static HRESULT WINAPI cache_QueryAssemblyInfo(
    IAssemblyCache *iface,
    DWORD flags,
    LPCWSTR name,
    ASSEMBLY_INFO *info )
{
    FIXME("%p, 0x%08x, %s, %p\n", iface, flags, debugstr_w(name), info);
    return E_NOTIMPL;
}

static HRESULT WINAPI cache_CreateAssemblyCacheItem(
    IAssemblyCache *iface,
    DWORD flags,
    PVOID reserved,
    IAssemblyCacheItem **item,
    LPCWSTR name )
{
    FIXME("%p, 0x%08x, %p, %p, %s\n", iface, flags, reserved, item, debugstr_w(name));
    return E_NOTIMPL;
}

static HRESULT WINAPI cache_Reserved(
    IAssemblyCache *iface,
    IUnknown **reserved)
{
    FIXME("%p\n", reserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI cache_InstallAssembly(
    IAssemblyCache *iface,
    DWORD flags,
    LPCWSTR path,
    LPCFUSION_INSTALL_REFERENCE ref )
{
    FIXME("%p, 0x%08x, %s, %p\n", iface, flags, debugstr_w(path), ref);
    return E_NOTIMPL;
}

static const IAssemblyCacheVtbl cache_vtbl =
{
    cache_QueryInterface,
    cache_AddRef,
    cache_Release,
    cache_UninstallAssembly,
    cache_QueryAssemblyInfo,
    cache_CreateAssemblyCacheItem,
    cache_Reserved,
    cache_InstallAssembly
};

/******************************************************************
 *  CreateAssemblyCache   (SXS.@)
 */
HRESULT WINAPI CreateAssemblyCache( IAssemblyCache **obj, DWORD reserved )
{
    struct cache *cache;

    TRACE("%p, %u\n", obj, reserved);

    if (!obj)
        return E_INVALIDARG;

    *obj = NULL;

    cache = HeapAlloc( GetProcessHeap(), 0, sizeof(struct cache) );
    if (!cache)
        return E_OUTOFMEMORY;

    cache->vtbl = &cache_vtbl;
    cache->refs = 1;

    *obj = (IAssemblyCache *)cache;
    return S_OK;
}
