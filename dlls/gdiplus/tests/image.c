/*
 * Unit test suite for images
 *
 * Copyright (C) 2007 Google (Evan Stade)
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

#include "windows.h"
#include "gdiplus.h"
#include "wine/test.h"

#define expect(expected, got) ok(((UINT)got) == ((UINT)expected), "Expected %.8x, got %.8x\n", (UINT)expected, (UINT)got)

static void test_Scan0()
{
    GpBitmap *bm;
    GpStatus stat;
    BYTE buff[360];

    bm = NULL;
    stat = GdipCreateBitmapFromScan0(10, 10, 10, PixelFormat24bppRGB, NULL, &bm);
    expect(Ok, stat);
    ok(NULL != bm, "Expected bitmap to be initialized\n");
    GdipDisposeImage((GpImage*)bm);

    bm = (GpBitmap*)0xdeadbeef;
    stat = GdipCreateBitmapFromScan0(10, -10, 10, PixelFormat24bppRGB, NULL, &bm);
    expect(InvalidParameter, stat);

    expect(NULL, bm);

    bm = (GpBitmap*)0xdeadbeef;
    stat = GdipCreateBitmapFromScan0(-10, 10, 10, PixelFormat24bppRGB, NULL, &bm);
    expect(InvalidParameter, stat);

    expect(NULL, bm);

    bm = (GpBitmap*)0xdeadbeef;
    stat = GdipCreateBitmapFromScan0(10, 0, 10, PixelFormat24bppRGB, NULL, &bm);
    expect(InvalidParameter, stat);

    expect(NULL, bm);

    bm = NULL;
    stat = GdipCreateBitmapFromScan0(10, 10, 12, PixelFormat24bppRGB, buff, &bm);
    expect(Ok, stat);
    ok(NULL != bm, "Expected bitmap to be initialized\n");
    GdipDisposeImage((GpImage*)bm);

    bm = (GpBitmap*) 0xdeadbeef;
    stat = GdipCreateBitmapFromScan0(10, 10, 10, PixelFormat24bppRGB, buff, &bm);
    expect(InvalidParameter, stat);
    expect(NULL, bm);

    bm = (GpBitmap*)0xdeadbeef;
    stat = GdipCreateBitmapFromScan0(10, 10, 0, PixelFormat24bppRGB, buff, &bm);
    expect(InvalidParameter, stat);
    expect(0xdeadbeef, bm);
}

START_TEST(image)
{
    test_Scan0();
}
