/*
 * Copyright 2019 Alistair Leslie-Hughes
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
#include "oleidl.h"
#include "rpcproxy.h"
#include "wine/heap.h"
#include "wine/debug.h"

#include "directmanipulation.h"

WINE_DEFAULT_DEBUG_CHANNEL(manipulation);

static HINSTANCE dm_instance;

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
    TRACE("(%p, %u, %p)\n", instance, reason, reserved);

    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            dm_instance = instance;
            DisableThreadLibraryCalls(instance);
            break;
    }

    return TRUE;
}

HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources( dm_instance );
}

HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources( dm_instance );
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}


struct directmanipulation
{
    IDirectManipulationManager2 IDirectManipulationManager2_iface;
    LONG ref;
};

static inline struct directmanipulation *impl_from_IDirectManipulationManager2(IDirectManipulationManager2 *iface)
{
    return CONTAINING_RECORD(iface, struct directmanipulation, IDirectManipulationManager2_iface);
}

static HRESULT WINAPI direct_manip_QueryInterface(IDirectManipulationManager2 *iface, REFIID riid, void **ppv)
{
    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDirectManipulationManager) ||
        IsEqualGUID(riid, &IID_IDirectManipulationManager2)) {
        IUnknown_AddRef(iface);
        *ppv = iface;
        return S_OK;
    }

    FIXME("(%p)->(%s,%p),not found\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI direct_manip_AddRef(IDirectManipulationManager2 *iface)
{
    struct directmanipulation *This = impl_from_IDirectManipulationManager2(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI direct_manip_Release(IDirectManipulationManager2 *iface)
{
    struct directmanipulation *This = impl_from_IDirectManipulationManager2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref)
    {
        heap_free(This);
    }
    return ref;
}

static HRESULT WINAPI direct_manip_Activate(IDirectManipulationManager2 *iface, HWND window)
{
    struct directmanipulation *This = impl_from_IDirectManipulationManager2(iface);
    FIXME("%p, %p\n", This, window);
    return E_NOTIMPL;
}

static HRESULT WINAPI direct_manip_Deactivate(IDirectManipulationManager2 *iface, HWND window)
{
    struct directmanipulation *This = impl_from_IDirectManipulationManager2(iface);
    FIXME("%p, %p\n", This, window);
    return E_NOTIMPL;
}

static HRESULT WINAPI direct_manip_RegisterHitTestTarget(IDirectManipulationManager2 *iface, HWND window,
                HWND hittest, DIRECTMANIPULATION_HITTEST_TYPE type)
{
    struct directmanipulation *This = impl_from_IDirectManipulationManager2(iface);
    FIXME("%p, %p, %p, %d\n", This, window, hittest, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI direct_manip_ProcessInput(IDirectManipulationManager2 *iface, const MSG *msg, BOOL *handled)
{
    struct directmanipulation *This = impl_from_IDirectManipulationManager2(iface);
    FIXME("%p, %p, %p\n", This, msg, handled);
    return E_NOTIMPL;
}

static HRESULT WINAPI direct_manip_GetUpdateManager(IDirectManipulationManager2 *iface, REFIID riid, void **obj)
{
    struct directmanipulation *This = impl_from_IDirectManipulationManager2(iface);
    FIXME("%p, %s, %p\n", This, debugstr_guid(riid), obj);
    return E_NOTIMPL;
}

static HRESULT WINAPI direct_manip_CreateViewport(IDirectManipulationManager2 *iface, IDirectManipulationFrameInfoProvider *frame,
                HWND window, REFIID riid, void **obj)
{
    struct directmanipulation *This = impl_from_IDirectManipulationManager2(iface);
    FIXME("%p, %p, %p, %s, %p\n", This, frame, window, debugstr_guid(riid), obj);
    return E_NOTIMPL;
}

static HRESULT WINAPI direct_manip_CreateContent(IDirectManipulationManager2 *iface, IDirectManipulationFrameInfoProvider *frame,
                REFCLSID clsid, REFIID riid, void **obj)
{
    struct directmanipulation *This = impl_from_IDirectManipulationManager2(iface);
    FIXME("%p, %p,  %s, %p\n", This, frame, debugstr_guid(riid), obj);
    return E_NOTIMPL;
}

static HRESULT WINAPI direct_manip_CreateBehavior(IDirectManipulationManager2 *iface, REFCLSID clsid, REFIID riid, void **obj)
{
    struct directmanipulation *This = impl_from_IDirectManipulationManager2(iface);
    FIXME("%p, %s,  %s, %p\n", This, debugstr_guid(clsid), debugstr_guid(riid), obj);
    return E_NOTIMPL;
}

static const struct IDirectManipulationManager2Vtbl directmanipVtbl =
{
    direct_manip_QueryInterface,
    direct_manip_AddRef,
    direct_manip_Release,
    direct_manip_Activate,
    direct_manip_Deactivate,
    direct_manip_RegisterHitTestTarget,
    direct_manip_ProcessInput,
    direct_manip_GetUpdateManager,
    direct_manip_CreateViewport,
    direct_manip_CreateContent,
    direct_manip_CreateBehavior
};

static HRESULT WINAPI DirectManipulation_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    struct directmanipulation *object;
    HRESULT ret;

    TRACE("(%p, %s, %p)\n", outer, debugstr_guid(riid), ppv);

    *ppv = NULL;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IDirectManipulationManager2_iface.lpVtbl = &directmanipVtbl;
    object->ref = 1;

    ret = direct_manip_QueryInterface(&object->IDirectManipulationManager2_iface, riid, ppv);
    direct_manip_Release(&object->IDirectManipulationManager2_iface);

    return ret;
}

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IClassFactory, riid)) {
        *ppv = iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    TRACE("(%p)\n", iface);
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    TRACE("(%p)\n", iface);
    return 1;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL fLock)
{
    TRACE("(%p)->(%x)\n", iface, fLock);
    return S_OK;
}

static const IClassFactoryVtbl DirectFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    DirectManipulation_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory direct_factory = { &DirectFactoryVtbl };

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    if(IsEqualGUID(&CLSID_DirectManipulationManager, rclsid) ||
       IsEqualGUID(&CLSID_DirectManipulationSharedManager, rclsid) ) {
        TRACE("(CLSID_DirectManipulationManager %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&direct_factory, riid, ppv);
    }

    FIXME("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}
