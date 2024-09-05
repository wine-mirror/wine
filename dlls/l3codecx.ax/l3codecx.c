/*
 * Copyright 2024 Rémi Bernon for CodeWeavers
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
#include <stddef.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"

#include "dshow.h"
#include "mpegtype.h"
#include "rpcproxy.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

#include "initguid.h"

DEFINE_GUID(CLSID_MPEGLayer3Decoder,0x38be3000,0xdbf4,0x11d0,0x86,0x0e,0x00,0xa0,0x24,0xcf,0xef,0x6d);
DEFINE_GUID(MEDIASUBTYPE_MP3,WAVE_FORMAT_MPEGLAYER3,0x0000,0x0010,0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71);

static inline HRESULT WINAPI class_factory_QueryInterface(IClassFactory *iface, REFIID riid, void **out)
{
    *out = IsEqualGUID(riid, &IID_IClassFactory) || IsEqualGUID(riid, &IID_IUnknown) ? iface : NULL;
    return *out ? S_OK : E_NOINTERFACE;
}
static inline ULONG WINAPI class_factory_AddRef(IClassFactory *iface)
{
    return 2;
}
static inline ULONG WINAPI class_factory_Release(IClassFactory *iface)
{
    return 1;
}
static HRESULT WINAPI mpeg_layer3_decoder_factory_CreateInstance(IClassFactory *iface,
        IUnknown *outer, REFIID riid, void **out)
{
    static const GUID CLSID_wg_mp3_decoder = {0x84cd8e3e,0xb221,0x434a,{0x88,0x82,0x9d,0x6c,0x8d,0xf4,0x90,0xe1}};
    return CoCreateInstance(&CLSID_wg_mp3_decoder, outer, CLSCTX_INPROC_SERVER, riid, out);
}
static inline HRESULT WINAPI class_factory_LockServer(IClassFactory *iface, BOOL dolock)
{
    return S_OK;
}

static const IClassFactoryVtbl mpeg_layer3_decoder_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    mpeg_layer3_decoder_factory_CreateInstance,
    class_factory_LockServer,
};

static IClassFactory mpeg_layer3_decoder_factory = {&mpeg_layer3_decoder_factory_vtbl};

/***********************************************************************
 *              DllGetClassObject (l3codecx.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void **out)
{
    if (IsEqualGUID(clsid, &CLSID_MPEGLayer3Decoder))
        return IClassFactory_QueryInterface(&mpeg_layer3_decoder_factory, iid, out);

    *out = NULL;
    FIXME("Unknown clsid %s.\n", debugstr_guid(clsid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *              DllRegisterServer (l3codecx.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    static const REGPINTYPES mpeg_layer3_decoder_inputs[] =
    {
        {&MEDIATYPE_Audio, &MEDIASUBTYPE_MP3},
    };
    static const REGPINTYPES mpeg_layer3_decoder_outputs[] =
    {
        {&MEDIATYPE_Audio, &MEDIASUBTYPE_PCM},
    };
    static const REGFILTERPINS2 mpeg_layer3_decoder_pins[] =
    {
        {
            .nMediaTypes = ARRAY_SIZE(mpeg_layer3_decoder_inputs),
            .lpMediaType = mpeg_layer3_decoder_inputs,
        },
        {
            .dwFlags = REG_PINFLAG_B_OUTPUT,
            .nMediaTypes = ARRAY_SIZE(mpeg_layer3_decoder_outputs),
            .lpMediaType = mpeg_layer3_decoder_outputs,
        },
    };
    static const REGFILTER2 mpeg_layer3_decoder_desc =
    {
        .dwVersion = 2,
        .dwMerit = 0x00810000,
        .cPins2 = ARRAY_SIZE(mpeg_layer3_decoder_pins),
        .rgPins2 = mpeg_layer3_decoder_pins,
    };

    IFilterMapper2 *mapper;
    HRESULT hr;

    TRACE(".\n");

    if (FAILED(hr = __wine_register_resources()))
        return hr;

    if (FAILED(hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
            &IID_IFilterMapper2, (void **)&mapper)))
        return hr;
    hr = IFilterMapper2_RegisterFilter(mapper, &CLSID_MPEGLayer3Decoder, L"MPEG Layer-3 Decoder", NULL,
            NULL, NULL, &mpeg_layer3_decoder_desc);
    IFilterMapper2_Release(mapper);
    return hr;
}

/***********************************************************************
 *              DllUnregisterServer (l3codecx.@)
 */
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
    hr = IFilterMapper2_UnregisterFilter(mapper, NULL, NULL, &CLSID_MPEGLayer3Decoder);
    IFilterMapper2_Release(mapper);
    return S_OK;
}
