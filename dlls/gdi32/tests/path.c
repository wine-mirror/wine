/*
 * Unit test suite for paths
 *
 * Copyright 2007 Laurent Vromman
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

static void test_widenpath(void)
{
    HDC hdc = GetDC(0);
    HPEN greenPen, narrowPen;
    HPEN oldPen;
    POINT pnt[6];
    INT nSize, ret;
    DWORD error;

    /* Create a pen to be used in WidenPath */
    greenPen = CreatePen(PS_SOLID, 10, RGB(0,0,0));
    oldPen = SelectObject(hdc, greenPen);

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
    ok(nSize > 6, "Path number of points is to low. Should be more than 6 but is %d\n", nSize);

    AbortPath(hdc);

    /* Test WidenPath with an open path */
    SetLastError(0xdeadbeef);
    BeginPath(hdc);
    ret = WidenPath(hdc);
    error = GetLastError();
    ok(ret == FALSE && GetLastError() == ERROR_CAN_NOT_COMPLETE, "WidenPath fails while widening an open path. Return value is %d, should be %d. Error is %08x, should be %08x\n", ret, FALSE, GetLastError(), ERROR_CAN_NOT_COMPLETE);

    AbortPath(hdc);

    /* Test when the pen width is equal to 1. The path should not change */
    narrowPen = CreatePen(PS_SOLID, 1, RGB(0,0,0));
    oldPen = SelectObject(hdc, narrowPen);
    BeginPath(hdc);
    Polyline(hdc, pnt, 6);
    EndPath(hdc);
    nSize = GetPath(hdc, NULL, NULL, 0);
    ok(nSize == 6, "WidenPath fails detecting 1px wide pen. Path length is %d, should be 6\n", nSize);

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

    /* How many extra entries before this one only on wine
     * but not on native? */
    int wine_only_entries_preceding;

    /* Does this entry itself not match on wine? */
    int todo;
} path_test_t;

/* Helper function to verify that the current path in the given DC matches the expected path.
 *
 * We use a "smart" matching algorithm that allows us to detect partial improvements
 * in conformance. Specifically, two running indices are kept, one through the actual
 * path and one through the expected path. The actual path index always increases,
 * whereas, if the wine_entries_preceding field of the appropriate path_test_t element is
 * non-zero, the expected path index does not increase for that many elements as long as
 * there is no match. This allows us to todo_wine extra path elements that are present only
 * in wine but not on native.
 *
 * Note that if expected_size is zero and the WINETEST_DEBUG environment variable is
 * greater than 2, the trace() output is a C path_test_t array structure, useful for making
 * new tests that use this function.
 */
static void ok_path(HDC hdc, const path_test_t *expected, int expected_size, BOOL todo_size)
{
    static const char *type_string[8] = { "Unknown (0)", "PT_CLOSEFIGURE", "PT_LINETO",
                                          "PT_LINETO | PT_CLOSEFIGURE", "PT_BEZIERTO",
                                          "PT_BEZIERTO | PT_CLOSEFIGURE", "PT_MOVETO", "PT_MOVETO | PT_CLOSEFIGURE"};
    POINT *pnt = NULL;
    BYTE *types = NULL;
    int size, numskip,
        idx, eidx = 0;

    /* Get the path */
    assert(hdc != 0);
    size = GetPath(hdc, NULL, NULL, 0);
    ok(size > 0, "GetPath returned size %d, last error %d\n", size, GetLastError());
    if (size <= 0)
    {
        skip("Cannot perform path comparisons due to failure to retrieve path.\n");
        return;
    }
    pnt = HeapAlloc(GetProcessHeap(), 0, size*sizeof(POINT));
    assert(pnt != 0);
    types = HeapAlloc(GetProcessHeap(), 0, size*sizeof(BYTE));
    assert(types != 0);
    size = GetPath(hdc, pnt, types, size);
    assert(size > 0);

    if (todo_size) todo_wine
        ok(size == expected_size, "Path size %d does not match expected size %d\n", size, expected_size);
    else
        ok(size == expected_size, "Path size %d does not match expected size %d\n", size, expected_size);

    if (expected_size) numskip = expected[eidx].wine_only_entries_preceding;
    for (idx = 0; idx < size && eidx < expected_size; idx++)
    {
        /* We allow a few pixels fudge in matching X and Y coordinates to account for imprecision in
         * floating point to integer conversion */
        BOOL match = (types[idx] == expected[eidx].type) &&
            (pnt[idx].x >= expected[eidx].x-1 && pnt[idx].x <= expected[eidx].x+1) &&
            (pnt[idx].y >= expected[eidx].y-1 && pnt[idx].y <= expected[eidx].y+1);

        if (winetest_debug > 2)
            trace("{%d, %d, %s, 0, 0}, /* %d */\n", pnt[idx].x, pnt[idx].y, type_string[types[idx]], idx);

        if (expected[eidx].todo || numskip) todo_wine
            ok(match, "Expected #%d: %s (%d,%d) but got %s (%d,%d)\n", eidx,
               type_string[expected[eidx].type], expected[eidx].x, expected[eidx].y,
               type_string[types[idx]], pnt[idx].x, pnt[idx].y);
        else
            ok(match, "Expected #%d: %s (%d,%d) but got %s (%d,%d)\n", eidx,
               type_string[expected[eidx].type], expected[eidx].x, expected[eidx].y,
               type_string[types[idx]], pnt[idx].x, pnt[idx].y);

        if (match || !numskip--)
            numskip = expected[++eidx].wine_only_entries_preceding;
    }

    /* If we are debugging and the actual path is longer than the expected path, make
     * sure to display the entire path */
    if (winetest_debug > 2 && idx < size)
        for (; idx < size; idx++)
            trace("{%d, %d, %s, 0, 0}, /* %d */\n", pnt[idx].x, pnt[idx].y, type_string[types[idx]], idx);

    HeapFree(GetProcessHeap(), 0, types);
    HeapFree(GetProcessHeap(), 0, pnt);
}

static const path_test_t arcto_path[] = {
    {0, 0, PT_MOVETO, 0, 0}, /* 0 */
    {229, 215, PT_LINETO, 0, 0}, /* 1 */
    {248, 205, PT_BEZIERTO, 0, 0}, /* 2 */
    {273, 200, PT_BEZIERTO, 0, 0}, /* 3 */
    {300, 200, PT_BEZIERTO, 0, 0}, /* 4 */
    {355, 200, PT_BEZIERTO, 0, 0}, /* 5 */
    {399, 222, PT_BEZIERTO, 0, 0}, /* 6 */
    {399, 250, PT_BEZIERTO, 0, 0}, /* 7 */
    {399, 263, PT_BEZIERTO, 0, 0}, /* 8 */
    {389, 275, PT_BEZIERTO, 0, 0}, /* 9 */
    {370, 285, PT_BEZIERTO, 0, 0}, /* 10 */
    {363, 277, PT_LINETO, 0, 0}, /* 11 */
    {380, 270, PT_BEZIERTO, 0, 0}, /* 12 */
    {389, 260, PT_BEZIERTO, 0, 0}, /* 13 */
    {389, 250, PT_BEZIERTO, 0, 0}, /* 14 */
    {389, 228, PT_BEZIERTO, 0, 0}, /* 15 */
    {349, 210, PT_BEZIERTO, 0, 0}, /* 16 */
    {300, 210, PT_BEZIERTO, 0, 0}, /* 17 */
    {276, 210, PT_BEZIERTO, 0, 0}, /* 18 */
    {253, 214, PT_BEZIERTO, 0, 0}, /* 19 */
    {236, 222, PT_BEZIERTO | PT_CLOSEFIGURE, 0, 0}}; /* 20 */

static void test_arcto(void)
{
    HDC hdc = GetDC(0);

    BeginPath(hdc);
    SetArcDirection(hdc, AD_CLOCKWISE);
    if (!ArcTo(hdc, 200, 200, 400, 300, 200, 200, 400, 300) &&
        GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        /* ArcTo is only available on Win2k and later */
        skip("ArcTo is not available\n");
        goto done;
    }
    SetArcDirection(hdc, AD_COUNTERCLOCKWISE);
    ArcTo(hdc, 210, 210, 390, 290, 390, 290, 210, 210);
    CloseFigure(hdc);
    EndPath(hdc);

    ok_path(hdc, arcto_path, sizeof(arcto_path)/sizeof(path_test_t), 0);
done:
    ReleaseDC(0, hdc);
}

START_TEST(path)
{
    test_widenpath();
    test_arcto();
}
