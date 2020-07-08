/*
 * Copyright 2019 Hans Leidekker for CodeWeavers
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
#include "windef.h"
#include "winbase.h"
#define COBJMACROS
#include "objbase.h"
#include "rpcproxy.h"
#include "msado15_backcompat.h"

#include "wine/debug.h"
#include "wine/heap.h"

#include "msado15_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msado15);

static HINSTANCE hinstance;

BOOL WINAPI DllMain( HINSTANCE dll, DWORD reason, LPVOID reserved )
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        hinstance = dll;
        DisableThreadLibraryCalls( dll );
        break;
    }
    return TRUE;
}

typedef HRESULT (*fnCreateInstance)( void **obj );

struct msadocf
{
    IClassFactory    IClassFactory_iface;
    fnCreateInstance pfnCreateInstance;
};

static inline struct msadocf *impl_from_IClassFactory( IClassFactory *iface )
{
    return CONTAINING_RECORD( iface, struct msadocf, IClassFactory_iface );
}

static HRESULT WINAPI msadocf_QueryInterface( IClassFactory *iface, REFIID riid, void **obj )
{
    if (IsEqualGUID( riid, &IID_IUnknown ) || IsEqualGUID( riid, &IID_IClassFactory ))
    {
        IClassFactory_AddRef( iface );
        *obj = iface;
        return S_OK;
    }
    FIXME( "interface %s not implemented\n", debugstr_guid(riid) );
    return E_NOINTERFACE;
}

static ULONG WINAPI msadocf_AddRef( IClassFactory *iface )
{
    return 2;
}

static ULONG WINAPI msadocf_Release( IClassFactory *iface )
{
    return 1;
}

static HRESULT WINAPI msadocf_CreateInstance( IClassFactory *iface, LPUNKNOWN outer, REFIID riid, void **obj )
{
    struct msadocf *cf = impl_from_IClassFactory( iface );
    IUnknown *unknown;
    HRESULT hr;

    TRACE( "%p, %s, %p\n", outer, debugstr_guid(riid), obj );

    *obj = NULL;
    if (outer)
        return CLASS_E_NOAGGREGATION;

    hr = cf->pfnCreateInstance( (void **)&unknown );
    if (FAILED(hr))
        return hr;

    hr = IUnknown_QueryInterface( unknown, riid, obj );
    IUnknown_Release( unknown );
    return hr;
}

static HRESULT WINAPI msadocf_LockServer( IClassFactory *iface, BOOL dolock )
{
    FIXME( "%p, %d\n", iface, dolock );
    return S_OK;
}

static const struct IClassFactoryVtbl msadocf_vtbl =
{
    msadocf_QueryInterface,
    msadocf_AddRef,
    msadocf_Release,
    msadocf_CreateInstance,
    msadocf_LockServer
};

static struct msadocf command_cf = { { &msadocf_vtbl }, Command_create };
static struct msadocf connection_cf = { { &msadocf_vtbl }, Connection_create };
static struct msadocf recordset_cf = { { &msadocf_vtbl }, Recordset_create };
static struct msadocf stream_cf = { { &msadocf_vtbl }, Stream_create };

/***********************************************************************
 *          DllGetClassObject
 */
HRESULT WINAPI DllGetClassObject( REFCLSID clsid, REFIID iid, void **obj )
{
    IClassFactory *cf = NULL;

    TRACE( "%s, %s, %p\n", debugstr_guid(clsid), debugstr_guid(iid), obj );

    if (IsEqualGUID( clsid, &CLSID_Connection ))
    {
        cf = &connection_cf.IClassFactory_iface;
    }
    else if (IsEqualGUID( clsid, &CLSID_Recordset ))
    {
        cf = &recordset_cf.IClassFactory_iface;
    }
    else if (IsEqualGUID( clsid, &CLSID_Stream ))
    {
        cf = &stream_cf.IClassFactory_iface;
    }
    else if (IsEqualGUID( clsid, &CLSID_Command ))
    {
        cf = &command_cf.IClassFactory_iface;
    }
    if (!cf) return CLASS_E_CLASSNOTAVAILABLE;
    return IClassFactory_QueryInterface( cf, iid, obj );
}

/******************************************************************
 *          DllCanUnloadNow
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

/***********************************************************************
 *          DllRegisterServer
 */
HRESULT WINAPI DllRegisterServer( void )
{
    return __wine_register_resources( hinstance );
}

/***********************************************************************
 *          DllUnregisterServer
 */
HRESULT WINAPI DllUnregisterServer( void )
{
    return __wine_unregister_resources( hinstance );
}
