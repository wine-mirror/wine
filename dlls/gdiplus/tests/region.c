/*
 * Unit test suite for gdiplus regions
 *
 * Copyright (C) 2008 Huw Davies
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
#include "wingdi.h"
#include "wine/test.h"

#define RGNDATA_RECT            0x10000000
#define RGNDATA_PATH            0x10000001
#define RGNDATA_EMPTY_RECT      0x10000002
#define RGNDATA_INFINITE_RECT   0x10000003

#define RGNDATA_MAGIC           0xdbc01001
#define RGNDATA_MAGIC2          0xdbc01002

#define expect(expected, got) ok(got == expected, "Expected %.8x, got %.8x\n", expected, got)

#define expect_magic(value) ok(*value == RGNDATA_MAGIC || *value == RGNDATA_MAGIC2, "Expected a known magic value, got %8x\n", *value)

static inline void expect_dword(DWORD *value, DWORD expected)
{
    ok(*value == expected, "expected %08x got %08x\n", expected, *value);
}

static inline void expect_float(DWORD *value, FLOAT expected)
{
    FLOAT valuef = *(FLOAT*)value;
    ok(valuef == expected, "expected %f got %f\n", expected, valuef);
}

/* We get shorts back, not INTs like a GpPoint */
typedef struct RegionDataPoint
{
    short X, Y;
} RegionDataPoint;

static void test_getregiondata(void)
{
    GpStatus status;
    GpRegion *region, *region2;
    RegionDataPoint *point;
    UINT needed;
    DWORD buf[100];
    GpRect rect;
    GpPath *path;

    memset(buf, 0xee, sizeof(buf));

    status = GdipCreateRegion(&region);
    ok(status == Ok, "status %08x\n", status);

    status = GdipGetRegionDataSize(region, &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(20, needed);
todo_wine
{
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    ok(status == Ok, "status %08x\n", status);
}
    expect(20, needed);
todo_wine
{
    expect_dword(buf, 12);
    trace("buf[1] = %08x\n", buf[1]);
    expect_magic((DWORD*)(buf + 2));
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_INFINITE_RECT);

    status = GdipSetEmpty(region);
}
    ok(status == Ok, "status %08x\n", status);
todo_wine
{
    status = GdipGetRegionDataSize(region, &needed);
}
    ok(status == Ok, "status %08x\n", status);
    expect(20, needed);
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
todo_wine
    ok(status == Ok, "status %08x\n", status);
    expect(20, needed);
todo_wine
{
    expect_dword(buf, 12);
    trace("buf[1] = %08x\n", buf[1]);
    expect_magic((DWORD*)(buf + 2));
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_EMPTY_RECT);
}

    status = GdipSetInfinite(region);
    ok(status == Ok, "status %08x\n", status);
    status = GdipGetRegionDataSize(region, &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(20, needed);
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
todo_wine
    ok(status == Ok, "status %08x\n", status);
    expect(20, needed);
todo_wine
{
    expect_dword(buf, 12);
    trace("buf[1] = %08x\n", buf[1]);
    expect_magic((DWORD*)(buf + 2));
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_INFINITE_RECT);
}

    status = GdipDeleteRegion(region);
    ok(status == Ok, "status %08x\n", status);

    rect.X = 10;
    rect.Y = 20;
    rect.Width = 100;
    rect.Height = 200;
todo_wine
{
    status = GdipCreateRegionRectI(&rect, &region);
    ok(status == Ok, "status %08x\n", status);
    status = GdipGetRegionDataSize(region, &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(36, needed);
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(36, needed);
    expect_dword(buf, 28);
    trace("buf[1] = %08x\n", buf[1]);
    expect_magic((DWORD*)(buf + 2));
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_RECT);
    expect_float(buf + 5, 10.0);
    expect_float(buf + 6, 20.0);
    expect_float(buf + 7, 100.0);
    expect_float(buf + 8, 200.0);

    rect.X = 50;
    rect.Y = 30;
    rect.Width = 10;
    rect.Height = 20;
    status = GdipCombineRegionRectI(region, &rect, CombineModeIntersect);
    ok(status == Ok, "status %08x\n", status);
    rect.X = 100;
    rect.Y = 300;
    rect.Width = 30;
    rect.Height = 50;
    status = GdipCombineRegionRectI(region, &rect, CombineModeXor);
    ok(status == Ok, "status %08x\n", status);

    rect.X = 200;
    rect.Y = 100;
    rect.Width = 133;
    rect.Height = 266;
    status = GdipCreateRegionRectI(&rect, &region2);
    ok(status == Ok, "status %08x\n", status);
    rect.X = 20;
    rect.Y = 10;
    rect.Width = 40;
    rect.Height = 66;
    status = GdipCombineRegionRectI(region2, &rect, CombineModeUnion);
    ok(status == Ok, "status %08x\n", status);

    status = GdipCombineRegionRegion(region, region2, CombineModeComplement);
    ok(status == Ok, "status %08x\n", status);

    rect.X = 400;
    rect.Y = 500;
    rect.Width = 22;
    rect.Height = 55;
    status = GdipCombineRegionRectI(region, &rect, CombineModeExclude);
    ok(status == Ok, "status %08x\n", status);

    status = GdipGetRegionDataSize(region, &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(156, needed);
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(156, needed);
    expect_dword(buf, 148);
    trace("buf[1] = %08x\n", buf[1]);
    expect_magic((DWORD*)(buf + 2));
    expect_dword(buf + 3, 10);
    expect_dword(buf + 4, CombineModeExclude);
    expect_dword(buf + 5, CombineModeComplement);
    expect_dword(buf + 6, CombineModeXor);
    expect_dword(buf + 7, CombineModeIntersect);
    expect_dword(buf + 8, RGNDATA_RECT);
    expect_float(buf + 9, 10.0);
    expect_float(buf + 10, 20.0);
    expect_float(buf + 11, 100.0);
    expect_float(buf + 12, 200.0);
    expect_dword(buf + 13, RGNDATA_RECT);
    expect_float(buf + 14, 50.0);
    expect_float(buf + 15, 30.0);
    expect_float(buf + 16, 10.0);
    expect_float(buf + 17, 20.0);
    expect_dword(buf + 18, RGNDATA_RECT);
    expect_float(buf + 19, 100.0);
    expect_float(buf + 20, 300.0);
    expect_float(buf + 21, 30.0);
    expect_float(buf + 22, 50.0);
    expect_dword(buf + 23, CombineModeUnion);
    expect_dword(buf + 24, RGNDATA_RECT);
    expect_float(buf + 25, 200.0);
    expect_float(buf + 26, 100.0);
    expect_float(buf + 27, 133.0);
    expect_float(buf + 28, 266.0);
    expect_dword(buf + 29, RGNDATA_RECT);
    expect_float(buf + 30, 20.0);
    expect_float(buf + 31, 10.0);
    expect_float(buf + 32, 40.0);
    expect_float(buf + 33, 66.0);
    expect_dword(buf + 34, RGNDATA_RECT);
    expect_float(buf + 35, 400.0);
    expect_float(buf + 36, 500.0);
    expect_float(buf + 37, 22.0);
    expect_float(buf + 38, 55.0);

    status = GdipDeleteRegion(region2);
    ok(status == Ok, "status %08x\n", status);
    status = GdipDeleteRegion(region);
    ok(status == Ok, "status %08x\n", status);
}

    /* Try some paths */

    status = GdipCreatePath(FillModeAlternate, &path);
    ok(status == Ok, "status %08x\n", status);
todo_wine
{
    GdipAddPathRectangle(path, 12.5, 13.0, 14.0, 15.0);

    status = GdipCreateRegionPath(path, &region);
    ok(status == Ok, "status %08x\n", status);
    status = GdipGetRegionDataSize(region, &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(72, needed);
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(72, needed);
    expect_dword(buf, 64);
    trace("buf[1] = %08x\n", buf[1]);
    expect_magic((DWORD*)(buf + 2));
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_PATH);
    expect_dword(buf + 5, 0x00000030);
    expect_magic((DWORD*)(buf + 6));
    expect_dword(buf + 7, 0x00000004);
    expect_dword(buf + 8, 0x00000000);
    expect_float(buf + 9, 12.5);
    expect_float(buf + 10, 13.0);
    expect_float(buf + 11, 26.5);
    expect_float(buf + 12, 13.0);
    expect_float(buf + 13, 26.5);
    expect_float(buf + 14, 28.0);
    expect_float(buf + 15, 12.5);
    expect_float(buf + 16, 28.0);
    expect_dword(buf + 17, 0x81010100);


    rect.X = 50;
    rect.Y = 30;
    rect.Width = 10;
    rect.Height = 20;
    status = GdipCombineRegionRectI(region, &rect, CombineModeIntersect);
    ok(status == Ok, "status %08x\n", status);
    status = GdipGetRegionDataSize(region, &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(96, needed);
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    ok(status == Ok, "status %08x\n", status);
    expect(96, needed);
    expect_dword(buf, 88);
    trace("buf[1] = %08x\n", buf[1]);
    expect_magic((DWORD*)(buf + 2));
    expect_dword(buf + 3, 2);
    expect_dword(buf + 4, CombineModeIntersect);
    expect_dword(buf + 5, RGNDATA_PATH);
    expect_dword(buf + 6, 0x00000030);
    expect_magic((DWORD*)(buf + 7));
    expect_dword(buf + 8, 0x00000004);
    expect_dword(buf + 9, 0x00000000);
    expect_float(buf + 10, 12.5);
    expect_float(buf + 11, 13.0);
    expect_float(buf + 12, 26.5);
    expect_float(buf + 13, 13.0);
    expect_float(buf + 14, 26.5);
    expect_float(buf + 15, 28.0);
    expect_float(buf + 16, 12.5);
    expect_float(buf + 17, 28.0);
    expect_dword(buf + 18, 0x81010100);
    expect_dword(buf + 19, RGNDATA_RECT);
    expect_float(buf + 20, 50.0);
    expect_float(buf + 21, 30.0);
    expect_float(buf + 22, 10.0);
    expect_float(buf + 23, 20.0);

    status = GdipDeleteRegion(region);
    ok(status == Ok, "status %08x\n", status);
}
    status = GdipDeletePath(path);
    ok(status == Ok, "status %08x\n", status);

    /* Test an empty path */
    status = GdipCreatePath(FillModeAlternate, &path);
    expect(Ok, status);
todo_wine
{
    status = GdipCreateRegionPath(path, &region);
    expect(Ok, status);
    status = GdipGetRegionDataSize(region, &needed);
    expect(Ok, status);
    expect(36, needed);
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    expect(Ok, status);
    expect(36, needed);
    expect_dword(buf, 28);
    trace("buf[1] = %08x\n", buf[1]);
    expect_magic((DWORD*)(buf + 2));
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_PATH);

    /* Second signature for pathdata */
    expect_dword(buf + 5, 12);
    expect_magic((DWORD*)(buf + 6));
    expect_dword(buf + 7, 0);
    expect_dword(buf + 8, 0x00004000);

    status = GdipDeleteRegion(region);
    expect(Ok, status);
}

    /* Test a simple triangle of INTs */
    status = GdipAddPathLine(path, 5, 6, 7, 8);
    expect(Ok, status);
    status = GdipAddPathLine(path, 8, 1, 5, 6);
    expect(Ok, status);
    status = GdipClosePathFigure(path);
    expect(Ok, status);
todo_wine
{
    status = GdipCreateRegionPath(path, &region);
    expect(Ok, status);
    status = GdipGetRegionDataSize(region, &needed);
    expect(Ok, status);
    expect(56, needed);
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    expect(Ok, status);
    expect(56, needed);
    expect_dword(buf, 48);
    trace("buf[1] = %08x\n", buf[1]);
    expect_magic((DWORD*)(buf + 2));
    expect_dword(buf + 3 , 0);
    expect_dword(buf + 4 , RGNDATA_PATH);

    expect_dword(buf + 5, 32);
    expect_magic((DWORD*)(buf + 6));
    expect_dword(buf + 7, 4);
    expect_dword(buf + 8, 0x00004000); /* ?? */

    point = (RegionDataPoint*)buf + 9;
    expect(5, point[0].X);
    expect(6, point[0].Y);
    expect(7, point[1].X); /* buf + 10 */
    expect(8, point[1].Y);
    expect(8, point[2].X); /* buf + 11 */
    expect(1, point[2].Y);
    expect(5, point[3].X); /* buf + 12 */
    expect(6, point[3].Y);
    expect_dword(buf + 13, 0x81010100); /* 0x01010100 if we don't close the path */
}
    status = GdipDeletePath(path);
    expect(Ok, status);
    status = GdipDeleteRegion(region);
todo_wine
    expect(Ok, status);

    /* Test a floating-point triangle */
    status = GdipCreatePath(FillModeAlternate, &path);
    expect(Ok, status);
    status = GdipAddPathLine(path, 5.6, 6.2, 7.2, 8.9);
    expect(Ok, status);
    status = GdipAddPathLine(path, 8.1, 1.6, 5.6, 6.2);
    expect(Ok, status);
todo_wine
{
    status = GdipCreateRegionPath(path, &region);
    expect(Ok, status);
    status = GdipGetRegionDataSize(region, &needed);
    expect(Ok, status);
    expect(72, needed);
    status = GdipGetRegionData(region, (BYTE*)buf, sizeof(buf), &needed);
    expect(Ok, status);
    expect(72, needed);
    expect_dword(buf, 64);
    trace("buf[1] = %08x\n", buf[1]);
    expect_magic((DWORD*)(buf + 2));
    expect_dword(buf + 3, 0);
    expect_dword(buf + 4, RGNDATA_PATH);

    expect_dword(buf + 5, 48);
    expect_magic((DWORD*)(buf + 6));
    expect_dword(buf + 7, 4);
    expect_dword(buf + 8, 0);
    expect_float(buf + 9, 5.6);
    expect_float(buf + 10, 6.2);
    expect_float(buf + 11, 7.2);
    expect_float(buf + 12, 8.9);
    expect_float(buf + 13, 8.1);
    expect_float(buf + 14, 1.6);
    expect_float(buf + 15, 5.6);
    expect_float(buf + 16, 6.2);
}

    status = GdipDeletePath(path);
    expect(Ok, status);
    status = GdipDeleteRegion(region);
todo_wine
    expect(Ok, status);
}

START_TEST(region)
{
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    gdiplusStartupInput.GdiplusVersion              = 1;
    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = 0;
    gdiplusStartupInput.SuppressExternalCodecs      = 0;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    test_getregiondata();

    GdiplusShutdown(gdiplusToken);

}
