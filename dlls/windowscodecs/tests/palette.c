/*
 * Copyright 2009 Vincent Povirk for CodeWeavers
 * Copyright 2012 Dmitry Timoshkov
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
#include <assert.h>

#define COBJMACROS

#include "windef.h"
#include "objbase.h"
#include "wincodec.h"
#include "wine/test.h"

static void test_custom_palette(void)
{
    IWICImagingFactory *factory;
    IWICPalette *palette;
    HRESULT hr;
    WICBitmapPaletteType type=0xffffffff;
    UINT count=1;
    const WICColor initcolors[4]={0xff000000,0xff0000ff,0xffffff00,0xffffffff};
    WICColor colors[4];
    BOOL boolresult;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICImagingFactory, (void**)&factory);
    ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%x\n", hr);
    if (FAILED(hr)) return;

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(SUCCEEDED(hr), "CreatePalette failed, hr=%x\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IWICPalette_GetType(palette, &type);
        ok(SUCCEEDED(hr), "GetType failed, hr=%x\n", hr);
        ok(type == WICBitmapPaletteTypeCustom, "expected WICBitmapPaletteTypeCustom, got %x\n", type);

        hr = IWICPalette_GetColorCount(palette, &count);
        ok(SUCCEEDED(hr), "GetColorCount failed, hr=%x\n", hr);
        ok(count == 0, "expected 0, got %u\n", count);

        hr = IWICPalette_GetColors(palette, 0, colors, &count);
        ok(SUCCEEDED(hr), "GetColors failed, hr=%x\n", hr);
        ok(count == 0, "expected 0, got %u\n", count);

        hr = IWICPalette_GetColors(palette, 4, colors, &count);
        ok(SUCCEEDED(hr), "GetColors failed, hr=%x\n", hr);
        ok(count == 0, "expected 0, got %u\n", count);

        memcpy(colors, initcolors, sizeof(initcolors));
        hr = IWICPalette_InitializeCustom(palette, colors, 4);
        ok(SUCCEEDED(hr), "InitializeCustom failed, hr=%x\n", hr);

        hr = IWICPalette_GetType(palette, &type);
        ok(SUCCEEDED(hr), "GetType failed, hr=%x\n", hr);
        ok(type == WICBitmapPaletteTypeCustom, "expected WICBitmapPaletteTypeCustom, got %x\n", type);

        hr = IWICPalette_GetColorCount(palette, &count);
        ok(SUCCEEDED(hr), "GetColorCount failed, hr=%x\n", hr);
        ok(count == 4, "expected 4, got %u\n", count);

        memset(colors, 0, sizeof(colors));
        count = 0;
        hr = IWICPalette_GetColors(palette, 4, colors, &count);
        ok(SUCCEEDED(hr), "GetColors failed, hr=%x\n", hr);
        ok(count == 4, "expected 4, got %u\n", count);
        ok(!memcmp(colors, initcolors, sizeof(colors)), "got unexpected palette data\n");

        memset(colors, 0, sizeof(colors));
        count = 0;
        hr = IWICPalette_GetColors(palette, 2, colors, &count);
        ok(SUCCEEDED(hr), "GetColors failed, hr=%x\n", hr);
        ok(count == 2, "expected 2, got %u\n", count);
        ok(!memcmp(colors, initcolors, sizeof(WICColor)*2), "got unexpected palette data\n");

        count = 0;
        hr = IWICPalette_GetColors(palette, 6, colors, &count);
        ok(SUCCEEDED(hr), "GetColors failed, hr=%x\n", hr);
        ok(count == 4, "expected 4, got %u\n", count);

        hr = IWICPalette_HasAlpha(palette, &boolresult);
        ok(SUCCEEDED(hr), "HasAlpha failed, hr=%x\n", hr);
        ok(!boolresult, "expected FALSE, got TRUE\n");

        hr = IWICPalette_IsBlackWhite(palette, &boolresult);
        ok(SUCCEEDED(hr), "IsBlackWhite failed, hr=%x\n", hr);
        ok(!boolresult, "expected FALSE, got TRUE\n");

        hr = IWICPalette_IsGrayscale(palette, &boolresult);
        ok(SUCCEEDED(hr), "IsGrayscale failed, hr=%x\n", hr);
        ok(!boolresult, "expected FALSE, got TRUE\n");

        /* try a palette with some alpha in it */
        colors[2] = 0x80ffffff;
        hr = IWICPalette_InitializeCustom(palette, colors, 4);
        ok(SUCCEEDED(hr), "InitializeCustom failed, hr=%x\n", hr);

        hr = IWICPalette_HasAlpha(palette, &boolresult);
        ok(SUCCEEDED(hr), "HasAlpha failed, hr=%x\n", hr);
        ok(boolresult, "expected TRUE, got FALSE\n");

        /* setting to a 0-color palette is acceptable */
        hr = IWICPalette_InitializeCustom(palette, NULL, 0);
        ok(SUCCEEDED(hr), "InitializeCustom failed, hr=%x\n", hr);

        /* IWICPalette is paranoid about NULL pointers */
        hr = IWICPalette_GetType(palette, NULL);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %x\n", hr);

        hr = IWICPalette_GetColorCount(palette, NULL);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %x\n", hr);

        hr = IWICPalette_InitializeCustom(palette, NULL, 4);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %x\n", hr);

        hr = IWICPalette_GetColors(palette, 4, NULL, &count);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %x\n", hr);

        hr = IWICPalette_GetColors(palette, 4, colors, NULL);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %x\n", hr);

        hr = IWICPalette_HasAlpha(palette, NULL);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %x\n", hr);

        hr = IWICPalette_IsBlackWhite(palette, NULL);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %x\n", hr);

        hr = IWICPalette_IsGrayscale(palette, NULL);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %x\n", hr);

        IWICPalette_Release(palette);
    }

    IWICImagingFactory_Release(factory);
}

static void generate_gray16_palette(DWORD *entries, UINT count)
{
    UINT i;

    assert(count == 16);

    for (i = 0; i < 16; i++)
    {
        entries[i] = 0xff000000;
        entries[i] |= (i << 20) | (i << 16) | (i << 12) | (i << 8) | (i << 4) | i;
    }
}

static void generate_gray256_palette(DWORD *entries, UINT count)
{
    UINT i;

    assert(count == 256);

    for (i = 0; i < 256; i++)
    {
        entries[i] = 0xff000000;
        entries[i] |= (i << 16) | (i << 8) | i;
    }
}

static void generate_halftone8_palette(DWORD *entries, UINT count)
{
    UINT i;

    assert(count == 16);

    for (i = 0; i < 8; i++)
    {
        entries[i] = 0xff000000;
        if (i & 1) entries[i] |= 0xff;
        if (i & 2) entries[i] |= 0xff00;
        if (i & 4) entries[i] |= 0xff0000;
    }

    for (i = 8; i < 16; i++)
    {
        static const DWORD halftone[8] = { 0xc0c0c0, 0x808080, 0x800000, 0x008000,
                                           0x000080, 0x808000, 0x800080, 0x008080 };
        entries[i] = 0xff000000;
        entries[i] |= halftone[i-8];
    }
}

static void generate_halftone64_palette(DWORD *entries, UINT count)
{
    static const BYTE halftone_values[4] = { 0x00,0x55,0xaa,0xff };
    UINT i;

    assert(count == 72);

    for (i = 0; i < 64; i++)
    {
        entries[i] = 0xff000000;
        entries[i] |= halftone_values[i%4];
        entries[i] |= halftone_values[(i/4)%4] << 8;
        entries[i] |= halftone_values[(i/16)%4] << 16;
    }

    for (i = 64; i < 72; i++)
    {
        static const DWORD halftone[8] = { 0xc0c0c0, 0x808080, 0x800000, 0x008000,
                                           0x000080, 0x808000, 0x800080, 0x008080 };
        entries[i] = 0xff000000;
        entries[i] |= halftone[i-64];
    }
}

static void generate_halftone256_palette(DWORD *entries, UINT count)
{
    static const BYTE halftone_values_b[4] = { 0x00,0x55,0xaa,0xff };
    static const BYTE halftone_values_gr[8] = { 0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff };
    UINT i;

    assert(count == 256);

    for (i = 0; i < 256; i++)
    {
        entries[i] = 0xff000000;
        entries[i] |= halftone_values_b[i%4];
        entries[i] |= halftone_values_gr[(i/4)%8] << 8;
        entries[i] |= halftone_values_gr[(i/32)%8] << 16;
    }
}

static void test_predefined_palette(void)
{
    static struct test_data
    {
        WICBitmapPaletteType type;
        BOOL is_bw, is_gray;
        UINT count;
        WICColor color[256];
    } td[] =
    {
        { WICBitmapPaletteTypeFixedBW, 1, 1, 2, { 0xff000000, 0xffffffff } },
        { WICBitmapPaletteTypeFixedGray4, 0, 1, 4,
          { 0xff000000, 0xff555555, 0xffaaaaaa, 0xffffffff } },
        { WICBitmapPaletteTypeFixedGray16, 0, 1, 16, { 0 } },
        { WICBitmapPaletteTypeFixedGray256, 0, 1, 256, { 0 } },
        { WICBitmapPaletteTypeFixedHalftone8, 0, 0, 16, { 0 } },
        { WICBitmapPaletteTypeFixedHalftone64, 0, 0, 72, { 0 } },
        { WICBitmapPaletteTypeFixedHalftone256, 0, 0, 256, { 0 } },
    };
    IWICImagingFactory *factory;
    IWICPalette *palette;
    HRESULT hr;
    WICBitmapPaletteType type;
    UINT count, i, ret;
    BOOL bret;
    WICColor color[256];

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICImagingFactory, (void **)&factory);
    ok(hr == S_OK, "CoCreateInstance error %#x\n", hr);

    for (i = 0; i < sizeof(td)/sizeof(td[0]); i++)
    {
        hr = IWICImagingFactory_CreatePalette(factory, &palette);
        ok(hr == S_OK, "%u: CreatePalette error %#x\n", i, hr);

        hr = IWICPalette_InitializePredefined(palette, td[i].type, FALSE);
        ok(hr == S_OK, "%u: InitializePredefined error %#x\n", i, hr);

        bret = -1;
        hr = IWICPalette_IsBlackWhite(palette, &bret);
        ok(hr == S_OK, "%u: IsBlackWhite error %#x\n", i, hr);
        ok(bret == td[i].is_bw ||
           broken(td[i].type == WICBitmapPaletteTypeFixedBW && bret != td[i].is_bw), /* XP */
           "%u: expected %d, got %d\n",i, td[i].is_bw, bret);

        bret = -1;
        hr = IWICPalette_IsGrayscale(palette, &bret);
        ok(hr == S_OK, "%u: IsGrayscale error %#x\n", i, hr);
        ok(bret == td[i].is_gray, "%u: expected %d, got %d\n", i, td[i].is_gray, bret);

        type = -1;
        hr = IWICPalette_GetType(palette, &type);
        ok(hr == S_OK, "%u: GetType error %#x\n", i, hr);
        ok(type == td[i].type, "%u: expected %#x, got %#x\n", i, td[i].type, type);

        count = 0xdeadbeef;
        hr = IWICPalette_GetColorCount(palette, &count);
        ok(hr == S_OK, "%u: GetColorCount error %#x\n", i, hr);
        ok(count == td[i].count, "%u: expected %u, got %u\n", i, td[i].count, count);

        hr = IWICPalette_GetColors(palette, count, color, &ret);
        ok(hr == S_OK, "%u: GetColors error %#x\n", i, hr);
        ok(ret == count, "%u: expected %u, got %u\n", i, count, ret);
        if (ret == td[i].count)
        {
            UINT j;

            if (td[i].type == WICBitmapPaletteTypeFixedGray16)
                generate_gray16_palette(td[i].color, td[i].count);
            else if (td[i].type == WICBitmapPaletteTypeFixedGray256)
                generate_gray256_palette(td[i].color, td[i].count);
            else if (td[i].type == WICBitmapPaletteTypeFixedHalftone8)
                generate_halftone8_palette(td[i].color, td[i].count);
            else if (td[i].type == WICBitmapPaletteTypeFixedHalftone64)
                generate_halftone64_palette(td[i].color, td[i].count);
            else if (td[i].type == WICBitmapPaletteTypeFixedHalftone256)
                generate_halftone256_palette(td[i].color, td[i].count);

            for (j = 0; j < count; j++)
            {
                ok(color[j] == td[i].color[j], "%u:[%u]: expected %#x, got %#x\n",
                   i, j, td[i].color[j], color[j]);
            }
        }

        IWICPalette_Release(palette);
    }

    IWICImagingFactory_Release(factory);
}

START_TEST(palette)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    test_custom_palette();
    test_predefined_palette();

    CoUninitialize();
}
