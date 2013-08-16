/*
 * Copyright 2013 Hans Leidekker for CodeWeavers
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
#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "wbemdisp.h"
#include "rpcproxy.h"

#include "wine/debug.h"
#include "wbemdisp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wbemdisp);

static HINSTANCE instance;

struct factory
{
    IClassFactory IClassFactory_iface;
    HRESULT (*fnCreateInstance)( IUnknown *, LPVOID * );
};

static inline struct factory *impl_from_IClassFactory( IClassFactory *iface )
{
    return CONTAINING_RECORD( iface, struct factory, IClassFactory_iface );
}

static HRESULT WINAPI factory_QueryInterface( IClassFactory *iface, REFIID riid, LPVOID *obj )
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

static ULONG WINAPI factory_AddRef( IClassFactory *iface )
{
    return 2;
}

static ULONG WINAPI factory_Release( IClassFactory *iface )
{
    return 1;
}

static HRESULT WINAPI factory_CreateInstance( IClassFactory *iface, LPUNKNOWN outer, REFIID riid,
                                              LPVOID *obj )
{
    struct factory *factory = impl_from_IClassFactory( iface );
    IUnknown *unk;
    HRESULT hr;

    TRACE( "%p, %s, %p\n", outer, debugstr_guid(riid), obj );

    *obj = NULL;
    if (outer) return CLASS_E_NOAGGREGATION;

    hr = factory->fnCreateInstance( outer, (LPVOID *)&unk );
    if (FAILED( hr ))
        return hr;

    hr = IUnknown_QueryInterface( unk, riid, obj );
    if (FAILED( hr ))
        return hr;

    IUnknown_Release( unk );
    return hr;
}

static HRESULT WINAPI factory_LockServer( IClassFactory *iface, BOOL lock )
{
    FIXME( "%p, %d\n", iface, lock );
    return S_OK;
}

static const struct IClassFactoryVtbl factory_vtbl =
{
    factory_QueryInterface,
    factory_AddRef,
    factory_Release,
    factory_CreateInstance,
    factory_LockServer
};

static struct factory swbem_locator_cf = { { &factory_vtbl }, SWbemLocator_create };

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{

    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            instance = hinst;
            DisableThreadLibraryCalls( hinst );
            break;
    }
    return TRUE;
}

HRESULT WINAPI DllGetClassObject( REFCLSID rclsid, REFIID iid, LPVOID *obj )
{
    IClassFactory *cf = NULL;

    TRACE( "%s, %s, %p\n", debugstr_guid(rclsid), debugstr_guid(iid), obj );

    if (IsEqualGUID( rclsid, &CLSID_SWbemLocator ))
    {
       cf = &swbem_locator_cf.IClassFactory_iface;
    }
    if (!cf) return CLASS_E_CLASSNOTAVAILABLE;
    return IClassFactory_QueryInterface( cf, iid, obj );
}

/***********************************************************************
 *      DllCanUnloadNow (WBEMDISP.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

/***********************************************************************
 *      DllRegisterServer (WBEMDISP.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources( instance );
}

/***********************************************************************
 *      DllUnregisterServer (WBEMDISP.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources( instance );
}
