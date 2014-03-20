/*
 * Copyright 2014 Alistair Leslie-Hughes
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
#include <assert.h>

#define COBJMACROS

#include "netcfgx.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL( netcfgx );


typedef struct NetConfiguration
{
    INetCfg INetCfg_iface;
    LONG ref;
} NetConfiguration;

static inline NetConfiguration *impl_from_INetCfg(INetCfg *iface)
{
    return CONTAINING_RECORD(iface, NetConfiguration, INetCfg_iface);
}

static HRESULT WINAPI netcfg_QueryInterface(INetCfg *iface, REFIID riid, void **ppvObject)
{
    TRACE("%p %s %p\n", iface, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_INetCfg) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    INetCfg_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI netcfg_AddRef(INetCfg *iface)
{
    NetConfiguration *This = impl_from_INetCfg(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI netcfg_Release(INetCfg *iface)
{
    NetConfiguration *This = impl_from_INetCfg(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p ref=%u\n", This, ref);

    if (ref == 0)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI netcfg_Initialize(INetCfg *iface, PVOID pvReserved)
{
    NetConfiguration *This = impl_from_INetCfg(iface);
    FIXME("%p %p\n", This, pvReserved);

    return S_OK;
}

static HRESULT WINAPI netcfg_Uninitialize(INetCfg *iface)
{
    NetConfiguration *This = impl_from_INetCfg(iface);
    FIXME("%p\n", This);

    return S_OK;
}

static HRESULT WINAPI netcfg_Apply(INetCfg *iface)
{
    NetConfiguration *This = impl_from_INetCfg(iface);
    FIXME("%p\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI netcfg_Cancel(INetCfg *iface)
{
    NetConfiguration *This = impl_from_INetCfg(iface);
    FIXME("%p\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI netcfg_EnumComponents(INetCfg *iface, const GUID *pguidClass, IEnumNetCfgComponent **ppenumComponent)
{
    NetConfiguration *This = impl_from_INetCfg(iface);
    FIXME("%p %s %p\n", This, debugstr_guid(pguidClass), ppenumComponent);

    return E_NOTIMPL;
}

static HRESULT WINAPI netcfg_FindComponent(INetCfg *iface, LPCWSTR pszwInfId, INetCfgComponent **pComponent)
{
    NetConfiguration *This = impl_from_INetCfg(iface);
    FIXME("%p %s %p\n", This, debugstr_w(pszwInfId), pComponent);

    return E_NOTIMPL;
}

static HRESULT WINAPI netcfg_QueryNetCfgClass(INetCfg *iface, const GUID *pguidClass, REFIID riid, void **ppvObject)
{
    NetConfiguration *This = impl_from_INetCfg(iface);
    FIXME("%p %s %p\n", This, debugstr_guid(pguidClass), ppvObject);

    return E_NOTIMPL;
}

static const struct INetCfgVtbl NetCfgVtbl =
{
    netcfg_QueryInterface,
    netcfg_AddRef,
    netcfg_Release,
    netcfg_Initialize,
    netcfg_Uninitialize,
    netcfg_Apply,
    netcfg_Cancel,
    netcfg_EnumComponents,
    netcfg_FindComponent,
    netcfg_QueryNetCfgClass
};

HRESULT INetCfg_CreateInstance(IUnknown **ppUnk)
{
    NetConfiguration *This;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(NetConfiguration));
    if (!This)
        return E_OUTOFMEMORY;

    This->INetCfg_iface.lpVtbl = &NetCfgVtbl;
    This->ref = 1;

    *ppUnk = (IUnknown*)This;

    return S_OK;
}
