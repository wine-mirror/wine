/*
 * Copyright 2009 Vincent Povirk for CodeWeavers
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
#include <math.h>

#define COBJMACROS

#include "windef.h"
#include "objbase.h"
#include "wincodec.h"
#include "wine/test.h"

static void test_decoder_info(void)
{
    IWICImagingFactory *factory;
    IWICComponentInfo *info;
    IWICBitmapDecoderInfo *decoder_info;
    HRESULT hr;
    ULONG len;
    WCHAR value[256];
    const WCHAR expected_mimetype[] = {'i','m','a','g','e','/','b','m','p',0};
    CLSID clsid;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICImagingFactory, (void**)&factory);
    ok(hr == S_OK, "CoCreateInstance failed, hr=%x\n", hr);
    if (FAILED(hr)) return;

    hr = IWICImagingFactory_CreateComponentInfo(factory, &CLSID_WICBmpDecoder, &info);
    ok(hr == S_OK, "CreateComponentInfo failed, hr=%x\n", hr);

    hr = IWICComponentInfo_QueryInterface(info, &IID_IWICBitmapDecoderInfo, (void**)&decoder_info);
    ok(hr == S_OK, "QueryInterface failed, hr=%x\n", hr);

    hr = IWICBitmapDecoderInfo_GetCLSID(decoder_info, NULL);
    ok(hr == E_INVALIDARG, "GetCLSID failed, hr=%x\n", hr);

    hr = IWICBitmapDecoderInfo_GetCLSID(decoder_info, &clsid);
    ok(hr == S_OK, "GetCLSID failed, hr=%x\n", hr);
    ok(IsEqualGUID(&CLSID_WICBmpDecoder, &clsid), "GetCLSID returned wrong result\n");

    hr = IWICBitmapDecoderInfo_GetMimeTypes(decoder_info, 0, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetMimeType failed, hr=%x\n", hr);

    hr = IWICBitmapDecoderInfo_GetMimeTypes(decoder_info, 1, NULL, &len);
    ok(hr == E_INVALIDARG, "GetMimeType failed, hr=%x\n", hr);
    ok(len == lstrlenW(expected_mimetype)+1, "GetMimeType returned wrong len %i\n", len);

    hr = IWICBitmapDecoderInfo_GetMimeTypes(decoder_info, len, value, NULL);
    ok(hr == E_INVALIDARG, "GetMimeType failed, hr=%x\n", hr);

    hr = IWICBitmapDecoderInfo_GetMimeTypes(decoder_info, 0, NULL, &len);
    ok(hr == S_OK, "GetMimeType failed, hr=%x\n", hr);
    ok(len == lstrlenW(expected_mimetype)+1, "GetMimeType returned wrong len %i\n", len);

    value[0] = 0;
    hr = IWICBitmapDecoderInfo_GetMimeTypes(decoder_info, len, value, &len);
    ok(hr == S_OK, "GetMimeType failed, hr=%x\n", hr);
    ok(lstrcmpW(value, expected_mimetype) == 0, "GetMimeType returned wrong value %s\n", wine_dbgstr_w(value));
    ok(len == lstrlenW(expected_mimetype)+1, "GetMimeType returned wrong len %i\n", len);

    hr = IWICBitmapDecoderInfo_GetMimeTypes(decoder_info, 1, value, &len);
    ok(hr == WINCODEC_ERR_INSUFFICIENTBUFFER, "GetMimeType failed, hr=%x\n", hr);
    ok(len == lstrlenW(expected_mimetype)+1, "GetMimeType returned wrong len %i\n", len);

    hr = IWICBitmapDecoderInfo_GetMimeTypes(decoder_info, 256, value, &len);
    ok(hr == S_OK, "GetMimeType failed, hr=%x\n", hr);
    ok(lstrcmpW(value, expected_mimetype) == 0, "GetMimeType returned wrong value %s\n", wine_dbgstr_w(value));
    ok(len == lstrlenW(expected_mimetype)+1, "GetMimeType returned wrong len %i\n", len);

    IWICBitmapDecoderInfo_Release(decoder_info);

    IWICComponentInfo_Release(info);

    IWICImagingFactory_Release(factory);
}

START_TEST(info)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    test_decoder_info();

    CoUninitialize();
}
