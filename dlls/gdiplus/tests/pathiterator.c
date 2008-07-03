/*
 * Unit test suite for pathiterator
 *
 * Copyright (C) 2008 Nikolay Sivov
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

#define expect(expected, got) ok(got == expected, "Expected %.8x, got %.8x\n", expected, got)

static void test_constructor_destructor(void)
{
    GpPath *path;
    GpPathIterator *iter;
    GpStatus stat;

    GdipCreatePath(FillModeAlternate, &path);
    GdipAddPathRectangle(path, 5.0, 5.0, 100.0, 50.0);

    /* NULL args */
    stat = GdipCreatePathIter(NULL, NULL);
    expect(InvalidParameter, stat);
    stat = GdipCreatePathIter(&iter, NULL);
    expect(InvalidParameter, stat);
    stat = GdipCreatePathIter(NULL, path);
    expect(InvalidParameter, stat);
    stat = GdipDeletePathIter(NULL);
    expect(InvalidParameter, stat);

    /* valid args */
    stat = GdipCreatePathIter(&iter, path);
    expect(Ok, stat);

    GdipDeletePathIter(iter);
    GdipDeletePath(path);
}

START_TEST(pathiterator)
{
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    gdiplusStartupInput.GdiplusVersion              = 1;
    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = 0;
    gdiplusStartupInput.SuppressExternalCodecs      = 0;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    test_constructor_destructor();

    GdiplusShutdown(gdiplusToken);
}
