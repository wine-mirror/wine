/*
 * Copyright 2026 Paul Gofman for CodeWeavers
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

WINE_DEFAULT_DEBUG_CHANNEL(aaec);

extern IClassFactory aaec_factory;

/***********************************************************************
 *              DllGetClassObject (resampledmo.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **out)
{
    if (IsEqualGUID(clsid, &CLSID_CWMAudioAEC))
        return IClassFactory_QueryInterface(&aaec_factory, riid, out);

    *out = NULL;
    FIXME("Unknown clsid %s.\n", debugstr_guid(clsid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *              DllRegisterServer (resampledmo.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    MFT_REGISTER_TYPE_INFO aaec_mft_types[] =
    {
        {MFMediaType_Audio, MFAudioFormat_PCM},
        {MFMediaType_Audio, MFAudioFormat_Float},
    };
    DMO_PARTIAL_MEDIATYPE aaec_dmo_types[] =
    {
        {.type = MEDIATYPE_Audio, .subtype = MEDIASUBTYPE_PCM},
        {.type = MEDIATYPE_Audio, .subtype = MEDIASUBTYPE_IEEE_FLOAT},
    };
    HRESULT hr;

    TRACE("\n");

    if (FAILED(hr = __wine_register_resources()))
        return hr;
    if (FAILED(hr = MFTRegister(CLSID_CWMAudioAEC, MFT_CATEGORY_AUDIO_EFFECT,
            (WCHAR *)L"AEC", MFT_ENUM_FLAG_SYNCMFT,
            ARRAY_SIZE(aaec_mft_types), aaec_mft_types,
            ARRAY_SIZE(aaec_mft_types), aaec_mft_types, NULL)))
        return hr;

    if (FAILED(hr = DMORegister(L"AEC", &CLSID_CWMAudioAEC, &DMOCATEGORY_AUDIO_EFFECT, 0,
            ARRAY_SIZE(aaec_dmo_types), aaec_dmo_types,
            ARRAY_SIZE(aaec_dmo_types), aaec_dmo_types)))
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
