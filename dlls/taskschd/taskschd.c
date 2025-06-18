/*
 * Task Scheduler
 *
 * Copyright 2013 Dmitry Timoshkov
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "rpcproxy.h"
#include "taskschd.h"
#include "taskschd_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(taskschd);

typedef struct
{
    IClassFactory IClassFactory_iface;
    HRESULT (*constructor)(void **);
} TaskScheduler_factory;

static inline TaskScheduler_factory *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, TaskScheduler_factory, IClassFactory_iface);
}

static HRESULT WINAPI factory_QueryInterface(IClassFactory *iface, REFIID riid, LPVOID *obj)
{
    if (!riid || !obj) return E_INVALIDARG;

    TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IClassFactory))
    {
        IClassFactory_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    *obj = NULL;
    FIXME("interface %s is not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI factory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI factory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **obj)
{
    TaskScheduler_factory *factory = impl_from_IClassFactory(iface);
    IUnknown *unknown;
    HRESULT hr;

    if (!riid || !obj) return E_INVALIDARG;

    TRACE("%p,%s,%p\n", outer, debugstr_guid(riid), obj);

    *obj = NULL;
    if (outer) return CLASS_E_NOAGGREGATION;

    hr = factory->constructor((void **)&unknown);
    if (hr != S_OK) return hr;

    hr = IUnknown_QueryInterface(unknown, riid, obj);
    IUnknown_Release(unknown);

    return hr;
}

static HRESULT WINAPI factory_LockServer(IClassFactory *iface, BOOL lock)
{
    FIXME("%p,%d: stub\n", iface, lock);
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

static TaskScheduler_factory TaskScheduler_cf = { { &factory_vtbl }, TaskService_create };

/***********************************************************************
 *      DllGetClassObject
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *obj)
{
    IClassFactory *factory = NULL;

    if (!rclsid || !riid || !obj) return E_INVALIDARG;

    TRACE("%s,%s,%p\n", debugstr_guid(rclsid), debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualGUID(rclsid, &CLSID_TaskScheduler))
    {
        factory = &TaskScheduler_cf.IClassFactory_iface;
    }

    if (factory) return IClassFactory_QueryInterface(factory, riid, obj);

    FIXME("class %s/%s is not implemented\n", debugstr_guid(rclsid), debugstr_guid(riid));
    return CLASS_E_CLASSNOTAVAILABLE;
}
