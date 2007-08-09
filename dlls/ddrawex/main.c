/*        DirectDrawEx
 *
 * Copyright 2006 Ulrich Czekalla
 *
 * This file contains the (internal) driver registration functions,
 * driver enumeration APIs and DirectDraw creation functions.
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

#include "ddraw.h"

#include "initguid.h"
#include "ddrawex_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);


/*******************************************************************************
 * IDirectDrawClassFactory::QueryInterface
 *
 *******************************************************************************/
static HRESULT WINAPI
IDirectDrawClassFactoryImpl_QueryInterface(IClassFactory *iface,
                    REFIID riid,
                    void **obj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl*) iface;

    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IClassFactory))
    {
        IClassFactory_AddRef(iface);
        *obj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),obj);
    return E_NOINTERFACE;
}

/*******************************************************************************
 * IDirectDrawClassFactory::AddRef
 *
 *******************************************************************************/
static ULONG WINAPI
IDirectDrawClassFactoryImpl_AddRef(IClassFactory *iface)
{
    IClassFactoryImpl *This = (IClassFactoryImpl*) iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->() incrementing from %d.\n", This, ref - 1);

    return ref;
}

/*******************************************************************************
 * IDirectDrawClassFactory::Release
 *
 *******************************************************************************/
static ULONG WINAPI
IDirectDrawClassFactoryImpl_Release(IClassFactory *iface)
{
    IClassFactoryImpl *This = (IClassFactoryImpl*) iface;
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
static HRESULT WINAPI
IDirectDrawClassFactoryImpl_CreateInstance(IClassFactory *iface,
                                           IUnknown *UnkOuter,
                                           REFIID riid,
                                           void **obj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl*) iface;

    TRACE("(%p)->(%p,%s,%p)\n",This,UnkOuter,debugstr_guid(riid),obj);

    return This->pfnCreateInstance(UnkOuter, riid, obj);
}

/*******************************************************************************
 * IDirectDrawClassFactory::LockServer
 *
 *******************************************************************************/
static HRESULT WINAPI
IDirectDrawClassFactoryImpl_LockServer(IClassFactory *iface,BOOL dolock)
{
    IClassFactoryImpl *This = (IClassFactoryImpl*) iface;
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


/*******************************************************************************
 * IDirectDrawFactory::QueryInterface
 *
 *******************************************************************************/
static HRESULT WINAPI
IDirectDrawFactoryImpl_QueryInterface(IDirectDrawFactory *iface,
                    REFIID riid,
                    void **obj)
{
    IDirectDrawFactoryImpl *This = (IDirectDrawFactoryImpl*) iface;

    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirectDrawFactory))
    {
        IDirectDrawFactory_AddRef(iface);
        *obj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),obj);
    return E_NOINTERFACE;
}

/*******************************************************************************
 * IDirectDrawFactory::AddRef
 *
 *******************************************************************************/
static ULONG WINAPI
IDirectDrawFactoryImpl_AddRef(IDirectDrawFactory *iface)
{
    IDirectDrawFactoryImpl *This = (IDirectDrawFactoryImpl*) iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->() incrementing from %d.\n", This, ref - 1);

    return ref;
}

/*******************************************************************************
 * IDirectDrawFactory::Release
 *
 *******************************************************************************/
static ULONG WINAPI
IDirectDrawFactoryImpl_Release(IDirectDrawFactory *iface)
{
    IDirectDrawFactoryImpl *This = (IDirectDrawFactoryImpl*) iface;
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p)->() decrementing from %d.\n", This, ref+1);

    if (ref == 0)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}


/*******************************************************************************
 * IDirectDrawFactoryImpl_CreateDirectDraw
 *******************************************************************************/
static HRESULT WINAPI
IDirectDrawFactoryImpl_CreateDirectDraw(IDirectDrawFactory* iface,
                                        GUID * pGUID,
                                        HWND hWnd,
                                        DWORD dwCoopLevelFlags,
                                        DWORD dwReserved,
                                        IUnknown *pUnkOuter,
                                        IDirectDraw **ppDirectDraw)
{
    HRESULT hr;

    TRACE("\n");

    hr = DirectDrawCreateEx(pGUID, (void**)ppDirectDraw, &IID_IDirectDraw3, pUnkOuter);

    if (FAILED(hr))
        return hr;

    hr = IDirectDraw_SetCooperativeLevel(*ppDirectDraw, hWnd, dwCoopLevelFlags);
    if (FAILED(hr))
        IDirectDraw_Release(*ppDirectDraw);

    return hr;
}


/*******************************************************************************
 * IDirectDrawFactoryImpl_DirectDrawEnumerate
 *******************************************************************************/
static HRESULT WINAPI
IDirectDrawFactoryImpl_DirectDrawEnumerate(IDirectDrawFactory* iface,
                                           LPDDENUMCALLBACKW lpCallback,
                                           LPVOID lpContext)
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

    This->lpVtbl = &IDirectDrawFactory_Vtbl;

    hr = IDirectDrawFactory_QueryInterface((IDirectDrawFactory *)This, iid,  obj);

    if (FAILED(hr))
        HeapFree(GetProcessHeap(), 0, This);

    return hr;
}


/*******************************************************************************
 * DllCanUnloadNow [DDRAWEX.@]  Determines whether the DLL is in use.
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    FIXME("(void): stub\n");
    return S_FALSE;
}


/*******************************************************************************
 * DllGetClassObject [DDRAWEX.@]
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    IClassFactoryImpl *factory;

    TRACE("ddrawex (%s,%s,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

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

    factory->lpVtbl = &IClassFactory_Vtbl;
    factory->ref = 1;

    factory->pfnCreateInstance = CreateDirectDrawFactory;

    *ppv = (IClassFactory*) factory;

    return S_OK;
}


/***********************************************************************
 * DllMain (DDRAWEX.0)
 *
 ***********************************************************************/
BOOL WINAPI
DllMain(HINSTANCE hInstDLL,
        DWORD Reason,
        LPVOID lpv)
{
    TRACE("(%p,%x,%p)\n", hInstDLL, Reason, lpv);
    return TRUE;
}
