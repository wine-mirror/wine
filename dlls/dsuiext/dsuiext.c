/*
 * Copyright 2020 Dmitry Timoshkov
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
#include "iads.h"
#include "dsclient.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsuiext);

typedef struct
{
    IDsDisplaySpecifier IDsDisplaySpecifier_iface;
    LONG ref;
} DisplaySpec;

static inline DisplaySpec *impl_from_IDsDisplaySpecifier(IDsDisplaySpecifier *iface)
{
    return CONTAINING_RECORD(iface, DisplaySpec, IDsDisplaySpecifier_iface);
}

static HRESULT WINAPI dispspec_QueryInterface(IDsDisplaySpecifier *iface, REFIID iid, void **obj)
{
    TRACE("%p,%s,%p\n", iface, debugstr_guid(iid), obj);

    if (!iid || !obj) return E_INVALIDARG;

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IDsDisplaySpecifier))
    {
        iface->lpVtbl->AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    FIXME("interface %s is not implemented\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI dispspec_AddRef(IDsDisplaySpecifier *iface)
{
    DisplaySpec *dispspec = impl_from_IDsDisplaySpecifier(iface);
    return InterlockedIncrement(&dispspec->ref);
}

static ULONG WINAPI dispspec_Release(IDsDisplaySpecifier *iface)
{
    DisplaySpec *dispspec = impl_from_IDsDisplaySpecifier(iface);
    LONG ref = InterlockedDecrement(&dispspec->ref);

    if (!ref)
    {
        TRACE("destroying %p\n", iface);
        free(dispspec);
    }

    return ref;
}

static HRESULT WINAPI dispspec_SetServer(IDsDisplaySpecifier *iface, LPCWSTR server, LPCWSTR user,
                                         LPCWSTR password, DWORD flags)
{
    FIXME("%p,%s,%s,%p,%08lx: stub\n", iface, debugstr_w(server), debugstr_w(user), password, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI dispspec_SetLanguageID(IDsDisplaySpecifier *iface, LANGID lang)
{
    FIXME("%p,%08x: stub\n", iface, lang);
    return E_NOTIMPL;
}

static HRESULT WINAPI dispspec_GetDisplaySpecifier(IDsDisplaySpecifier *iface, LPCWSTR object,
                                                   REFIID iid, void **obj)
{
    FIXME("%p,%s,%s,%p: stub\n", iface, debugstr_w(object), debugstr_guid(iid), obj);
    return E_NOTIMPL;
}

static HRESULT WINAPI dispspec_GetIconLocation(IDsDisplaySpecifier *iface, LPCWSTR object,
                                               DWORD flags, LPWSTR buffer, INT size, INT *id)
{
    FIXME("%p,%s,%08lx,%p,%d,%p: stub\n", iface, debugstr_w(object), flags, buffer, size, id);
    return E_NOTIMPL;
}

static HICON WINAPI dispspec_GetIcon(IDsDisplaySpecifier *iface, LPCWSTR object,
                                     DWORD flags, INT cx, INT cy)
{
    FIXME("%p,%s,%08lx,%dx%d: stub\n", iface, debugstr_w(object), flags, cx, cy);
    return 0;
}

static HRESULT WINAPI dispspec_GetFriendlyClassName(IDsDisplaySpecifier *iface, LPCWSTR object,
                                                    LPWSTR buffer, INT size)
{
    FIXME("%p,%s,%p,%d: stub\n", iface, debugstr_w(object), buffer, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI dispspec_GetFriendlyAttributeName(IDsDisplaySpecifier *iface, LPCWSTR object,
                                                        LPCWSTR name, LPWSTR buffer, UINT size)
{
    FIXME("%p,%s,%s,%p,%d: stub\n", iface, debugstr_w(object), debugstr_w(name), buffer, size);
    return E_NOTIMPL;
}

static BOOL WINAPI dispspec_IsClassContainer(IDsDisplaySpecifier *iface, LPCWSTR object,
                                             LPCWSTR path, DWORD flags)
{
    FIXME("%p,%s,%s,%08lx: stub\n", iface, debugstr_w(object), debugstr_w(path), flags);
    return FALSE;
}

static HRESULT WINAPI dispspec_GetClassCreationInfo(IDsDisplaySpecifier *iface, LPCWSTR object,
                                                    LPDSCLASSCREATIONINFO *info)
{
    FIXME("%p,%s,%p: stub\n", iface, debugstr_w(object), info);
    return E_NOTIMPL;
}

static HRESULT WINAPI dispspec_EnumClassAttributes(IDsDisplaySpecifier *iface, LPCWSTR object,
                                                   LPDSENUMATTRIBUTES cb, LPARAM param)
{
    FIXME("%p,%s,%p,%08Ix: stub\n", iface, debugstr_w(object), cb, param);
    return E_NOTIMPL;
}

static ADSTYPE WINAPI dispspec_GetAttributeADsType(IDsDisplaySpecifier *iface, LPCWSTR name)
{
    FIXME("%p,%s: stub\n", iface, debugstr_w(name));
    return ADSTYPE_INVALID;
}

static const IDsDisplaySpecifierVtbl IDsDisplaySpecifier_vtbl =
{
    dispspec_QueryInterface,
    dispspec_AddRef,
    dispspec_Release,
    dispspec_SetServer,
    dispspec_SetLanguageID,
    dispspec_GetDisplaySpecifier,
    dispspec_GetIconLocation,
    dispspec_GetIcon,
    dispspec_GetFriendlyClassName,
    dispspec_GetFriendlyAttributeName,
    dispspec_IsClassContainer,
    dispspec_GetClassCreationInfo,
    dispspec_EnumClassAttributes,
    dispspec_GetAttributeADsType
};

static HRESULT DsDisplaySpecifier_create(REFIID iid, void **obj)
{
    DisplaySpec *dispspec;
    HRESULT hr;

    dispspec = malloc(sizeof(*dispspec));
    if (!dispspec) return E_OUTOFMEMORY;

    dispspec->IDsDisplaySpecifier_iface.lpVtbl = &IDsDisplaySpecifier_vtbl;
    dispspec->ref = 1;

    hr = dispspec->IDsDisplaySpecifier_iface.lpVtbl->QueryInterface(&dispspec->IDsDisplaySpecifier_iface, iid, obj);
    dispspec->IDsDisplaySpecifier_iface.lpVtbl->Release(&dispspec->IDsDisplaySpecifier_iface);

    return hr;
}

static const struct class_info
{
    const CLSID *clsid;
    HRESULT (*constructor)(REFIID, void **);
} class_info[] =
{
    { &CLSID_DsDisplaySpecifier, DsDisplaySpecifier_create }
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

static HRESULT WINAPI factory_QueryInterface(IClassFactory *iface, REFIID iid, LPVOID *obj)
{
    TRACE("%p,%s,%p\n", iface, debugstr_guid(iid), obj);

    if (!iid || !obj) return E_INVALIDARG;

    if (IsEqualIID(iid, &IID_IUnknown) ||
        IsEqualIID(iid, &IID_IClassFactory))
    {
        IClassFactory_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    *obj = NULL;
    FIXME("interface %s is not implemented\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef(IClassFactory *iface)
{
    class_factory *factory = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedIncrement(&factory->ref);

    TRACE("(%p) ref %lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI factory_Release(IClassFactory *iface)
{
    class_factory *factory = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedDecrement(&factory->ref);

    TRACE("(%p) ref %lu\n", iface, ref);

    if (!ref)
        free(factory);

    return ref;
}

static HRESULT WINAPI factory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID iid, void **obj)
{
    class_factory *factory = impl_from_IClassFactory(iface);

    TRACE("%p,%s,%p\n", outer, debugstr_guid(iid), obj);

    if (!iid || !obj) return E_INVALIDARG;

    *obj = NULL;
    if (outer) return CLASS_E_NOAGGREGATION;

    return factory->info->constructor(iid, obj);
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

    factory = malloc(sizeof(*factory));
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
