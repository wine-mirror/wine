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

#include <stddef.h>
#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"

#include "dmoreg.h"
#include "dshow.h"
#include "mfapi.h"
#include "rpcproxy.h"
#include "wmcodecdsp.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmo);

static HRESULT WINAPI resampler_factory_CreateInstance(IClassFactory *iface, IUnknown *outer,
        REFIID riid, void **out)
{
    static const GUID CLSID_wg_resampler = {0x92f35e78,0x15a5,0x486b,{0x88,0x8e,0x57,0x5f,0x99,0x65,0x1c,0xe2}};
    return CoCreateInstance(&CLSID_wg_resampler, outer, CLSCTX_INPROC_SERVER, riid, out);
}

static HRESULT WINAPI class_factory_QueryInterface(IClassFactory *iface, REFIID riid, void **out)
{
    *out = IsEqualGUID(riid, &IID_IClassFactory) || IsEqualGUID(riid, &IID_IUnknown) ? iface : NULL;
    return *out ? S_OK : E_NOINTERFACE;
}
static ULONG WINAPI class_factory_AddRef(IClassFactory *iface)
{
    return 2;
}
static ULONG WINAPI class_factory_Release(IClassFactory *iface)
{
    return 1;
}
static HRESULT WINAPI class_factory_LockServer(IClassFactory *iface, BOOL dolock)
{
    return S_OK;
}

static const IClassFactoryVtbl resampler_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    resampler_factory_CreateInstance,
    class_factory_LockServer,
};

static IClassFactory resampler_factory = {&resampler_factory_vtbl};

/***********************************************************************
 *              DllGetClassObject (resampledmo.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **out)
{
    if (IsEqualGUID(clsid, &CLSID_CResamplerMediaObject))
        return IClassFactory_QueryInterface(&resampler_factory, riid, out);

    *out = NULL;
    FIXME("Unknown clsid %s.\n", debugstr_guid(clsid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *              DllRegisterServer (resampledmo.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    MFT_REGISTER_TYPE_INFO resampler_mft_types[] =
    {
        {MFMediaType_Audio, MFAudioFormat_PCM},
        {MFMediaType_Audio, MFAudioFormat_Float},
    };
    DMO_PARTIAL_MEDIATYPE resampler_dmo_types[] =
    {
        {.type = MEDIATYPE_Audio, .subtype = MEDIASUBTYPE_PCM},
        {.type = MEDIATYPE_Audio, .subtype = MEDIASUBTYPE_IEEE_FLOAT},
    };
    HRESULT hr;

    TRACE("\n");

    if (FAILED(hr = __wine_register_resources()))
        return hr;
    if (FAILED(hr = MFTRegister(CLSID_CResamplerMediaObject, MFT_CATEGORY_AUDIO_EFFECT,
            (WCHAR *)L"Resampler MFT", MFT_ENUM_FLAG_SYNCMFT,
            ARRAY_SIZE(resampler_mft_types), resampler_mft_types,
            ARRAY_SIZE(resampler_mft_types), resampler_mft_types, NULL)))
        return hr;
    if (FAILED(hr = DMORegister(L"Resampler DMO", &CLSID_CResamplerMediaObject, &DMOCATEGORY_AUDIO_EFFECT, 0,
            ARRAY_SIZE(resampler_dmo_types), resampler_dmo_types,
            ARRAY_SIZE(resampler_dmo_types), resampler_dmo_types)))
        return hr;

    return S_OK;
}

/***********************************************************************
 *              DllUnregisterServer (resampledmo.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    HRESULT hr;

    TRACE("\n");

    if (FAILED(hr = __wine_unregister_resources()))
        return hr;
    if (FAILED(hr = MFTUnregister(CLSID_CResamplerMediaObject)))
        return hr;
    if (FAILED(hr = DMOUnregister(&CLSID_CResamplerMediaObject, &DMOCATEGORY_AUDIO_EFFECT)))
        return hr;

    return S_OK;
}
