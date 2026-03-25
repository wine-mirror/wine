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
        winetest_pop_context();
    }
}
