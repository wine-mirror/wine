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
#include "mfidl.h"
#include "rpcproxy.h"
#include "wmcodecdsp.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmo);

static HRESULT WINAPI wma_decoder_factory_CreateInstance(IClassFactory *iface, IUnknown *outer,
        REFIID riid, void **out)
{
    static const GUID CLSID_wg_wma_decoder = {0x5b4d4e54,0x0620,0x4cf9,{0x94,0xae,0x78,0x23,0x96,0x5c,0x28,0xb6}};
    return CoCreateInstance(&CLSID_wg_wma_decoder, outer, CLSCTX_INPROC_SERVER, riid, out);
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

static const IClassFactoryVtbl wma_decoder_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    wma_decoder_factory_CreateInstance,
    class_factory_LockServer,
};

static IClassFactory wma_decoder_factory = {&wma_decoder_factory_vtbl};

/***********************************************************************
 *              DllGetClassObject (wmadmod.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **out)
{
    if (IsEqualGUID(clsid, &CLSID_WMADecMediaObject))
        return IClassFactory_QueryInterface(&wma_decoder_factory, riid, out);

    *out = NULL;
    FIXME("Unknown clsid %s.\n", debugstr_guid(clsid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *              DllRegisterServer (wmadmod.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    MFT_REGISTER_TYPE_INFO wma_decoder_mft_inputs[] =
    {
        {MFMediaType_Audio, MEDIASUBTYPE_MSAUDIO1},
        {MFMediaType_Audio, MFAudioFormat_WMAudioV8},
        {MFMediaType_Audio, MFAudioFormat_WMAudioV9},
        {MFMediaType_Audio, MFAudioFormat_WMAudio_Lossless},
    };
    MFT_REGISTER_TYPE_INFO wma_decoder_mft_outputs[] =
    {
        {MFMediaType_Audio, MFAudioFormat_PCM},
        {MFMediaType_Audio, MFAudioFormat_Float},
    };
    DMO_PARTIAL_MEDIATYPE wma_decoder_dmo_outputs[] =
    {
        {.type = MEDIATYPE_Audio, .subtype = MEDIASUBTYPE_PCM},
        {.type = MEDIATYPE_Audio, .subtype = MEDIASUBTYPE_IEEE_FLOAT},
    };
    DMO_PARTIAL_MEDIATYPE wma_decoder_dmo_inputs[] =
    {
        {.type = MEDIATYPE_Audio, .subtype = MEDIASUBTYPE_MSAUDIO1},
        {.type = MEDIATYPE_Audio, .subtype = MEDIASUBTYPE_WMAUDIO2},
        {.type = MEDIATYPE_Audio, .subtype = MEDIASUBTYPE_WMAUDIO3},
        {.type = MEDIATYPE_Audio, .subtype = MEDIASUBTYPE_WMAUDIO_LOSSLESS},
    };
    HRESULT hr;

    TRACE("\n");

    if (FAILED(hr = __wine_register_resources()))
        return hr;
    if (FAILED(hr = MFTRegister(CLSID_WMADecMediaObject, MFT_CATEGORY_AUDIO_DECODER,
            (WCHAR *)L"WMAudio Decoder MFT", MFT_ENUM_FLAG_SYNCMFT,
            ARRAY_SIZE(wma_decoder_mft_inputs), wma_decoder_mft_inputs,
            ARRAY_SIZE(wma_decoder_mft_outputs), wma_decoder_mft_outputs, NULL)))
        return hr;
    if (FAILED(hr = DMORegister(L"WMAudio Decoder DMO", &CLSID_WMADecMediaObject, &DMOCATEGORY_AUDIO_DECODER, 0,
            ARRAY_SIZE(wma_decoder_dmo_inputs), wma_decoder_dmo_inputs,
            ARRAY_SIZE(wma_decoder_dmo_outputs), wma_decoder_dmo_outputs)))
        return hr;

    return S_OK;
}

/***********************************************************************
 *              DllUnregisterServer (wmadmod.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    HRESULT hr;

    TRACE("\n");

    if (FAILED(hr = __wine_unregister_resources()))
        return hr;
    if (FAILED(hr = MFTUnregister(CLSID_WMADecMediaObject)))
        return hr;
    if (FAILED(hr = DMOUnregister(&CLSID_WMADecMediaObject, &DMOCATEGORY_AUDIO_DECODER)))
        return hr;

    return S_OK;
}
