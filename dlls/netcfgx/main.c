/*
 * Network Configuration Objects implementation
 *
 * Copyright 2013 Austin English
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
#include "ole2.h"
#include "rpcproxy.h"
#include "wine/debug.h"

#include "netcfgx.h"
#include "netcfg_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(netcfgx);

typedef HRESULT (*ClassFactoryCreateInstanceFunc)(IUnknown **);

typedef struct netcfgcf
{
    IClassFactory    IClassFactory_iface;
    ClassFactoryCreateInstanceFunc fnCreateInstance;
} netcfgcf;

static inline netcfgcf *impl_from_IClassFactory( IClassFactory *iface )
{
    return CONTAINING_RECORD(iface, netcfgcf, IClassFactory_iface);
}

static HRESULT WINAPI netcfgcf_QueryInterface(IClassFactory *iface, REFIID riid, LPVOID *ppobj )
{
    TRACE("%s %p\n", debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IClassFactory))
    {
        IClassFactory_AddRef( iface );
        *ppobj = iface;
        return S_OK;
    }

    ERR("interface %s not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI netcfgcf_AddRef(IClassFactory *iface )
{
    TRACE("%p\n", iface);

    return 2;
}

static ULONG WINAPI netcfgcf_Release(IClassFactory *iface )
{
    TRACE("%p\n", iface);

    return 1;
}

static HRESULT WINAPI netcfgcf_CreateInstance(IClassFactory *iface,LPUNKNOWN pOuter,
                            REFIID riid, LPVOID *ppobj )
{
    netcfgcf *This = impl_from_IClassFactory( iface );
    HRESULT hr;
    IUnknown *punk;

    TRACE("%p %s %p\n", pOuter, debugstr_guid(riid), ppobj );

    *ppobj = NULL;

    if (pOuter)
        return CLASS_E_NOAGGREGATION;

    hr = This->fnCreateInstance( &punk );
    if (SUCCEEDED(hr))
    {
        hr = IUnknown_QueryInterface( punk, riid, ppobj );

        IUnknown_Release( punk );
    }
    else
    {
        WARN("Cannot create an instance object. 0x%08lx\n", hr);
    }
    return hr;
}

static HRESULT WINAPI netcfgcf_LockServer(IClassFactory *iface, BOOL dolock)
{
    FIXME("(%p)->(%d),stub!\n",iface,dolock);
    return S_OK;
}

static const struct IClassFactoryVtbl netcfgcf_vtbl =
{
    netcfgcf_QueryInterface,
    netcfgcf_AddRef,
    netcfgcf_Release,
    netcfgcf_CreateInstance,
    netcfgcf_LockServer
};

static netcfgcf netconfigcf = { {&netcfgcf_vtbl}, INetCfg_CreateInstance };

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    IClassFactory *cf = NULL;

    TRACE("(%s, %s, %p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if(IsEqualCLSID(rclsid, &CLSID_CNetCfg))
    {
        cf = &netconfigcf.IClassFactory_iface;
    }

    if(!cf)
        return CLASS_E_CLASSNOTAVAILABLE;

    return IClassFactory_QueryInterface(cf, riid, ppv);
}
