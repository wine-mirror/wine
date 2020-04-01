/*
 * Copyright 2020 Dmitry Timoshkov
 *
 * This file contains only stubs to get the printui.dll up and running
 * activeds.dll is much much more than this
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
#include "iads.h"

#include "wine/heap.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(activeds);

#include "initguid.h"
DEFINE_GUID(CLSID_Pathname,0x080d0d78,0xf421,0x11d0,0xa3,0x6e,0x00,0xc0,0x4f,0xb9,0x50,0xdc);

typedef struct
{
    IADsPathname IADsPathname_iface;
    LONG ref;
    BSTR adspath;
} Pathname;

static inline Pathname *impl_from_IADsPathname(IADsPathname *iface)
{
    return CONTAINING_RECORD(iface, Pathname, IADsPathname_iface);
}

static HRESULT WINAPI path_QueryInterface(IADsPathname *iface, REFIID riid, void **obj)
{
    TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), obj);

    if (!riid || !obj) return E_INVALIDARG;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IADsPathname))
    {
        IADsPathname_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    FIXME("interface %s is not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI path_AddRef(IADsPathname *iface)
{
    Pathname *path = impl_from_IADsPathname(iface);
    return InterlockedIncrement(&path->ref);
}

static ULONG WINAPI path_Release(IADsPathname *iface)
{
    Pathname *path = impl_from_IADsPathname(iface);
    LONG ref = InterlockedDecrement(&path->ref);

    if (!ref)
    {
        TRACE("destroying %p\n", iface);
        heap_free(path);
    }

    return ref;
}

static HRESULT WINAPI path_GetTypeInfoCount(IADsPathname *iface, UINT *count)
{
    FIXME("%p,%p: stub\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_GetTypeInfo(IADsPathname *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    FIXME("%p,%u,%#x,%p: stub\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_GetIDsOfNames(IADsPathname *iface, REFIID riid, LPOLESTR *names,
                                         UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p,%s,%p,%u,%u,%p: stub\n", iface, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_Invoke(IADsPathname *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                  DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p,%d,%s,%04x,%04x,%p,%p,%p,%p: stub\n", iface, dispid, debugstr_guid(riid), lcid, flags,
          params, result, excepinfo, argerr);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_Set(IADsPathname *iface, BSTR adspath, LONG type)
{
    Pathname *path = impl_from_IADsPathname(iface);

    FIXME("%p,%s,%d: stub\n", iface, debugstr_w(adspath), type);

    if (!adspath) return E_INVALIDARG;

    path->adspath = SysAllocString(adspath);
    return path->adspath ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI path_SetDisplayType(IADsPathname *iface, LONG type)
{
    FIXME("%p,%d: stub\n", iface, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_Retrieve(IADsPathname *iface, LONG type, BSTR *adspath)
{
    Pathname *path = impl_from_IADsPathname(iface);

    FIXME("%p,%d,%p: stub\n", iface, type, adspath);

    if (!adspath) return E_INVALIDARG;

    *adspath = SysAllocString(path->adspath);
    return *adspath ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI path_GetNumElements(IADsPathname *iface, LONG *count)
{
    FIXME("%p,%p: stub\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_GetElement(IADsPathname *iface, LONG index, BSTR *element)
{
    FIXME("%p,%d,%p: stub\n", iface, index, element);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_AddLeafElement(IADsPathname *iface, BSTR element)
{
    FIXME("%p,%s: stub\n", iface, debugstr_w(element));
    return E_NOTIMPL;
}

static HRESULT WINAPI path_RemoveLeafElement(IADsPathname *iface)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_CopyPath(IADsPathname *iface, IDispatch **path)
{
    FIXME("%p,%p: stub\n", iface, path);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_GetEscapedElement(IADsPathname *iface, LONG reserved, BSTR element, BSTR *str)
{
    FIXME("%p,%d,%s,%p: stub\n", iface, reserved, debugstr_w(element), str);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_get_EscapedMode(IADsPathname *iface, LONG *mode)
{
    FIXME("%p,%p: stub\n", iface, mode);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_put_EscapedMode(IADsPathname *iface, LONG mode)
{
    FIXME("%p,%d: stub\n", iface, mode);
    return E_NOTIMPL;
}

static const IADsPathnameVtbl IADsPathname_vtbl =
{
    path_QueryInterface,
    path_AddRef,
    path_Release,
    path_GetTypeInfoCount,
    path_GetTypeInfo,
    path_GetIDsOfNames,
    path_Invoke,
    path_Set,
    path_SetDisplayType,
    path_Retrieve,
    path_GetNumElements,
    path_GetElement,
    path_AddLeafElement,
    path_RemoveLeafElement,
    path_CopyPath,
    path_GetEscapedElement,
    path_get_EscapedMode,
    path_put_EscapedMode
};

static HRESULT Pathname_create(REFIID riid, void **obj)
{
    Pathname *path;
    HRESULT hr;

    path = heap_alloc(sizeof(*path));
    if (!path) return E_OUTOFMEMORY;

    path->IADsPathname_iface.lpVtbl = &IADsPathname_vtbl;
    path->ref = 1;
    path->adspath = NULL;

    hr = IADsPathname_QueryInterface(&path->IADsPathname_iface, riid, obj);
    IADsPathname_Release(&path->IADsPathname_iface);

    return hr;
}

static const struct class_info
{
    const CLSID *clsid;
    HRESULT (*constructor)(REFIID, void **);
} class_info[] =
{
    { &CLSID_Pathname, Pathname_create }
};

typedef struct
{
    IClassFactory IClassFactory_iface;
    LONG ref;
    const struct class_info *info;
} class_factory;

static inline class_factory *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, class_factory, IClassFactory_iface);
}

static HRESULT WINAPI factory_QueryInterface(IClassFactory *iface, REFIID riid, LPVOID *obj)
{
    TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), obj);

    if (!riid || !obj) return E_INVALIDARG;

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
    class_factory *factory = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedIncrement(&factory->ref);

    TRACE("(%p) ref %u\n", iface, ref);

    return ref;
}

static ULONG WINAPI factory_Release(IClassFactory *iface)
{
    class_factory *factory = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedDecrement(&factory->ref);

    TRACE("(%p) ref %u\n", iface, ref);

    if (!ref)
        heap_free(factory);

    return ref;
}

static HRESULT WINAPI factory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **obj)
{
    class_factory *factory = impl_from_IClassFactory(iface);

    TRACE("%p,%s,%p\n", outer, debugstr_guid(riid), obj);

    if (!riid || !obj) return E_INVALIDARG;

    *obj = NULL;
    if (outer) return CLASS_E_NOAGGREGATION;

    return factory->info->constructor(riid, obj);
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

static HRESULT factory_constructor(const struct class_info *info, REFIID riid, void **obj)
{
    class_factory *factory;
    HRESULT hr;

    factory = heap_alloc(sizeof(*factory));
    if (!factory) return E_OUTOFMEMORY;

    factory->IClassFactory_iface.lpVtbl = &factory_vtbl;
    factory->ref = 1;
    factory->info = info;

    hr = IClassFactory_QueryInterface(&factory->IClassFactory_iface, riid, obj);
    IClassFactory_Release(&factory->IClassFactory_iface);

    return hr;
}

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID *obj)
{
    int i;

    TRACE("%s,%s,%p\n", debugstr_guid(clsid), debugstr_guid(iid), obj);

    if (!clsid || !iid || !obj) return E_INVALIDARG;

    *obj = NULL;

    for (i = 0; i < ARRAY_SIZE(class_info); i++)
    {
        if (IsEqualCLSID(class_info[i].clsid, clsid))
            return factory_constructor(&class_info[i], iid, obj);
    }

    FIXME("class %s/%s is not implemented\n", debugstr_guid(clsid), debugstr_guid(iid));
    return CLASS_E_CLASSNOTAVAILABLE;
}
