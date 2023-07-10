/*
 * msdaps initialisation.
 *
 * Copyright 2010 Huw Davies
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
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "initguid.h"
#include "objbase.h"
#include "oleauto.h"
#define DBINITCONSTANTS
#include "oledb.h"

#include "row_server.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(oledb);

extern BOOL WINAPI msdaps_DllMain(HINSTANCE, DWORD, LPVOID);
extern HRESULT WINAPI msdaps_DllGetClassObject(REFCLSID, REFIID, LPVOID *);
extern HRESULT WINAPI msdaps_DllCanUnloadNow(void);
extern HRESULT WINAPI msdaps_DllRegisterServer(void);
extern HRESULT WINAPI msdaps_DllUnregisterServer(void);

/*****************************************************************************
 *              DllMain
 */
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    return msdaps_DllMain(instance, reason, reserved);
}


/******************************************************************************
 * ClassFactory
 */
typedef struct
{
    IClassFactory IClassFactory_iface;
    HRESULT (*create_object)( IUnknown*, LPVOID* );
} cf;

static inline cf *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, cf, IClassFactory_iface);
}

static HRESULT WINAPI CF_QueryInterface(IClassFactory *iface, REFIID riid, void **obj)
{
    cf *This = impl_from_IClassFactory(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), obj);

    if( IsEqualCLSID( riid, &IID_IUnknown ) ||
        IsEqualCLSID( riid, &IID_IClassFactory ) )
    {
        IClassFactory_AddRef( iface );
        *obj = iface;
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI CF_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI CF_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI CF_CreateInstance(IClassFactory *iface, IUnknown *pOuter, REFIID riid, void **obj)
{
    cf *This = impl_from_IClassFactory(iface);
    IUnknown *unk = NULL;
    HRESULT r;

    TRACE("(%p, %p, %s, %p)\n", This, pOuter, debugstr_guid(riid), obj);

    r = This->create_object( pOuter, (void **) &unk );
    if (SUCCEEDED(r))
    {
        r = IUnknown_QueryInterface( unk, riid, obj );
        IUnknown_Release( unk );
    }
    return r;
}

static HRESULT WINAPI CF_LockServer(IClassFactory *iface, BOOL dolock)
{
    FIXME("(%p, %d): stub\n", iface, dolock);
    return S_OK;
}

static const IClassFactoryVtbl CF_Vtbl =
{
    CF_QueryInterface,
    CF_AddRef,
    CF_Release,
    CF_CreateInstance,
    CF_LockServer
};

static cf row_server_cf = { { &CF_Vtbl }, create_row_server };
static cf row_proxy_cf  = { { &CF_Vtbl }, create_row_marshal };
static cf rowset_server_cf = { { &CF_Vtbl }, create_rowset_server };
static cf rowset_proxy_cf  = { { &CF_Vtbl }, create_rowset_marshal };

/***********************************************************************
 *              DllGetClassObject
 */
HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID *obj)
{
    TRACE("(%s, %s, %p)\n", debugstr_guid(clsid), debugstr_guid(iid), obj);

    if (IsEqualCLSID(clsid, &CLSID_wine_row_server))
    {
        *obj = &row_server_cf;
        return S_OK;
    }

    if (IsEqualCLSID(clsid, &CLSID_wine_row_proxy))
    {
        *obj = &row_proxy_cf;
        return S_OK;
    }

    if (IsEqualCLSID(clsid, &CLSID_wine_rowset_server))
    {
        *obj = &rowset_server_cf;
        return S_OK;
    }

    if (IsEqualCLSID(clsid, &CLSID_wine_rowset_proxy))
    {
        *obj = &rowset_proxy_cf;
        return S_OK;
    }

    return msdaps_DllGetClassObject(clsid, iid, obj);
}

/***********************************************************************
 *              DllCanUnloadNow
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return msdaps_DllCanUnloadNow();
}

/***********************************************************************
 *                DllRegisterServer
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return msdaps_DllRegisterServer();
}

/***********************************************************************
 *                DllUnregisterServer
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return msdaps_DllUnregisterServer();
}
