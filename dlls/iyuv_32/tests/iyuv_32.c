/*
 * Copyright 2026 Brendan McGrath for CodeWeavers
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

#include "wine/test.h"
#include <vfw.h>

#define FOURCC_I420 mmioFOURCC('I', '4', '2', '0')
#define FOURCC_IYUV mmioFOURCC('I', 'Y', 'U', 'V')

#pragma pack(push, 2)
struct bmp_header
{
    char magic[2];
    DWORD length;
    DWORD reserved;
    DWORD offset;
};
#pragma pack(pop)

static void write_bitmap_to_file(const BITMAPINFOHEADER *bitmap_header, const BYTE *data, HANDLE output)
{
    struct bmp_header header =
    {
        .magic = "BM",
        .length = sizeof(header) + sizeof(*bitmap_header) + bitmap_header->biSizeImage,
        .offset = sizeof(header) + sizeof(*bitmap_header),
    };
    DWORD written;
    BOOL ret;

    ret = WriteFile(output, &header, sizeof(header), &written, NULL);
    ok(ret, "WriteFile failed, error %lu\n", GetLastError());
    ok(written == sizeof(header), "written %lu bytes\n", written);
    ret = WriteFile(output, bitmap_header, sizeof(*bitmap_header), &written, NULL);
    ok(ret, "WriteFile failed, error %lu\n", GetLastError());
    ok(written == sizeof(*bitmap_header), "written %lu bytes\n", written);
    ret = WriteFile(output, data, bitmap_header->biSizeImage, &written, NULL);
    ok(ret, "WriteFile failed, error %lu\n", GetLastError());
    ok(written == bitmap_header->biSizeImage, "written %lu bytes\n", written);
}

static HANDLE open_temp_file(const WCHAR *output_filename)
{
    WCHAR path[MAX_PATH];
    HANDLE output;

    GetTempPathW(ARRAY_SIZE(path), path);
    lstrcatW(path, output_filename);

    output = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(output != INVALID_HANDLE_VALUE, "CreateFileW failed, error %lu\n", GetLastError());

    trace("created %s\n", debugstr_w(path));

    return output;
}

static void write_bitmap(const BITMAPINFOHEADER *bitmap_header, const BYTE *data, const WCHAR *output_filename)
{
    HANDLE output;

    output = open_temp_file(output_filename);
    write_bitmap_to_file(bitmap_header, data, output);

    CloseHandle(output);
}

static void test_formats(DWORD handler)
{
    BITMAPINFOHEADER in = {.biSize = sizeof(BITMAPINFOHEADER)};
    BITMAPINFOHEADER out = {.biSize = sizeof(BITMAPINFOHEADER)};
    ICINFO info;
    LRESULT lr;
    DWORD res;
    HIC hic;

    in.biCompression = handler;

    hic = ICLocate(ICTYPE_VIDEO, handler, &in, NULL, ICMODE_DECOMPRESS);
    ok(!hic, "Should fail to open codec without width and/or height.\n");

    in.biWidth = 320;
    in.biHeight = 240;

    hic = ICLocate(ICTYPE_VIDEO, handler, &in, NULL, ICMODE_DECOMPRESS);
    ok(!!hic, "Failed to open codec.\n");

    lr = ICGetInfo(hic, &info, sizeof(info));
    ok(lr == sizeof(info), "Got %Id.\n", lr);
    ok(info.dwSize == sizeof(info), "Got size %lu.\n", info.dwSize);
    ok(info.fccType == ICTYPE_VIDEO, "Got type %#lx.\n", info.fccType);
    ok(info.fccHandler == FOURCC_IYUV, "Got handler %#lx.\n", info.fccHandler);
    ok(info.dwFlags == 0, "Got flags %#lx.\n", info.dwFlags);
    ok(info.dwVersion == 0, "Got version %#lx.\n", info.dwVersion);
    ok(info.dwVersionICM == ICVERSION, "Got ICM version %#lx.\n", info.dwVersionICM);

    res = ICDecompressQuery(hic, &in, NULL);
    ok(res == ICERR_OK, "Got %ld.\n", res);

    res = ICDecompressGetFormat(hic, &in, NULL);
    ok(res == sizeof(BITMAPINFOHEADER), "Got %ld.\n", res);

    res = ICDecompressGetFormat(hic, &in, &out);
    ok(res == ICERR_OK, "Got %ld.\n", res);
    ok(out.biSize == sizeof(BITMAPINFOHEADER), "Got size %lu.\n", out.biSize);
    ok(out.biWidth == 320, "Got width %ld.\n", out.biWidth);
    ok(out.biHeight == 240, "Got height %ld.\n", out.biHeight);
    ok(out.biPlanes == 1, "Got %u planes.\n", out.biPlanes);
    ok(out.biBitCount == 24, "Got depth %u.\n", out.biBitCount);
    ok(out.biCompression == BI_RGB, "Got compression %#lx.\n", out.biCompression);
    ok(out.biSizeImage == 320 * 240 * 3, "Got image size %lu.\n", out.biSizeImage);
    ok(!out.biXPelsPerMeter, "Got horizontal resolution %ld.\n", out.biXPelsPerMeter);
    ok(!out.biYPelsPerMeter, "Got vertical resolution %ld.\n", out.biYPelsPerMeter);
    ok(!out.biClrUsed, "Got %lu used colours.\n", out.biClrUsed);
    ok(!out.biClrImportant, "Got %lu important colours.\n", out.biClrImportant);

    res = ICDecompressQuery(hic, &in, &out);
    ok(res == ICERR_OK, "Got %ld.\n", res);

    out.biHeight = -240;
    res = ICDecompressQuery(hic, &in, &out);
    ok(res == ICERR_BADFORMAT, "Got %ld.\n", res);

    out.biHeight = -480;
    res = ICDecompressQuery(hic, &in, &out);
    ok(res == ICERR_BADFORMAT, "Got %ld.\n", res);

    out.biHeight = 480;
    res = ICDecompressQuery(hic, &in, &out);
    ok(res == ICERR_BADFORMAT, "Got %ld.\n", res);

    out.biWidth = 640;
    res = ICDecompressQuery(hic, &in, &out);
    ok(res == ICERR_BADFORMAT, "Got %ld.\n", res);

    out.biWidth = 160;
    out.biHeight = 120;
    res = ICDecompressQuery(hic, &in, &out);
    ok(res == ICERR_BADFORMAT, "Got %ld.\n", res);

    out.biWidth = 320;
    out.biHeight = 240;
    out.biBitCount = 8;
    res = ICDecompressQuery(hic, &in, &out);
    ok(res == ICERR_OK, "Got %ld.\n", res);
    out.biBitCount = 16;
    res = ICDecompressQuery(hic, &in, &out);
    ok(res == ICERR_OK, "Got %ld.\n", res);
    out.biBitCount = 32;
    res = ICDecompressQuery(hic, &in, &out);
    ok(res == ICERR_BADFORMAT, "Got %ld.\n", res);

    out.biBitCount = 16;
    out.biCompression = BI_BITFIELDS;
    res = ICDecompressQuery(hic, &in, &out);
    ok(res == ICERR_BADFORMAT, "Got %ld.\n", res);

    out.biCompression = mmioFOURCC('C', 'L', 'J', 'R');
    out.biBitCount = 8;
    res = ICDecompressQuery(hic, &in, &out);
    ok(res == ICERR_BADFORMAT, "Got %ld.\n", res);

    out.biBitCount = 16;
    out.biCompression = mmioFOURCC('U', 'Y', 'V', 'Y');
    res = ICDecompressQuery(hic, &in, &out);
    ok(res == ICERR_BADFORMAT, "Got %ld.\n", res);
    out.biCompression = mmioFOURCC('Y', 'U', 'Y', '2');
    res = ICDecompressQuery(hic, &in, &out);
    ok(res == ICERR_BADFORMAT, "Got %ld.\n", res);

    res = ICDecompressEnd(hic);
    ok(res == ICERR_OK, "Got %ld.\n", res);

    out.biCompression = BI_RGB;
    out.biBitCount = 24;
    res = ICDecompressBegin(hic, &in, &out);
    todo_wine
    ok(res == ICERR_OK, "Got %ld.\n", res);

    res = ICDecompressEnd(hic);
    ok(res == ICERR_OK, "Got %ld.\n", res);

    lr = ICDecompressExQuery(hic, 0, &in, NULL, 0, 0, 320, 240, NULL, NULL, 0, 0, 0, 0);
    ok(lr == ICERR_UNSUPPORTED, "Got %Id.\n", lr);

    lr = ICDecompressExBegin(hic, 0, &in, NULL, 0, 0, 320, 240, &out, NULL, 20, 20, 320, 240);
    ok(lr == ICERR_UNSUPPORTED, "Got %Id.\n", lr);
    lr = ICDecompressExEnd(hic);
    ok(lr == ICERR_UNSUPPORTED, "Got %Id.\n", lr);

    res = ICClose(hic);
    ok(res == ICERR_OK, "Got %ld.\n", res);
}

static void test_decompress(DWORD handler)
{
    static const DWORD width = 96, height = 96;

    const struct bmp_header *expect_rgb_header;
    const BYTE *expect_rgb_data;
    BYTE rgb_data[96 * 96 * 3];
    DWORD res, diff;
    BYTE *i420_data;
    HRSRC resource;
    HIC hic;

    BITMAPINFOHEADER i420_info =
    {
        .biSize = sizeof(i420_info),
        .biWidth = width,
        .biHeight = height,
        .biPlanes = 1,
        .biBitCount = 12,
        .biCompression = handler,
    };

    BITMAPINFOHEADER rgb_info =
    {
        .biSize = sizeof(rgb_info),
        .biWidth = width,
        .biHeight = height,
        .biPlanes = 1,
        .biBitCount = 24,
        .biCompression = BI_RGB,
        .biSizeImage = sizeof(rgb_data),
    };

    resource = FindResourceW(NULL, L"i420frame.bmp", (const WCHAR *)RT_RCDATA);
    i420_data = LockResource(LoadResource(GetModuleHandleW(NULL), resource));
    i420_data += *(DWORD *)(i420_data + 2);
    i420_info.biSizeImage = SizeofResource(GetModuleHandleW(NULL), resource);

    hic = ICOpen(ICTYPE_VIDEO, handler, ICMODE_DECOMPRESS);
    ok(!!hic, "Failed to open codec.\n");

    res = ICDecompressBegin(hic, &i420_info, &rgb_info);
    todo_wine
    ok(res == ICERR_OK, "Got %ld.\n", res);

    if (res != ICERR_OK)
        goto skip_decompression_tests;

    res = ICDecompress(hic, 0, &i420_info, i420_data, &rgb_info, rgb_data);
    todo_wine
    ok(res == ICERR_OK, "Got %ld.\n", res);
    res = ICDecompressEnd(hic);
    ok(res == ICERR_OK, "Got %ld.\n", res);

    write_bitmap(&rgb_info, rgb_data, L"rgb24frame_flip.bmp");

    resource = FindResourceW(NULL, L"rgb24frame_flip.bmp", (const WCHAR *)RT_RCDATA);
    expect_rgb_header = ((const struct bmp_header *)LockResource(LoadResource(GetModuleHandleW(NULL), resource)));
    expect_rgb_data = (const BYTE *)((uintptr_t)expect_rgb_header + expect_rgb_header->offset);

    diff = 0;
    for (unsigned int i = 0; i < rgb_info.biSizeImage; ++i)
        diff += abs((int)rgb_data[i] - (int)expect_rgb_data[i]);
    diff = diff * 100 / 256 / rgb_info.biSizeImage;
    todo_wine
    ok(diff == 0, "Got %lu%% difference.\n", diff);

skip_decompression_tests:
    res = ICClose(hic);
    ok(res == ICERR_OK, "Got %ld.\n", res);
}

START_TEST(iyuv_32)
{
    static const DWORD handler[] = {FOURCC_IYUV, FOURCC_I420};
    BITMAPINFOHEADER in =
    {
        .biSize = sizeof(BITMAPINFOHEADER),
        .biCompression = FOURCC_I420,
        .biPlanes = 1,
        .biWidth = 96,
        .biHeight = 96,
        .biSizeImage = 96 * 96 * 3 / 2,
    };
    HIC hic;
    int i;

    hic = ICLocate(ICTYPE_VIDEO, FOURCC_I420, &in, NULL, ICMODE_DECOMPRESS);
    todo_wine
    ok(!!hic, "Failed to locate iyuv codec\n");
    if (!hic)
        return;
    ICClose(hic);

    for (i = 0; i < ARRAY_SIZE(handler); i++)
    {
        winetest_push_context("handler %.4s", (char *)&handler[i]);
        test_formats(handler[i]);
        test_decompress(handler[i]);
        winetest_pop_context();
    }
}
