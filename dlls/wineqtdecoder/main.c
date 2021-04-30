/*
 * DirectShow filter for QuickTime Toolkit on Mac OS X
 *
 * Copyright (C) 2010 Aric Stewart, CodeWeavers
 * Copyright (C) 2019 Zebediah Figura
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
#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

#define COBJMACROS
#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#define WINE_NO_NAMELESS_EXTENSION

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winerror.h"
#include "objbase.h"
#include "uuids.h"
#include "strmif.h"
#include "rpcproxy.h"

#include "wine/unicode.h"
#include "wine/debug.h"
#include "wine/strmbase.h"

#include "wineqtdecoder_classes.h"

#include "initguid.h"
DEFINE_GUID(WINESUBTYPE_QTSplitter,    0xFFFFFFFF, 0x5927, 0x4894, 0xA3,0x86, 0x35,0x94,0x60,0xEE,0x87,0xC9);

WINE_DEFAULT_DEBUG_CHANNEL(qtdecoder);

extern HRESULT video_decoder_create(IUnknown *outer, IUnknown **out);
extern HRESULT qt_splitter_create(IUnknown *outer, IUnknown **out);

static const WCHAR wQTVName[] =
{'Q','T',' ','V','i','d','e','o',' ','D','e','c','o','d','e','r',0};
static const WCHAR wQTDName[] =
{'Q','T',' ','V','i','d','e','o',' ','D','e','m','u','x',0};

struct class_factory
{
    IClassFactory IClassFactory_iface;
    HRESULT (*create_instance)(IUnknown *outer, IUnknown **out);
};

static inline struct class_factory *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, struct class_factory, IClassFactory_iface);
}

static HRESULT WINAPI class_factory_QueryInterface(IClassFactory *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IClassFactory))
    {
        *out = iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }

    *out = NULL;
    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
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

static HRESULT WINAPI class_factory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID iid, void **out)
{
    struct class_factory *factory = impl_from_IClassFactory(iface);
    IUnknown *unk;
    HRESULT hr;

    TRACE("iface %p, outer %p, iid %s, out %p.\n", iface, outer, debugstr_guid(iid), out);

    if (outer && !IsEqualGUID(iid, &IID_IUnknown))
        return E_NOINTERFACE;

    *out = NULL;
    if (SUCCEEDED(hr = factory->create_instance(outer, &unk)))
    {
        hr = IUnknown_QueryInterface(unk, iid, out);
        IUnknown_Release(unk);
    }
    return hr;
}

static HRESULT WINAPI class_factory_LockServer(IClassFactory *iface, BOOL lock)
{
    TRACE("iface %p, lock %d.\n", iface, lock);
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

static struct class_factory qt_splitter_cf = {{&class_factory_vtbl}, qt_splitter_create};
static struct class_factory video_decoder_cf = {{&class_factory_vtbl}, video_decoder_create};

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void **out)
{
    struct class_factory *factory;

    TRACE("clsid %s, iid %s, out %p.\n", debugstr_guid(clsid), debugstr_guid(iid), out);

    if (IsEqualGUID(clsid, &CLSID_QTSplitter))
        factory = &qt_splitter_cf;
    else if (IsEqualGUID(clsid, &CLSID_QTVDecoder))
        factory = &video_decoder_cf;
    else
    {
        FIXME("%s not implemented, returning CLASS_E_CLASSNOTAVAILABLE.\n", debugstr_guid(clsid));
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    return IClassFactory_QueryInterface(&factory->IClassFactory_iface, iid, out);
}

static const REGPINTYPES reg_audio_mt = {&MEDIATYPE_Audio, &GUID_NULL};
static const REGPINTYPES reg_stream_mt = {&MEDIATYPE_Stream, &GUID_NULL};
static const REGPINTYPES reg_video_mt = {&MEDIATYPE_Video, &GUID_NULL};

static const REGFILTERPINS2 reg_qt_splitter_pins[3] =
{
    {
        .nMediaTypes = 1,
        .lpMediaType = &reg_stream_mt,
    },
    {
        .dwFlags = REG_PINFLAG_B_OUTPUT | REG_PINFLAG_B_ZERO,
        .nMediaTypes = 1,
        .lpMediaType = &reg_audio_mt,
    },
    {
        .dwFlags = REG_PINFLAG_B_OUTPUT | REG_PINFLAG_B_ZERO,
        .nMediaTypes = 1,
        .lpMediaType = &reg_video_mt,
    },
};

static const REGFILTER2 reg_qt_splitter =
{
    .dwVersion = 2,
    .dwMerit = MERIT_NORMAL - 1,
    .u.s2.cPins2 = 3,
    .u.s2.rgPins2 = reg_qt_splitter_pins,
};

static const REGFILTERPINS2 reg_video_decoder_pins[2] =
{
    {
        .nMediaTypes = 1,
        .lpMediaType = &reg_video_mt,
    },
    {
        .dwFlags = REG_PINFLAG_B_OUTPUT,
        .nMediaTypes = 1,
        .lpMediaType = &reg_video_mt,
    },
};

static const REGFILTER2 reg_video_decoder =
{
    .dwVersion = 2,
    .dwMerit = MERIT_NORMAL - 1,
    .u.s2.cPins2 = 2,
    .u.s2.rgPins2 = reg_video_decoder_pins,
};

HRESULT WINAPI DllRegisterServer(void)
{
    IFilterMapper2 *mapper;
    HRESULT hr;

    TRACE(".\n");

    if (FAILED(hr = __wine_register_resources()))
        return hr;

    if (FAILED(hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterMapper2, (void **)&mapper)))
        return hr;

    IFilterMapper2_RegisterFilter(mapper, &CLSID_QTSplitter, wQTDName, NULL, NULL, NULL, &reg_qt_splitter);
    IFilterMapper2_RegisterFilter(mapper, &CLSID_QTVDecoder, wQTVName, NULL, NULL, NULL, &reg_video_decoder);

    IFilterMapper2_Release(mapper);
    return S_OK;
}

HRESULT WINAPI DllUnregisterServer(void)
{
    IFilterMapper2 *mapper;
    HRESULT hr;

    TRACE(".\n");

    if (FAILED(hr = __wine_unregister_resources()))
        return hr;

    if (FAILED(hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterMapper2, (void **)&mapper)))
        return hr;

    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &CLSID_QTSplitter);
    IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &CLSID_QTVDecoder);

    IFilterMapper2_Release(mapper);
    return S_OK;
}
