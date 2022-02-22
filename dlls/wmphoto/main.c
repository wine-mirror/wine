/*
 * Copyright 2017 Vincent Povirk for CodeWeavers
 * Copyright 2020 Rémi Bernon for CodeWeavers
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

#include "wincodecs_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

extern HRESULT CDECL wmp_decoder_create(struct decoder_info *info, struct decoder **result);

HRESULT create_instance(const CLSID *clsid, const IID *iid, void **ppv)
{
    return CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, iid, ppv);
}

HRESULT get_decoder_info(REFCLSID clsid, IWICBitmapDecoderInfo **info)
{
    IWICImagingFactory* factory;
    IWICComponentInfo *compinfo;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICImagingFactory, (void **)&factory);
    if (FAILED(hr))
        return hr;

    hr = IWICImagingFactory_CreateComponentInfo(factory, clsid, &compinfo);
    if (FAILED(hr))
    {
        IWICImagingFactory_Release(factory);
        return hr;
    }

    hr = IWICComponentInfo_QueryInterface(compinfo, &IID_IWICBitmapDecoderInfo,
        (void **)info);

    IWICComponentInfo_Release(compinfo);
    IWICImagingFactory_Release(factory);
    return hr;
}

struct class_factory
{
    IClassFactory IClassFactory_iface;
};

static HRESULT WINAPI wmp_class_factory_QueryInterface(IClassFactory *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IClassFactory))
    {
        *out = iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }

    *out = NULL;
    FIXME("%s not implemented.\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI wmp_class_factory_AddRef(IClassFactory *iface) { return 2; }

static ULONG WINAPI wmp_class_factory_Release(IClassFactory *iface) { return 1; }

static HRESULT WINAPI wmp_class_factory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID iid, void **out)
{
    struct decoder_info decoder_info;
    struct decoder *decoder;
    HRESULT hr;

    TRACE("iface %p, outer %p, iid %s, out %p.\n", iface, outer, debugstr_guid(iid), out);

    *out = NULL;
    if (outer) return CLASS_E_NOAGGREGATION;

    hr = wmp_decoder_create(&decoder_info, &decoder);

    if (SUCCEEDED(hr))
        hr = CommonDecoder_CreateInstance(decoder, &decoder_info, iid, out);

    return hr;
}

static HRESULT WINAPI wmp_class_factory_LockServer(IClassFactory *iface, BOOL lock)
{
    FIXME("iface %p, lock %d, stub!\n", iface, lock);
    return S_OK;
}

static const IClassFactoryVtbl wmp_class_factory_vtbl =
{
    wmp_class_factory_QueryInterface,
    wmp_class_factory_AddRef,
    wmp_class_factory_Release,
    wmp_class_factory_CreateInstance,
    wmp_class_factory_LockServer,
};

static struct class_factory wmp_class_factory = {{&wmp_class_factory_vtbl}};

HMODULE windowscodecs_module = 0;

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    TRACE("instance %p, reason %ld, reserved %p\n", instance, reason, reserved);

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        windowscodecs_module = instance;
        DisableThreadLibraryCalls(instance);
        break;
    }

    return TRUE;
}

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID *out)
{
    struct class_factory *factory;

    TRACE("clsid %s, iid %s, out %p.\n", debugstr_guid(clsid), debugstr_guid(iid), out);

    if (IsEqualGUID(clsid, &CLSID_WICWmpDecoder)) factory = &wmp_class_factory;
    else
    {
        FIXME("%s not implemented.\n", debugstr_guid(clsid));
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    return IClassFactory_QueryInterface(&factory->IClassFactory_iface, iid, out);
}
