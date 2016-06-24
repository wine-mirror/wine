/*
 * Unit test suite for paths
 *
 * Copyright 2007 Laurent Vromman
 * Copyright 2007 Misha Koshelev
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
#include <stdio.h>
#include <assert.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"

#include "wine/test.h"

#include "winuser.h"
#include "winerror.h"

#define expect(expected, got) ok(got == expected, "Expected %.8x, got %.8x\n", expected, got)

static void test_path_state(void)
{
    BYTE buffer[sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD)];
    BITMAPINFO *bi = (BITMAPINFO *)buffer;
    HDC hdc;
    HRGN rgn;
    HBITMAP orig, dib;
    void *bits;
    BOOL ret;

    hdc = CreateCompatibleDC( 0 );
    memset( buffer, 0, sizeof(buffer) );
    bi->bmiHeader.biSize = sizeof(bi->bmiHeader);
    bi->bmiHeader.biHeight = 256;
    bi->bmiHeader.biWidth = 256;
    bi->bmiHeader.biBitCount = 32;
    bi->bmiHeader.biPlanes = 1;
    bi->bmiHeader.biCompression = BI_RGB;
    dib = CreateDIBSection( 0, bi, DIB_RGB_COLORS, (void**)&bits, NULL, 0 );
    orig = SelectObject( hdc, dib );

    BeginPath( hdc );
    LineTo( hdc, 100, 100 );
    ret = WidenPath( hdc );
    ok( !ret, "WidenPath succeeded\n" );

    /* selecting another bitmap doesn't affect the path */
    SelectObject( hdc, orig );
    ret = WidenPath( hdc );
    ok( !ret, "WidenPath succeeded\n" );

    SelectObject( hdc, dib );
    ret = WidenPath( hdc );
    ok( !ret, "WidenPath succeeded\n" );

    ret = EndPath( hdc );
    ok( ret, "EndPath failed error %u\n", GetLastError() );
    ret = WidenPath( hdc );
    ok( ret, "WidenPath failed error %u\n", GetLastError() );

    SelectObject( hdc, orig );
    ret = WidenPath( hdc );
    ok( ret, "WidenPath failed error %u\n", GetLastError() );

    BeginPath( hdc );
    LineTo( hdc, 100, 100 );
    ret = WidenPath( hdc );
    ok( !ret, "WidenPath succeeded\n" );
    SaveDC( hdc );
    SelectObject( hdc, dib );
    ret = EndPath( hdc );
    ok( ret, "EndPath failed error %u\n", GetLastError() );
    ret = WidenPath( hdc );
    ok( ret, "WidenPath failed error %u\n", GetLastError() );

    /* path should be open again after RestoreDC */
    RestoreDC( hdc, -1  );
    ret = WidenPath( hdc );
    ok( !ret, "WidenPath succeeded\n" );
    ret = EndPath( hdc );
    ok( ret, "EndPath failed error %u\n", GetLastError() );

    SaveDC( hdc );
    BeginPath( hdc );
    RestoreDC( hdc, -1  );
    ret = WidenPath( hdc );
    ok( ret, "WidenPath failed error %u\n", GetLastError() );

    /* test all functions with no path at all */
    AbortPath( hdc );
    SetLastError( 0xdeadbeef );
    ret = WidenPath( hdc );
    ok( !ret, "WidenPath succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = FlattenPath( hdc );
    ok( !ret, "FlattenPath succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = StrokePath( hdc );
    ok( !ret, "StrokePath succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = FillPath( hdc );
    ok( !ret, "FillPath succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = StrokeAndFillPath( hdc );
    ok( !ret, "StrokeAndFillPath succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = SelectClipPath( hdc, RGN_OR );
    ok( !ret, "SelectClipPath succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    rgn = PathToRegion( hdc );
    ok( !rgn, "PathToRegion succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = EndPath( hdc );
    ok( !ret, "SelectClipPath succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = CloseFigure( hdc );
    ok( !ret, "CloseFigure succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %u\n", GetLastError() );

    /* test all functions with an open path */
    AbortPath( hdc );
    BeginPath( hdc );
    SetLastError( 0xdeadbeef );
    ret = WidenPath( hdc );
    ok( !ret, "WidenPath succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %u\n", GetLastError() );

    AbortPath( hdc );
    BeginPath( hdc );
    SetLastError( 0xdeadbeef );
    ret = FlattenPath( hdc );
    ok( !ret, "FlattenPath succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %u\n", GetLastError() );

    AbortPath( hdc );
    BeginPath( hdc );
    SetLastError( 0xdeadbeef );
    ret = StrokePath( hdc );
    ok( !ret, "StrokePath succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %u\n", GetLastError() );

    AbortPath( hdc );
    BeginPath( hdc );
    SetLastError( 0xdeadbeef );
    ret = FillPath( hdc );
    ok( !ret, "FillPath succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %u\n", GetLastError() );

    AbortPath( hdc );
    BeginPath( hdc );
    SetLastError( 0xdeadbeef );
    ret = StrokeAndFillPath( hdc );
    ok( !ret, "StrokeAndFillPath succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %u\n", GetLastError() );

    AbortPath( hdc );
    BeginPath( hdc );
    Rectangle( hdc, 1, 1, 10, 10 );  /* region needs some contents */
    SetLastError( 0xdeadbeef );
    ret = SelectClipPath( hdc, RGN_OR );
    ok( !ret, "SelectClipPath succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %u\n", GetLastError() );

    AbortPath( hdc );
    BeginPath( hdc );
    Rectangle( hdc, 1, 1, 10, 10 );  /* region needs some contents */
    SetLastError( 0xdeadbeef );
    rgn = PathToRegion( hdc );
    ok( !rgn, "PathToRegion succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %u\n", GetLastError() );

    AbortPath( hdc );
    BeginPath( hdc );
    ret = CloseFigure( hdc );
    ok( ret, "CloseFigure failed\n" );

    /* test all functions with a closed path */
    AbortPath( hdc );
    BeginPath( hdc );
    EndPath( hdc );
    ret = WidenPath( hdc );
    ok( ret, "WidenPath failed\n" );
    ok( GetPath( hdc, NULL, NULL, 0 ) != -1, "path deleted\n" );

    AbortPath( hdc );
    BeginPath( hdc );
    EndPath( hdc );
    ret = FlattenPath( hdc );
    ok( ret, "FlattenPath failed\n" );
    ok( GetPath( hdc, NULL, NULL, 0 ) != -1, "path deleted\n" );

    AbortPath( hdc );
    BeginPath( hdc );
    EndPath( hdc );
    ret = StrokePath( hdc );
    ok( ret, "StrokePath failed\n" );
    ok( GetPath( hdc, NULL, NULL, 0 ) == -1, "path not deleted\n" );

    BeginPath( hdc );
    EndPath( hdc );
    ret = FillPath( hdc );
    ok( ret, "FillPath failed\n" );
    ok( GetPath( hdc, NULL, NULL, 0 ) == -1, "path not deleted\n" );

    BeginPath( hdc );
    EndPath( hdc );
    ret = StrokeAndFillPath( hdc );
    ok( ret, "StrokeAndFillPath failed\n" );
    ok( GetPath( hdc, NULL, NULL, 0 ) == -1, "path not deleted\n" );

    BeginPath( hdc );
    Rectangle( hdc, 1, 1, 10, 10 );  /* region needs some contents */
    EndPath( hdc );
    ret = SelectClipPath( hdc, RGN_OR );
    ok( ret, "SelectClipPath failed\n" );
    ok( GetPath( hdc, NULL, NULL, 0 ) == -1, "path not deleted\n" );

    BeginPath( hdc );
    EndPath( hdc );
    SetLastError( 0xdeadbeef );
    ret = SelectClipPath( hdc, RGN_OR );
    ok( !ret, "SelectClipPath succeeded on empty path\n" );
    ok( GetLastError() == 0xdeadbeef, "wrong error %u\n", GetLastError() );
    ok( GetPath( hdc, NULL, NULL, 0 ) == -1, "path not deleted\n" );

    BeginPath( hdc );
    Rectangle( hdc, 1, 1, 10, 10 );  /* region needs some contents */
    EndPath( hdc );
    rgn = PathToRegion( hdc );
    ok( rgn != 0, "PathToRegion failed\n" );
    DeleteObject( rgn );
    ok( GetPath( hdc, NULL, NULL, 0 ) == -1, "path not deleted\n" );

    BeginPath( hdc );
    EndPath( hdc );
    SetLastError( 0xdeadbeef );
    rgn = PathToRegion( hdc );
    ok( !rgn, "PathToRegion succeeded on empty path\n" );
    ok( GetLastError() == 0xdeadbeef, "wrong error %u\n", GetLastError() );
    DeleteObject( rgn );
    ok( GetPath( hdc, NULL, NULL, 0 ) == -1, "path not deleted\n" );

    BeginPath( hdc );
    EndPath( hdc );
    SetLastError( 0xdeadbeef );
    ret = CloseFigure( hdc );
    ok( !ret, "CloseFigure succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %u\n", GetLastError() );

    AbortPath( hdc );
    BeginPath( hdc );
    EndPath( hdc );
    SetLastError( 0xdeadbeef );
    ret = EndPath( hdc );
    ok( !ret, "EndPath succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %u\n", GetLastError() );

    DeleteDC( hdc );
    DeleteObject( dib );
}

static void test_widenpath(void)
{
    HDC hdc = GetDC(0);
    HPEN greenPen, narrowPen;
    POINT pnt[6];
    INT nSize;
    BOOL ret;

    /* Create a pen to be used in WidenPath */
    greenPen = CreatePen(PS_SOLID, 10, RGB(0,0,0));
    SelectObject(hdc, greenPen);

    /* Prepare a path */
    pnt[0].x = 100;
    pnt[0].y = 0;
    pnt[1].x = 200;
    pnt[1].y = 0;
    pnt[2].x = 300;
    pnt[2].y = 100;
    pnt[3].x = 300;
    pnt[3].y = 200;
    pnt[4].x = 200;
    pnt[4].y = 300;
    pnt[5].x = 100;
    pnt[5].y = 300;

    /* Set a polyline path */
    BeginPath(hdc);
    Polyline(hdc, pnt, 6);
    EndPath(hdc);

    /* Widen the polyline path */
    ok(WidenPath(hdc), "WidenPath fails while widening a poyline path.\n");

    /* Test if WidenPath seems to have done his job */
    nSize = GetPath(hdc, NULL, NULL, 0);
    ok(nSize != -1, "GetPath fails after calling WidenPath.\n");
    ok(nSize > 6, "Path number of points is too low. Should be more than 6 but is %d\n", nSize);

    AbortPath(hdc);

    /* Test WidenPath with an open path (last error only set on Win2k and later) */
    SetLastError(0xdeadbeef);
    BeginPath(hdc);
    ret = WidenPath(hdc);
    ok(ret == FALSE && (GetLastError() == ERROR_CAN_NOT_COMPLETE || GetLastError() == 0xdeadbeef),
       "WidenPath fails while widening an open path. Return value is %d, should be %d. Error is %u\n", ret, FALSE, GetLastError());

    AbortPath(hdc);

    /* Test when the pen width is equal to 1. The path should change too */
    narrowPen = CreatePen(PS_SOLID, 1, RGB(0,0,0));
    SelectObject(hdc, narrowPen);
    BeginPath(hdc);
    Polyline(hdc, pnt, 6);
    EndPath(hdc);
    ret = WidenPath(hdc);
    ok(ret == TRUE, "WidenPath failed: %d\n", GetLastError());
    nSize = GetPath(hdc, NULL, NULL, 0);
    ok(nSize > 6, "WidenPath should compute a widened path with a 1px wide pen. Path length is %d, should be more than 6\n", nSize);

    ReleaseDC(0, hdc);
    return;
}

/*
 * Tests for GDI drawing functions in paths
 */

typedef struct
{
    int x, y;
    BYTE type;
} path_test_t;

/* Helper function to verify that the current path in the given DC matches the expected path.
 *
 * We use a "smart" matching algorithm that allows us to detect partial improvements
 * in conformance. Specifically, two running indices are kept, one through the actual
 * path and one through the expected path. The actual path index increases unless there is
 * no match and the todo field of the appropriate path_test_t element is 2. Similarly,
 * if the wine_entries_preceding field of the appropriate path_test_t element is non-zero,
 * the expected path index does not increase for that many elements as long as there
 * is no match. This allows us to todo_wine extra path elements that are present only
 * on wine but not on native and vice versa.
 *
 * Note that if expected_size is zero and the WINETEST_DEBUG environment variable is
 * greater than 2, the trace() output is a C path_test_t array structure, useful for making
 * new tests that use this function.
 */
static void ok_path(HDC hdc, const char *path_name, const path_test_t *expected, int expected_size)
{
    static const char *type_string[8] = { "Unknown (0)", "PT_CLOSEFIGURE", "PT_LINETO",
                                          "PT_LINETO | PT_CLOSEFIGURE", "PT_BEZIERTO",
                                          "PT_BEZIERTO | PT_CLOSEFIGURE", "PT_MOVETO", "PT_MOVETO | PT_CLOSEFIGURE"};
    POINT *pnt;
    BYTE *types;
    int size, idx;

    /* Get the path */
    assert(hdc != 0);
    size = GetPath(hdc, NULL, NULL, 0);
    ok(size > 0, "GetPath returned size %d, last error %d\n", size, GetLastError());
    if (size <= 0) return;

    pnt = HeapAlloc(GetProcessHeap(), 0, size*sizeof(POINT));
    assert(pnt != 0);
    types = HeapAlloc(GetProcessHeap(), 0, size*sizeof(BYTE));
    assert(types != 0);
    size = GetPath(hdc, pnt, types, size);
    assert(size > 0);

    ok( size == expected_size, "%s: Path size %d does not match expected size %d\n",
        path_name, size, expected_size);

    for (idx = 0; idx < min( size, expected_size ); idx++)
    {
        /* We allow a few pixels fudge in matching X and Y coordinates to account for imprecision in
         * floating point to integer conversion */
        static const int fudge = 2;

        ok( types[idx] == expected[idx].type, "%s: Expected #%d: %s (%d,%d) but got %s (%d,%d)\n",
            path_name, idx, type_string[expected[idx].type], expected[idx].x, expected[idx].y,
            type_string[types[idx]], pnt[idx].x, pnt[idx].y);

        if (types[idx] == expected[idx].type)
            ok( (pnt[idx].x >= expected[idx].x - fudge && pnt[idx].x <= expected[idx].x + fudge) &&
                (pnt[idx].y >= expected[idx].y - fudge && pnt[idx].y <= expected[idx].y + fudge),
                "%s: Expected #%d: %s  position (%d,%d) but got (%d,%d)\n", path_name, idx,
                type_string[expected[idx].type], expected[idx].x, expected[idx].y, pnt[idx].x, pnt[idx].y);
    }

    if (winetest_debug > 2)
    {
        printf("static const path_test_t %s[] =\n{\n", path_name);
        for (idx = 0; idx < size; idx++)
            printf("    {%d, %d, %s}, /* %d */\n", pnt[idx].x, pnt[idx].y, type_string[types[idx]], idx);
        printf("};\n" );
    }

    HeapFree(GetProcessHeap(), 0, types);
    HeapFree(GetProcessHeap(), 0, pnt);
}

static const path_test_t arcto_path[] =
{
    {0, 0, PT_MOVETO}, /* 0 */
    {229, 215, PT_LINETO}, /* 1 */
    {248, 205, PT_BEZIERTO}, /* 2 */
    {273, 200, PT_BEZIERTO}, /* 3 */
    {300, 200, PT_BEZIERTO}, /* 4 */
    {355, 200, PT_BEZIERTO}, /* 5 */
    {399, 222, PT_BEZIERTO}, /* 6 */
    {399, 250, PT_BEZIERTO}, /* 7 */
    {399, 263, PT_BEZIERTO}, /* 8 */
    {389, 275, PT_BEZIERTO}, /* 9 */
    {370, 285, PT_BEZIERTO}, /* 10 */
    {363, 277, PT_LINETO}, /* 11 */
    {380, 270, PT_BEZIERTO}, /* 12 */
    {389, 260, PT_BEZIERTO}, /* 13 */
    {389, 250, PT_BEZIERTO}, /* 14 */
    {389, 228, PT_BEZIERTO}, /* 15 */
    {349, 210, PT_BEZIERTO}, /* 16 */
    {300, 210, PT_BEZIERTO}, /* 17 */
    {276, 210, PT_BEZIERTO}, /* 18 */
    {253, 214, PT_BEZIERTO}, /* 19 */
    {236, 222, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 20 */
};

static void test_arcto(void)
{
    HDC hdc = GetDC(0);

    BeginPath(hdc);
    SetArcDirection(hdc, AD_CLOCKWISE);
    if (!ArcTo(hdc, 200, 200, 400, 300, 200, 200, 400, 300) &&
        GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        /* ArcTo is only available on Win2k and later */
        win_skip("ArcTo is not available\n");
        goto done;
    }
    SetArcDirection(hdc, AD_COUNTERCLOCKWISE);
    ArcTo(hdc, 210, 210, 390, 290, 390, 290, 210, 210);
    CloseFigure(hdc);
    EndPath(hdc);

    ok_path(hdc, "arcto_path", arcto_path, sizeof(arcto_path)/sizeof(path_test_t));
done:
    ReleaseDC(0, hdc);
}

static const path_test_t anglearc_path[] =
{
    {0, 0, PT_MOVETO}, /* 0 */
    {371, 229, PT_LINETO}, /* 1 */
    {352, 211, PT_BEZIERTO}, /* 2 */
    {327, 200, PT_BEZIERTO}, /* 3 */
    {300, 200, PT_BEZIERTO}, /* 4 */
    {245, 200, PT_BEZIERTO}, /* 5 */
    {200, 245, PT_BEZIERTO}, /* 6 */
    {200, 300, PT_BEZIERTO}, /* 7 */
    {200, 300, PT_BEZIERTO}, /* 8 */
    {200, 300, PT_BEZIERTO}, /* 9 */
    {200, 300, PT_BEZIERTO}, /* 10 */
    {231, 260, PT_LINETO}, /* 11 */
    {245, 235, PT_BEZIERTO}, /* 12 */
    {271, 220, PT_BEZIERTO}, /* 13 */
    {300, 220, PT_BEZIERTO}, /* 14 */
    {344, 220, PT_BEZIERTO}, /* 15 */
    {380, 256, PT_BEZIERTO}, /* 16 */
    {380, 300, PT_BEZIERTO}, /* 17 */
    {380, 314, PT_BEZIERTO}, /* 18 */
    {376, 328, PT_BEZIERTO}, /* 19 */
    {369, 340, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 20 */
};

static void test_anglearc(void)
{
    HDC hdc = GetDC(0);
    BeginPath(hdc);
    if (!AngleArc(hdc, 300, 300, 100, 45.0, 135.0) &&
        GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        /* AngleArc is only available on Win2k and later */
        win_skip("AngleArc is not available\n");
        goto done;
    }
    AngleArc(hdc, 300, 300, 80, 150.0, -180.0);
    CloseFigure(hdc);
    EndPath(hdc);

    ok_path(hdc, "anglearc_path", anglearc_path, sizeof(anglearc_path)/sizeof(path_test_t));
done:
    ReleaseDC(0, hdc);
}

static const path_test_t polydraw_path[] =
{
    {-20, -20, PT_MOVETO}, /* 0 */
    {10, 10, PT_LINETO}, /* 1 */
    {10, 15, PT_LINETO | PT_CLOSEFIGURE}, /* 2 */
    {-20, -20, PT_MOVETO}, /* 3 */
    {-10, -10, PT_LINETO}, /* 4 */
    {100, 100, PT_MOVETO}, /* 5 */
    {95, 95, PT_LINETO}, /* 6 */
    {10, 10, PT_LINETO}, /* 7 */
    {10, 15, PT_LINETO | PT_CLOSEFIGURE}, /* 8 */
    {100, 100, PT_MOVETO}, /* 9 */
    {15, 15, PT_LINETO}, /* 10 */
    {25, 25, PT_MOVETO}, /* 11 */
    {25, 30, PT_LINETO}, /* 12 */
    {100, 100, PT_MOVETO}, /* 13 */
    {30, 30, PT_BEZIERTO}, /* 14 */
    {30, 35, PT_BEZIERTO}, /* 15 */
    {35, 35, PT_BEZIERTO}, /* 16 */
    {35, 40, PT_LINETO}, /* 17 */
    {40, 40, PT_MOVETO}, /* 18 */
    {40, 45, PT_LINETO}, /* 19 */
    {35, 40, PT_MOVETO}, /* 20 */
    {45, 50, PT_LINETO}, /* 21 */
    {35, 40, PT_MOVETO}, /* 22 */
    {50, 55, PT_LINETO}, /* 23 */
    {45, 50, PT_LINETO}, /* 24 */
    {35, 40, PT_MOVETO}, /* 25 */
    {60, 60, PT_LINETO}, /* 26 */
    {60, 65, PT_MOVETO}, /* 27 */
    {65, 65, PT_LINETO}, /* 28 */
    {75, 75, PT_MOVETO}, /* 29 */
    {80, 80, PT_LINETO | PT_CLOSEFIGURE}, /* 30 */
};

static POINT polydraw_pts[] = {
    {10, 10}, {10, 15},
    {15, 15}, {15, 20}, {20, 20}, {20, 25},
    {25, 25}, {25, 30},
    {30, 30}, {30, 35}, {35, 35}, {35, 40},
    {40, 40}, {40, 45}, {45, 45},
    {45, 50}, {50, 50},
    {50, 55}, {45, 50}, {55, 60},
    {60, 60}, {60, 65}, {65, 65},
    {70, 70}, {75, 70}, {75, 75}, {80, 80}};

static BYTE polydraw_tps[] =
    {PT_LINETO, PT_CLOSEFIGURE | PT_LINETO, /* 2 */
     PT_LINETO, PT_BEZIERTO, PT_LINETO, PT_LINETO, /* 6 */
     PT_MOVETO, PT_LINETO, /* 8 */
     PT_BEZIERTO, PT_BEZIERTO, PT_BEZIERTO, PT_LINETO, /* 12 */
     PT_MOVETO, PT_LINETO, PT_CLOSEFIGURE, /* 15 */
     PT_LINETO, PT_MOVETO | PT_CLOSEFIGURE, /* 17 */
     PT_LINETO, PT_LINETO, PT_MOVETO | PT_CLOSEFIGURE, /* 20 */
     PT_LINETO, PT_MOVETO | PT_LINETO, PT_LINETO,  /* 23 */
     PT_MOVETO, PT_MOVETO, PT_MOVETO, PT_LINETO | PT_CLOSEFIGURE}; /* 27 */

static void test_polydraw(void)
{
    BOOL retb;
    POINT pos;
    HDC hdc = GetDC(0);

    MoveToEx( hdc, -20, -20, NULL );

    BeginPath(hdc);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == -20 && pos.y == -20, "wrong pos %d,%d\n", pos.x, pos.y );

    /* closefigure with no previous moveto */
    if (!(retb = PolyDraw(hdc, polydraw_pts, polydraw_tps, 2)) &&
        GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        /* PolyDraw is only available on Win2k and later */
        win_skip("PolyDraw is not available\n");
        goto done;
    }
    expect(TRUE, retb);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 10 && pos.y == 15, "wrong pos %d,%d\n", pos.x, pos.y );
    LineTo(hdc, -10, -10);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == -10 && pos.y == -10, "wrong pos %d,%d\n", pos.x, pos.y );

    MoveToEx(hdc, 100, 100, NULL);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 100 && pos.y == 100, "wrong pos %d,%d\n", pos.x, pos.y );
    LineTo(hdc, 95, 95);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 95 && pos.y == 95, "wrong pos %d,%d\n", pos.x, pos.y );
    /* closefigure with previous moveto */
    retb = PolyDraw(hdc, polydraw_pts, polydraw_tps, 2);
    expect(TRUE, retb);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 10 && pos.y == 15, "wrong pos %d,%d\n", pos.x, pos.y );
    /* bad bezier points */
    retb = PolyDraw(hdc, &(polydraw_pts[2]), &(polydraw_tps[2]), 4);
    expect(FALSE, retb);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 10 && pos.y == 15, "wrong pos %d,%d\n", pos.x, pos.y );
    retb = PolyDraw(hdc, &(polydraw_pts[6]), &(polydraw_tps[6]), 4);
    expect(FALSE, retb);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 10 && pos.y == 15, "wrong pos %d,%d\n", pos.x, pos.y );
    /* good bezier points */
    retb = PolyDraw(hdc, &(polydraw_pts[8]), &(polydraw_tps[8]), 4);
    expect(TRUE, retb);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 35 && pos.y == 40, "wrong pos %d,%d\n", pos.x, pos.y );
    /* does lineto or bezierto take precedence? */
    retb = PolyDraw(hdc, &(polydraw_pts[12]), &(polydraw_tps[12]), 4);
    expect(FALSE, retb);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 35 && pos.y == 40, "wrong pos %d,%d\n", pos.x, pos.y );
    /* bad point type, has already moved cursor position */
    retb = PolyDraw(hdc, &(polydraw_pts[15]), &(polydraw_tps[15]), 4);
    expect(FALSE, retb);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 35 && pos.y == 40, "wrong pos %d,%d\n", pos.x, pos.y );
    /* bad point type, cursor position is moved, but back to its original spot */
    retb = PolyDraw(hdc, &(polydraw_pts[17]), &(polydraw_tps[17]), 4);
    expect(FALSE, retb);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 35 && pos.y == 40, "wrong pos %d,%d\n", pos.x, pos.y );
    /* does lineto or moveto take precedence? */
    retb = PolyDraw(hdc, &(polydraw_pts[20]), &(polydraw_tps[20]), 3);
    expect(TRUE, retb);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 65 && pos.y == 65, "wrong pos %d,%d\n", pos.x, pos.y );
    /* consecutive movetos */
    retb = PolyDraw(hdc, &(polydraw_pts[23]), &(polydraw_tps[23]), 4);
    expect(TRUE, retb);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 80 && pos.y == 80, "wrong pos %d,%d\n", pos.x, pos.y );

    EndPath(hdc);
    ok_path(hdc, "polydraw_path", polydraw_path, sizeof(polydraw_path)/sizeof(path_test_t));
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 80 && pos.y == 80, "wrong pos %d,%d\n", pos.x, pos.y );
done:
    ReleaseDC(0, hdc);
}

static void test_closefigure(void) {
    int nSize, nSizeWitness;
    POINT pos;
    HDC hdc = GetDC(0);

    MoveToEx( hdc, 100, 100, NULL );
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 100 && pos.y == 100, "wrong pos %d,%d\n", pos.x, pos.y );

    BeginPath(hdc);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 100 && pos.y == 100, "wrong pos %d,%d\n", pos.x, pos.y );
    MoveToEx(hdc, 95, 95, NULL);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 95 && pos.y == 95, "wrong pos %d,%d\n", pos.x, pos.y );
    LineTo(hdc, 95,  0);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 95 && pos.y == 0, "wrong pos %d,%d\n", pos.x, pos.y );
    LineTo(hdc,  0, 95);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 0 && pos.y == 95, "wrong pos %d,%d\n", pos.x, pos.y );

    CloseFigure(hdc);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 0 && pos.y == 95, "wrong pos %d,%d\n", pos.x, pos.y );
    EndPath(hdc);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 0 && pos.y == 95, "wrong pos %d,%d\n", pos.x, pos.y );
    nSize = GetPath(hdc, NULL, NULL, 0);

    AbortPath(hdc);

    BeginPath(hdc);
    MoveToEx(hdc, 95, 95, NULL);
    LineTo(hdc, 95,  0);
    LineTo(hdc,  0, 95);

    EndPath(hdc);
    nSizeWitness = GetPath(hdc, NULL, NULL, 0);

    /* This test shows CloseFigure does not have to add a point at the end of the path */
    ok(nSize == nSizeWitness, "Wrong number of points, no point should be added by CloseFigure\n");

    ReleaseDC(0, hdc);
}

static void WINAPI linedda_callback(INT x, INT y, LPARAM lparam)
{
    POINT **pt = (POINT**)lparam;
    ok((*pt)->x == x && (*pt)->y == y, "point mismatch expect(%d,%d) got(%d,%d)\n",
       (*pt)->x, (*pt)->y, x, y);

    (*pt)++;
    return;
}

static void test_linedda(void)
{
    const POINT *pt;
    static const POINT array_10_20_20_40[] = {{10,20},{10,21},{11,22},{11,23},
                                              {12,24},{12,25},{13,26},{13,27},
                                              {14,28},{14,29},{15,30},{15,31},
                                              {16,32},{16,33},{17,34},{17,35},
                                              {18,36},{18,37},{19,38},{19,39},
                                              {-1,-1}};
    static const POINT array_10_20_20_43[] = {{10,20},{10,21},{11,22},{11,23},
                                              {12,24},{12,25},{13,26},{13,27},
                                              {13,28},{14,29},{14,30},{15,31},
                                              {15,32},{16,33},{16,34},{17,35},
                                              {17,36},{17,37},{18,38},{18,39},
                                              {19,40},{19,41},{20,42},{-1,-1}};

    static const POINT array_10_20_10_20[] = {{-1,-1}};
    static const POINT array_10_20_11_27[] = {{10,20},{10,21},{10,22},{10,23},
                                              {11,24},{11,25},{11,26},{-1,-1}};

    static const POINT array_20_43_10_20[] = {{20,43},{20,42},{19,41},{19,40},
                                              {18,39},{18,38},{17,37},{17,36},
                                              {17,35},{16,34},{16,33},{15,32},
                                              {15,31},{14,30},{14,29},{13,28},
                                              {13,27},{13,26},{12,25},{12,24},
                                              {11,23},{11,22},{10,21},{-1,-1}};

    static const POINT array_20_20_10_43[] = {{20,20},{20,21},{19,22},{19,23},
                                              {18,24},{18,25},{17,26},{17,27},
                                              {17,28},{16,29},{16,30},{15,31},
                                              {15,32},{14,33},{14,34},{13,35},
                                              {13,36},{13,37},{12,38},{12,39},
                                              {11,40},{11,41},{10,42},{-1,-1}};

    static const POINT array_20_20_43_10[] = {{20,20},{21,20},{22,19},{23,19},
                                              {24,18},{25,18},{26,17},{27,17},
                                              {28,17},{29,16},{30,16},{31,15},
                                              {32,15},{33,14},{34,14},{35,13},
                                              {36,13},{37,13},{38,12},{39,12},
                                              {40,11},{41,11},{42,10},{-1,-1}};


    pt = array_10_20_20_40;
    LineDDA(10, 20, 20, 40, linedda_callback, (LPARAM)&pt);
    ok(pt->x == -1 && pt->y == -1, "didn't find terminator\n");

    pt = array_10_20_20_43;
    LineDDA(10, 20, 20, 43, linedda_callback, (LPARAM)&pt);
    ok(pt->x == -1 && pt->y == -1, "didn't find terminator\n");

    pt = array_10_20_10_20;
    LineDDA(10, 20, 10, 20, linedda_callback, (LPARAM)&pt);
    ok(pt->x == -1 && pt->y == -1, "didn't find terminator\n");

    pt = array_10_20_11_27;
    LineDDA(10, 20, 11, 27, linedda_callback, (LPARAM)&pt);
    ok(pt->x == -1 && pt->y == -1, "didn't find terminator\n");

    pt = array_20_43_10_20;
    LineDDA(20, 43, 10, 20, linedda_callback, (LPARAM)&pt);
    ok(pt->x == -1 && pt->y == -1, "didn't find terminator\n");

    pt = array_20_20_10_43;
    LineDDA(20, 20, 10, 43, linedda_callback, (LPARAM)&pt);
    ok(pt->x == -1 && pt->y == -1, "didn't find terminator\n");

    pt = array_20_20_43_10;
    LineDDA(20, 20, 43, 10, linedda_callback, (LPARAM)&pt);
    ok(pt->x == -1 && pt->y == -1, "didn't find terminator\n");
}

static const path_test_t rectangle_path[] =
{
    {39, 20, PT_MOVETO}, /* 0 */
    {20, 20, PT_LINETO}, /* 1 */
    {20, 39, PT_LINETO}, /* 2 */
    {39, 39, PT_LINETO | PT_CLOSEFIGURE}, /* 3 */
    {54, 35, PT_MOVETO}, /* 4 */
    {30, 35, PT_LINETO}, /* 5 */
    {30, 49, PT_LINETO}, /* 6 */
    {54, 49, PT_LINETO | PT_CLOSEFIGURE}, /* 7 */
    {59, 45, PT_MOVETO}, /* 8 */
    {35, 45, PT_LINETO}, /* 9 */
    {35, 59, PT_LINETO}, /* 10 */
    {59, 59, PT_LINETO | PT_CLOSEFIGURE}, /* 11 */
    {80, 80, PT_MOVETO}, /* 12 */
    {80, 80, PT_LINETO}, /* 13 */
    {80, 80, PT_LINETO}, /* 14 */
    {80, 80, PT_LINETO | PT_CLOSEFIGURE}, /* 15 */
    {39, 39, PT_MOVETO}, /* 16 */
    {20, 39, PT_LINETO}, /* 17 */
    {20, 20, PT_LINETO}, /* 18 */
    {39, 20, PT_LINETO | PT_CLOSEFIGURE}, /* 19 */
    {54, 49, PT_MOVETO}, /* 20 */
    {30, 49, PT_LINETO}, /* 21 */
    {30, 35, PT_LINETO}, /* 22 */
    {54, 35, PT_LINETO | PT_CLOSEFIGURE}, /* 23 */
    {59, 59, PT_MOVETO}, /* 24 */
    {35, 59, PT_LINETO}, /* 25 */
    {35, 45, PT_LINETO}, /* 26 */
    {59, 45, PT_LINETO | PT_CLOSEFIGURE}, /* 27 */
    {80, 80, PT_MOVETO}, /* 28 */
    {80, 80, PT_LINETO}, /* 29 */
    {80, 80, PT_LINETO}, /* 30 */
    {80, 80, PT_LINETO | PT_CLOSEFIGURE}, /* 31 */
    {-41, 40, PT_MOVETO}, /* 32 */
    {-80, 40, PT_LINETO}, /* 33 */
    {-80, 79, PT_LINETO}, /* 34 */
    {-41, 79, PT_LINETO | PT_CLOSEFIGURE}, /* 35 */
    {-61, 70, PT_MOVETO}, /* 36 */
    {-110, 70, PT_LINETO}, /* 37 */
    {-110, 99, PT_LINETO}, /* 38 */
    {-61, 99, PT_LINETO | PT_CLOSEFIGURE}, /* 39 */
    {119, -120, PT_MOVETO}, /* 40 */
    {60, -120, PT_LINETO}, /* 41 */
    {60, -61, PT_LINETO}, /* 42 */
    {119, -61, PT_LINETO | PT_CLOSEFIGURE}, /* 43 */
    {164, -150, PT_MOVETO}, /* 44 */
    {90, -150, PT_LINETO}, /* 45 */
    {90, -106, PT_LINETO}, /* 46 */
    {164, -106, PT_LINETO | PT_CLOSEFIGURE}, /* 47 */
    {-4, -6, PT_MOVETO}, /* 48 */
    {-6, -6, PT_LINETO}, /* 49 */
    {-6, -4, PT_LINETO}, /* 50 */
    {-4, -4, PT_LINETO | PT_CLOSEFIGURE}, /* 51 */
    {40, 20, PT_MOVETO}, /* 52 */
    {20, 20, PT_LINETO}, /* 53 */
    {20, 40, PT_LINETO}, /* 54 */
    {40, 40, PT_LINETO | PT_CLOSEFIGURE}, /* 55 */
    {55, 35, PT_MOVETO}, /* 56 */
    {30, 35, PT_LINETO}, /* 57 */
    {30, 50, PT_LINETO}, /* 58 */
    {55, 50, PT_LINETO | PT_CLOSEFIGURE}, /* 59 */
    {60, 45, PT_MOVETO}, /* 60 */
    {35, 45, PT_LINETO}, /* 61 */
    {35, 60, PT_LINETO}, /* 62 */
    {60, 60, PT_LINETO | PT_CLOSEFIGURE}, /* 63 */
    {70, 70, PT_MOVETO}, /* 64 */
    {50, 70, PT_LINETO}, /* 65 */
    {50, 70, PT_LINETO}, /* 66 */
    {70, 70, PT_LINETO | PT_CLOSEFIGURE}, /* 67 */
    {75, 75, PT_MOVETO}, /* 68 */
    {75, 75, PT_LINETO}, /* 69 */
    {75, 85, PT_LINETO}, /* 70 */
    {75, 85, PT_LINETO | PT_CLOSEFIGURE}, /* 71 */
    {81, 80, PT_MOVETO}, /* 72 */
    {80, 80, PT_LINETO}, /* 73 */
    {80, 81, PT_LINETO}, /* 74 */
    {81, 81, PT_LINETO | PT_CLOSEFIGURE}, /* 75 */
    {40, 40, PT_MOVETO}, /* 76 */
    {20, 40, PT_LINETO}, /* 77 */
    {20, 20, PT_LINETO}, /* 78 */
    {40, 20, PT_LINETO | PT_CLOSEFIGURE}, /* 79 */
    {55, 50, PT_MOVETO}, /* 80 */
    {30, 50, PT_LINETO}, /* 81 */
    {30, 35, PT_LINETO}, /* 82 */
    {55, 35, PT_LINETO | PT_CLOSEFIGURE}, /* 83 */
    {60, 60, PT_MOVETO}, /* 84 */
    {35, 60, PT_LINETO}, /* 85 */
    {35, 45, PT_LINETO}, /* 86 */
    {60, 45, PT_LINETO | PT_CLOSEFIGURE}, /* 87 */
    {70, 70, PT_MOVETO}, /* 88 */
    {50, 70, PT_LINETO}, /* 89 */
    {50, 70, PT_LINETO}, /* 90 */
    {70, 70, PT_LINETO | PT_CLOSEFIGURE}, /* 91 */
    {75, 85, PT_MOVETO}, /* 92 */
    {75, 85, PT_LINETO}, /* 93 */
    {75, 75, PT_LINETO}, /* 94 */
    {75, 75, PT_LINETO | PT_CLOSEFIGURE}, /* 95 */
    {81, 81, PT_MOVETO}, /* 96 */
    {80, 81, PT_LINETO}, /* 97 */
    {80, 80, PT_LINETO}, /* 98 */
    {81, 80, PT_LINETO | PT_CLOSEFIGURE}, /* 99 */
};

static void test_rectangle(void)
{
    HDC hdc = GetDC( 0 );

    BeginPath( hdc );
    Rectangle( hdc, 20, 20, 40, 40 );
    Rectangle( hdc, 30, 50, 55, 35 );
    Rectangle( hdc, 60, 60, 35, 45 );
    Rectangle( hdc, 70, 70, 50, 70 );
    Rectangle( hdc, 75, 75, 75, 85 );
    Rectangle( hdc, 80, 80, 81, 81 );
    SetArcDirection( hdc, AD_CLOCKWISE );
    Rectangle( hdc, 20, 20, 40, 40 );
    Rectangle( hdc, 30, 50, 55, 35 );
    Rectangle( hdc, 60, 60, 35, 45 );
    Rectangle( hdc, 70, 70, 50, 70 );
    Rectangle( hdc, 75, 75, 75, 85 );
    Rectangle( hdc, 80, 80, 81, 81 );
    SetArcDirection( hdc, AD_COUNTERCLOCKWISE );
    SetMapMode( hdc, MM_ANISOTROPIC );
    SetViewportExtEx( hdc, -2, 2, NULL );
    Rectangle( hdc, 20, 20, 40, 40 );
    Rectangle( hdc, 30, 50, 55, 35 );
    SetViewportExtEx( hdc, 3, -3, NULL );
    Rectangle( hdc, 20, 20, 40, 40 );
    Rectangle( hdc, 30, 50, 55, 35 );
    SetWindowExtEx( hdc, -20, 20, NULL );
    Rectangle( hdc, 20, 20, 40, 40 );
    Rectangle( hdc, 24, 22, 21, 20 );
    SetMapMode( hdc, MM_TEXT );
    SetGraphicsMode( hdc, GM_ADVANCED );
    Rectangle( hdc, 20, 20, 40, 40 );
    Rectangle( hdc, 30, 50, 55, 35 );
    Rectangle( hdc, 60, 60, 35, 45 );
    Rectangle( hdc, 70, 70, 50, 70 );
    Rectangle( hdc, 75, 75, 75, 85 );
    Rectangle( hdc, 80, 80, 81, 81 );
    SetArcDirection( hdc, AD_CLOCKWISE );
    Rectangle( hdc, 20, 20, 40, 40 );
    Rectangle( hdc, 30, 50, 55, 35 );
    Rectangle( hdc, 60, 60, 35, 45 );
    Rectangle( hdc, 70, 70, 50, 70 );
    Rectangle( hdc, 75, 75, 75, 85 );
    Rectangle( hdc, 80, 80, 81, 81 );
    SetArcDirection( hdc, AD_COUNTERCLOCKWISE );
    EndPath( hdc );
    SetMapMode( hdc, MM_TEXT );
    ok_path( hdc, "rectangle_path", rectangle_path, sizeof(rectangle_path)/sizeof(path_test_t) );
    ReleaseDC( 0, hdc );
}

START_TEST(path)
{
    test_path_state();
    test_widenpath();
    test_arcto();
    test_anglearc();
    test_polydraw();
    test_closefigure();
    test_linedda();
    test_rectangle();
}
