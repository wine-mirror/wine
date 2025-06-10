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
    todo_wine ok(!!hic, "Failed to open codec.\n");
    if (!hic)
        return;

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
    todo_wine ok(ret == offsetof(BITMAPINFO, bmiColors[256]), "Got %Id.\n", ret);

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
    todo_wine ok(out->bmiHeader.biPlanes == 1, "Got %u planes.\n", out->bmiHeader.biPlanes);
    todo_wine ok(out->bmiHeader.biBitCount == 24, "Got depth %u.\n", out->bmiHeader.biBitCount);
    ok(out->bmiHeader.biCompression == BI_RGB, "Got compression %#lx.\n", out->bmiHeader.biCompression);
    todo_wine ok(out->bmiHeader.biSizeImage == 320 * 240 * 3, "Got image size %lu.\n", out->bmiHeader.biSizeImage);
    todo_wine ok(!out->bmiHeader.biXPelsPerMeter, "Got horizontal resolution %ld.\n", out->bmiHeader.biXPelsPerMeter);
    todo_wine ok(!out->bmiHeader.biYPelsPerMeter, "Got vertical resolution %ld.\n", out->bmiHeader.biYPelsPerMeter);
    todo_wine ok(!out->bmiHeader.biClrUsed, "Got %lu used colours.\n", out->bmiHeader.biClrUsed);
    todo_wine ok(!out->bmiHeader.biClrImportant, "Got %lu important colours.\n", out->bmiHeader.biClrImportant);
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
    todo_wine ok(ret == ICERR_BADPARAM, "Got %Id.\n", ret);
    out->bmiHeader.biHeight = -240;

    out->bmiHeader.biWidth = 640;
    ret = ICDecompressQuery(hic, &in, out);
    todo_wine ok(ret == ICERR_BADPARAM, "Got %Id.\n", ret);
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
    todo_wine ok(ret == ICERR_OK, "Got %Id.\n", ret);

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
    todo_wine ok(ret == ICERR_OK, "Got %Id.\n", ret);

    out->bmiHeader.biCompression = BI_RGB;
    out->bmiHeader.biBitCount = 24;
    ret = ICDecompressBegin(hic, &in, out);
    ok(ret == ICERR_OK, "Got %Id.\n", ret);

    ret = ICDecompressEnd(hic);
    todo_wine ok(ret == ICERR_OK, "Got %Id.\n", ret);

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
}
