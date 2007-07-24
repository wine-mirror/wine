/*
 * Unit test suite for pens (and init)
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

#include <stdarg.h>

#include "windef.h"
#include "gdiplus.h"
#include "wine/test.h"

#define expect(expected, got) ok(got == expected, "Expected %.8x, got %.8x\n", expected, got)

static void test_startup(void)
{
    GpPen *pen;
    Status status;
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    gdiplusStartupInput.GdiplusVersion              = 1;
    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = 0;
    gdiplusStartupInput.SuppressExternalCodecs      = 0;

    status = GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    expect(Ok, status);
    GdiplusShutdown(gdiplusToken);

    gdiplusStartupInput.GdiplusVersion = 2;

    status = GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    expect(UnsupportedGdiplusVersion, status);
    GdiplusShutdown(gdiplusToken);

    status = GdipCreatePen1((ARGB)0xffff00ff, 10.0f, UnitPixel, &pen);

    todo_wine
        expect(GdiplusNotInitialized, status);

    GdipDeletePen(pen);
}

static void test_constructor_destructor(void)
{
    GpStatus status;
    GpPen *pen = NULL;

    status = GdipCreatePen1((ARGB)0xffff00ff, 10.0f, UnitPixel, &pen);
    expect(Ok, status);
    ok(pen != NULL, "Expected pen to be initialized\n");

    status = GdipDeletePen(NULL);
    expect(InvalidParameter, status);

    status = GdipDeletePen(pen);
    expect(Ok, status);
}

static void test_brushfill(void)
{
    GpStatus status;
    GpPen *pen;
    GpBrush *brush, *brush2;
    GpBrushType type;
    ARGB color = 0;

    /* default solid */
    GdipCreatePen1(0xdeadbeef, 4.5, UnitWorld, &pen);
    status = GdipGetPenBrushFill(pen, &brush);
    expect(Ok, status);
    GdipGetBrushType(brush, &type);
    expect(BrushTypeSolidColor, type);
    GdipGetPenColor(pen, &color);
    todo_wine
        expect(0xdeadbeef, color);
    GdipDeleteBrush(brush);

    /* color controlled by brush */
    GdipCreateSolidFill(0xabaddeed, (GpSolidFill**)&brush);
    status = GdipSetPenBrushFill(pen, brush);
    expect(Ok, status);
    GdipGetPenColor(pen, &color);
    todo_wine
        expect(0xabaddeed, color);
    GdipDeleteBrush(brush);
    color = 0;

    /* get returns a clone, not a reference */
    GdipGetPenBrushFill(pen, &brush);
    GdipSetSolidFillColor((GpSolidFill*)brush, 0xbeadfeed);
    GdipGetPenBrushFill(pen, &brush2);
    ok(brush != brush2, "Expected to get a clone, not a copy of the reference\n");
    GdipGetSolidFillColor((GpSolidFill*)brush2, &color);
    expect(0xabaddeed, color);
    GdipDeleteBrush(brush);
    GdipDeleteBrush(brush2);

    /* brush cannot be NULL */
    status = GdipSetPenBrushFill(pen, NULL);
    expect(InvalidParameter, status);

    GdipDeletePen(pen);
}

START_TEST(pen)
{
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    test_startup();

    gdiplusStartupInput.GdiplusVersion              = 1;
    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = 0;
    gdiplusStartupInput.SuppressExternalCodecs      = 0;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    test_constructor_destructor();
    test_brushfill();

    GdiplusShutdown(gdiplusToken);
}
