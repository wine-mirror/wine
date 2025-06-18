/*
 * DirectShow DVD support
 *
 * Copyright 2009 Austin English
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

#include "qdvd_private.h"
#include "rpcproxy.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

struct class_factory
{
    IClassFactory IClassFactory_iface;
    HRESULT (*create_instance)(IUnknown *outer, IUnknown **out);
};

static struct class_factory *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, struct class_factory, IClassFactory_iface);
}

static HRESULT WINAPI class_factory_QueryInterface(IClassFactory *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IClassFactory))
    {
        IClassFactory_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    *out = NULL;
    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI class_factory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI class_factory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI class_factory_CreateInstance(IClassFactory *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    struct class_factory *factory = impl_from_IClassFactory(iface);
    IUnknown *unk;
    HRESULT hr;

    TRACE("iface %p, outer %p, iid %s, out %p.\n", iface, outer, debugstr_guid(iid), out);

    *out = NULL;

    if (outer && !IsEqualGUID(iid, &IID_IUnknown))
        return E_NOINTERFACE;

    if (SUCCEEDED(hr = factory->create_instance(outer, &unk)))
    {
        hr = IUnknown_QueryInterface(unk, iid, out);
        IUnknown_Release(unk);
    }
    return hr;
}

static HRESULT WINAPI class_factory_LockServer(IClassFactory *iface, BOOL lock)
{
    FIXME("lock %d, stub!\n", lock);
    return S_OK;
}

static const IClassFactoryVtbl class_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    class_factory_CreateInstance,
    class_factory_LockServer,
};

static struct class_factory graph_builder_cf = {{&class_factory_vtbl}, graph_builder_create};
static struct class_factory navigator_cf = {{&class_factory_vtbl}, navigator_create};

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void **out)
{
    TRACE("clsid %s, iid %s, out %p.\n", debugstr_guid(clsid), debugstr_guid(iid), out);

    if (IsEqualGUID(clsid, &CLSID_DvdGraphBuilder))
        return IClassFactory_QueryInterface(&graph_builder_cf.IClassFactory_iface, iid, out);
    if (IsEqualGUID(clsid, &CLSID_DVDNavigator))
        return IClassFactory_QueryInterface(&navigator_cf.IClassFactory_iface, iid, out);

    FIXME("%s not available, returning CLASS_E_CLASSNOTAVAILABLE.\n", debugstr_guid(clsid));
    return CLASS_E_CLASSNOTAVAILABLE;
}
