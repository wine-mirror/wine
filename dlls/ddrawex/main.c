/*        DirectDrawEx
 *
 * Copyright 2006 Ulrich Czekalla
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


#include "wine/debug.h"

#define COBJMACROS

#include "winbase.h"
#include "wingdi.h"
#include "objbase.h"
#include "rpcproxy.h"

#include "ddraw.h"

#include "initguid.h"
#include "ddrawex_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddrawex);

static HINSTANCE instance;

/******************************************************************************
 * DirectDraw ClassFactory implementation
 ******************************************************************************/
typedef struct
{
    IClassFactory IClassFactory_iface;
    LONG ref;
    HRESULT (*pfnCreateInstance)(IUnknown *outer, REFIID iid, void **out);
} IClassFactoryImpl;

static inline IClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, IClassFactoryImpl, IClassFactory_iface);
}

/*******************************************************************************
 * IDirectDrawClassFactory::QueryInterface
 *
 *******************************************************************************/
static HRESULT WINAPI IDirectDrawClassFactoryImpl_QueryInterface(IClassFactory *iface, REFIID riid,
        void **obj)
{
    TRACE("(%p)->(%s,%p)\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IClassFactory))
    {
        IClassFactory_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *obj = NULL;
    return E_NOINTERFACE;
}

/*******************************************************************************
 * IDirectDrawClassFactory::AddRef
 *
 *******************************************************************************/
static ULONG WINAPI IDirectDrawClassFactoryImpl_AddRef(IClassFactory *iface)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->() incrementing from %d.\n", This, ref - 1);

    return ref;
}

/*******************************************************************************
 * IDirectDrawClassFactory::Release
 *
 *******************************************************************************/
static ULONG WINAPI IDirectDrawClassFactoryImpl_Release(IClassFactory *iface)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->() decrementing from %d.\n", This, ref+1);

    if (ref == 0)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}


/*******************************************************************************
 * IDirectDrawClassFactory::CreateInstance
 *
 *******************************************************************************/
static HRESULT WINAPI IDirectDrawClassFactoryImpl_CreateInstance(IClassFactory *iface,
        IUnknown *UnkOuter, REFIID riid, void **obj)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);

    TRACE("(%p)->(%p,%s,%p)\n",This,UnkOuter,debugstr_guid(riid),obj);

    return This->pfnCreateInstance(UnkOuter, riid, obj);
}

/*******************************************************************************
 * IDirectDrawClassFactory::LockServer
 *
 *******************************************************************************/
static HRESULT WINAPI IDirectDrawClassFactoryImpl_LockServer(IClassFactory *iface,BOOL dolock)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);

    FIXME("(%p)->(%d),stub!\n",This,dolock);

    return S_OK;
}


/*******************************************************************************
 * The class factory VTable
 *******************************************************************************/
static const IClassFactoryVtbl IClassFactory_Vtbl =
{
    IDirectDrawClassFactoryImpl_QueryInterface,
    IDirectDrawClassFactoryImpl_AddRef,
    IDirectDrawClassFactoryImpl_Release,
    IDirectDrawClassFactoryImpl_CreateInstance,
    IDirectDrawClassFactoryImpl_LockServer
};


/******************************************************************************
 * DirectDrawFactory implementation
 ******************************************************************************/
typedef struct
{
    IDirectDrawFactory IDirectDrawFactory_iface;
    LONG ref;
} IDirectDrawFactoryImpl;

static inline IDirectDrawFactoryImpl *impl_from_IDirectDrawFactory(IDirectDrawFactory *iface)
{
    return CONTAINING_RECORD(iface, IDirectDrawFactoryImpl, IDirectDrawFactory_iface);
}

/*******************************************************************************
 * IDirectDrawFactory::QueryInterface
 *
 *******************************************************************************/
static HRESULT WINAPI IDirectDrawFactoryImpl_QueryInterface(IDirectDrawFactory *iface, REFIID riid,
        void **obj)
{
    TRACE("(%p)->(%s,%p)\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirectDrawFactory))
    {
        IDirectDrawFactory_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *obj = NULL;
    return E_NOINTERFACE;
}

/*******************************************************************************
 * IDirectDrawFactory::AddRef
 *
 *******************************************************************************/
static ULONG WINAPI IDirectDrawFactoryImpl_AddRef(IDirectDrawFactory *iface)
{
    IDirectDrawFactoryImpl *This = impl_from_IDirectDrawFactory(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->() incrementing from %d.\n", This, ref - 1);

    return ref;
}

/*******************************************************************************
 * IDirectDrawFactory::Release
 *
 *******************************************************************************/
static ULONG WINAPI IDirectDrawFactoryImpl_Release(IDirectDrawFactory *iface)
{
    IDirectDrawFactoryImpl *This = impl_from_IDirectDrawFactory(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->() decrementing from %d.\n", This, ref+1);

    if (ref == 0)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

static HRESULT WINAPI IDirectDrawFactoryImpl_DirectDrawEnumerate(IDirectDrawFactory *iface,
        LPDDENUMCALLBACKW cb, void *ctx)
{
    FIXME("Stub!\n");
    return E_FAIL;
}


/*******************************************************************************
 * Direct Draw Factory VTable
 *******************************************************************************/
static const IDirectDrawFactoryVtbl IDirectDrawFactory_Vtbl =
{
    IDirectDrawFactoryImpl_QueryInterface,
    IDirectDrawFactoryImpl_AddRef,
    IDirectDrawFactoryImpl_Release,
    IDirectDrawFactoryImpl_CreateDirectDraw,
    IDirectDrawFactoryImpl_DirectDrawEnumerate,
};

/***********************************************************************
 * CreateDirectDrawFactory
 *
 ***********************************************************************/
static HRESULT
CreateDirectDrawFactory(IUnknown* UnkOuter, REFIID iid, void **obj)
{
    HRESULT hr;
    IDirectDrawFactoryImpl *This = NULL;

    TRACE("(%p,%s,%p)\n", UnkOuter, debugstr_guid(iid), obj);

    if (UnkOuter != NULL)
        return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectDrawFactoryImpl));

    if(!This)
    {
        ERR("Out of memory when creating DirectDraw\n");
        return E_OUTOFMEMORY;
    }

    This->IDirectDrawFactory_iface.lpVtbl = &IDirectDrawFactory_Vtbl;

    hr = IDirectDrawFactory_QueryInterface(&This->IDirectDrawFactory_iface, iid,  obj);

    if (FAILED(hr))
        HeapFree(GetProcessHeap(), 0, This);

    return hr;
}


/*******************************************************************************
 * DllCanUnloadNow [DDRAWEX.@]  Determines whether the DLL is in use.
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}


/*******************************************************************************
 * DllGetClassObject [DDRAWEX.@]
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **out)
{
    IClassFactoryImpl *factory;

    TRACE("rclsid %s, riid %s, out %p.\n", debugstr_guid(rclsid), debugstr_guid(riid), out);

    if (!IsEqualGUID( &IID_IClassFactory, riid)
        && !IsEqualGUID( &IID_IUnknown, riid))
        return E_NOINTERFACE;

    if (!IsEqualGUID(&CLSID_DirectDrawFactory, rclsid))
    {
        FIXME("%s: no class found.\n", debugstr_guid(rclsid));
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    factory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*factory));
    if (factory == NULL) return E_OUTOFMEMORY;

    factory->IClassFactory_iface.lpVtbl = &IClassFactory_Vtbl;
    factory->ref = 1;

    factory->pfnCreateInstance = CreateDirectDrawFactory;

    *out = factory;

    return S_OK;
}


/***********************************************************************
 * DllMain
 */
BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, void *reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        instance = inst;
        DisableThreadLibraryCalls( inst );
        break;
    }
    return TRUE;
}

/***********************************************************************
 *		DllRegisterServer (DDRAWEX.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources( instance );
}

/***********************************************************************
 *		DllUnregisterServer (DDRAWEX.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources( instance );
}
