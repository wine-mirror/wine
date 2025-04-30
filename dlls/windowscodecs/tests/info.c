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

#include <stdio.h>
#include <stdarg.h>
#include <math.h>

#define COBJMACROS

#include "windef.h"
#include "objbase.h"
#include "wincodec.h"
#include "wincodecsdk.h"
#include "wine/test.h"

static const GUID *metadata_handlers[] =
{
    &CLSID_WICUnknownMetadataReader,
    &CLSID_WICLSDMetadataReader,
    &CLSID_WICIMDMetadataReader,
    &CLSID_WICAPEMetadataReader,
    &CLSID_WICGifCommentMetadataReader,
    &CLSID_WICPngTextMetadataReader,
    &CLSID_WICPngGamaMetadataReader,
    &CLSID_WICPngChrmMetadataReader,
    &CLSID_WICPngHistMetadataReader,
    &CLSID_WICPngTimeMetadataReader,
    &CLSID_WICIfdMetadataReader,
    &CLSID_WICGpsMetadataReader,
    &CLSID_WICExifMetadataReader,
    &CLSID_WICApp1MetadataReader,

    &CLSID_WICUnknownMetadataWriter,
    &CLSID_WICIfdMetadataWriter,
    &CLSID_WICGpsMetadataWriter,
    &CLSID_WICExifMetadataWriter,
    &CLSID_WICApp1MetadataWriter,
};

static void run_child_test(const char *name)
{
    char path_name[MAX_PATH];
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;
    char **argv;

    winetest_get_mainargs(&argv);

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    sprintf(path_name, "%s info %s", argv[0], name);
    ok(CreateProcessA( NULL, path_name, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info),
            "CreateProcess failed.\n" );
    wait_child_process(info.hProcess);
    CloseHandle(info.hProcess);
    CloseHandle(info.hThread);
}

static HRESULT get_component_info(const GUID *clsid, IWICComponentInfo **result)
{
    IWICImagingFactory *factory;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICImagingFactory, (void**)&factory);
    ok(hr == S_OK, "CoCreateInstance failed, hr=%lx\n", hr);
    if (FAILED(hr)) return hr;

    hr = IWICImagingFactory_CreateComponentInfo(factory, clsid, result);

    IWICImagingFactory_Release(factory);

    return hr;
}

static BOOL is_pixelformat(GUID *format)
{
    IWICComponentInfo *info;
    HRESULT hr;
    WICComponentType componenttype;

    hr = get_component_info(format, &info);
    if (FAILED(hr))
        return FALSE;

    hr = IWICComponentInfo_GetComponentType(info, &componenttype);

    IWICComponentInfo_Release(info);

    return SUCCEEDED(hr) && componenttype == WICPixelFormat;
}

static void test_decoder_info(void)
{
    static const struct decoder_info_test
    {
        const CLSID *clsid;
        const WCHAR *mimetype;
        const WCHAR *extensions;
        unsigned int todo;
    } decoder_info_tests[] =
    {
        {
            &CLSID_WICBmpDecoder,
            L"image/bmp",
            L".bmp,.dib,.rle"
        },
        {
            &CLSID_WICGifDecoder,
            L"image/gif",
            L".gif"
        },
        {
            &CLSID_WICIcoDecoder,
            L"image/ico,image/x-icon",
            L".ico,.icon",
        },
        {
            &CLSID_WICJpegDecoder,
            L"image/jpeg,image/jpe,image/jpg",
            L".jpeg,.jpe,.jpg,.jfif,.exif",
            1
        },
        {
            &CLSID_WICPngDecoder,
            L"image/png",
            L".png"
        },
        {
            &CLSID_WICTiffDecoder,
            L"image/tiff,image/tif",
            L".tiff,.tif",
            1
        },
        {
            &CLSID_WICDdsDecoder,
            L"image/vnd.ms-dds",
            L".dds",
        }
    };
    IWICBitmapDecoderInfo *decoder_info, *decoder_info2;
    IWICComponentInfo *info;
    HRESULT hr;
    UINT len;
    WCHAR value[256];
    CLSID clsid;
    GUID pixelformats[32];
    UINT num_formats, count;
    int i, j;

    for (i = 0; i < ARRAY_SIZE(decoder_info_tests); i++)
    {
        const struct decoder_info_test *test = &decoder_info_tests[i];
        IWICBitmapDecoder *decoder, *decoder2;

        hr = CoCreateInstance(test->clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IWICBitmapDecoder, (void **)&decoder);
        if (test->clsid == &CLSID_WICDdsDecoder && hr != S_OK) {
            win_skip("DDS decoder is not supported\n");
            continue;
        }
        ok(SUCCEEDED(hr), "Failed to create decoder, hr %#lx.\n", hr);

        decoder_info = NULL;
        hr = IWICBitmapDecoder_GetDecoderInfo(decoder, &decoder_info);
        ok(hr == S_OK || broken(IsEqualCLSID(&CLSID_WICBmpDecoder, test->clsid) && FAILED(hr)) /* Fails on Windows */,
            "%u: failed to get decoder info, hr %#lx.\n", i, hr);

        if (hr == S_OK)
        {
            decoder_info2 = NULL;
            hr = IWICBitmapDecoder_GetDecoderInfo(decoder, &decoder_info2);
            ok(hr == S_OK, "Failed to get decoder info, hr %#lx.\n", hr);
            ok(decoder_info == decoder_info2, "Unexpected decoder info instance.\n");

            hr = IWICBitmapDecoderInfo_QueryInterface(decoder_info, &IID_IWICBitmapDecoder, (void **)&decoder2);
            ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);

            IWICBitmapDecoderInfo_Release(decoder_info);
            IWICBitmapDecoderInfo_Release(decoder_info2);
        }
        IWICBitmapDecoder_Release(decoder);

        hr = get_component_info(test->clsid, &info);
        ok(hr == S_OK, "CreateComponentInfo failed, hr=%lx\n", hr);

        hr = IWICComponentInfo_QueryInterface(info, &IID_IWICBitmapDecoderInfo, (void **)&decoder_info);
        ok(hr == S_OK, "QueryInterface failed, hr=%lx\n", hr);

        hr = IWICBitmapDecoderInfo_GetCLSID(decoder_info, NULL);
        ok(hr == E_INVALIDARG, "GetCLSID failed, hr=%lx\n", hr);

        hr = IWICBitmapDecoderInfo_GetCLSID(decoder_info, &clsid);
        ok(hr == S_OK, "GetCLSID failed, hr=%lx\n", hr);
        ok(IsEqualGUID(test->clsid, &clsid), "GetCLSID returned wrong result\n");

        hr = IWICBitmapDecoderInfo_GetMimeTypes(decoder_info, 0, NULL, NULL);
        ok(hr == E_INVALIDARG, "GetMimeType failed, hr=%lx\n", hr);

        len = 0;
        hr = IWICBitmapDecoderInfo_GetMimeTypes(decoder_info, 1, NULL, &len);
        ok(hr == E_INVALIDARG, "GetMimeType failed, hr=%lx\n", hr);
        todo_wine_if(test->todo)
        ok(len == lstrlenW(test->mimetype) + 1, "GetMimeType returned wrong len %i\n", len);

        hr = IWICBitmapDecoderInfo_GetMimeTypes(decoder_info, len, value, NULL);
        ok(hr == E_INVALIDARG, "GetMimeType failed, hr=%lx\n", hr);

        len = 0;
        hr = IWICBitmapDecoderInfo_GetMimeTypes(decoder_info, 0, NULL, &len);
        ok(hr == S_OK, "GetMimeType failed, hr=%lx\n", hr);
        todo_wine_if(test->todo)
        ok(len == lstrlenW(test->mimetype) + 1, "GetMimeType returned wrong len %i\n", len);

        value[0] = 0;
        hr = IWICBitmapDecoderInfo_GetMimeTypes(decoder_info, len, value, &len);
        ok(hr == S_OK, "GetMimeType failed, hr=%lx\n", hr);
    todo_wine_if(test->todo) {
        ok(!lstrcmpW(value, test->mimetype), "GetMimeType returned wrong value %s\n", wine_dbgstr_w(value));
        ok(len == lstrlenW(test->mimetype) + 1, "GetMimeType returned wrong len %i\n", len);
    }
        hr = IWICBitmapDecoderInfo_GetMimeTypes(decoder_info, 1, value, &len);
        ok(hr == WINCODEC_ERR_INSUFFICIENTBUFFER, "GetMimeType failed, hr=%lx\n", hr);
        todo_wine_if(test->todo)
        ok(len == lstrlenW(test->mimetype) + 1, "GetMimeType returned wrong len %i\n", len);

        hr = IWICBitmapDecoderInfo_GetMimeTypes(decoder_info, 256, value, &len);
        ok(hr == S_OK, "GetMimeType failed, hr=%lx\n", hr);
    todo_wine_if(test->todo) {
        ok(!lstrcmpW(value, test->mimetype), "GetMimeType returned wrong value %s\n", wine_dbgstr_w(value));
        ok(len == lstrlenW(test->mimetype) + 1, "GetMimeType returned wrong len %i\n", len);
    }
        num_formats = 0xdeadbeef;
        hr = IWICBitmapDecoderInfo_GetPixelFormats(decoder_info, 0, NULL, &num_formats);
        ok(hr == S_OK, "GetPixelFormats failed, hr=%lx\n", hr);
        ok((num_formats <= 21 && num_formats >= 1) ||
            broken(IsEqualCLSID(test->clsid, &CLSID_WICIcoDecoder) && num_formats == 0) /* WinXP */,
            "%u: got %d formats\n", i, num_formats);

        hr = IWICBitmapDecoderInfo_GetPixelFormats(decoder_info, 0, NULL, NULL);
        ok(hr == E_INVALIDARG, "GetPixelFormats failed, hr=%lx\n", hr);

        count = 0xdeadbeef;
        hr = IWICBitmapDecoderInfo_GetPixelFormats(decoder_info, 0, pixelformats, &count);
        ok(hr == S_OK, "GetPixelFormats failed, hr=%lx\n", hr);
        ok(count == 0, "got %d formats\n", count);

        count = 0xdeadbeef;
        hr = IWICBitmapDecoderInfo_GetPixelFormats(decoder_info, 1, pixelformats, &count);
        ok(hr == S_OK, "GetPixelFormats failed, hr=%lx\n", hr);
        ok((count == 1) || broken(IsEqualCLSID(test->clsid, &CLSID_WICIcoDecoder) && count == 0) /* WinXP */,
            "%u: got %d formats\n", i, num_formats);
        ok(is_pixelformat(&pixelformats[0]), "got invalid pixel format\n");

        count = 0xdeadbeef;
        hr = IWICBitmapDecoderInfo_GetPixelFormats(decoder_info, num_formats, pixelformats, &count);
        ok(hr == S_OK, "GetPixelFormats failed, hr=%lx\n", hr);
        ok(count == num_formats, "got %d formats, expected %d\n", count, num_formats);
        for (j = 0; j < num_formats; j++)
            ok(is_pixelformat(&pixelformats[j]), "got invalid pixel format\n");

        hr = IWICBitmapDecoderInfo_GetPixelFormats(decoder_info, num_formats, pixelformats, NULL);
        ok(hr == E_INVALIDARG, "GetPixelFormats failed, hr=%lx\n", hr);

        count = 0xdeadbeef;
        hr = IWICBitmapDecoderInfo_GetPixelFormats(decoder_info, ARRAY_SIZE(pixelformats),
                                                   pixelformats, &count);
        ok(hr == S_OK, "GetPixelFormats failed, hr=%lx\n", hr);
        ok(count == num_formats, "got %d formats, expected %d\n", count, num_formats);

        hr = IWICBitmapDecoderInfo_GetFileExtensions(decoder_info, 0, NULL, NULL);
        ok(hr == E_INVALIDARG, "GetFileExtensions failed, hr=%lx\n", hr);

        hr = IWICBitmapDecoderInfo_GetFileExtensions(decoder_info, 1, NULL, &len);
        ok(hr == E_INVALIDARG, "GetFileExtensions failed, hr=%lx\n", hr);
        todo_wine_if(test->todo && !IsEqualCLSID(test->clsid, &CLSID_WICTiffDecoder))
        ok(len == lstrlenW(test->extensions) + 1, "%u: GetFileExtensions returned wrong len %i\n", i, len);

        hr = IWICBitmapDecoderInfo_GetFileExtensions(decoder_info, len, value, NULL);
        ok(hr == E_INVALIDARG, "GetFileExtensions failed, hr=%lx\n", hr);

        hr = IWICBitmapDecoderInfo_GetFileExtensions(decoder_info, 0, NULL, &len);
        ok(hr == S_OK, "GetFileExtensions failed, hr=%lx\n", hr);
        todo_wine_if(test->todo && !IsEqualCLSID(test->clsid, &CLSID_WICTiffDecoder))
        ok(len == lstrlenW(test->extensions) + 1, "GetFileExtensions returned wrong len %i\n", len);

        value[0] = 0;
        hr = IWICBitmapDecoderInfo_GetFileExtensions(decoder_info, len, value, &len);
        ok(hr == S_OK, "GetFileExtensions failed, hr=%lx\n", hr);
        todo_wine_if(test->todo)
        ok(lstrcmpW(value, test->extensions) == 0, "GetFileExtensions returned wrong value %s\n", wine_dbgstr_w(value));
        todo_wine_if(test->todo && !IsEqualCLSID(test->clsid, &CLSID_WICTiffDecoder))
        ok(len == lstrlenW(test->extensions) + 1, "GetFileExtensions returned wrong len %i\n", len);

        hr = IWICBitmapDecoderInfo_GetFileExtensions(decoder_info, 1, value, &len);
        ok(hr == WINCODEC_ERR_INSUFFICIENTBUFFER, "GetFileExtensions failed, hr=%lx\n", hr);
        todo_wine_if(test->todo && !IsEqualCLSID(test->clsid, &CLSID_WICTiffDecoder))
        ok(len == lstrlenW(test->extensions) + 1, "GetFileExtensions returned wrong len %i\n", len);

        hr = IWICBitmapDecoderInfo_GetFileExtensions(decoder_info, 256, value, &len);
        ok(hr == S_OK, "GetFileExtensions failed, hr=%lx\n", hr);
        todo_wine_if(test->todo)
        ok(!lstrcmpW(value, test->extensions), "GetFileExtensions returned wrong value %s\n", wine_dbgstr_w(value));
        todo_wine_if(test->todo && !IsEqualCLSID(test->clsid, &CLSID_WICTiffDecoder))
        ok(len == lstrlenW(test->extensions) + 1, "GetFileExtensions returned wrong len %i\n", len);

        IWICBitmapDecoderInfo_Release(decoder_info);
        IWICComponentInfo_Release(info);
    }
}

static void test_encoder_info(void)
{
    static const struct encoder_info_test
    {
        const CLSID *clsid;
        const WCHAR *mimetype;
        const WCHAR *extensions;
    } encoder_info_tests[] =
    {
        {
            &CLSID_WICBmpEncoder,
            L"image/bmp",
            L".bmp,.dib,.rle"
        },
        {
            &CLSID_WICGifEncoder,
            L"image/gif",
            L".gif"
        },
        {
            &CLSID_WICPngEncoder,
            L"image/png",
            L".png"
        },
        {
            &CLSID_WICJpegEncoder,
            L"image/jpeg,image/jpe,image/jpg",
            L".jpeg,.jpe,.jpg,.jfif,.exif",
        },
        {
            &CLSID_WICTiffEncoder,
            L"image/tiff,image/tif",
            L".tiff,.tif",
        },
    };
    IWICBitmapEncoderInfo *encoder_info, *encoder_info2;
    IWICComponentInfo *info;
    UINT num_formats, count;
    GUID pixelformats[32];
    WCHAR value[256];
    HRESULT hr;
    CLSID clsid;
    UINT len;
    int i, j;

    for (i = 0; i < ARRAY_SIZE(encoder_info_tests); i++)
    {
        const struct encoder_info_test *test = &encoder_info_tests[i];
        IWICBitmapEncoder *encoder, *encoder2;

        hr = CoCreateInstance(test->clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IWICBitmapEncoder, (void **)&encoder);
        if (IsEqualGUID(&test->clsid, &CLSID_WICDdsDecoder) && hr != S_OK)
        {
            win_skip("DDS encoder is not supported\n");
            continue;
        }
        ok(hr == S_OK, "Failed to create an encoder, hr %#lx.\n", hr);

        winetest_push_context("%s", wine_dbgstr_guid(test->clsid));

        encoder_info = NULL;
        hr = IWICBitmapEncoder_GetEncoderInfo(encoder, &encoder_info);
        ok(hr == S_OK || broken(IsEqualCLSID(&CLSID_WICBmpEncoder, test->clsid) && FAILED(hr)) /* Fails on Windows */,
            "Failed to get encoder info, hr %#lx.\n", hr);

        if (hr == S_OK)
        {
            encoder_info2 = NULL;
            hr = IWICBitmapEncoder_GetEncoderInfo(encoder, &encoder_info2);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(encoder_info == encoder_info2, "Unexpected info instance.\n");

            hr = IWICBitmapEncoderInfo_QueryInterface(encoder_info, &IID_IWICBitmapEncoder, (void **)&encoder2);
            ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);

            IWICBitmapEncoderInfo_Release(encoder_info);
            IWICBitmapEncoderInfo_Release(encoder_info2);
        }
        IWICBitmapEncoder_Release(encoder);

        hr = get_component_info(test->clsid, &info);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IWICComponentInfo_QueryInterface(info, &IID_IWICBitmapEncoderInfo, (void **)&encoder_info);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IWICBitmapEncoderInfo_GetCLSID(encoder_info, NULL);
        ok(hr == E_INVALIDARG, "GetCLSID failed, hr=%lx\n", hr);

        hr = IWICBitmapEncoderInfo_GetCLSID(encoder_info, &clsid);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(IsEqualGUID(test->clsid, &clsid), "GetCLSID returned wrong result\n");

        hr = IWICBitmapEncoderInfo_GetMimeTypes(encoder_info, 0, NULL, NULL);
        ok(hr == E_INVALIDARG, "GetMimeType failed, hr=%lx\n", hr);

        len = 0;
        hr = IWICBitmapEncoderInfo_GetMimeTypes(encoder_info, 1, NULL, &len);
        ok(hr == E_INVALIDARG, "GetMimeType failed, hr=%lx\n", hr);
        ok(len == lstrlenW(test->mimetype) + 1, "GetMimeType returned wrong len %i\n", len);

        hr = IWICBitmapEncoderInfo_GetMimeTypes(encoder_info, len, value, NULL);
        ok(hr == E_INVALIDARG, "GetMimeType failed, hr=%lx\n", hr);

        len = 0;
        hr = IWICBitmapEncoderInfo_GetMimeTypes(encoder_info, 0, NULL, &len);
        ok(hr == S_OK, "GetMimeType failed, hr=%lx\n", hr);
        ok(len == lstrlenW(test->mimetype) + 1, "GetMimeType returned wrong len %i\n", len);

        value[0] = 0;
        hr = IWICBitmapEncoderInfo_GetMimeTypes(encoder_info, len, value, &len);
        ok(hr == S_OK, "GetMimeType failed, hr=%lx\n", hr);
        ok(!lstrcmpW(value, test->mimetype), "GetMimeType returned wrong value %s\n", wine_dbgstr_w(value));
        ok(len == lstrlenW(test->mimetype) + 1, "GetMimeType returned wrong len %i\n", len);

        hr = IWICBitmapEncoderInfo_GetMimeTypes(encoder_info, 1, value, &len);
        ok(hr == WINCODEC_ERR_INSUFFICIENTBUFFER, "GetMimeType failed, hr=%lx\n", hr);
        ok(len == lstrlenW(test->mimetype) + 1, "GetMimeType returned wrong len %i\n", len);

        hr = IWICBitmapEncoderInfo_GetMimeTypes(encoder_info, 256, value, &len);
        ok(hr == S_OK, "GetMimeType failed, hr=%lx\n", hr);
        ok(lstrcmpW(value, test->mimetype) == 0, "GetMimeType returned wrong value %s\n", wine_dbgstr_w(value));
        ok(len == lstrlenW(test->mimetype) + 1, "GetMimeType returned wrong len %i\n", len);

        num_formats = 0;
        hr = IWICBitmapEncoderInfo_GetPixelFormats(encoder_info, 0, NULL, &num_formats);
        ok(hr == S_OK, "GetPixelFormats failed, hr=%lx\n", hr);
        ok(num_formats, "Unexpected number of formats %u.\n", num_formats);

        hr = IWICBitmapEncoderInfo_GetPixelFormats(encoder_info, 0, NULL, NULL);
        ok(hr == E_INVALIDARG, "GetPixelFormats failed, hr=%lx\n", hr);

        count = 0xdeadbeef;
        hr = IWICBitmapEncoderInfo_GetPixelFormats(encoder_info, 0, pixelformats, &count);
        ok(hr == S_OK, "GetPixelFormats failed, hr=%lx\n", hr);
        ok(count == 0, "got %d formats\n", count);

        count = 0;
        hr = IWICBitmapEncoderInfo_GetPixelFormats(encoder_info, 1, pixelformats, &count);
        ok(hr == S_OK, "GetPixelFormats failed, hr=%lx\n", hr);
        ok(count == 1, "%u: got %d formats\n", i, num_formats);
        ok(is_pixelformat(&pixelformats[0]), "got invalid pixel format\n");

        count = 0xdeadbeef;
        hr = IWICBitmapEncoderInfo_GetPixelFormats(encoder_info, num_formats, pixelformats, &count);
        ok(hr == S_OK, "GetPixelFormats failed, hr=%lx\n", hr);
        ok(count == num_formats, "got %d formats, expected %d\n", count, num_formats);
        for (j = 0; j < num_formats; j++)
            ok(is_pixelformat(&pixelformats[j]), "got invalid pixel format\n");

        hr = IWICBitmapEncoderInfo_GetPixelFormats(encoder_info, num_formats, pixelformats, NULL);
        ok(hr == E_INVALIDARG, "GetPixelFormats failed, hr=%lx\n", hr);

        count = 0xdeadbeef;
        hr = IWICBitmapEncoderInfo_GetPixelFormats(encoder_info, ARRAY_SIZE(pixelformats), pixelformats, &count);
        ok(hr == S_OK, "GetPixelFormats failed, hr=%lx\n", hr);
        ok(count == num_formats, "got %d formats, expected %d\n", count, num_formats);

        hr = IWICBitmapEncoderInfo_GetFileExtensions(encoder_info, 0, NULL, NULL);
        ok(hr == E_INVALIDARG, "GetFileExtensions failed, hr=%lx\n", hr);

        hr = IWICBitmapEncoderInfo_GetFileExtensions(encoder_info, 1, NULL, &len);
        ok(hr == E_INVALIDARG, "GetFileExtensions failed, hr=%lx\n", hr);
        ok(len == lstrlenW(test->extensions) + 1, "Unexpected length %i.\n", len);

        hr = IWICBitmapEncoderInfo_GetFileExtensions(encoder_info, len, value, NULL);
        ok(hr == E_INVALIDARG, "GetFileExtensions failed, hr=%lx\n", hr);

        hr = IWICBitmapEncoderInfo_GetFileExtensions(encoder_info, 0, NULL, &len);
        ok(hr == S_OK, "GetFileExtensions failed, hr=%lx\n", hr);
        ok(len == lstrlenW(test->extensions) + 1, "GetFileExtensions returned wrong len %i\n", len);

        value[0] = 0;
        hr = IWICBitmapEncoderInfo_GetFileExtensions(encoder_info, len, value, &len);
        ok(hr == S_OK, "GetFileExtensions failed, hr=%lx\n", hr);
        ok(lstrcmpW(value, test->extensions) == 0, "GetFileExtensions returned wrong value %s\n", wine_dbgstr_w(value));
        ok(len == lstrlenW(test->extensions) + 1, "GetFileExtensions returned wrong len %i\n", len);

        hr = IWICBitmapEncoderInfo_GetFileExtensions(encoder_info, 1, value, &len);
        ok(hr == WINCODEC_ERR_INSUFFICIENTBUFFER, "GetFileExtensions failed, hr=%lx\n", hr);
        ok(len == lstrlenW(test->extensions) + 1, "GetFileExtensions returned wrong len %i\n", len);

        hr = IWICBitmapEncoderInfo_GetFileExtensions(encoder_info, 256, value, &len);
        ok(hr == S_OK, "GetFileExtensions failed, hr=%lx\n", hr);
        ok(lstrcmpW(value, test->extensions) == 0, "GetFileExtensions returned wrong value %s\n", wine_dbgstr_w(value));
        ok(len == lstrlenW(test->extensions) + 1, "GetFileExtensions returned wrong len %i\n", len);

        IWICBitmapEncoderInfo_Release(encoder_info);
        IWICComponentInfo_Release(info);

        winetest_pop_context();
    }
}

static void test_pixelformat_info(void)
{
    IWICComponentInfo *info;
    IWICPixelFormatInfo *pixelformat_info;
    IWICPixelFormatInfo2 *pixelformat_info2;
    HRESULT hr;
    UINT len, known_len;
    WCHAR value[256];
    GUID guid;
    WICComponentType componenttype;
    WICPixelFormatNumericRepresentation numericrepresentation;
    DWORD signing;
    UINT uiresult;
    BYTE abbuffer[256];
    BOOL supportstransparency;

    hr = get_component_info(&GUID_WICPixelFormat32bppBGRA, &info);
    ok(hr == S_OK, "CreateComponentInfo failed, hr=%lx\n", hr);

    if (FAILED(hr))
        return;

    hr = IWICComponentInfo_GetAuthor(info, 0, NULL, 0);
    ok(hr == E_INVALIDARG, "GetAuthor failed, hr=%lx\n", hr);

    len = 0xdeadbeef;
    hr = IWICComponentInfo_GetAuthor(info, 0, NULL, &len);
    ok(hr == S_OK, "GetAuthor failed, hr=%lx\n", hr);
    ok(len < 255 && len > 0, "invalid length 0x%x\n", len);
    known_len = len;

    memset(value, 0xaa, 256 * sizeof(WCHAR));
    hr = IWICComponentInfo_GetAuthor(info, len-1, value, NULL);
    ok(hr == E_INVALIDARG, "GetAuthor failed, hr=%lx\n", hr);
    ok(value[0] == 0xaaaa, "string modified\n");

    len = 0xdeadbeef;
    memset(value, 0xaa, 256 * sizeof(WCHAR));
    hr = IWICComponentInfo_GetAuthor(info, known_len-1, value, &len);
    ok(hr == WINCODEC_ERR_INSUFFICIENTBUFFER, "GetAuthor failed, hr=%lx\n", hr);
    ok(len == known_len, "got length of 0x%x, expected 0x%x\n", len, known_len);
    ok(value[known_len-1] == 0xaaaa, "string modified past given length\n");
    ok(value[0] == 0xaaaa, "string modified\n");

    len = 0xdeadbeef;
    memset(value, 0xaa, 256 * sizeof(WCHAR));
    hr = IWICComponentInfo_GetAuthor(info, known_len, value, &len);
    ok(hr == S_OK, "GetAuthor failed, hr=%lx\n", hr);
    ok(len == known_len, "got length of 0x%x, expected 0x%x\n", len, known_len);
    ok(value[known_len-1] == 0, "string not terminated at expected length\n");
    ok(value[known_len-2] != 0xaaaa, "string not modified at given length\n");

    len = 0xdeadbeef;
    memset(value, 0xaa, 256 * sizeof(WCHAR));
    hr = IWICComponentInfo_GetAuthor(info, known_len+1, value, &len);
    ok(hr == S_OK, "GetAuthor failed, hr=%lx\n", hr);
    ok(len == known_len, "got length of 0x%x, expected 0x%x\n", len, known_len);
    ok(value[known_len] == 0xaaaa, "string modified past end\n");
    ok(value[known_len-1] == 0, "string not terminated at expected length\n");
    ok(value[known_len-2] != 0xaaaa, "string not modified at given length\n");

    hr = IWICComponentInfo_GetCLSID(info, NULL);
    ok(hr == E_INVALIDARG, "GetCLSID failed, hr=%lx\n", hr);

    memset(&guid, 0xaa, sizeof(guid));
    hr = IWICComponentInfo_GetCLSID(info, &guid);
    ok(hr == S_OK, "GetCLSID failed, hr=%lx\n", hr);
    ok(IsEqualGUID(&guid, &GUID_WICPixelFormat32bppBGRA), "unexpected CLSID %s\n", wine_dbgstr_guid(&guid));

    hr = IWICComponentInfo_GetComponentType(info, NULL);
    ok(hr == E_INVALIDARG, "GetComponentType failed, hr=%lx\n", hr);

    hr = IWICComponentInfo_GetComponentType(info, &componenttype);
    ok(hr == S_OK, "GetComponentType failed, hr=%lx\n", hr);
    ok(componenttype == WICPixelFormat, "unexpected component type 0x%x\n", componenttype);

    len = 0xdeadbeef;
    hr = IWICComponentInfo_GetFriendlyName(info, 0, NULL, &len);
    ok(hr == S_OK, "GetFriendlyName failed, hr=%lx\n", hr);
    ok(len < 255 && len > 0, "invalid length 0x%x\n", len);

    hr = IWICComponentInfo_GetSigningStatus(info, NULL);
    ok(hr == E_INVALIDARG, "GetSigningStatus failed, hr=%lx\n", hr);

    hr = IWICComponentInfo_GetSigningStatus(info, &signing);
    ok(hr == S_OK, "GetSigningStatus failed, hr=%lx\n", hr);
    ok(signing == WICComponentSigned, "unexpected signing status 0x%lx\n", signing);

    len = 0xdeadbeef;
    hr = IWICComponentInfo_GetSpecVersion(info, 0, NULL, &len);
    ok(hr == S_OK, "GetSpecVersion failed, hr=%lx\n", hr);
    ok(len == 0, "invalid length 0x%x\n", len); /* spec version does not apply to pixel formats */

    memset(&guid, 0xaa, sizeof(guid));
    hr = IWICComponentInfo_GetVendorGUID(info, &guid);
    ok(hr == S_OK, "GetVendorGUID failed, hr=%lx\n", hr);
    ok(IsEqualGUID(&guid, &GUID_VendorMicrosoft) ||
       broken(IsEqualGUID(&guid, &GUID_NULL)) /* XP */, "unexpected GUID %s\n", wine_dbgstr_guid(&guid));

    len = 0xdeadbeef;
    hr = IWICComponentInfo_GetVersion(info, 0, NULL, &len);
    ok(hr == S_OK, "GetVersion failed, hr=%lx\n", hr);
    ok(len == 0, "invalid length 0x%x\n", len); /* version does not apply to pixel formats */

    hr = IWICComponentInfo_QueryInterface(info, &IID_IWICPixelFormatInfo, (void**)&pixelformat_info);
    ok(hr == S_OK, "QueryInterface failed, hr=%lx\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = IWICPixelFormatInfo_GetBitsPerPixel(pixelformat_info, NULL);
        ok(hr == E_INVALIDARG, "GetBitsPerPixel failed, hr=%lx\n", hr);

        hr = IWICPixelFormatInfo_GetBitsPerPixel(pixelformat_info, &uiresult);
        ok(hr == S_OK, "GetBitsPerPixel failed, hr=%lx\n", hr);
        ok(uiresult == 32, "unexpected bpp %i\n", uiresult);

        hr = IWICPixelFormatInfo_GetChannelCount(pixelformat_info, &uiresult);
        ok(hr == S_OK, "GetChannelCount failed, hr=%lx\n", hr);
        ok(uiresult == 4, "unexpected channel count %i\n", uiresult);

        hr = IWICPixelFormatInfo_GetChannelMask(pixelformat_info, 0, 0, NULL, NULL);
        ok(hr == E_INVALIDARG, "GetChannelMask failed, hr=%lx\n", hr);

        uiresult = 0xdeadbeef;
        hr = IWICPixelFormatInfo_GetChannelMask(pixelformat_info, 0, 0, NULL, &uiresult);
        ok(hr == S_OK, "GetChannelMask failed, hr=%lx\n", hr);
        ok(uiresult == 4, "unexpected length %i\n", uiresult);

        memset(abbuffer, 0xaa, sizeof(abbuffer));
        hr = IWICPixelFormatInfo_GetChannelMask(pixelformat_info, 0, known_len, abbuffer, NULL);
        ok(hr == E_INVALIDARG, "GetChannelMask failed, hr=%lx\n", hr);
        ok(abbuffer[0] == 0xaa, "buffer modified\n");

        uiresult = 0xdeadbeef;
        memset(abbuffer, 0xaa, sizeof(abbuffer));
        hr = IWICPixelFormatInfo_GetChannelMask(pixelformat_info, 0, 3, abbuffer, &uiresult);
        ok(hr == E_INVALIDARG, "GetChannelMask failed, hr=%lx\n", hr);
        ok(abbuffer[0] == 0xaa, "buffer modified\n");
        ok(uiresult == 4, "unexpected length %i\n", uiresult);

        memset(abbuffer, 0xaa, sizeof(abbuffer));
        hr = IWICPixelFormatInfo_GetChannelMask(pixelformat_info, 0, 4, abbuffer, &uiresult);
        ok(hr == S_OK, "GetChannelMask failed, hr=%lx\n", hr);
        ok(*((ULONG*)abbuffer) == 0xff, "unexpected mask 0x%lx\n", *((ULONG*)abbuffer));
        ok(uiresult == 4, "unexpected length %i\n", uiresult);

        memset(abbuffer, 0xaa, sizeof(abbuffer));
        hr = IWICPixelFormatInfo_GetChannelMask(pixelformat_info, 0, 5, abbuffer, &uiresult);
        ok(hr == S_OK, "GetChannelMask failed, hr=%lx\n", hr);
        ok(*((ULONG*)abbuffer) == 0xff, "unexpected mask 0x%lx\n", *((ULONG*)abbuffer));
        ok(abbuffer[4] == 0xaa, "buffer modified past actual length\n");
        ok(uiresult == 4, "unexpected length %i\n", uiresult);

        memset(&guid, 0xaa, sizeof(guid));
        hr = IWICPixelFormatInfo_GetFormatGUID(pixelformat_info, &guid);
        ok(hr == S_OK, "GetFormatGUID failed, hr=%lx\n", hr);
        ok(IsEqualGUID(&guid, &GUID_WICPixelFormat32bppBGRA), "unexpected GUID %s\n", wine_dbgstr_guid(&guid));

        IWICPixelFormatInfo_Release(pixelformat_info);
    }

    hr = IWICComponentInfo_QueryInterface(info, &IID_IWICPixelFormatInfo2, (void**)&pixelformat_info2);

    if (FAILED(hr))
        win_skip("IWICPixelFormatInfo2 not supported\n");
    else
    {
        hr = IWICPixelFormatInfo2_GetNumericRepresentation(pixelformat_info2, NULL);
        ok(hr == E_INVALIDARG, "GetNumericRepresentation failed, hr=%lx\n", hr);

        numericrepresentation = 0xdeadbeef;
        hr = IWICPixelFormatInfo2_GetNumericRepresentation(pixelformat_info2, &numericrepresentation);
        ok(hr == S_OK, "GetNumericRepresentation failed, hr=%lx\n", hr);
        ok(numericrepresentation == WICPixelFormatNumericRepresentationUnsignedInteger, "unexpected numeric representation %i\n", numericrepresentation);

        hr = IWICPixelFormatInfo2_SupportsTransparency(pixelformat_info2, NULL);
        ok(hr == E_INVALIDARG, "SupportsTransparency failed, hr=%lx\n", hr);

        supportstransparency = 0xdeadbeef;
        hr = IWICPixelFormatInfo2_SupportsTransparency(pixelformat_info2, &supportstransparency);
        ok(hr == S_OK, "SupportsTransparency failed, hr=%lx\n", hr);
        ok(supportstransparency == 1, "unexpected value %i\n", supportstransparency);

        IWICPixelFormatInfo2_Release(pixelformat_info2);
    }

    IWICComponentInfo_Release(info);
}

static DWORD WINAPI cache_across_threads_test(void *arg)
{
    IWICComponentInfo *info;
    HRESULT hr;

    CoInitialize(NULL);

    hr = get_component_info(&CLSID_WICUnknownMetadataReader, &info);
    ok(hr == S_OK, "CreateComponentInfo failed, hr=%lx\n", hr);
    ok(info == arg, "unexpected info pointer %p\n", info);
    IWICComponentInfo_Release(info);

    CoUninitialize();
    return 0;
}

static void test_reader_info(void)
{
    IWICMetadataHandlerInfo *handler_info;
    IWICImagingFactory *factory;
    IWICComponentInfo *info, *info2;
    IWICMetadataReaderInfo *reader_info;
    IWICMetadataReader *reader;
    HRESULT hr;
    CLSID clsid;
    GUID container_formats[10];
    UINT count, size;
    DWORD tid;
    HANDLE thread;
    WICMetadataPattern *patterns;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICImagingFactory, (void**)&factory);
    ok(hr == S_OK, "CoCreateInstance failed, hr=%lx\n", hr);

    for (int i = 0; i < ARRAY_SIZE(metadata_handlers); ++i)
    {
        hr = IWICImagingFactory_CreateComponentInfo(factory, metadata_handlers[i], &info);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        IWICComponentInfo_Release(info);

        hr = CoCreateInstance(metadata_handlers[i], NULL, CLSCTX_INPROC_SERVER,
                &IID_IWICMetadataReader, (void **)&reader);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IWICMetadataReader_GetMetadataHandlerInfo(reader, &handler_info);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        hr = IWICMetadataHandlerInfo_GetCLSID(handler_info, &clsid);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(IsEqualGUID(&clsid, metadata_handlers[i]), "Unexpected clsid %s.\n", wine_dbgstr_guid(&clsid));
        IWICMetadataHandlerInfo_Release(handler_info);

        IWICMetadataReader_Release(reader);
    }

    hr = get_component_info(&CLSID_WICUnknownMetadataReader, &info2);
    ok(hr == S_OK, "CreateComponentInfo failed, hr=%lx\n", hr);
    IWICComponentInfo_Release(info2);

    hr = IWICImagingFactory_CreateComponentInfo(factory, &CLSID_WICUnknownMetadataReader, &info);
    ok(hr == S_OK, "CreateComponentInfo failed, hr=%lx\n", hr);
    ok(info == info2, "info != info2\n");

    thread = CreateThread(NULL, 0, cache_across_threads_test, info, 0, &tid);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);

    hr = IWICComponentInfo_QueryInterface(info, &IID_IWICMetadataReaderInfo, (void**)&reader_info);
    ok(hr == S_OK, "QueryInterface failed, hr=%lx\n", hr);

    hr = IWICMetadataReaderInfo_GetCLSID(reader_info, NULL);
    ok(hr == E_INVALIDARG, "GetCLSID failed, hr=%lx\n", hr);

    hr = IWICMetadataReaderInfo_GetCLSID(reader_info, &clsid);
    ok(hr == S_OK, "GetCLSID failed, hr=%lx\n", hr);
    ok(IsEqualGUID(&CLSID_WICUnknownMetadataReader, &clsid), "GetCLSID returned wrong result\n");

    hr = IWICMetadataReaderInfo_GetMetadataFormat(reader_info, &clsid);
    ok(hr == S_OK, "GetMetadataFormat failed, hr=%lx\n", hr);
    ok(IsEqualGUID(&GUID_MetadataFormatUnknown, &clsid), "GetMetadataFormat returned wrong result\n");

    hr = IWICMetadataReaderInfo_GetContainerFormats(reader_info, 0, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetContainerFormats failed, hr=%lx\n", hr);

    count = 0xdeadbeef;
    hr = IWICMetadataReaderInfo_GetContainerFormats(reader_info, 0, NULL, &count);
    ok(hr == S_OK, "GetContainerFormats failed, hr=%lx\n", hr);
    ok(count == 0, "unexpected count %d\n", count);

    hr = IWICMetadataReaderInfo_GetPatterns(reader_info, &GUID_ContainerFormatPng,
        0, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetPatterns failed, hr=%lx\n", hr);

    count = size = 0xdeadbeef;
    hr = IWICMetadataReaderInfo_GetPatterns(reader_info, &GUID_ContainerFormatPng,
        0, NULL, &count, &size);
    ok(hr == WINCODEC_ERR_COMPONENTNOTFOUND || broken(hr == S_OK) /* Windows XP */,
        "GetPatterns failed, hr=%lx\n", hr);
    ok(count == 0xdeadbeef, "unexpected count %d\n", count);
    ok(size == 0xdeadbeef, "unexpected size %d\n", size);

    IWICMetadataReaderInfo_Release(reader_info);

    IWICComponentInfo_Release(info);

    hr = IWICImagingFactory_CreateComponentInfo(factory, &CLSID_WICXMPStructMetadataReader, &info);
    todo_wine
    ok(hr == S_OK, "CreateComponentInfo failed, hr=%lx\n", hr);

    if (FAILED(hr))
    {
        IWICImagingFactory_Release(factory);
        return;
    }

    hr = IWICComponentInfo_QueryInterface(info, &IID_IWICMetadataReaderInfo, (void**)&reader_info);
    ok(hr == S_OK, "QueryInterface failed, hr=%lx\n", hr);

    hr = IWICMetadataReaderInfo_GetCLSID(reader_info, NULL);
    ok(hr == E_INVALIDARG, "GetCLSID failed, hr=%lx\n", hr);

    hr = IWICMetadataReaderInfo_GetCLSID(reader_info, &clsid);
    ok(hr == S_OK, "GetCLSID failed, hr=%lx\n", hr);
    ok(IsEqualGUID(&CLSID_WICXMPStructMetadataReader, &clsid), "GetCLSID returned wrong result\n");

    hr = IWICMetadataReaderInfo_GetMetadataFormat(reader_info, &clsid);
    ok(hr == S_OK, "GetMetadataFormat failed, hr=%lx\n", hr);
    ok(IsEqualGUID(&GUID_MetadataFormatXMPStruct, &clsid), "GetMetadataFormat returned wrong result\n");

    hr = IWICMetadataReaderInfo_GetContainerFormats(reader_info, 0, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetContainerFormats failed, hr=%lx\n", hr);

    count = 0xdeadbeef;
    hr = IWICMetadataReaderInfo_GetContainerFormats(reader_info, 0, NULL, &count);
    ok(hr == S_OK, "GetContainerFormats failed, hr=%lx\n", hr);
    ok(count >= 2, "unexpected count %d\n", count);

    count = 0xdeadbeef;
    hr = IWICMetadataReaderInfo_GetContainerFormats(reader_info, 1, container_formats, &count);
    ok(hr == S_OK, "GetContainerFormats failed, hr=%lx\n", hr);
    ok(count == 1, "unexpected count %d\n", count);

    count = 0xdeadbeef;
    hr = IWICMetadataReaderInfo_GetContainerFormats(reader_info, 10, container_formats, &count);
    ok(hr == S_OK, "GetContainerFormats failed, hr=%lx\n", hr);
    ok(count == min(count, 10), "unexpected count %d\n", count);

    count = size = 0xdeadbeef;
    hr = IWICMetadataReaderInfo_GetPatterns(reader_info, &GUID_ContainerFormatPng,
        0, NULL, &count, &size);
    ok(hr == WINCODEC_ERR_COMPONENTNOTFOUND || broken(hr == S_OK) /* Windows XP */,
        "GetPatterns failed, hr=%lx\n", hr);
    ok(count == 0xdeadbeef, "unexpected count %d\n", count);
    ok(size == 0xdeadbeef, "unexpected size %d\n", size);

    count = size = 0xdeadbeef;
    hr = IWICMetadataReaderInfo_GetPatterns(reader_info, &GUID_MetadataFormatXMP,
        0, NULL, &count, &size);
    ok(hr == S_OK, "GetPatterns failed, hr=%lx\n", hr);
    ok(count == 1, "unexpected count %d\n", count);
    ok(size > sizeof(WICMetadataPattern), "unexpected size %d\n", size);

    if (hr == S_OK)
    {
        patterns = HeapAlloc(GetProcessHeap(), 0, size);

        count = size = 0xdeadbeef;
        hr = IWICMetadataReaderInfo_GetPatterns(reader_info, &GUID_MetadataFormatXMP,
            size-1, patterns, &count, &size);
        ok(hr == S_OK, "GetPatterns failed, hr=%lx\n", hr);
        ok(count == 1, "unexpected count %d\n", count);
        ok(size > sizeof(WICMetadataPattern), "unexpected size %d\n", size);

        count = size = 0xdeadbeef;
        hr = IWICMetadataReaderInfo_GetPatterns(reader_info, &GUID_MetadataFormatXMP,
            size, patterns, &count, &size);
        ok(hr == S_OK, "GetPatterns failed, hr=%lx\n", hr);
        ok(count == 1, "unexpected count %d\n", count);
        ok(size == sizeof(WICMetadataPattern) + patterns->Length * 2, "unexpected size %d\n", size);

        HeapFree(GetProcessHeap(), 0, patterns);
    }

    IWICMetadataReaderInfo_Release(reader_info);

    IWICComponentInfo_Release(info);

    IWICImagingFactory_Release(factory);
}

static void test_imagingfactory_interfaces(void)
{
    IWICComponentFactory *component_factory;
    IWICImagingFactory2 *factory2;
    IWICImagingFactory *factory;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICImagingFactory2, (void **)&factory2);
    if (FAILED(hr))
    {
        win_skip("IWICImagingFactory2 is not supported.\n");
        return;
    }

    hr = IWICImagingFactory2_QueryInterface(factory2, &IID_IWICComponentFactory, (void **)&component_factory);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IWICComponentFactory_QueryInterface(component_factory, &IID_IWICImagingFactory, (void **)&factory);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(factory == (IWICImagingFactory *)component_factory, "Unexpected factory pointer.\n");
    IWICImagingFactory_Release(factory);

    hr = IWICImagingFactory2_QueryInterface(factory2, &IID_IWICImagingFactory, (void **)&factory);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(factory == (IWICImagingFactory *)component_factory, "Unexpected factory pointer.\n");

    IWICComponentFactory_Release(component_factory);
    IWICImagingFactory2_Release(factory2);
    IWICImagingFactory_Release(factory);
}

static void test_component_enumerator(void)
{
    static const unsigned int types[] =
    {
        WICDecoder,
        WICEncoder,
        WICPixelFormatConverter,
        WICMetadataReader,
        WICPixelFormat,
    };
    IWICImagingFactory *factory;
    IEnumUnknown *enumerator;
    unsigned int i;
    IUnknown *item;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
            &IID_IWICImagingFactory, (void **)&factory);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(types); ++i)
    {
        hr = IWICImagingFactory_CreateComponentEnumerator(factory, types[i], 0, &enumerator);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IEnumUnknown_Next(enumerator, 1, &item, NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        IUnknown_Release(item);

        IEnumUnknown_Release(enumerator);
    }

    IWICImagingFactory_Release(factory);
}

static void test_handler_info_cold_cache(void)
{
    IWICMetadataHandlerInfo *info;
    IWICMetadataReader *reader;
    HRESULT hr;

    for (int i = 0; i < ARRAY_SIZE(metadata_handlers); ++i)
    {
        hr = CoCreateInstance(metadata_handlers[i], NULL, CLSCTX_INPROC_SERVER,
                &IID_IWICMetadataReader, (void **)&reader);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IWICMetadataReader_GetMetadataHandlerInfo(reader, &info);
        todo_wine
        ok(hr == WINCODEC_ERR_COMPONENTNOTFOUND, "Unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
            IWICMetadataHandlerInfo_Release(info);

        IWICMetadataReader_Release(reader);
    }
}

START_TEST(info)
{
    char **argv;
    int argc;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    argc = winetest_get_mainargs(&argv);
    if (argc >= 3)
    {
        if (!strcmp(argv[2], "get_handler_info_cold_cache"))
            test_handler_info_cold_cache();

        CoUninitialize();
        return;
    }

    run_child_test("get_handler_info_cold_cache");
    test_decoder_info();
    test_encoder_info();
    test_reader_info();
    test_pixelformat_info();
    test_imagingfactory_interfaces();
    test_component_enumerator();

    CoUninitialize();
}
