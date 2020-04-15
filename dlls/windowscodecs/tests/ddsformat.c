/*
 * Copyright 2020 Ziqing Hui
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
#include "wincodec.h"
#include "wine/test.h"

/* 4x4 DDS image */
static BYTE test_dds_image[] = {
    'D',  'D',  'S',  ' ',  0x7C, 0x00, 0x00, 0x00, 0x07, 0x10, 0x08, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x44, 0x58, 0x54, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xF5, 0xA7, 0x08, 0x69, 0x74, 0xC0, 0xBF, 0xD7
};

static IWICImagingFactory *factory = NULL;

static IWICStream *create_stream(const void *image_data, UINT image_size)
{
    HRESULT hr;
    IWICStream *stream = NULL;

    hr = IWICImagingFactory_CreateStream(factory, &stream);
    ok(SUCCEEDED(hr), "CreateStream failed, hr=%x\n", hr);
    if (FAILED(hr)) goto fail;

    hr = IWICStream_InitializeFromMemory(stream, (BYTE *)image_data, image_size);
    ok(SUCCEEDED(hr), "InitializeFromMemory failed, hr=%x\n", hr);
    if (FAILED(hr)) goto fail;

    return stream;

fail:
    if (stream) IWICStream_Release(stream);
    return NULL;
}

static IWICBitmapDecoder *create_decoder(IWICStream *stream)
{
    HRESULT hr;
    IWICBitmapDecoder *decoder = NULL;
    GUID guidresult;

    hr = CoCreateInstance(&CLSID_WICDdsDecoder, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICBitmapDecoder, (void **)&decoder);
    if (FAILED(hr)) {
        win_skip("Dds decoder is not supported\n");
        return NULL;
    }

    memset(&guidresult, 0, sizeof(guidresult));
    hr = IWICBitmapDecoder_GetContainerFormat(decoder, &guidresult);
    ok(SUCCEEDED(hr), "GetContainerFormat failed, hr=%x\n", hr);
    ok(IsEqualGUID(&guidresult, &GUID_ContainerFormatDds), "Unexpected container format\n");

    hr = IWICBitmapDecoder_Initialize(decoder, (IStream*)stream, WICDecodeMetadataCacheOnDemand);
    todo_wine ok(hr == S_OK, "Decoder Initialize failed, hr=%x\n", hr);
    if (hr != S_OK) {
        IWICBitmapDecoder_Release(decoder);
        return NULL;
    }

    return decoder;
}

static void test_dds_decoder(void)
{
    IWICStream *stream = NULL;
    IWICBitmapDecoder *decoder = NULL;

    stream = create_stream(test_dds_image, sizeof(test_dds_image));
    if (!stream) goto end;

    decoder = create_decoder(stream);
    if (!decoder) goto end;

end:
    if (decoder) IWICBitmapDecoder_Release(decoder);
    if (stream) IWICStream_Release(stream);
}

START_TEST(ddsformat)
{
    HRESULT hr;
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICImagingFactory, (void **)&factory);
    ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%x\n", hr);
    if (FAILED(hr)) goto end;

    test_dds_decoder();

end:
    if(factory) IWICImagingFactory_Release(factory);
    CoUninitialize();
}
