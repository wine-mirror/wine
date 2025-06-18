/*
 * Copyright 2017 Zebediah Figura for CodeWeavers
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
#include "initguid.h"
#include "cmnquery.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsquery);

/******************************************************************
 *      IClassFactory implementation
 */
struct query_class_factory {
    IClassFactory IClassFactory_iface;
    LONG ref;
    HRESULT (*pfnCreateInstance)(IUnknown *outer, REFIID riid, void **out);
};

static inline struct query_class_factory *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, struct query_class_factory, IClassFactory_iface);
}

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **out)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IClassFactory) || IsEqualGUID(riid, &IID_IUnknown))
    {
        IClassFactory_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    FIXME("interface %s not implemented\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    struct query_class_factory *This = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) increasing refcount to %lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    struct query_class_factory *This = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) decreasing refcount to %lu\n", iface, ref);

    if (ref == 0)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface,
        IUnknown *outer, REFIID riid, void **out)
{
    struct query_class_factory *This = impl_from_IClassFactory(iface);

    TRACE("(%p)->(%p, %s, %p)\n", iface, outer, debugstr_guid(riid), out);

    return This->pfnCreateInstance(outer, riid, out);
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL dolock)
{
    FIXME("(%p)->(%d)\n", iface, dolock);

    return S_OK;
}

static const IClassFactoryVtbl query_class_factory_vtbl =
{
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ClassFactory_CreateInstance,
    ClassFactory_LockServer,
};

/******************************************************************
 *      ICommonQuery implementation
 */
struct common_query {
    ICommonQuery ICommonQuery_iface;
    LONG ref;
};

static inline struct common_query *impl_from_ICommonQuery(ICommonQuery *iface)
{
    return CONTAINING_RECORD(iface, struct common_query, ICommonQuery_iface);
}

static HRESULT WINAPI CommonQuery_QueryInterface(ICommonQuery *iface, REFIID riid, void **out)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_ICommonQuery) || IsEqualGUID(riid, &IID_IUnknown))
    {
        ICommonQuery_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    FIXME("interface %s not implemented\n", debugstr_guid(riid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI CommonQuery_AddRef(ICommonQuery *iface)
{
    struct common_query *This = impl_from_ICommonQuery(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) increasing refcount to %lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI CommonQuery_Release(ICommonQuery *iface)
{
    struct common_query *This = impl_from_ICommonQuery(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) decreasing refcount to %lu\n", iface, ref);

    if (ref == 0)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

static HRESULT WINAPI CommonQuery_OpenQueryWindow(ICommonQuery *iface,
        HWND parent, LPOPENQUERYWINDOW query_window, IDataObject **data_object)
{
    FIXME("(%p)->(%p, %p, %p) stub!\n", iface, parent, query_window, data_object);

    return E_NOTIMPL;
}

static const ICommonQueryVtbl CommonQuery_vtbl =
{
    CommonQuery_QueryInterface,
    CommonQuery_AddRef,
    CommonQuery_Release,
    CommonQuery_OpenQueryWindow,
};

static HRESULT CommonQuery_create(IUnknown *outer, REFIID riid, void **out)
{
    struct common_query *query;

    TRACE("outer %p, riid %s, out %p\n", outer, debugstr_guid(riid), out);

    if (outer)
        return CLASS_E_NOAGGREGATION;

    if (!(query = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*query))))
        return E_OUTOFMEMORY;

    query->ICommonQuery_iface.lpVtbl = &CommonQuery_vtbl;
    return ICommonQuery_QueryInterface(&query->ICommonQuery_iface, riid, out);
}

/***********************************************************************
 *		DllGetClassObject (DSQUERY.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **out)
{
    TRACE("rclsid %s, riid %s, out %p\n", debugstr_guid(rclsid), debugstr_guid(riid), out);

    if (!IsEqualGUID( &IID_IClassFactory, riid)
        && !IsEqualGUID( &IID_IUnknown, riid))
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        *out = NULL;
        return E_NOINTERFACE;
    }

    if (IsEqualGUID(&CLSID_CommonQuery, rclsid))
    {
        struct query_class_factory *factory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*factory));
        if (!factory) return E_OUTOFMEMORY;

        factory->IClassFactory_iface.lpVtbl = &query_class_factory_vtbl;
        factory->ref = 1;
        factory->pfnCreateInstance = CommonQuery_create;
        *out = factory;
        return S_OK;
    }

    FIXME("%s: no class found\n", debugstr_guid(rclsid));
    *out = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;
}
