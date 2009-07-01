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

START_TEST(bmpformat)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    test_decode_24bpp();

    CoUninitialize();
}
