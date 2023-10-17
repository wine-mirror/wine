/*
 * Copyright (C) 2015 Austin English
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

#include "ole2.h"
#include "rpcproxy.h"
#include "d3d9.h"

#include "evr_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(evr);

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(instance);
        break;
    case DLL_PROCESS_DETACH:
        if (reserved) break;
        strmbase_release_typelibs();
        break;
    }
    return TRUE;
}

struct class_factory
{
    IClassFactory IClassFactory_iface;
    LONG refcount;
    HRESULT (*pfnCreateInstance)(IUnknown *unk_outer, void **ppobj);
};

static inline struct class_factory *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, struct class_factory, IClassFactory_iface);
}

struct object_creation_info
{
    const CLSID *clsid;
    HRESULT (*pfnCreateInstance)(IUnknown *unk_outer, void **ppobj);
};

static const struct object_creation_info object_creation[] =
{
    { &CLSID_EnhancedVideoRenderer, evr_filter_create },
    { &CLSID_MFVideoMixer9, evr_mixer_create },
    { &CLSID_MFVideoPresenter9, evr_presenter_create },
};

static HRESULT WINAPI classfactory_QueryInterface(IClassFactory *iface, REFIID riid, void **out)
{
    struct class_factory *factory = impl_from_IClassFactory(iface);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IClassFactory))
    {
        IClassFactory_AddRef(iface);
        *out = &factory->IClassFactory_iface;
        return S_OK;
    }

    WARN("Unimplemented interface %s.\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI classfactory_AddRef(IClassFactory *iface)
{
    struct class_factory *factory = impl_from_IClassFactory(iface);
    return InterlockedIncrement(&factory->refcount);
}

static ULONG WINAPI classfactory_Release(IClassFactory *iface)
{
    struct class_factory *factory = impl_from_IClassFactory(iface);
    ULONG refcount = InterlockedDecrement(&factory->refcount);

    if (!refcount)
        free(factory);

    return refcount;
}

static HRESULT WINAPI classfactory_CreateInstance(IClassFactory *iface, IUnknown *outer_unk, REFIID riid, void **ppobj)
{
    struct class_factory *factory = impl_from_IClassFactory(iface);
    IUnknown *unk;
    HRESULT hr;

    TRACE("%p, %p, %s, %p.\n", iface, outer_unk, debugstr_guid(riid), ppobj);

    *ppobj = NULL;

    if (outer_unk && !IsEqualGUID(riid, &IID_IUnknown))
        return E_NOINTERFACE;

    hr = factory->pfnCreateInstance(outer_unk, (void **) &unk);
    if (SUCCEEDED(hr))
    {
        hr = IUnknown_QueryInterface(unk, riid, ppobj);
        IUnknown_Release(unk);
    }
    return hr;
}

static HRESULT WINAPI classfactory_LockServer(IClassFactory *iface, BOOL dolock)
{
    FIXME("%p, %d stub!\n", iface, dolock);

    return S_OK;
}

static const IClassFactoryVtbl classfactory_Vtbl =
{
    classfactory_QueryInterface,
    classfactory_AddRef,
    classfactory_Release,
    classfactory_CreateInstance,
    classfactory_LockServer
};

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **out)
{
    struct class_factory *factory;
    unsigned int i;

    TRACE("%s, %s, %p.\n", debugstr_guid(rclsid), debugstr_guid(riid), out);

    if (!IsEqualGUID(&IID_IClassFactory, riid)
         && !IsEqualGUID( &IID_IUnknown, riid))
        return E_NOINTERFACE;

    for (i = 0; i < ARRAY_SIZE(object_creation); i++)
    {
        if (IsEqualGUID(object_creation[i].clsid, rclsid))
            break;
    }

    if (i == ARRAY_SIZE(object_creation))
    {
        FIXME("%s: no class found.\n", debugstr_guid(rclsid));
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    if (!(factory = malloc(sizeof(*factory))))
        return E_OUTOFMEMORY;

    factory->IClassFactory_iface.lpVtbl = &classfactory_Vtbl;
    factory->refcount = 1;

    factory->pfnCreateInstance = object_creation[i].pfnCreateInstance;

    *out = &factory->IClassFactory_iface;

    return S_OK;
}

HRESULT WINAPI MFCreateVideoMixerAndPresenter(IUnknown *mixer_outer, IUnknown *presenter_outer,
        REFIID riid_mixer, void **mixer, REFIID riid_presenter, void **presenter)
{
    HRESULT hr;

    TRACE("%p, %p, %s, %p, %s, %p.\n", mixer_outer, presenter_outer, debugstr_guid(riid_mixer), mixer,
            debugstr_guid(riid_presenter), presenter);

    if (!mixer || !presenter)
        return E_POINTER;

    *mixer = *presenter = NULL;

    if (SUCCEEDED(hr = CoCreateInstance(&CLSID_MFVideoMixer9, mixer_outer, CLSCTX_INPROC_SERVER, riid_mixer, mixer)))
        hr = CoCreateInstance(&CLSID_MFVideoPresenter9, presenter_outer, CLSCTX_INPROC_SERVER, riid_presenter, presenter);

    if (FAILED(hr))
    {
        if (*mixer)
            IUnknown_Release((IUnknown *)*mixer);
        if (*presenter)
            IUnknown_Release((IUnknown *)*presenter);
        *mixer = *presenter = NULL;
    }

    return hr;
}

/***********************************************************************
 *      MFIsFormatYUV (evr.@)
 */
BOOL WINAPI MFIsFormatYUV(DWORD format)
{
    TRACE("%s.\n", debugstr_fourcc(format));

    switch (format)
    {
        case D3DFMT_UYVY:
        case D3DFMT_YUY2:
        case MAKEFOURCC('A','Y','U','V'):
        case MAKEFOURCC('I','M','C','1'):
        case MAKEFOURCC('I','M','C','2'):
        case MAKEFOURCC('Y','V','1','2'):
        case MAKEFOURCC('N','V','1','1'):
        case MAKEFOURCC('N','V','1','2'):
        case MAKEFOURCC('Y','2','1','0'):
        case MAKEFOURCC('Y','2','1','6'):
            return TRUE;
        default:
            return FALSE;
    }
}
