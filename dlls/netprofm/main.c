/*
 * Copyright 2014 Hans Leidekker for CodeWeavers
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
#include "windef.h"
#include "winbase.h"
#define COBJMACROS
#include "objbase.h"
#include "rpcproxy.h"
#include "netlistmgr.h"

#include "wine/debug.h"
#include "netprofm_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(netprofm);

static HINSTANCE instance;

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

struct netprofm_cf
{
    const struct IClassFactoryVtbl *vtbl;
    HRESULT (*create_instance)(void **);
};

static inline struct netprofm_cf *impl_from_IClassFactory( IClassFactory *iface )
{
    return (struct netprofm_cf *)((char *)iface - FIELD_OFFSET( struct netprofm_cf, vtbl ));
}

static HRESULT WINAPI netprofm_cf_QueryInterface( IClassFactory *iface, REFIID riid, LPVOID *ppobj )
{
    if (IsEqualGUID( riid, &IID_IUnknown ) || IsEqualGUID( riid, &IID_IClassFactory ))
    {
        IClassFactory_AddRef( iface );
        *ppobj = iface;
        return S_OK;
    }
    FIXME( "interface %s not implemented\n", debugstr_guid(riid) );
    return E_NOINTERFACE;
}

static ULONG WINAPI netprofm_cf_AddRef( IClassFactory *iface )
{
    return 2;
}

static ULONG WINAPI netprofm_cf_Release( IClassFactory *iface )
{
    return 1;
}

static HRESULT WINAPI netprofm_cf_CreateInstance( IClassFactory *iface, LPUNKNOWN outer,
                                                  REFIID riid, LPVOID *obj )
{
    struct netprofm_cf *factory = impl_from_IClassFactory( iface );
    IUnknown *unk;
    HRESULT r;

    TRACE( "%p %s %p\n", outer, debugstr_guid(riid), obj );

    *obj = NULL;
    if (outer) return CLASS_E_NOAGGREGATION;

    r = factory->create_instance( (void **)&unk );
    if (FAILED( r ))
        return r;

    r = IUnknown_QueryInterface( unk, riid, obj );
    if (FAILED( r ))
        return r;

    IUnknown_Release( unk );
    return r;
}

static HRESULT WINAPI netprofm_cf_LockServer( IClassFactory *iface, BOOL dolock )
{
    FIXME( "%p, %d\n", iface, dolock );
    return S_OK;
}

static const struct IClassFactoryVtbl netprofm_cf_vtbl =
{
    netprofm_cf_QueryInterface,
    netprofm_cf_AddRef,
    netprofm_cf_Release,
    netprofm_cf_CreateInstance,
    netprofm_cf_LockServer
};

static struct netprofm_cf list_manager_cf = { &netprofm_cf_vtbl, list_manager_create };

/***********************************************************************
 *      DllGetClassObject (NETPROFM.@)
 */
HRESULT WINAPI DllGetClassObject( REFCLSID rclsid, REFIID iid, LPVOID *ppv )
{
    IClassFactory *cf = NULL;

    TRACE("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv);

    if (IsEqualGUID( rclsid, &CLSID_NetworkListManager ))
    {
       cf = (IClassFactory *)&list_manager_cf.vtbl;
    }
    if (!cf) return CLASS_E_CLASSNOTAVAILABLE;
    return IClassFactory_QueryInterface( cf, iid, ppv );
}

/***********************************************************************
 *      DllCanUnloadNow (NETPROFM.@)
 */
HRESULT WINAPI DllCanUnloadNow( void )
{
    return S_FALSE;
}
