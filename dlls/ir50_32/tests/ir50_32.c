/*
 * Copyright 2025 Elizabeth Figura for CodeWeavers
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

#include <windef.h>
#include <wingdi.h>
#include <vfw.h>
#include "wine/test.h"

static void test_formats(void)
{
    char buffer[offsetof(BITMAPINFO, bmiColors[256])];
    BITMAPINFOHEADER in = {.biSize = sizeof(BITMAPINFOHEADER)};
    BITMAPINFO *out = (BITMAPINFO *)buffer;
    LRESULT ret;
    ICINFO info;
    HIC hic;

    in.biCompression = mmioFOURCC('i','v','5','0');

    hic = ICLocate(ICTYPE_VIDEO, mmioFOURCC('i','v','5','0'), &in, NULL, ICMODE_DECOMPRESS);
    ok(!!hic, "Failed to open codec.\n");

    ret = ICGetInfo(hic, &info, sizeof(info));
    ok(ret == sizeof(info), "Got %Id.\n", ret);
    ok(info.dwSize == sizeof(info), "Got size %lu.\n", info.dwSize);
    ok(info.fccType == ICTYPE_VIDEO, "Got type %#lx.\n", info.fccType);
    ok(info.fccHandler == mmioFOURCC('I','V','5','0'), "Got handler %#lx.\n", info.fccHandler);
    todo_wine ok(info.dwFlags == (VIDCF_QUALITY | VIDCF_CRUNCH | VIDCF_TEMPORAL | VIDCF_COMPRESSFRAMES | VIDCF_FASTTEMPORALC),
            "Got flags %#lx.\n", info.dwFlags);
    todo_wine ok(info.dwVersion == 0x32, "Got version %#lx.\n", info.dwVersion);
    ok(info.dwVersionICM == ICVERSION, "Got ICM version %#lx.\n", info.dwVersionICM);

    ret = ICDecompressQuery(hic, &in, NULL);
    ok(ret == ICERR_OK, "Got %Id.\n", ret);

    ret = ICDecompressGetFormat(hic, &in, NULL);
    ok(ret == offsetof(BITMAPINFO, bmiColors[256]), "Got %Id.\n", ret);

    in.biWidth = 320;
    in.biHeight = 240;
    in.biXPelsPerMeter = 12;
    in.biYPelsPerMeter = 34;
    in.biClrUsed = 111;
    in.biClrImportant = 111;

    memset(buffer, 0xcc, sizeof(buffer));
    ret = ICDecompressGetFormat(hic, &in, out);
    ok(ret == ICERR_OK, "Got %Id.\n", ret);
    ok(out->bmiHeader.biSize == sizeof(BITMAPINFOHEADER), "Got size %lu.\n", out->bmiHeader.biSize);
    ok(out->bmiHeader.biWidth == 320, "Got width %ld.\n", out->bmiHeader.biWidth);
    ok(out->bmiHeader.biHeight == 240, "Got height %ld.\n", out->bmiHeader.biHeight);
    ok(out->bmiHeader.biPlanes == 1, "Got %u planes.\n", out->bmiHeader.biPlanes);
    ok(out->bmiHeader.biBitCount == 24, "Got depth %u.\n", out->bmiHeader.biBitCount);
    ok(out->bmiHeader.biCompression == BI_RGB, "Got compression %#lx.\n", out->bmiHeader.biCompression);
    ok(out->bmiHeader.biSizeImage == 320 * 240 * 3, "Got image size %lu.\n", out->bmiHeader.biSizeImage);
    ok(!out->bmiHeader.biXPelsPerMeter, "Got horizontal resolution %ld.\n", out->bmiHeader.biXPelsPerMeter);
    ok(!out->bmiHeader.biYPelsPerMeter, "Got vertical resolution %ld.\n", out->bmiHeader.biYPelsPerMeter);
    ok(!out->bmiHeader.biClrUsed, "Got %lu used colours.\n", out->bmiHeader.biClrUsed);
    ok(!out->bmiHeader.biClrImportant, "Got %lu important colours.\n", out->bmiHeader.biClrImportant);
    ok(out->bmiColors[0].rgbRed == 0xcc, "Expected colours to be unmodified.\n");

    ret = ICDecompressQuery(hic, &in, out);
    ok(ret == ICERR_OK, "Got %Id.\n", ret);

    out->bmiHeader.biHeight = -240;
    out->bmiHeader.biSizeImage = 0;
    out->bmiHeader.biPlanes = 0;
    ret = ICDecompressQuery(hic, &in, out);
    ok(ret == ICERR_OK, "Got %Id.\n", ret);

    out->bmiHeader.biHeight = -480;
    ret = ICDecompressQuery(hic, &in, out);
    ok(ret == ICERR_BADPARAM, "Got %Id.\n", ret);
    out->bmiHeader.biHeight = -240;

    out->bmiHeader.biWidth = 640;
    ret = ICDecompressQuery(hic, &in, out);
    ok(ret == ICERR_BADPARAM, "Got %Id.\n", ret);
    out->bmiHeader.biWidth = 320;

    out->bmiHeader.biBitCount = 8;
    ret = ICDecompressQuery(hic, &in, out);
    todo_wine ok(ret == ICERR_OK, "Got %Id.\n", ret);
    out->bmiHeader.biBitCount = 16;
    ret = ICDecompressQuery(hic, &in, out);
    ok(ret == ICERR_OK, "Got %Id.\n", ret);
    out->bmiHeader.biBitCount = 32;
    ret = ICDecompressQuery(hic, &in, out);
    ok(ret == ICERR_OK, "Got %Id.\n", ret);

    out->bmiHeader.biBitCount = 16;
    out->bmiHeader.biCompression = BI_BITFIELDS;
    ret = ICDecompressQuery(hic, &in, out);
    ok(ret == ICERR_BADFORMAT, "Got %Id.\n", ret);
    ((DWORD *)&out->bmiColors[0])[0] = 0xf800;
    ((DWORD *)&out->bmiColors[0])[1] = 0x07e0;
    ((DWORD *)&out->bmiColors[0])[2] = 0x001f;
    ret = ICDecompressQuery(hic, &in, out);
    ok(ret == ICERR_OK, "Got %Id.\n", ret);

    out->bmiHeader.biCompression = mmioFOURCC('C','L','J','R');
    out->bmiHeader.biBitCount = 8;
    ret = ICDecompressQuery(hic, &in, out);
    ok(ret == ICERR_BADFORMAT, "Got %Id.\n", ret);

    out->bmiHeader.biBitCount = 16;
    out->bmiHeader.biCompression = mmioFOURCC('U','Y','V','Y');
    ret = ICDecompressQuery(hic, &in, out);
    todo_wine ok(ret == ICERR_OK, "Got %Id.\n", ret);
    out->bmiHeader.biCompression = mmioFOURCC('Y','U','Y','2');
    ret = ICDecompressQuery(hic, &in, out);
    todo_wine ok(ret == ICERR_OK, "Got %Id.\n", ret);

    ret = ICDecompressEnd(hic);
    ok(ret == ICERR_OK, "Got %Id.\n", ret);

    out->bmiHeader.biCompression = BI_RGB;
    out->bmiHeader.biBitCount = 24;
    ret = ICDecompressBegin(hic, &in, out);
    ok(ret == ICERR_OK, "Got %Id.\n", ret);

    ret = ICDecompressEnd(hic);
    ok(ret == ICERR_OK, "Got %Id.\n", ret);

    ret = ICClose(hic);
    ok(!ret, "Got %Id.\n", ret);
}

static void test_compress(void)
{
    const void *res_data, *rgb_data;
    BITMAPINFO rgb_info, iv50_info;
    BYTE iv50_data[0x5100];
    HRSRC resource;
    LRESULT ret;
    DWORD flags;
    HIC hic;

    resource = FindResourceW(NULL, L"rgb24frame.bmp", (const WCHAR *)RT_RCDATA);
    res_data = LockResource(LoadResource(GetModuleHandleW(NULL), resource));
    memcpy(&rgb_info, ((BITMAPFILEHEADER *)res_data) + 1, sizeof(BITMAPINFOHEADER));
    rgb_data = ((const char *)res_data) + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    hic = ICOpen(ICTYPE_VIDEO, mmioFOURCC('i','v','5','0'), ICMODE_COMPRESS);
    ok(!!hic, "Failed to open codec.\n");

    ret = ICCompressQuery(hic, &rgb_info, NULL);
    todo_wine ok(!ret, "Got %Id.\n", ret);
    if (ret)
    {
        ICClose(hic);
        return;
    }

    ret = ICCompressGetSize(hic, &rgb_info, NULL);
    ok(ret == 0x5100, "Got %Id.\n", ret);

    ret = ICCompressGetFormatSize(hic, &rgb_info);
    ok(ret == sizeof(BITMAPINFOHEADER), "Got %Id.\n", ret);

    memset(&iv50_info, 0xcc, sizeof(iv50_info));
    ret = ICCompressGetFormat(hic, &rgb_info, &iv50_info);
    ok(!ret, "Got %Id.\n", ret);
    ok(iv50_info.bmiHeader.biSize == sizeof(BITMAPINFOHEADER), "Got size %lu.\n", iv50_info.bmiHeader.biSize);
    ok(iv50_info.bmiHeader.biWidth == 96, "Got width %ld.\n", iv50_info.bmiHeader.biWidth);
    ok(iv50_info.bmiHeader.biHeight == 96, "Got height %ld.\n", iv50_info.bmiHeader.biHeight);
    ok(iv50_info.bmiHeader.biPlanes == 1, "Got %u planes.\n", iv50_info.bmiHeader.biPlanes);
    ok(iv50_info.bmiHeader.biBitCount == 24, "Got depth %u.\n", iv50_info.bmiHeader.biBitCount);
    ok(iv50_info.bmiHeader.biCompression == mmioFOURCC('I','V','5','0'),
            "Got compression %#lx.\n", iv50_info.bmiHeader.biCompression);
    ok(iv50_info.bmiHeader.biSizeImage == 0x5100, "Got image size %lu.\n", iv50_info.bmiHeader.biSizeImage);
    ok(!iv50_info.bmiHeader.biXPelsPerMeter, "Got horizontal resolution %ld.\n", iv50_info.bmiHeader.biXPelsPerMeter);
    ok(!iv50_info.bmiHeader.biYPelsPerMeter, "Got vertical resolution %ld.\n", iv50_info.bmiHeader.biYPelsPerMeter);
    ok(!iv50_info.bmiHeader.biClrUsed, "Got %lu used colours.\n", iv50_info.bmiHeader.biClrUsed);
    ok(!iv50_info.bmiHeader.biClrImportant, "Got %lu important colours.\n", iv50_info.bmiHeader.biClrImportant);
    ok(iv50_info.bmiColors[0].rgbRed == 0xcc, "Expected colours to be unmodified.\n");

    ret = ICCompressQuery(hic, &rgb_info, &iv50_info);
    ok(!ret, "Got %Id.\n", ret);

    ret = ICCompressBegin(hic, &rgb_info, &iv50_info);
    ok(!ret, "Got %Id.\n", ret);
    memset(iv50_data, 0xcd, sizeof(iv50_data));
    ret = ICCompress(hic, ICCOMPRESS_KEYFRAME, &iv50_info.bmiHeader, iv50_data,
            &rgb_info.bmiHeader, (void *)rgb_data, NULL, &flags, 1, 0, 0, NULL, NULL);
    ok(!ret, "Got %Id.\n", ret);
    ok(flags == AVIIF_KEYFRAME, "got flags %#lx\n", flags);
    ok(iv50_info.bmiHeader.biSizeImage < 0x5100, "Got size %ld.\n", iv50_info.bmiHeader.biSizeImage);
    ret = ICCompressEnd(hic);
    ok(!ret, "Got %Id.\n", ret);

    ret = ICClose(hic);
    ok(!ret, "Got %Id.\n", ret);
}

static void test_decompress(void)
{
    const BYTE *expect_rgb_data;
    BYTE rgb_data[96 * 96 * 3];
    unsigned int diff = 0;
    void *iv50_data;
    HRSRC resource;
    LRESULT ret;
    HIC hic;

    BITMAPINFO iv50_info =
    {
        .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
        .bmiHeader.biWidth = 96,
        .bmiHeader.biHeight = 96,
        .bmiHeader.biPlanes = 1,
        .bmiHeader.biBitCount = 24,
        .bmiHeader.biCompression = mmioFOURCC('I','V','5','0'),
    };
    static const BITMAPINFO rgb_info =
    {
        .bmiHeader.biSize = sizeof(BITMAPINFOHEADER),
        .bmiHeader.biWidth = 96,
        .bmiHeader.biHeight = 96,
        .bmiHeader.biPlanes = 1,
        .bmiHeader.biBitCount = 24,
        .bmiHeader.biCompression = BI_RGB,
        .bmiHeader.biSizeImage = 96 * 96 * 3,
    };

    resource = FindResourceW(NULL, L"iv50frame.bin", (const WCHAR *)RT_RCDATA);
    iv50_data = LockResource(LoadResource(GetModuleHandleW(NULL), resource));
    iv50_info.bmiHeader.biSizeImage = SizeofResource(GetModuleHandleW(NULL), resource);

    hic = ICOpen(ICTYPE_VIDEO, mmioFOURCC('i','v','5','0'), ICMODE_DECOMPRESS);
    ok(!!hic, "Failed to open codec.\n");

    ret = ICDecompressBegin(hic, &iv50_info, &rgb_info);
    ok(!ret, "Got %Id.\n", ret);
    ret = ICDecompress(hic, 0, &iv50_info.bmiHeader, iv50_data,
            (BITMAPINFOHEADER *)&rgb_info.bmiHeader, rgb_data);
    ok(!ret, "Got %Id.\n", ret);
    ret = ICDecompressEnd(hic);
    ok(!ret, "Got %Id.\n", ret);

    resource = FindResourceW(NULL, L"rgb24frame.bmp", (const WCHAR *)RT_RCDATA);
    expect_rgb_data = ((const BYTE *)LockResource(LoadResource(GetModuleHandleW(NULL), resource)))
            + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    for (unsigned int i = 0; i < rgb_info.bmiHeader.biSizeImage; ++i)
        diff += abs((int)rgb_data[i] - (int)expect_rgb_data[i]);
    diff = diff * 100 / 256 / rgb_info.bmiHeader.biSizeImage;
    ok(diff < 4, "Got %u%% difference.\n", diff);

    ret = ICClose(hic);
    ok(!ret, "Got %Id.\n", ret);
}

START_TEST(ir50_32)
{
    BITMAPINFOHEADER in = {.biSize = sizeof(BITMAPINFOHEADER), .biCompression = mmioFOURCC('I','V','5','0')};
    HIC hic = ICLocate(ICTYPE_VIDEO, mmioFOURCC('I','V','5','0'), &in, NULL, ICMODE_DECOMPRESS);
    if (!hic)
    {
        /* Indeo codecs are simply nonfunctional on Windows 10. */
        todo_wine win_skip("Failed to open codec.\n");
        return;
    }
    ICClose(hic);

    test_formats();
    test_compress();
    test_decompress();
}
