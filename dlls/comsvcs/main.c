/*
 * COM+ Services
 *
 * Copyright 2013 Alistair Leslie-Hughes
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

#define COBJMACROS

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "rpcproxy.h"
#include "comsvcs.h"
#include "wine/heap.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(comsvcs);

static HINSTANCE COMSVCS_hInstance;

typedef struct dispensermanager
{
    IDispenserManager IDispenserManager_iface;
    LONG ref;

} dispensermanager;

static inline dispensermanager *impl_from_IDispenserManager(IDispenserManager *iface)
{
    return CONTAINING_RECORD(iface, dispensermanager, IDispenserManager_iface);
}

static HRESULT WINAPI dismanager_QueryInterface(IDispenserManager *iface, REFIID riid, void **object)
{
    dispensermanager *This = impl_from_IDispenserManager(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), object);

    *object = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDispenserManager))
    {
        *object = &This->IDispenserManager_iface;
        IUnknown_AddRef( (IUnknown*)*object);

        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),object);
    return E_NOINTERFACE;
}

static ULONG WINAPI dismanager_AddRef(IDispenserManager *iface)
{
    dispensermanager *This = impl_from_IDispenserManager(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI dismanager_Release(IDispenserManager *iface)
{
    dispensermanager *This = impl_from_IDispenserManager(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
    {
       heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI dismanager_RegisterDispenser(IDispenserManager *iface, IDispenserDriver *driver,
                        LPCOLESTR name, IHolder **dispenser)
{
    dispensermanager *This = impl_from_IDispenserManager(iface);

    FIXME("(%p)->(%p, %s, %p) stub\n", This, driver, debugstr_w(name), dispenser);

    return E_NOTIMPL;
}

static HRESULT WINAPI dismanager_GetContext(IDispenserManager *iface, INSTID *id, TRANSID *transid)
{
    dispensermanager *This = impl_from_IDispenserManager(iface);

    FIXME("(%p)->(%p, %p) stub\n", This, id, transid);

    return E_NOTIMPL;
}

struct IDispenserManagerVtbl dismanager_vtbl =
{
    dismanager_QueryInterface,
    dismanager_AddRef,
    dismanager_Release,
    dismanager_RegisterDispenser,
    dismanager_GetContext
};

static HRESULT WINAPI comsvcscf_CreateInstance(IClassFactory *cf,IUnknown* outer, REFIID riid,void **object)
{
    dispensermanager *dismanager;
    HRESULT ret;

    TRACE("(%p %s %p)\n", outer, debugstr_guid(riid), object);

    dismanager = heap_alloc(sizeof(*dismanager));
    if (!dismanager)
    {
        *object = NULL;
        return E_OUTOFMEMORY;
    }

    dismanager->IDispenserManager_iface.lpVtbl = &dismanager_vtbl;
    dismanager->ref = 1;

    ret = dismanager_QueryInterface(&dismanager->IDispenserManager_iface, riid, object);
    dismanager_Release(&dismanager->IDispenserManager_iface);

    return ret;
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID lpv)
{
    switch(reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;    /* prefer native version */
    case DLL_PROCESS_ATTACH:
        COMSVCS_hInstance = hinst;
        DisableThreadLibraryCalls(hinst);
        break;
    }
    return TRUE;
}

static HRESULT WINAPI comsvcscf_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv )
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", iface, ppv);
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IClassFactory, riid)) {
        TRACE("(%p)->(IID_IClassFactory %p)\n", iface, ppv);
        *ppv = iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI comsvcscf_AddRef(IClassFactory *iface )
{
    TRACE("(%p)\n", iface);
    return 2;
}

static ULONG WINAPI comsvcscf_Release(IClassFactory *iface )
{
    TRACE("(%p)\n", iface);
    return 1;
}

static HRESULT WINAPI comsvcscf_LockServer(IClassFactory *iface, BOOL fLock)
{
    TRACE("(%p)->(%x)\n", iface, fLock);
    return S_OK;
}

static const struct IClassFactoryVtbl comsvcscf_vtbl =
{
    comsvcscf_QueryInterface,
    comsvcscf_AddRef,
    comsvcscf_Release,
    comsvcscf_CreateInstance,
    comsvcscf_LockServer
};

static IClassFactory DispenserManageFactory = { &comsvcscf_vtbl };

/******************************************************************
 * DllGetClassObject
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&CLSID_DispenserManager, rclsid)) {
        TRACE("(CLSID_DispenserManager %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&DispenserManageFactory, riid, ppv);
    }

    FIXME("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}

/******************************************************************
 * DllCanUnloadNow
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

/***********************************************************************
 *		DllRegisterServer (comsvcs.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources( COMSVCS_hInstance );
}

/***********************************************************************
 *		DllUnregisterServer (comsvcs.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources( COMSVCS_hInstance );
}
