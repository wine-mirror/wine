/*
 * Copyright 2019 Jactry Zeng for CodeWeavers
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

#include "mfmediaengine.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(instance);
            break;
    }

    return TRUE;
}

static HRESULT WINAPI media_engine_factory_QueryInterface(IMFMediaEngineClassFactory *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMFMediaEngineClassFactory) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMFMediaEngineClassFactory_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI media_engine_factory_AddRef(IMFMediaEngineClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI media_engine_factory_Release(IMFMediaEngineClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI media_engine_factory_CreateInstance(IMFMediaEngineClassFactory *iface, DWORD flags,
                                                          IMFAttributes *attributes, IMFMediaEngine **engine)
{
    FIXME("(%p, %#x, %p, %p): stub.\n", iface, flags, attributes, engine);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_factory_CreateTimeRange(IMFMediaEngineClassFactory *iface,
                                                           IMFMediaTimeRange **range)
{
    FIXME("(%p, %p): stub.\n", iface, range);

    return E_NOTIMPL;
}

static HRESULT WINAPI media_engine_factory_CreateError(IMFMediaEngineClassFactory *iface, IMFMediaError **error)
{
    FIXME("(%p, %p): stub.\n", iface, error);

    return E_NOTIMPL;
}

static const IMFMediaEngineClassFactoryVtbl media_engine_factory_vtbl =
{
    media_engine_factory_QueryInterface,
    media_engine_factory_AddRef,
    media_engine_factory_Release,
    media_engine_factory_CreateInstance,
    media_engine_factory_CreateTimeRange,
    media_engine_factory_CreateError,
};

static IMFMediaEngineClassFactory media_engine_factory = { &media_engine_factory_vtbl };

static HRESULT WINAPI classfactory_QueryInterface(IClassFactory *iface, REFIID riid, void **obj)
{
    TRACE("(%s, %p).\n", debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IClassFactory) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        IClassFactory_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    WARN("interface %s not implemented.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI classfactory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI classfactory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI classfactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **obj)
{
    TRACE("(%p, %s, %p).\n", outer, debugstr_guid(riid), obj);

    *obj = NULL;

    if (outer)
        return CLASS_E_NOAGGREGATION;

    return IMFMediaEngineClassFactory_QueryInterface(&media_engine_factory, riid, obj);
}

static HRESULT WINAPI classfactory_LockServer(IClassFactory *iface, BOOL dolock)
{
    FIXME("(%d): stub.\n", dolock);
    return S_OK;
}

static const struct IClassFactoryVtbl class_factory_vtbl =
{
    classfactory_QueryInterface,
    classfactory_AddRef,
    classfactory_Release,
    classfactory_CreateInstance,
    classfactory_LockServer,
};

static IClassFactory classfactory = { &class_factory_vtbl };

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **obj)
{
    TRACE("(%s, %s, %p).\n", debugstr_guid(clsid), debugstr_guid(riid), obj);

    if (IsEqualGUID(clsid, &CLSID_MFMediaEngineClassFactory))
        return IClassFactory_QueryInterface(&classfactory, riid, obj);

    WARN("Unsupported class %s.\n", debugstr_guid(clsid));
    *obj = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}
