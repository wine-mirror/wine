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

static IWICBitmapDecoder *create_decoder(void)
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

    return decoder;
}

static HRESULT init_decoder(IWICBitmapDecoder *decoder, IWICStream *stream, HRESULT expected, int index)
{
    HRESULT hr;

    hr = IWICBitmapDecoder_Initialize(decoder, (IStream*)stream, WICDecodeMetadataCacheOnDemand);
    if (index == -1) {
        ok(SUCCEEDED(hr), "Decoder Initialize failed, hr=%x\n", hr);
    } else {
        ok(hr == expected, "%d: Expected hr=%x, got %x\n", index, expected, hr);
    }
    return hr;
}

static void test_dds_decoder_initialize(void)
{
    static BYTE test_dds_bad_magic[sizeof(test_dds_image)];
    static BYTE test_dds_bad_header[sizeof(test_dds_image)];
    static BYTE byte = 0;
    static DWORD dword = 0;
    static BYTE qword1[8] = { 0 };
    static BYTE qword2[8] = "DDS ";

    static struct test_data {
        void *data;
        UINT size;
        HRESULT expected;
    } test_data[] = {
        { test_dds_image,      sizeof(test_dds_image),      S_OK },
        { test_dds_bad_magic,  sizeof(test_dds_bad_magic),  WINCODEC_ERR_UNKNOWNIMAGEFORMAT },
        { test_dds_bad_header, sizeof(test_dds_bad_header), WINCODEC_ERR_BADHEADER },
        { &byte,   sizeof(byte),   WINCODEC_ERR_STREAMREAD },
        { &dword,  sizeof(dword),  WINCODEC_ERR_UNKNOWNIMAGEFORMAT },
        { &qword1, sizeof(qword1), WINCODEC_ERR_UNKNOWNIMAGEFORMAT },
        { &qword2, sizeof(qword2), WINCODEC_ERR_STREAMREAD },
    };

    int i;

    memcpy(test_dds_bad_magic, test_dds_image, sizeof(test_dds_image));
    memcpy(test_dds_bad_header, test_dds_image, sizeof(test_dds_image));
    test_dds_bad_magic[0] = 0;
    test_dds_bad_header[4] = 0;

    for (i = 0; i < ARRAY_SIZE(test_data); i++)
    {
        IWICStream *stream = NULL;
        IWICBitmapDecoder *decoder = NULL;

        stream = create_stream(test_data[i].data, test_data[i].size);
        if (!stream) goto next;

        decoder = create_decoder();
        if (!decoder) goto next;

        init_decoder(decoder, stream, test_data[i].expected, i);

    next:
        if (decoder) IWICBitmapDecoder_Release(decoder);
        if (stream) IWICStream_Release(stream);
    }
}

static void test_dds_decoder_global_properties(IWICBitmapDecoder *decoder)
{
    HRESULT hr;
    IWICPalette *pallette = NULL;
    IWICMetadataQueryReader *metadata_reader = NULL;
    IWICBitmapSource *preview = NULL, *thumnail = NULL;
    IWICColorContext *color_context = NULL;
    UINT count;

    hr = IWICImagingFactory_CreatePalette(factory, &pallette);
    ok (hr == S_OK, "CreatePalette failed, hr=%x\n", hr);
    if (hr == S_OK) {
        hr = IWICBitmapDecoder_CopyPalette(decoder, pallette);
        ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "Expected hr=WINCODEC_ERR_PALETTEUNAVAILABLE, got %x\n", hr);
        hr = IWICBitmapDecoder_CopyPalette(decoder, NULL);
        ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "Expected hr=WINCODEC_ERR_PALETTEUNAVAILABLE, got %x\n", hr);
    }

    hr = IWICBitmapDecoder_GetMetadataQueryReader(decoder, &metadata_reader);
    todo_wine ok (hr == S_OK, "Expected hr=S_OK, got %x\n", hr);
    hr = IWICBitmapDecoder_GetMetadataQueryReader(decoder, NULL);
    ok (hr == E_INVALIDARG, "Expected hr=E_INVALIDARG, got %x\n", hr);

    hr = IWICBitmapDecoder_GetPreview(decoder, &preview);
    ok (hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "Expected hr=WINCODEC_ERR_UNSUPPORTEDOPERATION, got %x\n", hr);
    hr = IWICBitmapDecoder_GetPreview(decoder, NULL);
    ok (hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "Expected hr=WINCODEC_ERR_UNSUPPORTEDOPERATION, got %x\n", hr);

    hr = IWICBitmapDecoder_GetColorContexts(decoder, 1, &color_context, &count);
    ok (hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "Expected hr=WINCODEC_ERR_UNSUPPORTEDOPERATION, got %x\n", hr);
    hr = IWICBitmapDecoder_GetColorContexts(decoder, 1, NULL, NULL);
    ok (hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "Expected hr=WINCODEC_ERR_UNSUPPORTEDOPERATION, got %x\n", hr);

    hr = IWICBitmapDecoder_GetThumbnail(decoder, &thumnail);
    ok (hr == WINCODEC_ERR_CODECNOTHUMBNAIL, "Expected hr=WINCODEC_ERR_CODECNOTHUMBNAIL, got %x\n", hr);
    hr = IWICBitmapDecoder_GetThumbnail(decoder, NULL);
    ok (hr == WINCODEC_ERR_CODECNOTHUMBNAIL, "Expected hr=WINCODEC_ERR_CODECNOTHUMBNAIL, got %x\n", hr);

    if (pallette) IWICPalette_Release(pallette);
    if (metadata_reader) IWICMetadataQueryReader_Release(metadata_reader);
    if (preview) IWICBitmapSource_Release(preview);
    if (color_context) IWICColorContext_Release(color_context);
    if (thumnail) IWICBitmapSource_Release(thumnail);
}

static void test_dds_decoder(void)
{
    HRESULT hr;
    IWICStream *stream = NULL;
    IWICBitmapDecoder *decoder = NULL;

    stream = create_stream(test_dds_image, sizeof(test_dds_image));
    if (!stream) goto end;

    decoder = create_decoder();
    if (!decoder) goto end;

    hr = init_decoder(decoder, stream, S_OK, -1);
    if (FAILED(hr)) goto end;

    test_dds_decoder_initialize();
    test_dds_decoder_global_properties(decoder);

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
