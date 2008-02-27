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
#include <math.h>

#define expect(expected, got) ok(((UINT)got) == ((UINT)expected), "Expected %.8x, got %.8x\n", (UINT)expected, (UINT)got)

static void test_Scan0(void)
{
    GpBitmap *bm;
    GpStatus stat;
    BYTE buff[360];

    bm = NULL;
    stat = GdipCreateBitmapFromScan0(10, 10, 10, PixelFormat24bppRGB, NULL, &bm);
    expect(Ok, stat);
    ok(NULL != bm, "Expected bitmap to be initialized\n");
    if (stat == Ok)
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
    if (stat == Ok)
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

static void test_GetImageDimension(void)
{
    GpBitmap *bm;
    GpStatus stat;
    const REAL WIDTH = 10.0, HEIGHT = 20.0;
    REAL w,h;

    bm = (GpBitmap*)0xdeadbeef;
    stat = GdipCreateBitmapFromScan0(WIDTH, HEIGHT, 0, PixelFormat24bppRGB,NULL, &bm);
    expect(Ok,stat);
    ok((GpBitmap*)0xdeadbeef != bm, "Expected bitmap to not be 0xdeadbeef\n");
    ok(NULL != bm, "Expected bitmap to not be NULL\n");

    stat = GdipGetImageDimension(NULL,&w,&h);
    expect(InvalidParameter, stat);

    stat = GdipGetImageDimension((GpImage*)bm,NULL,&h);
    expect(InvalidParameter, stat);

    stat = GdipGetImageDimension((GpImage*)bm,&w,NULL);
    expect(InvalidParameter, stat);

    w = -1;
    h = -1;
    stat = GdipGetImageDimension((GpImage*)bm,&w,&h);
    expect(Ok, stat);
    ok(fabs(WIDTH - w) < 0.0001, "Width wrong");
    ok(fabs(HEIGHT - h) < 0.0001, "Height wrong");
    GdipDisposeImage((GpImage*)bm);
}

START_TEST(image)
{
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    gdiplusStartupInput.GdiplusVersion              = 1;
    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = 0;
    gdiplusStartupInput.SuppressExternalCodecs      = 0;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    test_Scan0();
    test_GetImageDimension();

    GdiplusShutdown(gdiplusToken);
}
