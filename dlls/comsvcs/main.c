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
    HANDLE mta_thread, mta_stop_event;
} dispensermanager;

typedef struct holder
{
    IHolder IHolder_iface;
    LONG ref;

    IDispenserDriver *driver;
} holder;

static inline dispensermanager *impl_from_IDispenserManager(IDispenserManager *iface)
{
    return CONTAINING_RECORD(iface, dispensermanager, IDispenserManager_iface);
}

static inline holder *impl_from_IHolder(IHolder *iface)
{
    return CONTAINING_RECORD(iface, holder, IHolder_iface);
}

static HRESULT WINAPI holder_QueryInterface(IHolder *iface, REFIID riid, void **object)
{
    holder *This = impl_from_IHolder(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), object);

    *object = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IHolder))
    {
        *object = &This->IHolder_iface;
        IUnknown_AddRef( (IUnknown*)*object);

        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),object);
    return E_NOINTERFACE;
}

static ULONG WINAPI holder_AddRef(IHolder *iface)
{
    holder *This = impl_from_IHolder(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI holder_Release(IHolder *iface)
{
    holder *This = impl_from_IHolder(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
    {
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI holder_AllocResource(IHolder *iface, const RESTYPID typeid, RESID *resid)
{
    holder *This = impl_from_IHolder(iface);
    HRESULT hr;
    TIMEINSECS secs;

    TRACE("(%p)->(%08lx, %p) stub\n", This, typeid, resid);

    hr = IDispenserDriver_CreateResource(This->driver, typeid, resid, &secs);

    TRACE("<- 0x%08x\n", hr);
    return hr;
}

static HRESULT WINAPI holder_FreeResource(IHolder *iface, const RESID resid)
{
    holder *This = impl_from_IHolder(iface);
    HRESULT hr;

    TRACE("(%p)->(%08lx) stub\n", This, resid);

    hr = IDispenserDriver_DestroyResource(This->driver, resid);

    TRACE("<- 0x%08x\n", hr);

    return hr;
}

static HRESULT WINAPI holder_TrackResource(IHolder *iface, const RESID resid)
{
    holder *This = impl_from_IHolder(iface);

    FIXME("(%p)->(%08lx) stub\n", This, resid);

    return E_NOTIMPL;
}

static HRESULT WINAPI holder_TrackResourceS(IHolder *iface, const SRESID resid)
{
    holder *This = impl_from_IHolder(iface);

    FIXME("(%p)->(%s) stub\n", This, debugstr_w(resid));

    return E_NOTIMPL;
}

static HRESULT WINAPI holder_UntrackResource(IHolder *iface, const RESID resid, const BOOL value)
{
    holder *This = impl_from_IHolder(iface);

    FIXME("(%p)->(%08lx, %d) stub\n", This, resid, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI holder_UntrackResourceS(IHolder *iface, const SRESID resid, const BOOL value)
{
    holder *This = impl_from_IHolder(iface);

    FIXME("(%p)->(%s, %d) stub\n", This, debugstr_w(resid), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI holder_Close(IHolder *iface)
{
    holder *This = impl_from_IHolder(iface);

    FIXME("(%p) stub\n", This);

    IDispenserDriver_Release(This->driver);

    return S_OK;
}

static HRESULT WINAPI holder_RequestDestroyResource(IHolder *iface, const RESID resid)
{
    holder *This = impl_from_IHolder(iface);

    FIXME("(%p)->(%08lx) stub\n", This, resid);

    return E_NOTIMPL;
}

struct IHolderVtbl holder_vtbl =
{
    holder_QueryInterface,
    holder_AddRef,
    holder_Release,
    holder_AllocResource,
    holder_FreeResource,
    holder_TrackResource,
    holder_TrackResourceS,
    holder_UntrackResource,
    holder_UntrackResourceS,
    holder_Close,
    holder_RequestDestroyResource
};

static HRESULT create_holder(IDispenserDriver *driver, IHolder **object)
{
    holder *hold;
    HRESULT ret;

    TRACE("(%p)\n", object);

    hold = heap_alloc(sizeof(*hold));
    if (!hold)
    {
        *object = NULL;
        return E_OUTOFMEMORY;
    }

    hold->IHolder_iface.lpVtbl = &holder_vtbl;
    hold->ref = 1;
    hold->driver = driver;

    ret = holder_QueryInterface(&hold->IHolder_iface, &IID_IHolder, (void**)object);
    holder_Release(&hold->IHolder_iface);

    return ret;
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
        if (This->mta_thread)
        {
            SetEvent(This->mta_stop_event);
            WaitForSingleObject(This->mta_thread, INFINITE);
            CloseHandle(This->mta_stop_event);
            CloseHandle(This->mta_thread);
        }
        heap_free(This);
    }

    return ref;
}

static DWORD WINAPI mta_thread_proc(void *arg)
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    WaitForSingleObject(arg, INFINITE);
    CoUninitialize();
    return 0;
}

static HRESULT WINAPI dismanager_RegisterDispenser(IDispenserManager *iface, IDispenserDriver *driver,
                        LPCOLESTR name, IHolder **dispenser)
{
    dispensermanager *This = impl_from_IDispenserManager(iface);
    HRESULT hr;

    TRACE("(%p)->(%p, %s, %p)\n", This, driver, debugstr_w(name), dispenser);

    if(!dispenser)
        return E_INVALIDARG;

    hr = create_holder(driver, dispenser);

    if (!This->mta_thread)
    {
        This->mta_stop_event = CreateEventA(NULL, TRUE, FALSE, NULL);
        This->mta_thread = CreateThread(NULL, 0, mta_thread_proc, This->mta_stop_event, 0, NULL);
    }

    TRACE("<-- 0x%08x, %p\n", hr, *dispenser);

    return hr;
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

    dismanager = heap_alloc_zero(sizeof(*dismanager));
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
