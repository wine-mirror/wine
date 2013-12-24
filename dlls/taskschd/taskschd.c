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
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "rpcproxy.h"
#include "taskschd.h"
#include "taskschd_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(taskschd);

static HINSTANCE schd_instance;

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

/******************************************************************
 *      DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE; /* prefer native version */

    case DLL_PROCESS_ATTACH:
        schd_instance = hinst;
        DisableThreadLibraryCalls(hinst);
        break;
    }
    return TRUE;
}

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

/***********************************************************************
 *      DllCanUnloadNow
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

/***********************************************************************
 *      DllRegisterServer
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources(schd_instance);
}

/***********************************************************************
 *      DllUnregisterServer
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources(schd_instance);
}

const char *debugstr_variant(const VARIANT *v)
{
    if (!v) return "(null)";

    switch (V_VT(v))
    {
    case VT_EMPTY:
        return "{VT_EMPTY}";
    case VT_NULL:
        return "{VT_NULL}";
    case VT_I2:
        return wine_dbg_sprintf("{VT_I2: %d}", V_I2(v));
    case VT_I4:
        return wine_dbg_sprintf("{VT_I4: %d}", V_I4(v));
    case VT_R8:
        return wine_dbg_sprintf("{VT_R8: %lf}", V_R8(v));
    case VT_BSTR:
        return wine_dbg_sprintf("{VT_BSTR: %s}", debugstr_w(V_BSTR(v)));
    case VT_DISPATCH:
        return wine_dbg_sprintf("{VT_DISPATCH: %p}", V_DISPATCH(v));
    case VT_ERROR:
        return wine_dbg_sprintf("{VT_ERROR: %08x}", V_ERROR(v));
    case VT_BOOL:
        return wine_dbg_sprintf("{VT_BOOL: %x}", V_BOOL(v));
    case VT_UINT:
        return wine_dbg_sprintf("{VT_UINT: %u}", V_UINT(v));
    default:
        return wine_dbg_sprintf("{vt %d}", V_VT(v));
    }
}
