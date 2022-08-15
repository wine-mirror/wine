/*
 * Copyright 2022 Nikolay Sivov for CodeWeavers
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

#include "mfidl.h"
#include "wine/mfinternal.h"

#include "mfsrcsnk_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

struct class_factory
{
    IClassFactory IClassFactory_iface;
    HRESULT (*create_instance)(REFIID riid, void **out);
};

static inline struct class_factory *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, struct class_factory, IClassFactory_iface);
}

static HRESULT WINAPI class_factory_QueryInterface(IClassFactory *iface, REFIID riid, void **out)
{
    if (IsEqualGUID(riid, &IID_IClassFactory) ||
            IsEqualGUID(riid, &IID_IUnknown))
    {
        IClassFactory_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    *out = NULL;
    WARN("Interface %s is not supported.\n", debugstr_guid(riid));
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

static HRESULT WINAPI class_factory_CreateInstance(IClassFactory *iface, IUnknown *outer,
        REFIID riid, void **out)
{
    struct class_factory *factory = impl_from_IClassFactory(iface);

    TRACE("%p, %s, %p.\n", outer, debugstr_guid(riid), out);

    *out = NULL;

    if (outer)
        return CLASS_E_NOAGGREGATION;

    return factory->create_instance(riid, out);
}

static HRESULT WINAPI class_factory_LockServer(IClassFactory *iface, BOOL dolock)
{
    FIXME("%p, %d.\n", iface, dolock);

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

static struct class_factory wave_sink_factory = { { &class_factory_vtbl }, wave_sink_factory_create };

/***********************************************************************
 *              DllGetClassObject (mfsrcsnk.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **out)
{
    *out = NULL;

    if (IsEqualGUID(clsid, &CLSID_MFWAVESinkClassFactory))
    {
        return IClassFactory_QueryInterface(&wave_sink_factory.IClassFactory_iface, riid, out);
    }
    else
    {
        FIXME("Unknown clsid %s.\n", debugstr_guid(clsid));
        return CLASS_E_CLASSNOTAVAILABLE;
    }
}
