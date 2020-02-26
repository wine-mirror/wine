/*
 * Copyright (C) 2015 Austin English
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

#include "ole2.h"
#include "rpcproxy.h"

#include "evr_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(evr);

static HINSTANCE instance_evr;

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    TRACE("(%p, %d, %p)\n", instance, reason, reserved);

    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            instance_evr = instance;
            DisableThreadLibraryCalls(instance);
            break;
    }

    return TRUE;
}

typedef struct {
    IClassFactory IClassFactory_iface;

    LONG ref;
    HRESULT (*pfnCreateInstance)(IUnknown *unk_outer, void **ppobj);
} IClassFactoryImpl;

static inline IClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, IClassFactoryImpl, IClassFactory_iface);
}

struct object_creation_info
{
    const CLSID *clsid;
    HRESULT (*pfnCreateInstance)(IUnknown *unk_outer, void **ppobj);
};

static const struct object_creation_info object_creation[] =
{
    { &CLSID_EnhancedVideoRenderer, evr_filter_create },
};

static HRESULT WINAPI classfactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppobj)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IClassFactory))
    {
        IClassFactory_AddRef(iface);
        *ppobj = &This->IClassFactory_iface;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI classfactory_AddRef(IClassFactory *iface)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI classfactory_Release(IClassFactory *iface)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    if (ref == 0)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

static HRESULT WINAPI classfactory_CreateInstance(IClassFactory *iface, IUnknown *outer_unk, REFIID riid, void **ppobj)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    HRESULT hres;
    IUnknown *unk;

    TRACE("(%p)->(%p,%s,%p)\n", This, outer_unk, debugstr_guid(riid), ppobj);

    *ppobj = NULL;

    if (outer_unk && !IsEqualGUID(riid, &IID_IUnknown))
        return E_NOINTERFACE;

    hres = This->pfnCreateInstance(outer_unk, (void **) &unk);
    if (SUCCEEDED(hres))
    {
        hres = IUnknown_QueryInterface(unk, riid, ppobj);
        IUnknown_Release(unk);
    }
    return hres;
}

static HRESULT WINAPI classfactory_LockServer(IClassFactory *iface, BOOL dolock)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    FIXME("(%p)->(%d), stub!\n", This, dolock);
    return S_OK;
}

static const IClassFactoryVtbl classfactory_Vtbl =
{
    classfactory_QueryInterface,
    classfactory_AddRef,
    classfactory_Release,
    classfactory_CreateInstance,
    classfactory_LockServer
};

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    unsigned int i;
    IClassFactoryImpl *factory;

    TRACE("(%s,%s,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if (!IsEqualGUID(&IID_IClassFactory, riid)
         && !IsEqualGUID( &IID_IUnknown, riid))
        return E_NOINTERFACE;

    for (i = 0; i < ARRAY_SIZE(object_creation); i++)
    {
        if (IsEqualGUID(object_creation[i].clsid, rclsid))
            break;
    }

    if (i == ARRAY_SIZE(object_creation))
    {
        FIXME("%s: no class found.\n", debugstr_guid(rclsid));
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    factory = HeapAlloc(GetProcessHeap(), 0, sizeof(*factory));
    if (factory == NULL)
        return E_OUTOFMEMORY;

    factory->IClassFactory_iface.lpVtbl = &classfactory_Vtbl;
    factory->ref = 1;

    factory->pfnCreateInstance = object_creation[i].pfnCreateInstance;

    *ppv = &(factory->IClassFactory_iface);
    return S_OK;
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources(instance_evr);
}

HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources(instance_evr);
}
