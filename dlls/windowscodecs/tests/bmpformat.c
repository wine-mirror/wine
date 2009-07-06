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
#include "initguid.h"
#include "objbase.h"
#include "wincodec.h"
#include "wine/test.h"

static const char testbmp_24bpp[] = {
    /* BITMAPFILEHEADER */
    66,77, /* "BM" */
    50,0,0,0, /* file size */
    0,0,0,0, /* reserved */
    26,0,0,0, /* offset to bits */
    /* BITMAPCOREHEADER */
    12,0,0,0, /* header size */
    2,0, /* width */
    3,0, /* height */
    1,0, /* planes */
    24,0, /* bit count */
    /* bits */
    0,0,0,     0,255,0,     0,0,
    255,0,0,   255,255,0,   0,0,
    255,0,255, 255,255,255, 0,0
};

static void test_decode_24bpp(void)
{
    IWICBitmapDecoder *decoder, *decoder2;
    IWICBitmapFrameDecode *framedecode;
    HRESULT hr;
    HGLOBAL hbmpdata;
    char *bmpdata;
    IStream *bmpstream;
    DWORD capability=0;
    GUID guidresult;
    UINT count=0, width=0, height=0;
    double dpiX, dpiY;
    BYTE imagedata[36] = {1};
    const BYTE expected_imagedata[36] = {
        255,0,255, 255,255,255,
        255,0,0,   255,255,0,
        0,0,0,     0,255,0};
    WICRect rc;

    hr = CoCreateInstance(&CLSID_WICBmpDecoder, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICBitmapDecoder, (void**)&decoder);
    ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%x\n", hr);
    if (!SUCCEEDED(hr)) return;

    hbmpdata = GlobalAlloc(GMEM_MOVEABLE, sizeof(testbmp_24bpp));
    ok(hbmpdata != 0, "GlobalAlloc failed\n");
    if (hbmpdata)
    {
        bmpdata = GlobalLock(hbmpdata);
        memcpy(bmpdata, testbmp_24bpp, sizeof(testbmp_24bpp));
        GlobalUnlock(hbmpdata);

        hr = CreateStreamOnHGlobal(hbmpdata, FALSE, &bmpstream);
        ok(SUCCEEDED(hr), "CreateStreamOnHGlobal failed, hr=%x\n", hr);
        if (SUCCEEDED(hr))
        {
            hr = IWICBitmapDecoder_Initialize(decoder, bmpstream, WICDecodeMetadataCacheOnLoad);
            ok(hr == S_OK, "Initialize failed, hr=%x\n", hr);

            hr = IWICBitmapDecoder_GetContainerFormat(decoder, &guidresult);
            ok(SUCCEEDED(hr), "GetContainerFormat failed, hr=%x\n", hr);
            ok(IsEqualGUID(&guidresult, &GUID_ContainerFormatBmp), "unexpected container format\n");

            hr = IWICBitmapDecoder_GetFrameCount(decoder, &count);
            ok(SUCCEEDED(hr), "GetFrameCount failed, hr=%x\n", hr);
            ok(count == 1, "unexpected count %u\n", count);

            hr = IWICBitmapDecoder_GetFrame(decoder, 1, &framedecode);
            ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %x\n", hr);

            hr = IWICBitmapDecoder_GetFrame(decoder, 0, &framedecode);
            ok(SUCCEEDED(hr), "GetFrame failed, hr=%x\n", hr);
            if (SUCCEEDED(hr))
            {
                IWICImagingFactory *factory;
                IWICPalette *palette;

                hr = IWICBitmapFrameDecode_GetSize(framedecode, &width, &height);
                ok(SUCCEEDED(hr), "GetSize failed, hr=%x\n", hr);
                ok(width == 2, "expected width=2, got %u\n", width);
                ok(height == 3, "expected height=2, got %u\n", height);

                hr = IWICBitmapFrameDecode_GetResolution(framedecode, &dpiX, &dpiY);
                ok(SUCCEEDED(hr), "GetResolution failed, hr=%x\n", hr);
                ok(dpiX == 96.0, "expected dpiX=96.0, got %f\n", dpiX);
                ok(dpiY == 96.0, "expected dpiY=96.0, got %f\n", dpiY);

                hr = IWICBitmapFrameDecode_GetPixelFormat(framedecode, &guidresult);
                ok(SUCCEEDED(hr), "GetPixelFormat failed, hr=%x\n", hr);
                ok(IsEqualGUID(&guidresult, &GUID_WICPixelFormat24bppBGR), "unexpected pixel format\n");

                hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                    &IID_IWICImagingFactory, (void**)&factory);
                ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%x\n", hr);
                if (SUCCEEDED(hr))
                {
                    hr = IWICImagingFactory_CreatePalette(factory, &palette);
                    ok(SUCCEEDED(hr), "CreatePalette failed, hr=%x\n", hr);
                    if (SUCCEEDED(hr))
                    {
                        hr = IWICBitmapDecoder_CopyPalette(decoder, palette);
                        ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "expected WINCODEC_ERR_PALETTEUNAVAILABLE, got %x\n", hr);

                        hr = IWICBitmapFrameDecode_CopyPalette(framedecode, palette);
                        ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "expected WINCODEC_ERR_PALETTEUNAVAILABLE, got %x\n", hr);

                        IWICPalette_Release(palette);
                    }

                    IWICImagingFactory_Release(factory);
                }

                rc.X = 0;
                rc.Y = 0;
                rc.Width = 3;
                rc.Height = 3;
                hr = IWICBitmapFrameDecode_CopyPixels(framedecode, &rc, 6, sizeof(imagedata), imagedata);
                ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %x\n", hr);

                rc.X = -1;
                rc.Y = 0;
                rc.Width = 2;
                rc.Height = 3;
                hr = IWICBitmapFrameDecode_CopyPixels(framedecode, &rc, 6, sizeof(imagedata), imagedata);
                ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %x\n", hr);

                rc.X = 0;
                rc.Y = 0;
                rc.Width = 2;
                rc.Height = 3;
                hr = IWICBitmapFrameDecode_CopyPixels(framedecode, &rc, 4, sizeof(imagedata), imagedata);
                ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %x\n", hr);

                rc.X = 0;
                rc.Y = 0;
                rc.Width = 2;
                rc.Height = 3;
                hr = IWICBitmapFrameDecode_CopyPixels(framedecode, &rc, 4, 5, imagedata);
                ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %x\n", hr);

                rc.X = 0;
                rc.Y = 0;
                rc.Width = 2;
                rc.Height = 3;
                hr = IWICBitmapFrameDecode_CopyPixels(framedecode, &rc, 6, sizeof(imagedata), imagedata);
                ok(SUCCEEDED(hr), "CopyPixels failed, hr=%x\n", hr);
                ok(!memcmp(imagedata, expected_imagedata, sizeof(imagedata)), "unexpected image data\n");

                IWICBitmapFrameDecode_Release(framedecode);
            }

            /* cannot initialize twice */
            hr = IWICBitmapDecoder_Initialize(decoder, bmpstream, WICDecodeMetadataCacheOnLoad);
            ok(hr == WINCODEC_ERR_WRONGSTATE, "expected WINCODEC_ERR_WRONGSTATE, hr=%x\n", hr);

            /* cannot querycapability after initialize */
            hr = IWICBitmapDecoder_QueryCapability(decoder, bmpstream, &capability);
            ok(hr == WINCODEC_ERR_WRONGSTATE, "expected WINCODEC_ERR_WRONGSTATE, hr=%x\n", hr);

            hr = CoCreateInstance(&CLSID_WICBmpDecoder, NULL, CLSCTX_INPROC_SERVER,
                &IID_IWICBitmapDecoder, (void**)&decoder2);
            ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%x\n", hr);
            if (SUCCEEDED(hr))
            {
                hr = IWICBitmapDecoder_QueryCapability(decoder2, bmpstream, &capability);
                ok(hr == S_OK, "QueryCapability failed, hr=%x\n", hr);
                ok(capability == (WICBitmapDecoderCapabilityCanDecodeAllImages),
                    "unexpected capabilities: %x\n", capability);

                /* cannot initialize after querycapability */
                hr = IWICBitmapDecoder_Initialize(decoder2, bmpstream, WICDecodeMetadataCacheOnLoad);
                ok(hr == WINCODEC_ERR_WRONGSTATE, "expected WINCODEC_ERR_WRONGSTATE, hr=%x\n", hr);

                /* cannot querycapability twice */
                hr = IWICBitmapDecoder_QueryCapability(decoder2, bmpstream, &capability);
                ok(hr == WINCODEC_ERR_WRONGSTATE, "expected WINCODEC_ERR_WRONGSTATE, hr=%x\n", hr);
            }

            IStream_Release(bmpstream);
        }

        GlobalFree(hbmpdata);
    }

    IWICBitmapDecoder_Release(decoder);
}

static const char testbmp_1bpp[] = {
    /* BITMAPFILEHEADER */
    66,77, /* "BM" */
    40,0,0,0, /* file size */
    0,0,0,0, /* reserved */
    32,0,0,0, /* offset to bits */
    /* BITMAPCOREHEADER */
    12,0,0,0, /* header size */
    2,0, /* width */
    2,0, /* height */
    1,0, /* planes */
    1,0, /* bit count */
    /* color table */
    255,0,0,
    0,255,0,
    /* bits */
    0xc0,0,0,0,
    0x80,0,0,0
};

static void test_decode_1bpp(void)
{
    IWICBitmapDecoder *decoder, *decoder2;
    IWICBitmapFrameDecode *framedecode;
    HRESULT hr;
    HGLOBAL hbmpdata;
    char *bmpdata;
    IStream *bmpstream;
    DWORD capability=0;
    GUID guidresult;
    UINT count=0, width=0, height=0;
    double dpiX, dpiY;
    BYTE imagedata[2] = {1};
    const BYTE expected_imagedata[2] = {0x80,0xc0};
    WICColor palettedata[2] = {1};
    const WICColor expected_palettedata[2] = {0xff0000ff,0xff00ff00};
    WICRect rc;

    hr = CoCreateInstance(&CLSID_WICBmpDecoder, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICBitmapDecoder, (void**)&decoder);
    ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%x\n", hr);
    if (!SUCCEEDED(hr)) return;

    hbmpdata = GlobalAlloc(GMEM_MOVEABLE, sizeof(testbmp_1bpp));
    ok(hbmpdata != 0, "GlobalAlloc failed\n");
    if (hbmpdata)
    {
        bmpdata = GlobalLock(hbmpdata);
        memcpy(bmpdata, testbmp_1bpp, sizeof(testbmp_1bpp));
        GlobalUnlock(hbmpdata);

        hr = CreateStreamOnHGlobal(hbmpdata, FALSE, &bmpstream);
        ok(SUCCEEDED(hr), "CreateStreamOnHGlobal failed, hr=%x\n", hr);
        if (SUCCEEDED(hr))
        {
            hr = IWICBitmapDecoder_Initialize(decoder, bmpstream, WICDecodeMetadataCacheOnLoad);
            ok(hr == S_OK, "Initialize failed, hr=%x\n", hr);

            hr = IWICBitmapDecoder_GetContainerFormat(decoder, &guidresult);
            ok(SUCCEEDED(hr), "GetContainerFormat failed, hr=%x\n", hr);
            ok(IsEqualGUID(&guidresult, &GUID_ContainerFormatBmp), "unexpected container format\n");

            hr = IWICBitmapDecoder_GetFrameCount(decoder, &count);
            ok(SUCCEEDED(hr), "GetFrameCount failed, hr=%x\n", hr);
            ok(count == 1, "unexpected count %u\n", count);

            hr = IWICBitmapDecoder_GetFrame(decoder, 0, &framedecode);
            ok(SUCCEEDED(hr), "GetFrame failed, hr=%x\n", hr);
            if (SUCCEEDED(hr))
            {
                IWICImagingFactory *factory;
                IWICPalette *palette;

                hr = IWICBitmapFrameDecode_GetSize(framedecode, &width, &height);
                ok(SUCCEEDED(hr), "GetSize failed, hr=%x\n", hr);
                ok(width == 2, "expected width=2, got %u\n", width);
                ok(height == 2, "expected height=2, got %u\n", height);

                hr = IWICBitmapFrameDecode_GetResolution(framedecode, &dpiX, &dpiY);
                ok(SUCCEEDED(hr), "GetResolution failed, hr=%x\n", hr);
                ok(dpiX == 96.0, "expected dpiX=96.0, got %f\n", dpiX);
                ok(dpiY == 96.0, "expected dpiY=96.0, got %f\n", dpiY);

                hr = IWICBitmapFrameDecode_GetPixelFormat(framedecode, &guidresult);
                ok(SUCCEEDED(hr), "GetPixelFormat failed, hr=%x\n", hr);
                ok(IsEqualGUID(&guidresult, &GUID_WICPixelFormat1bppIndexed), "unexpected pixel format\n");

                hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                    &IID_IWICImagingFactory, (void**)&factory);
                ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%x\n", hr);
                if (SUCCEEDED(hr))
                {
                    hr = IWICImagingFactory_CreatePalette(factory, &palette);
                    ok(SUCCEEDED(hr), "CreatePalette failed, hr=%x\n", hr);
                    if (SUCCEEDED(hr))
                    {
                        hr = IWICBitmapDecoder_CopyPalette(decoder, palette);
                        ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "expected WINCODEC_ERR_PALETTEUNAVAILABLE, got %x\n", hr);

                        hr = IWICBitmapFrameDecode_CopyPalette(framedecode, palette);
                        ok(SUCCEEDED(hr), "CopyPalette failed, hr=%x\n", hr);

                        hr = IWICPalette_GetColorCount(palette, &count);
                        ok(SUCCEEDED(hr), "GetColorCount failed, hr=%x\n", hr);
                        ok(count == 2, "expected count=2, got %u\n", count);

                        hr = IWICPalette_GetColors(palette, 2, palettedata, &count);
                        ok(SUCCEEDED(hr), "GetColorCount failed, hr=%x\n", hr);
                        ok(count == 2, "expected count=2, got %u\n", count);
                        ok(!memcmp(palettedata, expected_palettedata, sizeof(palettedata)), "unexpected palette data\n");

                        IWICPalette_Release(palette);
                    }

                    IWICImagingFactory_Release(factory);
                }

                rc.X = 0;
                rc.Y = 0;
                rc.Width = 2;
                rc.Height = 2;
                hr = IWICBitmapFrameDecode_CopyPixels(framedecode, &rc, 1, sizeof(imagedata), imagedata);
                ok(SUCCEEDED(hr), "CopyPixels failed, hr=%x\n", hr);
                ok(!memcmp(imagedata, expected_imagedata, sizeof(imagedata)), "unexpected image data\n");

                IWICBitmapFrameDecode_Release(framedecode);
            }

            hr = CoCreateInstance(&CLSID_WICBmpDecoder, NULL, CLSCTX_INPROC_SERVER,
                &IID_IWICBitmapDecoder, (void**)&decoder2);
            ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%x\n", hr);
            if (SUCCEEDED(hr))
            {
                hr = IWICBitmapDecoder_QueryCapability(decoder2, bmpstream, &capability);
                ok(hr == S_OK, "QueryCapability failed, hr=%x\n", hr);
                ok(capability == (WICBitmapDecoderCapabilityCanDecodeAllImages),
                    "unexpected capabilities: %x\n", capability);
            }

            IStream_Release(bmpstream);
        }

        GlobalFree(hbmpdata);
    }

    IWICBitmapDecoder_Release(decoder);
}

static const char testbmp_4bpp[] = {
    /* BITMAPFILEHEADER */
    66,77, /* "BM" */
    82,0,0,0, /* file size */
    0,0,0,0, /* reserved */
    74,0,0,0, /* offset to bits */
    /* BITMAPINFOHEADER */
    40,0,0,0, /* header size */
    2,0,0,0, /* width */
    254,255,255,255, /* height = -2 */
    1,0, /* planes */
    4,0, /* bit count */
    0,0,0,0, /* compression = BI_RGB */
    0,0,0,0, /* image size = 0 */
    16,39,0,0, /* X pixels per meter = 10000 */
    32,78,0,0, /* Y pixels per meter = 20000 */
    5,0,0,0, /* colors used */
    5,0,0,0, /* colors important */
    /* color table */
    255,0,0,0,
    0,255,0,255,
    0,0,255,23,
    128,0,128,1,
    255,255,255,0,
    /* bits */
    0x01,0,0,0,
    0x23,0,0,0,
};

static void test_decode_4bpp(void)
{
    IWICBitmapDecoder *decoder, *decoder2;
    IWICBitmapFrameDecode *framedecode;
    HRESULT hr;
    HGLOBAL hbmpdata;
    char *bmpdata;
    IStream *bmpstream;
    DWORD capability=0;
    GUID guidresult;
    UINT count=0, width=0, height=0;
    double dpiX, dpiY;
    BYTE imagedata[2] = {1};
    const BYTE expected_imagedata[2] = {0x01,0x23};
    WICColor palettedata[5] = {1};
    const WICColor expected_palettedata[5] =
        {0xff0000ff,0xff00ff00,0xffff0000,0xff800080,0xffffffff};
    WICRect rc;

    hr = CoCreateInstance(&CLSID_WICBmpDecoder, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICBitmapDecoder, (void**)&decoder);
    ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%x\n", hr);
    if (!SUCCEEDED(hr)) return;

    hbmpdata = GlobalAlloc(GMEM_MOVEABLE, sizeof(testbmp_4bpp));
    ok(hbmpdata != 0, "GlobalAlloc failed\n");
    if (hbmpdata)
    {
        bmpdata = GlobalLock(hbmpdata);
        memcpy(bmpdata, testbmp_4bpp, sizeof(testbmp_4bpp));
        GlobalUnlock(hbmpdata);

        hr = CreateStreamOnHGlobal(hbmpdata, FALSE, &bmpstream);
        ok(SUCCEEDED(hr), "CreateStreamOnHGlobal failed, hr=%x\n", hr);
        if (SUCCEEDED(hr))
        {
            hr = IWICBitmapDecoder_Initialize(decoder, bmpstream, WICDecodeMetadataCacheOnLoad);
            ok(hr == S_OK, "Initialize failed, hr=%x\n", hr);

            hr = IWICBitmapDecoder_GetContainerFormat(decoder, &guidresult);
            ok(SUCCEEDED(hr), "GetContainerFormat failed, hr=%x\n", hr);
            ok(IsEqualGUID(&guidresult, &GUID_ContainerFormatBmp), "unexpected container format\n");

            hr = IWICBitmapDecoder_GetFrameCount(decoder, &count);
            ok(SUCCEEDED(hr), "GetFrameCount failed, hr=%x\n", hr);
            ok(count == 1, "unexpected count %u\n", count);

            hr = IWICBitmapDecoder_GetFrame(decoder, 0, &framedecode);
            ok(SUCCEEDED(hr), "GetFrame failed, hr=%x\n", hr);
            if (SUCCEEDED(hr))
            {
                IWICImagingFactory *factory;
                IWICPalette *palette;

                hr = IWICBitmapFrameDecode_GetSize(framedecode, &width, &height);
                ok(SUCCEEDED(hr), "GetSize failed, hr=%x\n", hr);
                ok(width == 2, "expected width=2, got %u\n", width);
                ok(height == 2, "expected height=2, got %u\n", height);

                hr = IWICBitmapFrameDecode_GetResolution(framedecode, &dpiX, &dpiY);
                ok(SUCCEEDED(hr), "GetResolution failed, hr=%x\n", hr);
                ok(fabs(dpiX - 254.0) < 0.01, "expected dpiX=96.0, got %f\n", dpiX);
                ok(fabs(dpiY - 508.0) < 0.01, "expected dpiY=96.0, got %f\n", dpiY);

                hr = IWICBitmapFrameDecode_GetPixelFormat(framedecode, &guidresult);
                ok(SUCCEEDED(hr), "GetPixelFormat failed, hr=%x\n", hr);
                ok(IsEqualGUID(&guidresult, &GUID_WICPixelFormat4bppIndexed), "unexpected pixel format\n");

                hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                    &IID_IWICImagingFactory, (void**)&factory);
                ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%x\n", hr);
                if (SUCCEEDED(hr))
                {
                    hr = IWICImagingFactory_CreatePalette(factory, &palette);
                    ok(SUCCEEDED(hr), "CreatePalette failed, hr=%x\n", hr);
                    if (SUCCEEDED(hr))
                    {
                        hr = IWICBitmapDecoder_CopyPalette(decoder, palette);
                        ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "expected WINCODEC_ERR_PALETTEUNAVAILABLE, got %x\n", hr);

                        hr = IWICBitmapFrameDecode_CopyPalette(framedecode, palette);
                        ok(SUCCEEDED(hr), "CopyPalette failed, hr=%x\n", hr);

                        hr = IWICPalette_GetColorCount(palette, &count);
                        ok(SUCCEEDED(hr), "GetColorCount failed, hr=%x\n", hr);
                        ok(count == 5, "expected count=5, got %u\n", count);

                        hr = IWICPalette_GetColors(palette, 5, palettedata, &count);
                        ok(SUCCEEDED(hr), "GetColorCount failed, hr=%x\n", hr);
                        ok(count == 5, "expected count=5, got %u\n", count);
                        ok(!memcmp(palettedata, expected_palettedata, sizeof(palettedata)), "unexpected palette data\n");

                        IWICPalette_Release(palette);
                    }

                    IWICImagingFactory_Release(factory);
                }

                rc.X = 0;
                rc.Y = 0;
                rc.Width = 2;
                rc.Height = 2;
                hr = IWICBitmapFrameDecode_CopyPixels(framedecode, &rc, 1, sizeof(imagedata), imagedata);
                ok(SUCCEEDED(hr), "CopyPixels failed, hr=%x\n", hr);
                ok(!memcmp(imagedata, expected_imagedata, sizeof(imagedata)), "unexpected image data\n");

                IWICBitmapFrameDecode_Release(framedecode);
            }

            hr = CoCreateInstance(&CLSID_WICBmpDecoder, NULL, CLSCTX_INPROC_SERVER,
                &IID_IWICBitmapDecoder, (void**)&decoder2);
            ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%x\n", hr);
            if (SUCCEEDED(hr))
            {
                hr = IWICBitmapDecoder_QueryCapability(decoder2, bmpstream, &capability);
                ok(hr == S_OK, "QueryCapability failed, hr=%x\n", hr);
                ok(capability == (WICBitmapDecoderCapabilityCanDecodeAllImages),
                    "unexpected capabilities: %x\n", capability);
            }

            IStream_Release(bmpstream);
        }

        GlobalFree(hbmpdata);
    }

    IWICBitmapDecoder_Release(decoder);
}

START_TEST(bmpformat)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    test_decode_24bpp();
    test_decode_1bpp();
    test_decode_4bpp();

    CoUninitialize();
}
