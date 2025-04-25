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

#include "video_decoder.h"

#include "dmoreg.h"
#include "rpcproxy.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmo);

extern GUID MEDIASUBTYPE_WMV_Unknown;
extern GUID MEDIASUBTYPE_VC1S;

#include "initguid.h"

DEFINE_GUID(DMOVideoFormat_RGB32,D3DFMT_X8R8G8B8,0x524f,0x11ce,0x9f,0x53,0x00,0x20,0xaf,0x0b,0xa7,0x70);
DEFINE_GUID(DMOVideoFormat_RGB24,D3DFMT_R8G8B8,0x524f,0x11ce,0x9f,0x53,0x00,0x20,0xaf,0x0b,0xa7,0x70);
DEFINE_GUID(DMOVideoFormat_RGB565,D3DFMT_R5G6B5,0x524f,0x11ce,0x9f,0x53,0x00,0x20,0xaf,0x0b,0xa7,0x70);
DEFINE_GUID(DMOVideoFormat_RGB555,D3DFMT_X1R5G5B5,0x524f,0x11ce,0x9f,0x53,0x00,0x20,0xaf,0x0b,0xa7,0x70);
DEFINE_GUID(DMOVideoFormat_RGB8,D3DFMT_P8,0x524f,0x11ce,0x9f,0x53,0x00,0x20,0xaf,0x0b,0xa7,0x70);

/***********************************************************************
 *              DllGetClassObject (wmvdecod.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **out)
{
    if (IsEqualGUID(clsid, &CLSID_WMVDecoderMFT))
        return IClassFactory_QueryInterface(&wmv_decoder_factory, riid, out);

    *out = NULL;
    FIXME("Unknown clsid %s.\n", debugstr_guid(clsid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *              DllRegisterServer (wmvdecod.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    MFT_REGISTER_TYPE_INFO wmv_decoder_mft_inputs[] =
    {
        {MFMediaType_Video, MFVideoFormat_WMV1},
        {MFMediaType_Video, MFVideoFormat_WMV2},
        {MFMediaType_Video, MFVideoFormat_WMV3},
        {MFMediaType_Video, MEDIASUBTYPE_WMVP},
        {MFMediaType_Video, MEDIASUBTYPE_WVP2},
        {MFMediaType_Video, MEDIASUBTYPE_WMVR},
        {MFMediaType_Video, MEDIASUBTYPE_WMVA},
        {MFMediaType_Video, MFVideoFormat_WVC1},
        {MFMediaType_Video, MEDIASUBTYPE_VC1S},
    };
    MFT_REGISTER_TYPE_INFO wmv_decoder_mft_outputs[] =
    {
        {MFMediaType_Video, MFVideoFormat_YV12},
        {MFMediaType_Video, MFVideoFormat_YUY2},
        {MFMediaType_Video, MFVideoFormat_UYVY},
        {MFMediaType_Video, MFVideoFormat_YVYU},
        {MFMediaType_Video, MFVideoFormat_NV11},
        {MFMediaType_Video, MFVideoFormat_NV12},
        {MFMediaType_Video, DMOVideoFormat_RGB32},
        {MFMediaType_Video, DMOVideoFormat_RGB24},
        {MFMediaType_Video, DMOVideoFormat_RGB565},
        {MFMediaType_Video, DMOVideoFormat_RGB555},
        {MFMediaType_Video, DMOVideoFormat_RGB8},
    };
    DMO_PARTIAL_MEDIATYPE wmv_decoder_dmo_outputs[] =
    {
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_YV12},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_YUY2},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_UYVY},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_YVYU},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_NV11},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_NV12},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_RGB32},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_RGB24},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_RGB565},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_RGB555},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_RGB8},
    };
    DMO_PARTIAL_MEDIATYPE wmv_decoder_dmo_inputs[] =
    {
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_WMV1},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_WMV2},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_WMV3},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_WMVA},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_WVC1},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_WMVP},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_WVP2},
        {.type = MEDIATYPE_Video, .subtype = MEDIASUBTYPE_VC1S},
    };
    HRESULT hr;

    TRACE("\n");

    if (FAILED(hr = __wine_register_resources()))
        return hr;
    if (FAILED(hr = MFTRegister(CLSID_WMVDecoderMFT, MFT_CATEGORY_VIDEO_DECODER,
            (WCHAR *)L"WMVideo Decoder MFT", MFT_ENUM_FLAG_SYNCMFT,
            ARRAY_SIZE(wmv_decoder_mft_inputs), wmv_decoder_mft_inputs,
            ARRAY_SIZE(wmv_decoder_mft_outputs), wmv_decoder_mft_outputs, NULL)))
        return hr;
    if (FAILED(hr = DMORegister(L"WMVideo Decoder DMO", &CLSID_WMVDecoderMFT, &DMOCATEGORY_VIDEO_DECODER, 0,
            ARRAY_SIZE(wmv_decoder_dmo_inputs), wmv_decoder_dmo_inputs,
            ARRAY_SIZE(wmv_decoder_dmo_outputs), wmv_decoder_dmo_outputs)))
        return hr;

    return S_OK;
}

/***********************************************************************
 *              DllUnregisterServer (wmvdecod.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    HRESULT hr;

    TRACE("\n");

    if (FAILED(hr = __wine_unregister_resources()))
        return hr;
    if (FAILED(hr = MFTUnregister(CLSID_WMVDecoderMFT)))
        return hr;
    if (FAILED(hr = DMOUnregister(&CLSID_WMVDecoderMFT, &DMOCATEGORY_VIDEO_DECODER)))
        return hr;

    return S_OK;
}
