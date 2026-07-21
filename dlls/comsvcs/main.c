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

#include "comsvcs_private.h"
#include "wine/debug.h"
#include "wine/list.h"
#include "initguid.h"
#include "comsvcs_classes.h"

WINE_DEFAULT_DEBUG_CHANNEL(comsvcs);

static IDispenserManager *manager;

typedef struct dispensermanager
{
    IDispenserManager IDispenserManager_iface;
    LONG ref;

    CRITICAL_SECTION cs;
    HANDLE thread;              /* pool purging thread */
    HANDLE quit_event;          /* purging thread quit event */
    struct list holders;
} dispensermanager;

#define RESOURCE_IN_USE 0
typedef struct
{
    struct list entry;
    RESID resid;
    TIMEINSECS ttl;
    ULONGLONG timestamp;
} resource;

typedef struct holder
{
    IHolder IHolder_iface;
    LONG ref;

    CRITICAL_SECTION cs;
    IDispenserManager *manager;
    struct list entry;
    IDispenserDriver *driver;
    struct list pool;
} holder;

struct new_moniker
{
    IMoniker IMoniker_iface;
    IROTData IROTData_iface;
    LONG refcount;
    CLSID clsid;
    WCHAR *progid;
};

static HRESULT new_moniker_parse_displayname(IBindCtx *pbc, LPOLESTR name, ULONG *eaten, IMoniker **ret);

static inline dispensermanager *impl_from_IDispenserManager(IDispenserManager *iface)
{
    return CONTAINING_RECORD(iface, dispensermanager, IDispenserManager_iface);
}

static inline holder *impl_from_IHolder(IHolder *iface)
{
    return CONTAINING_RECORD(iface, holder, IHolder_iface);
}

static struct new_moniker *impl_from_IMoniker(IMoniker *iface)
{
    return CONTAINING_RECORD(iface, struct new_moniker, IMoniker_iface);
}

static struct new_moniker *impl_from_IROTData(IROTData *iface)
{
    return CONTAINING_RECORD(iface, struct new_moniker, IROTData_iface);
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
    TRACE("(%p)->(%ld)\n", This, ref);
    return ref;
}

static ULONG WINAPI holder_Release(IHolder *iface)
{
    holder *This = impl_from_IHolder(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p)->(%ld)\n", This, ref);

    if (!ref)
    {
        DeleteCriticalSection(&This->cs);
        if (!list_empty(&This->pool)) ERR("pool is not empty\n");
        free(This);
    }

    return ref;
}

static HRESULT WINAPI holder_AllocResource(IHolder *iface, const RESTYPID typeid, RESID *resid)
{
    holder *This = impl_from_IHolder(iface);
    RESOURCERATING best_rating = 0, rating;
    resource *res, *best = NULL;
    IDispenserDriver *driver;
    HRESULT hr;

    TRACE("(%p)->(%08Ix, %p) semi-stub\n", This, typeid, resid);

    /* TODO: add transactions support */

    EnterCriticalSection(&This->cs);
    if (!This->driver)
    {
        LeaveCriticalSection(&This->cs);
        return E_UNEXPECTED;
    }

    LIST_FOR_EACH_ENTRY(res, &This->pool, resource, entry)
    {
        if (res->timestamp == RESOURCE_IN_USE) continue;
        hr = IDispenserDriver_RateResource(This->driver, typeid, res->resid, FALSE, &rating);
        if (SUCCEEDED(hr) && rating && rating > best_rating)
        {
            best_rating = rating;
            best = res;

            if (rating >= 100)
                break;
        }
    }
    if (best)
    {
        best->timestamp = RESOURCE_IN_USE;
        *resid = best->resid;
        LeaveCriticalSection(&This->cs);
        return S_OK;
    }

    driver = This->driver;
    IDispenserDriver_AddRef(driver);
    LeaveCriticalSection(&This->cs);

    if (!(res = malloc(sizeof(*res))))
    {
        IDispenserDriver_Release(driver);
        return E_OUTOFMEMORY;
    }
    hr = IDispenserDriver_CreateResource(driver, typeid, &res->resid, &res->ttl);
    TRACE(" creating new resource returned 0x%08lx\n", hr);
    if (FAILED(hr))
    {
        IDispenserDriver_Release(driver);
        free(res);
        return hr;
    }

    EnterCriticalSection(&This->cs);
    if (!This->driver)
    {
        IDispenserDriver_DestroyResource(driver, res->resid);
        free(resid);
        hr = E_UNEXPECTED;
    }
    else
    {
        list_add_tail(&This->pool, &res->entry);
        res->timestamp = RESOURCE_IN_USE;
        *resid = res->resid;
    }
    LeaveCriticalSection(&This->cs);
    IDispenserDriver_Release(driver);
    return hr;
}

static HRESULT WINAPI holder_FreeResource(IHolder *iface, const RESID resid)
{
    holder *This = impl_from_IHolder(iface);
    resource *res;
    HRESULT hr;

    TRACE("(%p)->(%08Ix)\n", This, resid);

    EnterCriticalSection(&This->cs);
    if (!This->driver)
    {
        LeaveCriticalSection(&This->cs);
        return S_OK;
    }

    LIST_FOR_EACH_ENTRY(res, &This->pool, resource, entry)
    {
        if (res->resid != resid) continue;
        if (res->timestamp != RESOURCE_IN_USE)
            ERR("resource found in pool\n");

        hr = IDispenserDriver_ResetResource(This->driver, resid);
        if (FAILED(hr))
        {
            LeaveCriticalSection(&This->cs);
            return hr;
        }

        res->timestamp = GetTickCount64();
        list_remove(&res->entry);
        list_add_head(&This->pool, &res->entry);
        LeaveCriticalSection(&This->cs);
        return hr;
    }
    LeaveCriticalSection(&This->cs);

    ERR("resource not found\n");
    return E_INVALIDARG;
}

static HRESULT WINAPI holder_TrackResource(IHolder *iface, const RESID resid)
{
    holder *This = impl_from_IHolder(iface);

    FIXME("(%p)->(%08Ix) stub\n", This, resid);

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

    FIXME("(%p)->(%08Ix, %d) stub\n", This, resid, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI holder_UntrackResourceS(IHolder *iface, const SRESID resid, const BOOL value)
{
    holder *This = impl_from_IHolder(iface);

    FIXME("(%p)->(%s, %d) stub\n", This, debugstr_w(resid), value);

    return E_NOTIMPL;
}

static void unregister_dispenser(IDispenserManager *iface, holder *hold)
{
    dispensermanager *manager = impl_from_IDispenserManager(iface);
    HANDLE thread = NULL;
    HANDLE quit_event;

    EnterCriticalSection(&manager->cs);
    list_remove(&hold->entry);
    if (list_empty(&manager->holders))
    {
        thread = manager->thread;
        manager->thread = NULL;
        quit_event = manager->quit_event;
        manager->quit_event = NULL;
    }
    LeaveCriticalSection(&manager->cs);
    IHolder_Release(&hold->IHolder_iface);

    if (thread)
    {
        SetEvent(quit_event);
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
        CloseHandle(quit_event);
    }
}

static HRESULT WINAPI holder_Close(IHolder *iface)
{
    holder *This = impl_from_IHolder(iface);
    struct list pool = LIST_INIT(pool);
    IDispenserDriver *driver = NULL;
    IDispenserManager *manager;
    resource *res, *tmp;

    TRACE("(%p)\n", This);

    EnterCriticalSection(&This->cs);
    if (This->driver)
    {
        list_move_head(&pool, &This->pool);
        driver = This->driver;
        This->driver = NULL;
        manager = This->manager;
        This->manager = NULL;
    }
    LeaveCriticalSection(&This->cs);
    if (!driver) return S_OK;

    LIST_FOR_EACH_ENTRY_SAFE(res, tmp, &pool, resource, entry)
    {
        if (res->timestamp != RESOURCE_IN_USE)
            IDispenserDriver_DestroyResource(driver, res->resid);
        list_remove(&res->entry);
        free(res);
    }

    IDispenserDriver_Release(driver);
    unregister_dispenser(manager, This);
    IDispenserManager_Release(manager);
    return S_OK;
}

static HRESULT WINAPI holder_RequestDestroyResource(IHolder *iface, const RESID resid)
{
    holder *This = impl_from_IHolder(iface);

    FIXME("(%p)->(%08Ix) stub\n", This, resid);

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

static HRESULT create_holder(IDispenserManager *manager, IDispenserDriver *driver, holder **hold)
{
    *hold = malloc(sizeof(**hold));
    if (!*hold)
        return E_OUTOFMEMORY;

    (*hold)->IHolder_iface.lpVtbl = &holder_vtbl;
    (*hold)->ref = 1;
    InitializeCriticalSection(&(*hold)->cs);
    (*hold)->manager = manager;
    IDispenserManager_AddRef(manager);
    (*hold)->driver = driver;
    list_init(&(*hold)->pool);
    return S_OK;
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
    TRACE("(%p)->(%ld)\n", This, ref);
    return ref;
}

static ULONG WINAPI dismanager_Release(IDispenserManager *iface)
{
    dispensermanager *This = impl_from_IDispenserManager(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p)->(%ld)\n", This, ref);

    if (!ref)
    {
        DeleteCriticalSection(&This->cs);
        if (!list_empty(&This->holders))
            ERR("holders list is not empty\n");
        free(This);
    }

    return ref;
}

static DWORD WINAPI purge_expired_resources(void *arg)
{
    dispensermanager *manager = arg;
    resource *res, *tmp;
    ULONGLONG ticks;
    holder *hold;
    HRESULT com_init;

    com_init = CoInitializeEx(0, COINIT_MULTITHREADED);

    while(1)
    {
        DWORD r = WaitForSingleObject(manager->quit_event, 20000);
        if (r == WAIT_OBJECT_0) break;

        ticks = GetTickCount64();
        EnterCriticalSection(&manager->cs);
        LIST_FOR_EACH_ENTRY(hold, &manager->holders, holder, entry)
        {
            EnterCriticalSection(&hold->cs);
            LIST_FOR_EACH_ENTRY_SAFE(res, tmp, &hold->pool, resource, entry)
            {
                if (res->timestamp == RESOURCE_IN_USE) break;
                if (!res->ttl) continue;
                if (res->timestamp + res->ttl * 1000 > ticks) continue;

                IDispenserDriver_DestroyResource(hold->driver, res->resid);
                list_remove(&res->entry);
                free(res);
            }
            LeaveCriticalSection(&hold->cs);
        }
        LeaveCriticalSection(&manager->cs);
    }

    IDispenserManager_Release(&manager->IDispenserManager_iface);
    if (SUCCEEDED(com_init))
        CoUninitialize();
    return 0;
}

static HRESULT WINAPI dismanager_RegisterDispenser(IDispenserManager *iface, IDispenserDriver *driver,
                        LPCOLESTR name, IHolder **dispenser)
{
    dispensermanager *This = impl_from_IDispenserManager(iface);
    holder *hold;
    HRESULT hr;

    TRACE("(%p)->(%p, %s, %p)\n", This, driver, debugstr_w(name), dispenser);

    if(!dispenser)
        return E_INVALIDARG;
    *dispenser = NULL;

    if (FAILED((hr = create_holder(iface, driver, &hold))))
        return hr;
    EnterCriticalSection(&This->cs);
    list_add_tail(&This->holders, &hold->entry);

    if (!This->thread)
    {
        This->quit_event = CreateEventA(NULL, FALSE, FALSE, NULL);
        if (This->quit_event)
           This->thread = CreateThread(NULL, 0, purge_expired_resources, This, 0, NULL);
        if (This->thread)
        {
            IDispenserManager_AddRef(&This->IDispenserManager_iface);
        }
        else
        {
            if (This->quit_event)
            {
                CloseHandle(This->quit_event);
                This->quit_event = NULL;
            }
        }
    }
    LeaveCriticalSection(&This->cs);

    *dispenser = &hold->IHolder_iface;
    IHolder_AddRef(*dispenser);
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

static BOOL WINAPI create_dispenser_manager(INIT_ONCE *once, void *param, void **context)
{
    dispensermanager *dismanager;

    dismanager = calloc(1, sizeof(*dismanager));
    if (!dismanager)
        return FALSE;

    dismanager->IDispenserManager_iface.lpVtbl = &dismanager_vtbl;
    dismanager->ref = 1;
    InitializeCriticalSection(&dismanager->cs);
    list_init(&dismanager->holders);

    manager = &dismanager->IDispenserManager_iface;
    return TRUE;
}

static HRESULT WINAPI dispenser_manager_cf_CreateInstance(IClassFactory *iface, IUnknown* outer, REFIID riid,
        void **object)
{
    static INIT_ONCE init_once = INIT_ONCE_STATIC_INIT;

    TRACE("(%p %s %p)\n", outer, debugstr_guid(riid), object);

    *object = NULL;
    if (!InitOnceExecuteOnce(&init_once, create_dispenser_manager, NULL, NULL))
        return E_OUTOFMEMORY;

    return IDispenserManager_QueryInterface(manager, riid, object);
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

static const IClassFactoryVtbl comsvcscf_vtbl =
{
    comsvcscf_QueryInterface,
    comsvcscf_AddRef,
    comsvcscf_Release,
    dispenser_manager_cf_CreateInstance,
    comsvcscf_LockServer
};

static HRESULT WINAPI new_moniker_QueryInterface(IMoniker* iface, REFIID riid, void **obj)
{
    struct new_moniker *moniker = impl_from_IMoniker(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualIID(&IID_IUnknown, riid) ||
        IsEqualIID(&IID_IPersist, riid) ||
        IsEqualIID(&IID_IPersistStream, riid) ||
        IsEqualIID(&IID_IMoniker, riid))
    {
        *obj = iface;
    }
    else if (IsEqualIID(&IID_IROTData, riid))
    {
        *obj = &moniker->IROTData_iface;
    }

    if (*obj)
    {
        IUnknown_AddRef((IUnknown *)*obj);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI new_moniker_AddRef(IMoniker* iface)
{
    struct new_moniker *moniker = impl_from_IMoniker(iface);
    ULONG refcount = InterlockedIncrement(&moniker->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI new_moniker_Release(IMoniker* iface)
{
    struct new_moniker *moniker = impl_from_IMoniker(iface);
    ULONG refcount = InterlockedDecrement(&moniker->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        free(moniker->progid);
        free(moniker);
    }

    return refcount;
}

static HRESULT WINAPI new_moniker_GetClassID(IMoniker *iface, CLSID *clsid)
{
    TRACE("%p, %p.\n", iface, clsid);

    if (!clsid)
        return E_POINTER;

    *clsid = CLSID_NewMoniker;

    return S_OK;
}

static HRESULT WINAPI new_moniker_IsDirty(IMoniker* iface)
{
    TRACE("%p.\n", iface);

    return S_FALSE;
}

static HRESULT WINAPI new_moniker_Load(IMoniker *iface, IStream *stream)
{
    struct new_moniker *moniker = impl_from_IMoniker(iface);
    DWORD progid_len = 0, len, pad = ~0u;
    WCHAR *progid = NULL;
    CLSID clsid;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, stream);

    hr = IStream_Read(stream, &clsid, sizeof(clsid), &len);
    if (FAILED(hr))
        return hr;

    if (SUCCEEDED(hr))
        hr = IStream_Read(stream, &progid_len, sizeof(progid_len), &len);

    if (SUCCEEDED(hr) && progid_len)
    {
        if (!(progid = malloc(progid_len)))
           return E_OUTOFMEMORY;
        hr = IStream_Read(stream, progid, progid_len, &len);
    }

    /* Skip terminator. */
    if (SUCCEEDED(hr))
        hr = IStream_Read(stream, &pad, sizeof(pad), &len);

    if (SUCCEEDED(hr) && pad == 0)
    {
        moniker->clsid = clsid;
        free(moniker->progid);
        moniker->progid = progid;
        progid = NULL;
    }

    free(progid);

    return hr;
}

static HRESULT WINAPI new_moniker_Save(IMoniker *iface, IStream *stream, BOOL clear_dirty)
{
    struct new_moniker *moniker = impl_from_IMoniker(iface);
    ULONG written, pad = 0, progid_len = 0;
    HRESULT hr;

    TRACE("%p, %p, %d.\n", iface, stream, clear_dirty);

    if (moniker->progid)
        progid_len = lstrlenW(moniker->progid) * sizeof(WCHAR);

    hr = IStream_Write(stream, &moniker->clsid, sizeof(moniker->clsid), &written);
    if (SUCCEEDED(hr))
        hr = IStream_Write(stream, &progid_len, sizeof(progid_len), &written);
    if (SUCCEEDED(hr) && progid_len)
        hr = IStream_Write(stream, moniker->progid, progid_len, &written);
    if (SUCCEEDED(hr))
        hr = IStream_Write(stream, &pad, sizeof(pad), &written);

    return hr;
}

static HRESULT WINAPI new_moniker_GetSizeMax(IMoniker *iface, ULARGE_INTEGER *size)
{
    struct new_moniker *moniker = impl_from_IMoniker(iface);

    TRACE("%p, %p.\n", iface, size);

    if (!size)
        return E_POINTER;

    size->QuadPart = sizeof(CLSID) + 2 * sizeof(DWORD);
    if (moniker->progid)
        size->QuadPart += lstrlenW(moniker->progid) * sizeof(WCHAR);

    return S_OK;
}

static HRESULT WINAPI new_moniker_BindToObject(IMoniker *iface, IBindCtx *pbc, IMoniker *pmkToLeft,
        REFIID riid, void **ret)
{
    struct new_moniker *moniker = impl_from_IMoniker(iface);
    IClassActivator *activator;
    IClassFactory *factory;
    BIND_OPTS2 bindopts;
    MULTI_QI qi;
    HRESULT hr;

    TRACE("%p, %p, %p, %s, %p.\n", iface, pbc, pmkToLeft, debugstr_guid(riid), ret);

    bindopts.cbStruct = sizeof(bindopts);
    if (FAILED(hr = IBindCtx_GetBindOptions(pbc, (BIND_OPTS *)&bindopts)))
        return hr;

    if (!pmkToLeft)
    {
        qi.pIID = riid;
        qi.pItf = NULL;
        qi.hr = S_OK;
        hr = CoCreateInstanceEx(&moniker->clsid, NULL, bindopts.dwClassContext, bindopts.pServerInfo, 1, &qi);
        *ret = qi.pItf;
    }
    else
    {
        if (SUCCEEDED(hr = IMoniker_BindToObject(pmkToLeft, pbc, NULL, &IID_IClassActivator, (void **)&activator)))
        {
            hr = IClassActivator_GetClassObject(activator, &moniker->clsid, bindopts.dwClassContext, bindopts.locale, riid, ret);
            IClassActivator_Release(activator);
        }
        else if (SUCCEEDED(hr = IMoniker_BindToObject(pmkToLeft, pbc, NULL, &IID_IClassFactory, (void **)&factory)))
        {
            hr = IClassFactory_CreateInstance(factory, NULL, riid, ret);
            IClassFactory_Release(factory);
        }
    }

    return hr;
}

static HRESULT WINAPI new_moniker_BindToStorage(IMoniker *iface, IBindCtx *pbc, IMoniker *pmkToLeft, REFIID riid,
        void **ret)
{
    FIXME("%p, %p, %p, %s, %p.\n", iface, pbc, pmkToLeft, debugstr_guid(riid), ret);

    return E_NOTIMPL;
}

static HRESULT WINAPI new_moniker_Reduce(IMoniker *iface, IBindCtx *pbc, DWORD flags, IMoniker **ppmkToLeft,
        IMoniker **ret)
{
    TRACE("%p, %p, %ld, %p, %p.\n", iface, pbc, flags, ppmkToLeft, ret);

    if (!ret)
        return E_POINTER;

    *ret = iface;
    IMoniker_AddRef(iface);

    return MK_S_REDUCED_TO_SELF;
}

static HRESULT WINAPI new_moniker_ComposeWith(IMoniker *iface, IMoniker *mkRight, BOOL fOnlyIfNotGeneric,
        IMoniker **ret)
{
    FIXME("%p, %p, %d, %p.\n", iface, mkRight, fOnlyIfNotGeneric, ret);

    return E_NOTIMPL;
}

static HRESULT WINAPI new_moniker_Enum(IMoniker *iface, BOOL forward, IEnumMoniker **enum_moniker)
{
    TRACE("%p, %d, %p.\n", iface, forward, enum_moniker);

    if (!enum_moniker)
        return E_POINTER;

    *enum_moniker = NULL;

    return S_OK;
}

static HRESULT WINAPI new_moniker_IsEqual(IMoniker *iface, IMoniker *other_moniker)
{
    FIXME("%p, %p.\n", iface, other_moniker);

    return E_NOTIMPL;
}

static HRESULT WINAPI new_moniker_Hash(IMoniker *iface, DWORD *hash)
{
    struct new_moniker *moniker = impl_from_IMoniker(iface);

    TRACE("%p, %p.\n", iface, hash);

    *hash = moniker->clsid.Data1;

    return S_OK;
}

static HRESULT WINAPI new_moniker_IsRunning(IMoniker* iface, IBindCtx *pbc, IMoniker *pmkToLeft,
        IMoniker *pmkNewlyRunning)
{
    FIXME("%p, %p, %p, %p.\n", iface, pbc, pmkToLeft, pmkNewlyRunning);

    return E_NOTIMPL;
}

static HRESULT WINAPI new_moniker_GetTimeOfLastChange(IMoniker *iface, IBindCtx *pbc, IMoniker *pmkToLeft,
        FILETIME *itemtime)
{
    TRACE("%p, %p, %p, %p.\n", iface, pbc, pmkToLeft, itemtime);

    return MK_E_UNAVAILABLE;
}

static HRESULT WINAPI new_moniker_Inverse(IMoniker *iface, IMoniker **inverse)
{
    TRACE("%p, %p.\n", iface, inverse);

    return CreateAntiMoniker(inverse);
}

static HRESULT WINAPI new_moniker_CommonPrefixWith(IMoniker *iface, IMoniker *other, IMoniker **ret)
{
    FIXME("%p, %p, %p.\n", iface, other, ret);

    return E_NOTIMPL;
}

static HRESULT WINAPI new_moniker_RelativePathTo(IMoniker *iface, IMoniker *other, IMoniker **ret)
{
    FIXME("%p, %p, %p.\n", iface, other, ret);

    return E_NOTIMPL;
}

static HRESULT WINAPI new_moniker_GetDisplayName(IMoniker *iface, IBindCtx *pbc, IMoniker *pmkToLeft,
        LPOLESTR *name)
{
    FIXME("%p, %p, %p, %p.\n", iface, pbc, pmkToLeft, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI new_moniker_ParseDisplayName(IMoniker *iface, IBindCtx *pbc, IMoniker *pmkToLeft,
        LPOLESTR name, ULONG *eaten, IMoniker **ret)
{
    TRACE("%p, %p, %p, %s, %p, %p.\n", iface, pbc, pmkToLeft, debugstr_w(name), eaten, ret);

    return new_moniker_parse_displayname(pbc, name, eaten, ret);
}

static HRESULT WINAPI new_moniker_IsSystemMoniker(IMoniker *iface, DWORD *moniker_type)
{
    TRACE("%p, %p.\n", iface, moniker_type);

    *moniker_type = MKSYS_NONE;

    return S_FALSE;
}

static const IMonikerVtbl new_moniker_vtbl =
{
    new_moniker_QueryInterface,
    new_moniker_AddRef,
    new_moniker_Release,
    new_moniker_GetClassID,
    new_moniker_IsDirty,
    new_moniker_Load,
    new_moniker_Save,
    new_moniker_GetSizeMax,
    new_moniker_BindToObject,
    new_moniker_BindToStorage,
    new_moniker_Reduce,
    new_moniker_ComposeWith,
    new_moniker_Enum,
    new_moniker_IsEqual,
    new_moniker_Hash,
    new_moniker_IsRunning,
    new_moniker_GetTimeOfLastChange,
    new_moniker_Inverse,
    new_moniker_CommonPrefixWith,
    new_moniker_RelativePathTo,
    new_moniker_GetDisplayName,
    new_moniker_ParseDisplayName,
    new_moniker_IsSystemMoniker
};

static HRESULT WINAPI new_moniker_rotdata_QueryInterface(IROTData *iface, REFIID riid, void **obj)
{
    struct new_moniker *moniker = impl_from_IROTData(iface);
    return IMoniker_QueryInterface(&moniker->IMoniker_iface, riid, obj);
}

static ULONG WINAPI new_moniker_rotdata_AddRef(IROTData *iface)
{
    struct new_moniker *moniker = impl_from_IROTData(iface);
    return IMoniker_AddRef(&moniker->IMoniker_iface);
}

static ULONG WINAPI new_moniker_rotdata_Release(IROTData *iface)
{
    struct new_moniker *moniker = impl_from_IROTData(iface);
    return IMoniker_Release(&moniker->IMoniker_iface);
}

static HRESULT WINAPI new_moniker_rotdata_GetComparisonData(IROTData *iface, byte *data, ULONG data_len, ULONG *length)
{
    FIXME("%p, %p, %lu, %p.\n", iface, data, data_len, length);

    return E_NOTIMPL;
}

static const IROTDataVtbl new_moniker_rotdata_vtbl =
{
    new_moniker_rotdata_QueryInterface,
    new_moniker_rotdata_AddRef,
    new_moniker_rotdata_Release,
    new_moniker_rotdata_GetComparisonData,
};

static const BYTE guid_conv_table[256] =
{
    0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x00 */
    0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x10 */
    0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x20 */
    0,   1,   2,   3,   4,   5,   6, 7, 8, 9, 0, 0, 0, 0, 0, 0, /* 0x30 */
    0, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x40 */
    0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x50 */
    0, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf                             /* 0x60 */
};

static BOOL is_valid_hex(WCHAR c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
        (c >= 'A' && c <= 'F');
}

static HRESULT guid_from_string(const WCHAR *s, GUID *ret)
{
    BOOL has_brackets;
    GUID guid = { 0 };
    int i;

    memset(ret, 0, sizeof(*ret));

    /* Curly brackets are optional. */
    has_brackets = s[0] == '{';

    if (has_brackets)
        s++;

    for (i = 0; i < 8; i++)
    {
        if (!is_valid_hex(s[i])) return FALSE;
        guid.Data1 = (guid.Data1 << 4) | guid_conv_table[s[i]];
    }
    s += 8;

    if (s[0] != '-') return FALSE;
    s++;

    for (i = 0; i < 4; i++)
    {
        if (!is_valid_hex(s[0])) return FALSE;
        guid.Data2 = (guid.Data2 << 4) | guid_conv_table[s[i]];
    }
    s += 4;

    if (s[0] != '-') return FALSE;
    s++;

    for (i = 0; i < 4; i++)
    {
        if (!is_valid_hex(s[i])) return FALSE;
        guid.Data3 = (guid.Data3 << 4) | guid_conv_table[s[i]];
    }
    s += 4;

    if (s[0] != '-') return FALSE;
    s++;

    for (i = 0; i < 17; i += 2)
    {
        if (i == 4)
        {
            if (s[i] != '-') return FALSE;
            i++;
        }
        if (!is_valid_hex(s[i]) || !is_valid_hex(s[i+1])) return FALSE;
        guid.Data4[i / 2] = guid_conv_table[s[i]] << 4 | guid_conv_table[s[i+1]];
    }
    s += 17;

    if (has_brackets && s[0] != '}')
        return FALSE;

    *ret = guid;

    return TRUE;
}

static HRESULT new_moniker_parse_displayname(IBindCtx *pbc, LPOLESTR name, ULONG *eaten, IMoniker **ret)
{
    struct new_moniker *moniker;
    WCHAR *progid = NULL, *str;
    GUID guid;

    *ret = NULL;

    if (wcsnicmp(name, L"new:", 4))
        return MK_E_SYNTAX;
    str = name + 4;

    if (!guid_from_string(str, &guid))
    {
        if (FAILED(CLSIDFromProgID(str, &guid)))
            return MK_E_SYNTAX;
        progid = str;
    }

    moniker = calloc(1, sizeof(*moniker));
    if (!moniker)
        return E_OUTOFMEMORY;

    moniker->IMoniker_iface.lpVtbl = &new_moniker_vtbl;
    moniker->IROTData_iface.lpVtbl = &new_moniker_rotdata_vtbl;
    moniker->refcount = 1;
    moniker->clsid = guid;
    if (progid)
    {
        if (!(moniker->progid = wcsdup(progid)))
        {
            IMoniker_Release(&moniker->IMoniker_iface);
            return E_OUTOFMEMORY;
        }
    }

    *ret = &moniker->IMoniker_iface;

    if (eaten)
        *eaten = lstrlenW(name);

    return S_OK;
}

static HRESULT WINAPI new_moniker_parse_QueryInterface(IParseDisplayName *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IParseDisplayName) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IParseDisplayName_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI new_moniker_parse_AddRef(IParseDisplayName *iface)
{
    return 2;
}

static ULONG WINAPI new_moniker_parse_Release(IParseDisplayName *iface)
{
    return 1;
}

static HRESULT WINAPI new_moniker_parse_ParseDisplayName(IParseDisplayName *iface, IBindCtx *pbc, LPOLESTR name,
        ULONG *eaten, IMoniker **ret)
{
    TRACE("%p, %p, %s, %p, %p.\n", iface, pbc, debugstr_w(name), eaten, ret);

    return new_moniker_parse_displayname(pbc, name, eaten, ret);
}

static const IParseDisplayNameVtbl new_moniker_parse_vtbl =
{
    new_moniker_parse_QueryInterface,
    new_moniker_parse_AddRef,
    new_moniker_parse_Release,
    new_moniker_parse_ParseDisplayName,
};

static IParseDisplayName new_moniker_parse = { &new_moniker_parse_vtbl };

static HRESULT WINAPI new_moniker_cf_QueryInterface(IClassFactory *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualGUID(&IID_IUnknown, riid) ||
            IsEqualGUID(&IID_IClassFactory, riid))
    {
        *obj = iface;
    }
    else if (IsEqualIID(&IID_IParseDisplayName, riid))
    {
        *obj = &new_moniker_parse;
    }

    if (*obj)
    {
        IUnknown_AddRef((IUnknown *)*obj);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static HRESULT WINAPI new_moniker_cf_CreateInstance(IClassFactory *iface, IUnknown* outer, REFIID riid, void **object)
{
    TRACE("%p, %p, %s, %p.\n", iface, outer, debugstr_guid(riid), object);

    if (outer)
        FIXME("Aggregation is not supported.\n");

    return IParseDisplayName_QueryInterface(&new_moniker_parse, riid, object);
}

static const IClassFactoryVtbl newmoniker_cf_vtbl =
{
    new_moniker_cf_QueryInterface,
    comsvcscf_AddRef,
    comsvcscf_Release,
    new_moniker_cf_CreateInstance,
    comsvcscf_LockServer
};

static const IClassFactoryVtbl group_manager_cf_vtbl =
{
    comsvcscf_QueryInterface,
    comsvcscf_AddRef,
    comsvcscf_Release,
    group_manager_create,
    comsvcscf_LockServer
};

static IClassFactory DispenserManageFactory = { &comsvcscf_vtbl };
static IClassFactory NewMonikerFactory = { &newmoniker_cf_vtbl };
static IClassFactory GroupManagerFactory = { &group_manager_cf_vtbl };

/******************************************************************
 * DllGetClassObject
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&CLSID_DispenserManager, rclsid))
    {
        TRACE("(CLSID_DispenserManager %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&DispenserManageFactory, riid, ppv);
    }
    else if (IsEqualGUID(&CLSID_NewMoniker, rclsid))
    {
        TRACE("(CLSID_NewMoniker %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&NewMonikerFactory, riid, ppv);
    }
    else if (IsEqualGUID(&CLSID_SharedPropertyGroupManager, rclsid))
    {
        TRACE("(CLSID_SharedPropertyGroupManager %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&GroupManagerFactory, riid, ppv);
    }

    FIXME("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_DETACH && !reserved)
    {
        if (manager)
            IDispenserManager_Release(manager);
    }
    return TRUE;
}
